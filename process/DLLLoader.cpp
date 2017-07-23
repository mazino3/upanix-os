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
# include <DLLLoader.h>
# include <MemConstants.h>
# include <DynamicLinkLoader.h>
# include <DMM.h>
# include <StringUtil.h>
# include <MemManager.h>
# include <MemUtil.h>
# include <ElfParser.h>
# include <ProcessLoader.h>
# include <ProcessAllocator.h>
# include <Display.h>
# include <KernelService.h>
# include <ElfSectionHeader.h>
# include <ElfConstants.h>
# include <ElfRelocationSection.h>
# include <ElfSymbolTable.h>
# include <ElfDynamicSection.h>
# include <exception.h>
# include <result.h>
# include <uniq_ptr.h>
# include <try.h>

using namespace ELFSectionHeader ;
using namespace ELFHeader ;
using namespace ELFRelocSection ;
using namespace ELFSymbolTable ;

int DLLLoader_iNoOfProcessSharedObjectList ;

/************************ static functions *******************************************/

static int DLLLoader_GetFreeProcessDLLEntry()
{
	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;

	for(int i = 0; i < DLLLoader_iNoOfProcessSharedObjectList; i++)
		if(pProcessSharedObjectList[i].szName[0] == '\0')
			return i ;

  throw upan::exception(XLOC, "out of memory for process dll entries");
}

static unsigned DLLLoader_GetNoOfPagesAllocated()
{
	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;

  unsigned uiNoOfPagesAllocated = 0;
  for(uint32_t i = 0; pProcessSharedObjectList[i].szName[0] != '\0'; i++)
		uiNoOfPagesAllocated += pProcessSharedObjectList[i].uiNoOfPages ;

	return uiNoOfPagesAllocated ;
}

static void DLLLoader_CopyElfDLLImage(Process* processAddressSpace, unsigned uiNoOfPagesForDLL, byte* bDLLImage, unsigned uiMemImageSize)
{
	unsigned uiOffset = 0 ;
	unsigned uiAllocatedPagesCount = DLLLoader_GetNoOfPagesAllocated() ;
	unsigned uiCopySize = uiMemImageSize ;

	unsigned uiStartAddress = processAddressSpace->uiStartPDEForDLL * PAGE_TABLE_ENTRIES * PAGE_SIZE + 
								uiAllocatedPagesCount * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE ;

  for(uint32_t i = 0; i < uiNoOfPagesForDLL; i++)
	{
		if(uiCopySize <= PAGE_SIZE)
		{
			MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)(bDLLImage + uiOffset), MemUtil_GetDS(), 
						uiStartAddress, uiCopySize) ;
			return ;
		}
		else
		{
			MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)(bDLLImage + uiOffset), MemUtil_GetDS(), 
						uiStartAddress, PAGE_SIZE) ;
		}

		uiStartAddress += PAGE_SIZE ;
		uiOffset += PAGE_SIZE ;
		uiCopySize -= PAGE_SIZE ;
	}
}

/**************************************************************************************************/

int DLLLoader_GetProcessSharedObjectListIndexByName(const char* szDLLName)
{
	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;
	for(int i = 0; i < DLLLoader_iNoOfProcessSharedObjectList; i++)
		if(strcmp(pProcessSharedObjectList[i].szName, szDLLName) == 0)
      return i;
  throw upan::exception(XLOC, "Failed to find entry for shared object (.so) file: %s", szDLLName);
}

ProcessSharedObjectList* DLLLoader_GetProcessSharedObjectListByName(const char* szDLLName)
{
	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;

	for(int i = 0; i < DLLLoader_iNoOfProcessSharedObjectList; i++)
	{
		if(strcmp(pProcessSharedObjectList[i].szName, szDLLName) == 0)
			return &pProcessSharedObjectList[i] ;
	}

	return NULL ;
}

ProcessSharedObjectList* DLLLoader_GetProcessSharedObjectListByIndex(int iIndex)
{
	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;

	if(iIndex >= 0 && iIndex < DLLLoader_iNoOfProcessSharedObjectList && pProcessSharedObjectList[iIndex].szName[0] != '\0')
		return &pProcessSharedObjectList[iIndex] ;

	return NULL ;
}

int DLLLoader_GetRelativeDLLStartAddress(const char* szDLLName)
{
	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;

  int iRelDLLStartAddress = 0 ;
	for(int i = 0; i < DLLLoader_iNoOfProcessSharedObjectList; i++)
	{
		if(strcmp(pProcessSharedObjectList[i].szName, szDLLName) == 0)
			return iRelDLLStartAddress ;

		iRelDLLStartAddress += (pProcessSharedObjectList[i].uiNoOfPages * PAGE_SIZE) ;
	}

	return -1 ;
}

void DLLLoader_Initialize()
{
	DLLLoader_iNoOfProcessSharedObjectList = PAGE_SIZE / sizeof(ProcessSharedObjectList) ;
}

void DLLLoader_LoadELFDLL(const char* szDLLName, const char* szJustDLLName, Process* processAddressSpace)
{
	ELFParser mELFParser(szDLLName) ;

  const int iProcessDLLEntryIndex = DLLLoader_GetFreeProcessDLLEntry();

	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;
		
	unsigned uiMinMemAddr, uiMaxMemAddr ;

	mELFParser.GetMemImageSize(&uiMinMemAddr, &uiMaxMemAddr) ;
	if(uiMinMemAddr != 0)
    throw upan::exception(XLOC, "Not a PIC - DLL Min Address: %x", uiMinMemAddr);

	unsigned uiDLLSectionSize ;
  upan::uniq_ptr<byte[]> bDLLSectionImage(ProcessLoader::Instance().LoadDLLInitSection(uiDLLSectionSize));

	unsigned uiDLLImageSize = ProcessLoader_GetCeilAlignedAddress(uiMaxMemAddr - uiMinMemAddr, 4) ;
	unsigned uiMemImageSize = uiDLLImageSize + uiDLLSectionSize ;
	unsigned uiNoOfPagesForDLL = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize) + DLL_ELF_SEC_HEADER_PAGE ;

	if(uiNoOfPagesForDLL > MAX_PAGES_PER_PROCESS)
    throw upan::exception(XLOC, "No. of pages for DLL %u exceeds max limit per dll %u", uiNoOfPagesForDLL, MAX_PAGES_PER_PROCESS);

	unsigned uiNoOfPagesAllocatedForOtherDLLs = DLLLoader_GetNoOfPagesAllocated() ;

	if(!KC::MKernelService().RequestDLLAlloCopy(iProcessDLLEntryIndex, uiNoOfPagesAllocatedForOtherDLLs, uiNoOfPagesForDLL))
	{
    for(uint32_t i = 0; i < pProcessSharedObjectList[iProcessDLLEntryIndex].uiNoOfPages; i++)
			MemManager::Instance().DeAllocatePhysicalPage(pProcessSharedObjectList[iProcessDLLEntryIndex].uiAllocatedPageNumbers[i]) ;
    throw upan::exception(XLOC, "Failed to allocate memory for DLL via kernal service");
	}

	unsigned uiDLLLoadAddress = (processAddressSpace->uiStartPDEForDLL * PAGE_SIZE * PAGE_TABLE_ENTRIES) + uiNoOfPagesAllocatedForOtherDLLs * PAGE_SIZE - PROCESS_BASE ;

	// Last Page is for Elf Section Header
	unsigned a = (pProcessSharedObjectList[iProcessDLLEntryIndex].uiNoOfPages - 1) * PAGE_SIZE ;
	unsigned b = uiNoOfPagesAllocatedForOtherDLLs * PAGE_SIZE ;
	unsigned c = processAddressSpace->uiStartPDEForDLL * PAGE_TABLE_ENTRIES * PAGE_SIZE ;
	unsigned pRealELFSectionHeaderAddr = a + b + c - GLOBAL_DATA_SEGMENT_BASE ;

	unsigned uiCopySize = mELFParser.CopyELFSectionHeader((ELF32SectionHeader*)pRealELFSectionHeaderAddr) ;
	mELFParser.CopyELFSecStrTable((char*)(pRealELFSectionHeaderAddr + uiCopySize)) ;

  upan::uniq_ptr<byte[]> bDLLImage(new byte[sizeof(char) * uiMemImageSize]);

  upan::tryresult([&] () { mELFParser.CopyProcessImage(bDLLImage.get(), 0, uiMemImageSize); }).badMap([&] (const upan::error& err) {
    for(unsigned i = 0; i < pProcessSharedObjectList[iProcessDLLEntryIndex].uiNoOfPages; i++)
      MemManager::Instance().DeAllocatePhysicalPage(pProcessSharedObjectList[iProcessDLLEntryIndex].uiAllocatedPageNumbers[i]) ;
    throw upan::exception(XLOC, err);
  });

  MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)bDLLSectionImage.get(), MemUtil_GetDS(), (unsigned)(bDLLImage.get() + uiDLLImageSize), uiDLLSectionSize) ;

	// Setting the Dynamic Link Loader Address in GOT
  mELFParser.GetGOTAddress(bDLLImage.get(), uiMinMemAddr).goodMap([&](uint32_t* uiGOT) {
    uiGOT[1] = iProcessDLLEntryIndex ;
    uiGOT[2] = uiDLLImageSize + uiDLLLoadAddress ;

    mELFParser.GetNoOfGOTEntries().goodMap([&](uint32_t uiNoOfGOTEntries) {
      for(uint32_t i = 3; i < uiNoOfGOTEntries; i++)
        uiGOT[i] += uiDLLLoadAddress ;
    });
  });

/* Dynamic Relocation Entries are resolved here in Global Offset Table */
  mELFParser.GetSectionHeaderByTypeAndName(SHT_REL, REL_DYN_SUB_NAME).goodMap([&] (ELF32SectionHeader* pRelocationSectionHeader)
  {
    ELF32SectionHeader* pDynamicSymSectiomHeader = mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_link).goodValueOrThrow(XLOC);

    ELF32_Rel* pELFDynRelTable = (ELF32_Rel*)((unsigned)bDLLImage.get() + pRelocationSectionHeader->sh_addr) ;
    unsigned uiNoOfDynRelEntries = pRelocationSectionHeader->sh_size / pRelocationSectionHeader->sh_entsize ;

    ELF32SymbolEntry* pELFDynSymTable = (ELF32SymbolEntry*)((unsigned)bDLLImage.get() + pDynamicSymSectiomHeader->sh_addr) ;

    unsigned i ;
    unsigned uiRelType ;

    for(i = 0; i < uiNoOfDynRelEntries; i++)
    {
      uiRelType = ELF32_R_TYPE(pELFDynRelTable[i].r_info) ;

      if(uiRelType == R_386_RELATIVE)
      {
        ((unsigned*)((unsigned)bDLLImage.get() + pELFDynRelTable[i].r_offset))[0] += uiDLLLoadAddress ;
      }
      else if(uiRelType == R_386_GLOB_DAT)
      {
        ((unsigned*)((unsigned)bDLLImage.get() + pELFDynRelTable[i].r_offset))[0] =
          pELFDynSymTable[ELF32_R_SYM(pELFDynRelTable[i].r_info)].st_value + uiDLLLoadAddress ;
      }
    }
  });

/* Enf of Dynamic Relocation Entries resolution */
  DLLLoader_CopyElfDLLImage(processAddressSpace, pProcessSharedObjectList[iProcessDLLEntryIndex].uiNoOfPages, bDLLImage.get(), uiMemImageSize) ;
	strcpy(pProcessSharedObjectList[iProcessDLLEntryIndex].szName, szJustDLLName) ;
}

void DLLLoader_DeAllocateProcessDLLPTEPages(Process* processAddressSpace)
{
	unsigned i, uiPTEAddress, uiPresentBit;
	unsigned* uiPDEAddress = ((unsigned*)(processAddressSpace->taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE));

	for(i = 0; i < processAddressSpace->uiNoOfPagesForDLLPTE; i++)
	{
		uiPresentBit = uiPDEAddress[i + processAddressSpace->uiStartPDEForDLL] & 0x1 ;
		uiPTEAddress = uiPDEAddress[i + processAddressSpace->uiStartPDEForDLL] & 0xFFFFF000 ;

		if(uiPresentBit && uiPDEAddress != 0)
			MemManager::Instance().DeAllocatePhysicalPage(uiPTEAddress / PAGE_SIZE) ;
	}
}

unsigned DLLLoader_GetProcessDLLLoadAddress(Process* processAddressSpace, int iIndex)
{
	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;

	unsigned uiAllocatedPagesCount = 0 ;
	int i ;
	for(i = 0; i < iIndex; i++)
		uiAllocatedPagesCount += pProcessSharedObjectList[i].uiNoOfPages ;

	return (processAddressSpace->uiStartPDEForDLL * PAGE_SIZE * PAGE_TABLE_ENTRIES) + (uiAllocatedPagesCount * PAGE_SIZE) ;
}

byte DLLLoader_MapDLLPagesToProcess(Process* processAddressSpace,
												ProcessSharedObjectList* pProcessSharedObjectList,
												unsigned uiAllocatedPagesCount)
{
	unsigned uiNoOfPagesAllocatedForDLL = uiAllocatedPagesCount + pProcessSharedObjectList->uiNoOfPages ;
	unsigned uiNoOfPagesForDLLPTE = MemManager::Instance().GetPTESizeInPages(uiNoOfPagesAllocatedForDLL) ;
	
	unsigned uiPDEAddress = processAddressSpace->taskState.CR3_PDBR ;
	unsigned i, uiFreePageNo, uiPDEIndex, uiPTEIndex ;

	if(uiNoOfPagesForDLLPTE > processAddressSpace->uiNoOfPagesForDLLPTE)
	{
		for(i = processAddressSpace->uiNoOfPagesForDLLPTE; i < uiNoOfPagesForDLLPTE; i++)
		{
      uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
			uiPDEIndex = processAddressSpace->uiStartPDEForDLL + i ;
			((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] = 
															((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;
		}
	}

	unsigned uiPTEAddress ;
	for(i = 0; i < pProcessSharedObjectList->uiNoOfPages; i++)
	{
		uiPDEIndex = (i + uiAllocatedPagesCount) / PAGE_TABLE_ENTRIES + processAddressSpace->uiStartPDEForDLL ;
		uiPTEIndex = (i + uiAllocatedPagesCount) % PAGE_TABLE_ENTRIES ;

		uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;

		((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = 
						(((pProcessSharedObjectList->uiAllocatedPageNumbers[i]) * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;
	}

	processAddressSpace->uiNoOfPagesForDLLPTE = uiNoOfPagesForDLLPTE ;
	return DLLLoader_SUCCESS ;
}

unsigned DLLLoader_GetProcessDLLPageAdrressForKernel(Process* processAddressSpace)
{
	unsigned uiPDEAddress = processAddressSpace->taskState.CR3_PDBR ;
	unsigned uiPDEIndex = ((PROCESS_DLL_PAGE_ADDR >> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((PROCESS_DLL_PAGE_ADDR >> 12) & 0x3FF) ;
	unsigned uiPTEAddress = ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] & 0xFFFFF000 ;
	unsigned uiPageNumber = (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] & 0xFFFFF000) / PAGE_SIZE ;

	return (uiPageNumber * PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE ;
}

