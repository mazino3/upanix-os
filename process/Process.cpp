#include <uniq_ptr.h>
#include <try.h>
#include <Process.h>
#include <ProcessLoader.h>
#include <ProcessAllocator.h>
#include <DynamicLinkLoader.h>
#include <ElfParser.h>
#include <ElfRelocationSection.h>
#include <ElfSymbolTable.h>
#include <ElfDynamicSection.h>
#include <UserManager.h>
#include <ProcFileManager.h>

//#define INIT_NAME "_stdio_init-NOTINUSE"
//#define TERM_NAME "_stdio_term-NOTINUSE"

void Process::Load(const char* szProcessName, unsigned *uiPDEAddress,
                   unsigned* uiEntryAdddress, unsigned* uiProcessEntryStackSize, int iNumberOfParameters, char** szArgumentList)
{
  ELFParser mELFParser(szProcessName) ;

  unsigned uiMinMemAddr, uiMaxMemAddr ;

  mELFParser.GetMemImageSize(&uiMinMemAddr, &uiMaxMemAddr) ;
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

  *uiPDEAddress = AllocateAddressSpace();

  unsigned pRealELFSectionHeadeAddr = DynamicLinkLoader_Initialize(*uiPDEAddress);
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

  CopyElfImage(*uiPDEAddress, bProcessImage.get(), uiMemImageSize);

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

  PushProgramInitStackData(*uiPDEAddress, *uiProcessEntryStackSize, iNumberOfParameters, szArgumentList) ;

  *uiEntryAdddress = mELFParser.GetProgramStartAddress();// uiMinMemAddr + uiProcessImageSize ;
}

void Process::PushProgramInitStackData(unsigned uiPDEAddress, unsigned& uiProcessEntryStackSize, int iNumberOfParameters, char** szArgumentList) {
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

  uiProcessEntryStackSize = argc + argv + uiArgumentAddressListSize + uiArgumentDataSize;

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
}

void Process::DeAllocateAddressSpace()
{
  DeAllocateProcessSpace();
  DeAllocatePTE();
  MemManager::Instance().DeAllocatePhysicalPage(taskState.CR3_PDBR / PAGE_SIZE) ;
}

void Process::CopyElfImage(unsigned uiPDEAddr, byte* bProcessImage, unsigned uiMemImageSize)
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

    memcpy(uiPageAddress - GLOBAL_DATA_SEGMENT_BASE, (unsigned)bProcessImage + uiOffset, upan::min(uiCopySize, (uint32_t)PAGE_SIZE));
    if(uiCopySize <= PAGE_SIZE)
      break;

    uiOffset += PAGE_SIZE ;
    uiCopySize -= PAGE_SIZE ;
  }

  return ;
}

uint32_t Process::AllocateAddressSpace()
{
  const uint32_t uiPDEAddress = AllocatePDE();

  AllocatePTE(uiPDEAddress);
  InitializeProcessSpaceForOS(uiPDEAddress);
  InitializeProcessSpaceForProcess(uiPDEAddress);

  return uiPDEAddress;
}

uint32_t Process::AllocatePDE()
{
  unsigned uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
  unsigned uiPDEAddress = uiFreePageNo * PAGE_SIZE;

  for(unsigned i = 0; i < PAGE_TABLE_ENTRIES; i++)
    ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i] = 0x2;

  return uiPDEAddress;
}

void Process::AllocatePTE(const unsigned uiPDEAddress)
{
  unsigned uiPDEIndex = 0;
  unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;

  for(uint32_t i = 0; i < _noOfPagesForPTE; i++)
  {
    auto uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();

    for(int j = 0; j < PAGE_TABLE_ENTRIES; j++)
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
  for(int j = 0; j < PAGE_TABLE_ENTRIES; j++)
    ((unsigned*)((stackPTE * PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE))[j] = 0x2 ;
  ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[PROCESS_STACK_PDE_ID] = ((stackPTE * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;

  _startPDEForDLL = ++uiPDEIndex ;
}

void Process::InitializeProcessSpaceForOS(const unsigned uiPDEAddress)
{
  for(int i = 0; i < PROCESS_SPACE_FOR_OS; ++i)
  {
    unsigned kernelPTEAddress = (((unsigned*)(MEM_PDBR - GLOBAL_DATA_SEGMENT_BASE))[i]) & 0xFFFFF000;
    unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i]) & 0xFFFFF000 ;
    for(int j = 0; j < PAGE_TABLE_ENTRIES; ++j)
    {
      unsigned kernelPageMapValue = ((unsigned*)(kernelPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[j];
      ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[j] = (kernelPageMapValue & 0xFFFFF000) | 0x5;
    }
  }
}

void Process::InitializeProcessSpaceForProcess(const unsigned uiPDEAddress)
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

void Process::DeAllocateProcessSpace()
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
  for(int i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
    const unsigned pageEntry = (((unsigned*)(stackPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[i]);
    const bool isPresent = pageEntry & 0x1;
    const unsigned pageAddress = pageEntry & 0xFFFFF000;
    if (isPresent && pageAddress) {
      MemManager::Instance().DeAllocatePhysicalPage(pageAddress / PAGE_SIZE);
    }
  }
}

void Process::DeAllocatePTE() {
  const unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;

  for(uint32_t i = 0; i < _noOfPagesForPTE; i++) {
    const uint32_t pdeIndex = (i < PROCESS_SPACE_FOR_OS) ? i : i + uiProcessPDEBase;
    const uint32_t uiPTEAddress = ((unsigned*)(taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE))[pdeIndex] & 0xFFFFF000;
    MemManager::Instance().DeAllocatePhysicalPage(uiPTEAddress / PAGE_SIZE) ;
  }

  unsigned stackPTEAddress = ((unsigned*)(taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE))[PROCESS_STACK_PDE_ID] & 0xFFFFF000;
  MemManager::Instance().DeAllocatePhysicalPage(stackPTEAddress / PAGE_SIZE);
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
