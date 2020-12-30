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
#include <uniq_ptr.h>
#include <try.h>
#include <Process.h>
#include <ProcessLoader.h>
#include <ProcessAllocator.h>
#include <DynamicLinkLoader.h>
#include <MountManager.h>
#include <ElfParser.h>
#include <ElfRelocationSection.h>
#include <ElfSymbolTable.h>
#include <ElfDynamicSection.h>
#include <UserManager.h>
#include <ProcFileManager.h>
#include <ProcessEnv.h>
#include <ProcessGroup.h>
#include <MemUtil.h>
#include <DMM.h>

//#define INIT_NAME "_stdio_init-NOTINUSE"
//#define TERM_NAME "_stdio_term-NOTINUSE"

int Process::_nextPid = 0;

Process::Process(const upan::string& name, int parentID, bool isFGProcess)
  : _name(name), _dmmFlag(false), _stateInfo(*new ProcessStateInfo()), _processGroup(nullptr) {
  _processID = _nextPid++;
  iParentProcessID = parentID;
  uiAUTAddress = NULL;
  status = NEW;
  bIsKernelProcess = false;

  auto parentProcess = ProcessManager::Instance().GetAddressSpace(parentID);

  if(parentProcess.isEmpty()) {
    iDriveID = ROOT_DRIVE_ID ;
    if(iDriveID != CURRENT_DRIVE)
    {
      DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, false).goodValueOrThrow(XLOC);
      if(pDiskDrive->Mounted())
        processPWD = pDiskDrive->_fileSystem.FSpwd;
    }
    _processGroup = new ProcessGroup(isFGProcess);
  } else {
    iDriveID = parentProcess.value().iDriveID ;
    processPWD = parentProcess.value().processPWD;
    _processGroup = parentProcess.value()._processGroup;
  }

  _processGroup->AddProcess();
  if(isFGProcess)
    _processGroup->PutOnFGProcessList(_processID);
}

Process::~Process() {
  delete &_stateInfo;
}

void Process::Destroy() {
  Atomic::Swap((__volatile__ uint32_t&)(status), static_cast<int>(TERMINATED)) ;
  //MemManager::Instance().DisplayNoOfFreePages();
  //MemManager::Instance().DisplayNoOfAllocPages(0,0);

  // Deallocate Resources
  DeAllocateResources();

  // Release From Process Group
  _processGroup->RemoveFromFGProcessList(_processID);
  _processGroup->RemoveProcess();

  if(_processGroup->Size() == 0)
    delete _processGroup;

  if(iParentProcessID == NO_PROCESS_ID) {
    Release();
  }

  //MemManager::Instance().DisplayNoOfFreePages();
  //MemManager::Instance().DisplayNoOfAllocPages(0,0);
}

void Process::Release() {
  Atomic::Swap((__volatile__ uint32_t&)(status), static_cast<int>(RELEASED)) ;
}

void Process::Load() {
  MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&processLDT, SYS_LINEAR_SELECTOR_DEFINED, MEM_LDT_START, sizeof(ProcessLDT)) ;
  MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&taskState, SYS_LINEAR_SELECTOR_DEFINED, MEM_USER_TSS_START, sizeof(TaskState)) ;
}

void Process::Store() {
  MemUtil_CopyMemory(SYS_LINEAR_SELECTOR_DEFINED, MEM_LDT_START, MemUtil_GetDS(), (unsigned)&processLDT, sizeof(ProcessLDT)) ;
  MemUtil_CopyMemory(SYS_LINEAR_SELECTOR_DEFINED, MEM_USER_TSS_START,	MemUtil_GetDS(), (unsigned)&taskState, sizeof(TaskState)) ;
}

uint32_t Process::GetDLLPageAddressForKernel()
{
  unsigned uiPDEAddress = taskState.CR3_PDBR ;
  unsigned uiPDEIndex = ((PROCESS_DLL_PAGE_ADDR >> 22) & 0x3FF) ;
  unsigned uiPTEIndex = ((PROCESS_DLL_PAGE_ADDR >> 12) & 0x3FF) ;
  unsigned uiPTEAddress = ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] & 0xFFFFF000 ;
  unsigned uiPageNumber = (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] & 0xFFFFF000) / PAGE_SIZE ;

  return (uiPageNumber * PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE ;
}

KernelProcess::KernelProcess(const upan::string& name, uint32_t taskAddress, int parentID, bool isFGProcess, uint32_t param1, uint32_t param2)
  : Process(name, parentID, isFGProcess) {
  bIsKernelProcess = true;
  _mainThreadID = _processID;

  ProcessEnv_InitializeForKernelProcess() ;

  const uint32_t uiStackAddress = AllocateAddressSpace();

  const uint32_t uiStackTop = uiStackAddress - GLOBAL_DATA_SEGMENT_BASE + (PROCESS_KERNEL_STACK_PAGES * PAGE_SIZE) - 1;
  taskState.BuildForKernel(taskAddress, uiStackTop, param1, param2);
  processLDT.BuildForKernel();
  iUserID = ROOT_USER_ID ;
}

uint32_t KernelProcess::AllocateAddressSpace() {
  const int iStackBlockID = MemManager::Instance().GetFreeKernelProcessStackBlockID() ;

  if(iStackBlockID < 0)
    throw upan::exception(XLOC, "No free stack blocks available for kernel process");

  kernelStackBlockId = iStackBlockID ;

  for(int i = 0; i < PROCESS_KERNEL_STACK_PAGES; i++)
  {
    uint32_t uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
    MemManager::Instance().GetKernelProcessStackPTEBase()[iStackBlockID * PROCESS_KERNEL_STACK_PAGES + i] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x3 ;
  }
  Mem_FlushTLB();

  return KERNEL_PROCESS_PDE_ID * PAGE_TABLE_ENTRIES * PAGE_SIZE + iStackBlockID * PROCESS_KERNEL_STACK_PAGES * PAGE_SIZE;
}

void KernelProcess::DeAllocateResources() {
  for(int i = 0; i < PROCESS_KERNEL_STACK_PAGES; i++) {
    MemManager::Instance().DeAllocatePhysicalPage(
        (MemManager::Instance().GetKernelProcessStackPTEBase()[kernelStackBlockId * PROCESS_KERNEL_STACK_PAGES + i] & 0xFFFFF000) / PAGE_SIZE) ;
  }
  Mem_FlushTLB();

  MemManager::Instance().FreeKernelProcessStackBlock(kernelStackBlockId) ;
}

UserProcess::UserProcess(const upan::string &name, int parentID, int userID,
                         bool isFGProcess, int noOfParams, char** args)
                         : Process(name, parentID, isFGProcess) {
  _mainThreadID = _processID;

  Load(noOfParams, args);

  uiNoOfPagesForDLLPTE = 0 ;
  processLDT.BuildForUser();

  auto parentProcess = ProcessManager::Instance().GetAddressSpace(parentID);
  iUserID = userID == DERIVE_FROM_PARENT && !parentProcess.isEmpty() ? parentProcess.value().iUserID : iUserID;
}

void UserProcess::Load(int iNumberOfParameters, char** szArgumentList)
{
  ELFParser mELFParser(_name.c_str());

  unsigned uiMinMemAddr, uiMaxMemAddr;

  mELFParser.GetMemImageSize(&uiMinMemAddr, &uiMaxMemAddr);
  if(uiMinMemAddr < PROCESS_BASE)
    throw upan::exception(XLOC, "process min load address %x is less than PROCESS_BASE %x", uiMinMemAddr, PROCESS_BASE);

  if((uiMinMemAddr % PAGE_SIZE) != 0)
    throw upan::exception(XLOC, "process min load address %x is not page aligned", uiMinMemAddr);

  unsigned uiDLLSectionSize ;

  upan::uniq_ptr<byte[]> bDLLSectionImage(ProcessLoader::Instance().LoadDLLInitSection(uiDLLSectionSize));

  unsigned uiProcessImageSize = ProcessLoader_GetCeilAlignedAddress(uiMaxMemAddr - uiMinMemAddr, 4) ;
  unsigned uiMemImageSize = uiProcessImageSize + uiDLLSectionSize ;

  _noOfPagesForProcess = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize);

  if(_noOfPagesForProcess > MAX_PAGES_PER_PROCESS)
    throw upan::exception(XLOC, "process requires %u pages that's larger than supported %u", _noOfPagesForProcess, MAX_PAGES_PER_PROCESS);

  _processBase = uiMinMemAddr ;
  unsigned uiPageOverlapForProcessBase = ((_processBase / PAGE_SIZE) % PAGE_TABLE_ENTRIES) ;
  _noOfPagesForPTE = MemManager::Instance().GetPTESizeInPages(_noOfPagesForProcess + uiPageOverlapForProcessBase) + PROCESS_SPACE_FOR_OS;

  const uint32_t uiPDEAddress = AllocateAddressSpace();

  unsigned pRealELFSectionHeadeAddr = DynamicLinkLoader_Initialize(uiPDEAddress);
  unsigned uiCopySize = mELFParser.CopyELFSectionHeader((ELF32SectionHeader*)pRealELFSectionHeadeAddr) ;
  mELFParser.CopyELFSecStrTable((char*)(pRealELFSectionHeadeAddr + uiCopySize)) ;

  upan::uniq_ptr<byte[]> bProcessImage(new byte[sizeof(char) * uiMemImageSize]);

  upan::trycall([&] { mELFParser.CopyProcessImage(bProcessImage.get(), _processBase, uiMemImageSize); }).onBad([&] (const upan::error& err) {
    DeAllocateAddressSpace();
    throw upan::exception(XLOC, err);
  });

  memcpy((void*)(bProcessImage.get() + uiProcessImageSize), (void*)bDLLSectionImage.get(), uiDLLSectionSize) ;

  // Setting the Dynamic Link Loader Address in GOT
  mELFParser.GetGOTAddress(bProcessImage.get(), uiMinMemAddr).onGood([&](uint32_t* uiGOT) {
    uiGOT[1] = -1;
    uiGOT[2] = uiMinMemAddr + uiProcessImageSize;
  });

  // Initialize BSS segment to 0
  mELFParser.GetSectionHeaderByTypeAndName(ELFSectionHeader::SHT_NOBITS, BSS_SEC_NAME).onGood([&] (ELF32SectionHeader* pBSSSectionHeader)
  {
    unsigned* pBSS = (unsigned*)(bProcessImage.get() + pBSSSectionHeader->sh_addr - uiMinMemAddr) ;
    unsigned uiBSSSize = pBSSSectionHeader->sh_size ;

    unsigned q ;
    for(q = 0; q < uiBSSSize / sizeof(unsigned); q++) pBSS[q] = 0 ;

    unsigned r ;
    for(r = q * sizeof(unsigned); r < uiBSSSize; r++) ((char*)pBSS)[r] = 0 ;
  });

  CopyElfImage(uiPDEAddress, bProcessImage.get(), uiMemImageSize);

  /* Find init and term stdio functions in libc if any */
  /*** This code is not required anymore because init and term is now handled in crt0.s - which calls init_standard_library and exit functions */
//  unsigned uiInitRelocAddress = NULL ;
//  unsigned uiTermRelocAddress = NULL ;
//  mELFParser.GetSectionHeaderByTypeAndName(ELFSectionHeader::SHT_REL, REL_PLT_SUB_NAME).onGood([&] (ELF32SectionHeader* pRelocationSectionHeader)
//  {
//    mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_link).onGood([&] (ELF32SectionHeader* pDynamicSymSectiomHeader)
//    {
//      mELFParser.GetSectionHeaderByIndex(pDynamicSymSectiomHeader->sh_link).onGood([&] (ELF32SectionHeader* pDynamicSymStringSectionHeader)
//      {
//        ELFRelocSection::ELF32_Rel* pELFDynRelTable =
//          (ELFRelocSection::ELF32_Rel*)((unsigned)bProcessImage.get() + pRelocationSectionHeader->sh_addr - uiMinMemAddr) ;
//
//        unsigned uiNoOfDynRelEntries = pRelocationSectionHeader->sh_size / pRelocationSectionHeader->sh_entsize ;
//
//        ELFSymbolTable::ELF32SymbolEntry* pELFDynSymTable =
//          (ELFSymbolTable::ELF32SymbolEntry*)((unsigned)bProcessImage.get() + pDynamicSymSectiomHeader->sh_addr - uiMinMemAddr) ;
//
//        const char* pDynStrTable = (const char*)((unsigned)bProcessImage.get() + pDynamicSymStringSectionHeader->sh_addr - uiMinMemAddr) ;
//
//        for(unsigned i = 0; i < uiNoOfDynRelEntries && (uiInitRelocAddress == NULL || uiTermRelocAddress == NULL); i++)
//        {
//          if(ELF32_R_TYPE(pELFDynRelTable[i].r_info) == ELFRelocSection::R_386_JMP_SLOT)
//          {
//            int iSymIndex = ELF32_R_SYM(pELFDynRelTable[i].r_info);
//            int iStrIndex = pELFDynSymTable[ iSymIndex ].st_name ;
//            char* szSymName = (char*)&pDynStrTable[ iStrIndex ] ;
//
//            if(strcmp(szSymName, INIT_NAME) == 0)
//              uiInitRelocAddress = pELFDynRelTable[i].r_offset ;
//            else if(strcmp(szSymName, TERM_NAME) == 0)
//              uiTermRelocAddress = pELFDynRelTable[i].r_offset ;
//          }
//        }
//      });
//    });
//  });

  /* End of Find init and term stdio functions in libc if any */

  const uint32_t uiProcessEntryStackSize = PushProgramInitStackData(uiPDEAddress, iNumberOfParameters, szArgumentList) ;
  const uint32_t uiEntryAdddress = mELFParser.GetProgramStartAddress();// uiMinMemAddr + uiProcessImageSize ;

  ProcessEnv_Initialize(uiPDEAddress, iParentProcessID);
  ProcFileManager_Initialize(uiPDEAddress, iParentProcessID);

  const uint32_t stackTopAddress = PROCESS_STACK_TOP_ADDRESS - PROCESS_BASE;
  taskState.BuildForUser(stackTopAddress, uiPDEAddress, uiEntryAdddress, uiProcessEntryStackSize);
}

uint32_t UserProcess::PushProgramInitStackData(unsigned uiPDEAddress, int iNumberOfParameters, char** szArgumentList) {
  const uint32_t uiPDEIndex = PROCESS_STACK_PDE_ID;
  const uint32_t uiPTEIndex = PAGE_TABLE_ENTRIES - PROCESS_CG_STACK_PAGES - NO_OF_PAGES_FOR_STARTUP_ARGS;

  const uint32_t uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;
  uint32_t uiPageAddress = (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex]) & 0xFFFFF000 ;

  const unsigned argc = 4;
  const unsigned argv = 4; // address of argv first dimension
  const unsigned uiArgumentAddressListSize = iNumberOfParameters * 4; // address of char* entry (second dimension) of argv array

  unsigned uiStackTopAddress = PROCESS_STACK_TOP_ADDRESS - PROCESS_CG_STACK_PAGES * PAGE_SIZE - PROCESS_BASE;

  unsigned uiArgumentDataSize = 0;

  for(int i = 0; i < iNumberOfParameters; i++)
    uiArgumentDataSize += (strlen(szArgumentList[i]) + 1) ;

  const uint32_t uiProcessEntryStackSize = argc + argv + uiArgumentAddressListSize + uiArgumentDataSize;

  if (uiProcessEntryStackSize > (NO_OF_PAGES_FOR_STARTUP_ARGS * PAGE_SIZE)) {
    throw upan::exception(XLOC, "Startup arguments size is larger than %d pages", NO_OF_PAGES_FOR_STARTUP_ARGS);
  }

  uiPageAddress = uiPageAddress + PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE - uiProcessEntryStackSize ;

  int iStackIndex = 0 ;
  ((unsigned*)(uiPageAddress))[iStackIndex++] = iNumberOfParameters; // argc
  int argLength = (iStackIndex + 1) * 4;
  ((unsigned*)(uiPageAddress))[iStackIndex++] = uiStackTopAddress - uiProcessEntryStackSize + argLength; // argv

  uiArgumentDataSize = 0 ;// argv[0] through argv[argc - 1]
  for(int i = 0; i < iNumberOfParameters; i++) {
    ((unsigned*)(uiPageAddress))[iStackIndex + i] = ((unsigned*)(uiPageAddress))[iStackIndex - 1] + uiArgumentAddressListSize + uiArgumentDataSize ;
    strcpy((char*)&((unsigned*)(uiPageAddress))[iStackIndex + iNumberOfParameters] + uiArgumentDataSize,	szArgumentList[i]) ;
    uiArgumentDataSize += (strlen(szArgumentList[i]) + 1) ;
  }

  return uiProcessEntryStackSize;
}

void UserProcess::CopyElfImage(unsigned uiPDEAddr, byte* bProcessImage, unsigned uiMemImageSize)
{
  unsigned uiPDEAddress = uiPDEAddr ;
  unsigned uiPDEIndex, uiPTEIndex, uiPTEAddress, uiPageAddress ;
  unsigned uiOffset = 0 ;
  unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;
  unsigned uiProcessPageBase = ( _processBase / PAGE_SIZE ) % PAGE_TABLE_ENTRIES ;

  unsigned uiCopySize = uiMemImageSize ;
  unsigned uiNoOfPagesForProcess = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize) ;

  for(uint32_t i = 0; i < uiNoOfPagesForProcess; i++)
  {
    uiPDEIndex = (uiProcessPageBase + i) / PAGE_TABLE_ENTRIES + uiProcessPDEBase + PROCESS_SPACE_FOR_OS ;
    uiPTEIndex = (uiProcessPageBase + i) % PAGE_TABLE_ENTRIES ;
    //Kernel Heap PTE and PROCESS_SPACE_FOR_OS need not be part of uiPTEIndex calculation as they shall
    //be aligned at PAGE BOUNDARY

    uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;
    uiPageAddress = (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex]) & 0xFFFFF000 ;

    memcpy((void*)(uiPageAddress - GLOBAL_DATA_SEGMENT_BASE), (void*)((unsigned)bProcessImage + uiOffset), upan::min(uiCopySize, (uint32_t)PAGE_SIZE));
    if(uiCopySize <= PAGE_SIZE)
      break;

    uiOffset += PAGE_SIZE ;
    uiCopySize -= PAGE_SIZE ;
  }

  return ;
}

uint32_t UserProcess::AllocateAddressSpace()
{
  const uint32_t uiPDEAddress = AllocatePDE();

  AllocatePTE(uiPDEAddress);
  InitializeProcessSpaceForOS(uiPDEAddress);
  InitializeProcessSpaceForProcess(uiPDEAddress);

  return uiPDEAddress;
}

uint32_t UserProcess::AllocatePDE()
{
  unsigned uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
  unsigned uiPDEAddress = uiFreePageNo * PAGE_SIZE;

  for(unsigned i = 0; i < PAGE_TABLE_ENTRIES; i++)
    ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i] = 0x2;

  return uiPDEAddress;
}

void UserProcess::AllocatePTE(const unsigned uiPDEAddress)
{
  unsigned uiPDEIndex = 0;
  unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;

  for(uint32_t i = 0; i < _noOfPagesForPTE; i++)
  {
    auto uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();

    for(uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++)
      ((unsigned*)((uiFreePageNo * PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE))[j] = 0x2 ;

    if(i < PROCESS_SPACE_FOR_OS) {
      // Full Permission is given at the PDE level and access control is enforced at PTE level i.e, individual page level
      ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7;
    }
    else
    {
      uiPDEIndex = i + uiProcessPDEBase ;
      ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;
    }
  }

  auto stackPTE = MemManager::Instance().AllocatePhysicalPage();
  for(uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++)
    ((unsigned*)((stackPTE * PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE))[j] = 0x2 ;
  ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[PROCESS_STACK_PDE_ID] = ((stackPTE * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;

  _startPDEForDLL = ++uiPDEIndex ;
}

void UserProcess::InitializeProcessSpaceForOS(const unsigned uiPDEAddress)
{
  for(uint32_t i = 0; i < PROCESS_SPACE_FOR_OS; ++i)
  {
    unsigned kernelPTEAddress = (((unsigned*)(MEM_PDBR - GLOBAL_DATA_SEGMENT_BASE))[i]) & 0xFFFFF000;
    unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i]) & 0xFFFFF000 ;
    for(uint32_t  j = 0; j < PAGE_TABLE_ENTRIES; ++j)
    {
      unsigned kernelPageMapValue = ((unsigned*)(kernelPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[j];
      ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[j] = (kernelPageMapValue & 0xFFFFF000) | 0x5;
    }
  }
}

void UserProcess::InitializeProcessSpaceForProcess(const unsigned uiPDEAddress)
{
  unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;
  unsigned uiProcessPageBase = ( _processBase / PAGE_SIZE ) % PAGE_TABLE_ENTRIES ;

  for(uint32_t i = 0; i < _noOfPagesForProcess; i++)
  {
    uint32_t uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();

    auto uiPDEIndex = (uiProcessPageBase + i) / PAGE_TABLE_ENTRIES + uiProcessPDEBase + PROCESS_SPACE_FOR_OS ;
    auto uiPTEIndex = (uiProcessPageBase + i) % PAGE_TABLE_ENTRIES ;
    //Kernel Heap PTE and PROCESS_SPACE_FOR_OS need not be part of uiPTEIndex calculation as they shall be aligned at PAGE BOUNDARY

    auto uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;
    ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7;
  }

  //pre-allocate process stack - user (NO_OF_PAGES_FOR_STARTUP_ARGS) + call-gate
  //further expansion of user stack beyond NO_OF_PAGES_FOR_STARTUP_ARGS will happen as part of regular page fault handling flow
  auto uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[PROCESS_STACK_PDE_ID]) & 0xFFFFF000 ;
  for(int i = 0; i < PROCESS_CG_STACK_PAGES + NO_OF_PAGES_FOR_STARTUP_ARGS; ++i) {
    const uint32_t uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
    const uint32_t ptePageType = i > PROCESS_CG_STACK_PAGES - 1 ? 0x7 : 0x3;
    ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[PAGE_TABLE_ENTRIES - 1 - i] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | ptePageType;
  }
}

void UserProcess::DeAllocateResources() {
  DLLLoader_DeAllocateProcessDLLPTEPages(this) ;
  DynamicLinkLoader_UnInitialize(this) ;

  DMM_DeAllocatePhysicalPages(this) ;

  ProcessEnv_UnInitialize(*this) ;
  ProcFileManager_UnInitialize(this);

  DeAllocateAddressSpace();
}

void UserProcess::DeAllocateAddressSpace()
{
  DeAllocateProcessSpace();
  DeAllocatePTE();
  MemManager::Instance().DeAllocatePhysicalPage(taskState.CR3_PDBR / PAGE_SIZE) ;
}

void UserProcess::DeAllocateProcessSpace()
{
  unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;
  unsigned uiProcessPageBase = ( _processBase / PAGE_SIZE ) % PAGE_TABLE_ENTRIES ;

  for(uint32_t i = 0; i < _noOfPagesForProcess; i++)
  {
    auto uiPDEIndex = (uiProcessPageBase + i) / PAGE_TABLE_ENTRIES + uiProcessPDEBase + PROCESS_SPACE_FOR_OS ;
    auto uiPTEIndex = (uiProcessPageBase + i) % PAGE_TABLE_ENTRIES ;
    //Kernel Heap PTE and PROCESS_SPACE_FOR_OS need not be part of uiPTEIndex calculation as they shall be aligned at PAGE BOUNDARY

    auto uiPTEAddress = (((unsigned*)(taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;

    MemManager::Instance().DeAllocatePhysicalPage(
          (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] & 0xFFFFF000) / PAGE_SIZE) ;
  }

  //Deallocate process stack
  const unsigned stackPTEAddress = ((unsigned*)(taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE))[PROCESS_STACK_PDE_ID] & 0xFFFFF000 ;
  for(uint32_t i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
    const unsigned pageEntry = (((unsigned*)(stackPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[i]);
    const bool isPresent = pageEntry & 0x1;
    const unsigned pageAddress = pageEntry & 0xFFFFF000;
    if (isPresent && pageAddress) {
      MemManager::Instance().DeAllocatePhysicalPage(pageAddress / PAGE_SIZE);
    }
  }
}

void UserProcess::DeAllocatePTE() {
  const unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;

  for(uint32_t i = 0; i < _noOfPagesForPTE; i++) {
    const uint32_t pdeIndex = (i < PROCESS_SPACE_FOR_OS) ? i : i + uiProcessPDEBase;
    const uint32_t uiPTEAddress = ((unsigned*)(taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE))[pdeIndex] & 0xFFFFF000;
    MemManager::Instance().DeAllocatePhysicalPage(uiPTEAddress / PAGE_SIZE) ;
  }

  unsigned stackPTEAddress = ((unsigned*)(taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE))[PROCESS_STACK_PDE_ID] & 0xFFFFF000;
  MemManager::Instance().DeAllocatePhysicalPage(stackPTEAddress / PAGE_SIZE);
}

FILE_USER_TYPE Process::FileUserType(const FileSystem::Node &node) const
{
  if(bIsKernelProcess || iUserID == ROOT_USER_ID || node.UserID() == iUserID)
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

UserThread::UserThread(int parentID, bool isFGProcess) : Process("", parentID, isFGProcess) {
  auto parentPAS = ProcessManager::Instance().GetAddressSpace(parentID);

  if (parentPAS.isEmpty()) {
    throw upan::exception(XLOC, "Cannot create thread without parent process (%d)", parentID);
  }

  //TODO: enforce by adding a createThread() method in UserProcess
  if (parentPAS.value().bIsKernelProcess) {
    throw upan::exception(XLOC, "Threads can be created only by user processes");
  }

  _name = _name + "_T" + upan::string::to_string(_processID);
  _mainThreadID = parentID;
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