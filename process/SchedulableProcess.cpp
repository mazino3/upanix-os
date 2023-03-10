/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/
 */

#include <SchedulableProcess.h>
#include <MountManager.h>
#include <UserManager.h>
#include <ProcessGroup.h>
#include <MemUtil.h>
#include <ProcessManager.h>
#include <DMM.h>

//#define INIT_NAME "_stdio_init-NOTINUSE"
//#define TERM_NAME "_stdio_term-NOTINUSE"

int SchedulableProcess::_nextPid = 0;

SchedulableProcess::SchedulableProcess(const upan::string& name, int parentID, bool isFGProcess)
  : _name(name), _dmmFlag(false), _stateInfo(*new ProcessStateInfo()), _processGroup(nullptr) {
  _processID = _nextPid++;
  _parentProcessID = parentID;
  _status = NEW;

  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(parentID);

  if(parentProcess.isEmpty()) {
    _driveID = ROOT_DRIVE_ID ;
    if(_driveID != CURRENT_DRIVE)
    {
      DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(_driveID, false).goodValueOrThrow(XLOC);
      if(pDiskDrive->Mounted())
        _processPWD = pDiskDrive->_fileSystem.FSpwd;
    }
    _processGroup = new ProcessGroup(isFGProcess);
  } else {
    _driveID = parentProcess.value()._driveID ;
    _processPWD = parentProcess.value()._processPWD;
    _processGroup = parentProcess.value()._processGroup;
  }

  _processGroup->AddProcess();
  if(isFGProcess)
    _processGroup->PutOnFGProcessList(_processID);
  _sseRegs = (uint8_t*) DMM_AllocateForKernel(512, 16);
  //Initialize _sseRegs with some legitimate value
  FXSave();
}

SchedulableProcess::~SchedulableProcess() {
  DMM_DeAllocateForKernel(reinterpret_cast<unsigned int>(_sseRegs));
  delete &_stateInfo;
}

// 1. KernelProcess can have child processes of type either KernelProcess or KernelThread or UserProcess
// 2. KernelThread can have a child KernelProcess or KernelThread or UserProcess
// 3. UserProcess can have a child UserProcess or UserThread
// 4. UserThread can have a child UserProcess or UserThread
// 5. Thread created by another Thread will have its parent set to the parent of the creator Thread (which will be an AutonomousProcess - main thread)
// 6. If a parent process (Kernel or User) is terminated then
//   a. all terminated child processes are released and all non-terminated child processes are redirected to the parent of the current process
//   b. all child threads are destroyed and released
// 7. If a child thread terminates then
//   a. all terminated child processes are released and all non-terminated child processes are redirected to main thread process
//   b. as per (5), there can't be any child threads under another child thread
void SchedulableProcess::Destroy() {
  setStatus(TERMINATED);

  DestroyThreads();

  // child processes of this process (if any) will be redirected to the parent of the current process
  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(_parentProcessID);
  for(auto pid : _childProcessIDs) {
    ProcessManager::Instance().GetSchedulableProcess(pid).ifPresent([&parentProcess](SchedulableProcess &p) {
      if (p.status() == TERMINATED) {
        p.Release();
      } else {
        parentProcess.ifPresent([&p](SchedulableProcess& pp) {
          p.setParentProcessID(pp.processID());
          pp.addChildProcessID(p.processID());
        });
      }
    });
  }

  //MemManager::Instance().DisplayNoOfFreePages();
  //MemManager::Instance().DisplayNoOfAllocPages(0,0);

  // Deallocate Resources
  DeAllocateResources();

  // Release From Process Group
  _processGroup->RemoveFromFGProcessList(_processID);
  _processGroup->RemoveProcess();

  if(_processGroup->Size() == 0)
    delete _processGroup;

  heapMutex().ifPresent([this](upan::mutex& m) { m.unlock(_processID); });
  pageAllocMutex().ifPresent([this](upan::mutex& m) { m.unlock(_processID); });
  envMutex().ifPresent([this](upan::mutex& m) { m.unlock(_processID); });

  //TODO: release all the mutex held by the process or an individual thread

  if(_parentProcessID == NO_PROCESS_ID) {
    Release();
  }

  //MemManager::Instance().DisplayNoOfFreePages();
  //MemManager::Instance().DisplayNoOfAllocPages(0,0);
}

void SchedulableProcess::Release() {
  setStatus(RELEASED);
}

void SchedulableProcess::FXSave() {
//  uint32_t cr0;
//  __asm__ __volatile__("mov %%cr0, %0" :  "=r"(cr0) : );
//  printf("\n CR0: %x, %x", cr0, _sseRegs);
  /* As per Intel manuals, when TS flag is set and EM is clear then SSE instructions will cause GP
   * But in Qemu, this didn't cause any GP but I am doing it just to go by the doc */
  __asm__ __volatile__("clts");
  /* Below will not work because fxsave will then access _sseRegs like a pointer - in which case, it will require
   * both the address of _sseRegs and the address pointed by _sseRegs to be 16-byte aligned
   * However, only the address pointed by _sseRegs will be 16 byte aligned and the address of _sseRegs could
   * potentially be a non 16 byte aligned address - so, below code will cause a General Protection Fault --> Exception 13 (0xD)
   * __asm__ __volatile__("fxsave %0" : : "m"(_sseRegs));
   */
  __asm__ __volatile__("fxsave (%0)" : : "r"(_sseRegs));
}

void SchedulableProcess::FXRestore() {
  __asm__ __volatile__("clts");
  __asm__ __volatile__("fxrstor (%0)" : : "r"(_sseRegs));
}

void SchedulableProcess::Load() {
  onLoad();
  FXRestore();
  MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&_processLDT, SYS_LINEAR_SELECTOR_DEFINED, LDT_BASE_ADDR, sizeof(ProcessLDT)) ;
  MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&_taskState, SYS_LINEAR_SELECTOR_DEFINED, USER_TSS_BASE_ADDR, sizeof(TaskState)) ;
}

void SchedulableProcess::Store() {
  FXSave();
  MemUtil_CopyMemory(SYS_LINEAR_SELECTOR_DEFINED, LDT_BASE_ADDR, MemUtil_GetDS(), (unsigned)&_processLDT, sizeof(ProcessLDT)) ;
  MemUtil_CopyMemory(SYS_LINEAR_SELECTOR_DEFINED, USER_TSS_BASE_ADDR, MemUtil_GetDS(), (unsigned)&_taskState, sizeof(TaskState)) ;
}

FILE_USER_TYPE SchedulableProcess::fileUserType(const FileSystem::Node &node) const
{
  if(isKernelProcess() || _userID == ROOT_USER_ID || node.UserID() == _userID)
    return USER_OWNER ;

  return USER_OTHERS ;
}

bool SchedulableProcess::hasFilePermission(const FileSystem::Node& node, byte mode) const
{
  unsigned short usMode = FILE_PERM(node.Attribute());

  bool bHasRead, bHasWrite;

  switch(fileUserType(node))
  {
    case FILE_USER_TYPE::USER_OWNER:
      bHasRead = HAS_READ_PERM(G_OWNER(usMode));
      bHasWrite = HAS_WRITE_PERM(G_OWNER(usMode));
      break;

    case FILE_USER_TYPE::USER_OTHERS:
      bHasRead = HAS_READ_PERM(G_OTHERS(usMode));
      bHasWrite = HAS_WRITE_PERM(G_OTHERS(usMode));
      break;

    default:
      return false;
  }

  if(mode & O_RDONLY)
  {
    return bHasRead || bHasWrite;
  }
  else if((mode & O_WRONLY) || (mode & O_RDWR) || (mode & O_APPEND))
  {
    return bHasWrite;
  }
  return false;
}

uint32_t SchedulableProcess::Common::AllocatePDE() {
  unsigned uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
  unsigned uiPDEAddress = uiFreePageNo * PAGE_SIZE;

  for (unsigned i = 0; i < PAGE_TABLE_ENTRIES; i++)
    ((unsigned *) (uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i] = 0x2;

  return uiPDEAddress;
}

void SchedulableProcess::Common::UpdatePDEWithStackPTE(uint32_t pdeAddress, uint32_t stackPTEAddress) {
  ((unsigned *) (pdeAddress - GLOBAL_DATA_SEGMENT_BASE))[PROCESS_STACK_PDE_ID] = (stackPTEAddress & 0xFFFFF000) | 0x7;
}

uint32_t SchedulableProcess::Common::AllocatePTEForStack() {
  auto stackPTEAddress = MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE;
  for (uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++) {
    ((unsigned *) (stackPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[j] = 0x2;
  }
  return stackPTEAddress;
}

void SchedulableProcess::Common::AllocateStackSpace(uint32_t pteAddress) {
  //pre-allocate process stack - user (NO_OF_PAGES_FOR_STARTUP_ARGS) + call-gate
  //further expansion of user stack beyond NO_OF_PAGES_FOR_STARTUP_ARGS will happen as part of regular page fault handling flow
  for (int i = 0; i < PROCESS_CG_STACK_PAGES + NO_OF_PAGES_FOR_STARTUP_ARGS; ++i) {
    const uint32_t uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
    const uint32_t ptePageType = i > PROCESS_CG_STACK_PAGES - 1 ? 0x7 : 0x3;
    ((unsigned *) (pteAddress - GLOBAL_DATA_SEGMENT_BASE))[PAGE_TABLE_ENTRIES - 1 - i] =
        ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | ptePageType;
  }
}

void SchedulableProcess::Common::DeAllocateStackSpace(uint32_t stackPTEAddress) {
  //Deallocate process stack
  for (uint32_t i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
    const unsigned pageEntry = (((unsigned *) (stackPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[i]);
    const bool isPresent = pageEntry & 0x1;
    const unsigned pageAddress = pageEntry & 0xFFFFF000;
    if (isPresent && pageAddress) {
      MemManager::Instance().DeAllocatePhysicalPage(pageAddress / PAGE_SIZE);
    }
  }
}

ProcessStateInfo::ProcessStateInfo() :
  _sleepTime(0),
  _irq(&StdIRQ::Instance().NO_IRQ),
  _waitChildProcId(NO_PROCESS_ID),
  _waitResourceId(RESOURCE_NIL),
  _eventCompleted(false),
  _kernelServiceComplete(false) {
}

bool ProcessStateInfo::IsEventCompleted()
{
  if(_eventCompleted.get()) {
    _eventCompleted.set(false);
    return true;
  }
  return false;
}

void ProcessStateInfo::EventCompleted() {
  _eventCompleted.set(true);
}