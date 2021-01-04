/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa_prajwal@yahoo.co.in'
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

#include <Process.h>
#include <MountManager.h>
#include <UserManager.h>
#include <ProcessGroup.h>
#include <MemUtil.h>
#include <ProcessManager.h>
#include <Display.h>

//#define INIT_NAME "_stdio_init-NOTINUSE"
//#define TERM_NAME "_stdio_term-NOTINUSE"

int Process::_nextPid = 0;

Process::Process(const upan::string& name, int parentID, bool isFGProcess)
  : _name(name), _dmmFlag(false), _stateInfo(*new ProcessStateInfo()), _processGroup(nullptr) {
  _processID = _nextPid++;
  _parentProcessID = parentID;
  _status = NEW;

  auto parentProcess = ProcessManager::Instance().GetAddressSpace(parentID);

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
}

Process::~Process() {
  delete &_stateInfo;
}

// 1. KernelProcess can have child processes of type either KernelProcess or UserProcess
// 2. KernelProcess cannot have threads
// 3. UserProcess can have a child UserProcess or UserThread
// 4. UserThread can have a child UserProcess or UserThread
// 5. a UserThread created by another UserThread will have its parent set to the parent of the creator UserThread (which will be a UserProcess - main thread)
// 6. If a parent process (Kernel or User) is terminated then
//   a. all terminated child processes are released and all non-terminated child processes are redirected the parent of the current process
//   b. all child threads are destroyed and released
// 7. If a child thread terminates then
//   a. all terminated child processes are released and all non-terminated child processes are redirected to main thread process
//   b. as per (5), there can't be any child threads under another child thread
void Process::Destroy() {
  setStatus(TERMINATED);

  DestroyThreads();

  // child processes of this process (if any) will be redirected to the parent of the current process
  auto parentProcess = ProcessManager::Instance().GetAddressSpace(_parentProcessID);
  for(auto pid : _childProcessIDs) {
    ProcessManager::Instance().GetAddressSpace(pid).ifPresent([&parentProcess](Process &p) {
      if (p.status() == TERMINATED) {
        p.Release();
      } else {
        parentProcess.ifPresent([&p](Process& pp) {
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

  if(_parentProcessID == NO_PROCESS_ID) {
    Release();
  }

  //MemManager::Instance().DisplayNoOfFreePages();
  //MemManager::Instance().DisplayNoOfAllocPages(0,0);
}

void Process::Release() {
  setStatus(RELEASED);
}

void Process::Load() {
  onLoad();
  MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&_processLDT, SYS_LINEAR_SELECTOR_DEFINED, MEM_LDT_START, sizeof(ProcessLDT)) ;
  MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&_taskState, SYS_LINEAR_SELECTOR_DEFINED, MEM_USER_TSS_START, sizeof(TaskState)) ;
}

void Process::Store() {
  MemUtil_CopyMemory(SYS_LINEAR_SELECTOR_DEFINED, MEM_LDT_START, MemUtil_GetDS(), (unsigned)&_processLDT, sizeof(ProcessLDT)) ;
  MemUtil_CopyMemory(SYS_LINEAR_SELECTOR_DEFINED, MEM_USER_TSS_START, MemUtil_GetDS(), (unsigned)&_taskState, sizeof(TaskState)) ;
}

FILE_USER_TYPE Process::FileUserType(const FileSystem::Node &node) const
{
  if(isKernelProcess() || _userID == ROOT_USER_ID || node.UserID() == _userID)
    return USER_OWNER ;

  return USER_OTHERS ;
}

bool Process::HasFilePermission(const FileSystem::Node& node, byte mode) const
{
  unsigned short usMode = FILE_PERM(node.Attribute());

  bool bHasRead, bHasWrite;

  switch(FileUserType(node))
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

uint32_t Process::Common::AllocatePDE() {
  unsigned uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
  unsigned uiPDEAddress = uiFreePageNo * PAGE_SIZE;

  for (unsigned i = 0; i < PAGE_TABLE_ENTRIES; i++)
    ((unsigned *) (uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i] = 0x2;

  return uiPDEAddress;
}

void Process::Common::UpdatePDEWithStackPTE(uint32_t pdeAddress, uint32_t stackPTEAddress) {
  ((unsigned *) (pdeAddress - GLOBAL_DATA_SEGMENT_BASE))[PROCESS_STACK_PDE_ID] = (stackPTEAddress & 0xFFFFF000) | 0x7;
}

uint32_t Process::Common::AllocatePTEForStack() {
  auto stackPTEAddress = MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE;
  for (uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++) {
    ((unsigned *) (stackPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[j] = 0x2;
  }
  return stackPTEAddress;
}

void Process::Common::AllocateStackSpace(uint32_t pteAddress) {
  //pre-allocate process stack - user (NO_OF_PAGES_FOR_STARTUP_ARGS) + call-gate
  //further expansion of user stack beyond NO_OF_PAGES_FOR_STARTUP_ARGS will happen as part of regular page fault handling flow
  for (int i = 0; i < PROCESS_CG_STACK_PAGES + NO_OF_PAGES_FOR_STARTUP_ARGS; ++i) {
    const uint32_t uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
    const uint32_t ptePageType = i > PROCESS_CG_STACK_PAGES - 1 ? 0x7 : 0x3;
    ((unsigned *) (pteAddress - GLOBAL_DATA_SEGMENT_BASE))[PAGE_TABLE_ENTRIES - 1 - i] =
        ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | ptePageType;
  }
}

void Process::Common::DeAllocateStackSpace(uint32_t stackPTEAddress) {
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
  _eventCompleted(0),
  _kernelServiceComplete(false)
{
}

bool ProcessStateInfo::IsEventCompleted()
{
  if(_eventCompleted)
  {
    Atomic::Swap(_eventCompleted, 0);
    return true;
  }
  return false;
}

void ProcessStateInfo::EventCompleted()
{
  Atomic::Swap(_eventCompleted, 1);
}