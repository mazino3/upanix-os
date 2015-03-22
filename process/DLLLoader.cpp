/*
 *	Mother Operating System - An x86 based Operating System
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
# include <String.h>
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

using namespace ELFSectionHeader ;
using namespace ELFHeader ;
using namespace ELFRelocSection ;
using namespace ELFSymbolTable ;

int DLLLoader_iNoOfProcessSharedObjectList ;

/************************ static functions *******************************************/

static int DLLLoader_GetFreeProcessDLLEntry()
{
	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;

	unsigned i ;
	for(i = 0; i < DLLLoader_iNoOfProcessSharedObjectList; i++)
	{
		if(pProcessSharedObjectList[i].szName[0] == '\0')
			return i ;
	}

	return -1 ;
}

static unsigned DLLLoader_GetNoOfPagesAllocated()
{
	unsigned uiNoOfPagesAllocated = 0 ;
	unsigned i = 0 ;
	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;

	for(i = 0; pProcessSharedObjectList[i].szName[0] != '\0'; i++)
		uiNoOfPagesAllocated += pProcessSharedObjectList[i].uiNoOfPages ;

	return uiNoOfPagesAllocated ;
}

static void DLLLoader_CopyElfDLLImage(ProcessAddressSpace* processAddressSpace, unsigned uiNoOfPagesForDLL, byte* bDLLImage, unsigned uiMemImageSize)
{
	unsigned i ;
	unsigned uiOffset = 0 ;
	unsigned uiAllocatedPagesCount = DLLLoader_GetNoOfPagesAllocated() ;
	unsigned uiCopySize = uiMemImageSize ;

	unsigned uiStartAddress = processAddressSpace->uiStartPDEForDLL * PAGE_TABLE_ENTRIES * PAGE_SIZE + 
								uiAllocatedPagesCount * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE ;

	for(i = 0; i < uiNoOfPagesForDLL; i++)
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

    unsigned i ;
	for(i = 0; i < DLLLoader_iNoOfProcessSharedObjectList; i++)
	{
		if(String_Compare(pProcessSharedObjectList[i].szName, szDLLName) == 0)
			return i ;
	}

	return -1 ;
}

ProcessSharedObjectList* DLLLoader_GetProcessSharedObjectListByName(const char* szDLLName)
{
	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;

    unsigned i ;
	for(i = 0; i < DLLLoader_iNoOfProcessSharedObjectList; i++)
	{
		if(String_Compare(pProcessSharedObjectList[i].szName, szDLLName) == 0)
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
	unsigned i ;
	for(i = 0; i < DLLLoader_iNoOfProcessSharedObjectList; i++)
	{
		if(String_Compare(pProcessSharedObjectList[i].szName, szDLLName) == 0)
			return iRelDLLStartAddress ;

		iRelDLLStartAddress += (pProcessSharedObjectList[i].uiNoOfPages * PAGE_SIZE) ;
	}

	return -1 ;
}

void DLLLoader_Initialize()
{
	DLLLoader_iNoOfProcessSharedObjectList = PAGE_SIZE / sizeof(ProcessSharedObjectList) ;
}

byte DLLLoader_LoadELFDLL(const char* szDLLName, const char* szJustDLLName, ProcessAddressSpace* processAddressSpace)
{
	ELFParser mELFParser(szDLLName) ;

	if(!mELFParser.GetState())
		return DLLLoader_FAILURE ;

	int iProcessDLLEntryIndex = 0 ;

	if((iProcessDLLEntryIndex = DLLLoader_GetFreeProcessDLLEntry()) == -1)
		return DLLLoader_ERR_PROCESS_DLL_LIMIT_EXCEEDED ;

	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;
		
	unsigned uiMinMemAddr, uiMaxMemAddr ;

	mELFParser.GetMemImageSize(&uiMinMemAddr, &uiMaxMemAddr) ;
	if(uiMinMemAddr != 0)
		return DLLLoader_ERR_NOT_PIC ;

	byte bStatus ;
	unsigned uiDLLSectionSize ;
	byte* bDLLSectionImage = NULL ;

	RETURN_IF_NOT(bStatus, ProcessLoader_LoadInitSections(processAddressSpace, &uiDLLSectionSize, &bDLLSectionImage, PROCESS_DLL_FILE), ProcessLoader_SUCCESS) ;

	unsigned uiDLLImageSize = ProcessLoader_GetCeilAlignedAddress(uiMaxMemAddr - uiMinMemAddr, 4) ;
	unsigned uiMemImageSize = uiDLLImageSize + uiDLLSectionSize ;
	unsigned uiNoOfPagesForDLL = KC::MMemManager().GetProcessSizeInPages(uiMemImageSize) + DLL_ELF_SEC_HEADER_PAGE ;

	if(uiNoOfPagesForDLL > MAX_PAGES_PER_PROCESS)
	{
		DMM_DeAllocateForKernel((unsigned)bDLLSectionImage) ;
		return DLLLoader_ERR_HUGE_DLL_SIZE ;
	}

	unsigned uiNoOfPagesAllocatedForOtherDLLs = DLLLoader_GetNoOfPagesAllocated() ;

	if(!KC::MKernelService().RequestDLLAlloCopy(iProcessDLLEntryIndex, uiNoOfPagesAllocatedForOtherDLLs, uiNoOfPagesForDLL))
	{
		DMM_DeAllocateForKernel((unsigned)bDLLSectionImage) ;

		unsigned i ;
		for(i = 0; i < pProcessSharedObjectList[iProcessDLLEntryIndex].uiNoOfPages; i++)
			KC::MMemManager().DeAllocatePhysicalPage(pProcessSharedObjectList[iProcessDLLEntryIndex].uiAllocatedPageNumbers[i]) ;

		return DLLLoader_FAILURE ;
	}

	unsigned uiDLLLoadAddress = (processAddressSpace->uiStartPDEForDLL * PAGE_SIZE * PAGE_TABLE_ENTRIES) + uiNoOfPagesAllocatedForOtherDLLs * PAGE_SIZE - PROCESS_BASE ;

	// Last Page is for Elf Section Header
	unsigned a = (pProcessSharedObjectList[iProcessDLLEntryIndex].uiNoOfPages - 1) * PAGE_SIZE ;
	unsigned b = uiNoOfPagesAllocatedForOtherDLLs * PAGE_SIZE ;
	unsigned c = processAddressSpace->uiStartPDEForDLL * PAGE_TABLE_ENTRIES * PAGE_SIZE ;
	unsigned pRealELFSectionHeaderAddr = a + b + c - GLOBAL_DATA_SEGMENT_BASE ;

	unsigned uiCopySize = mELFParser.CopyELFSectionHeader((ELF32SectionHeader*)pRealELFSectionHeaderAddr) ;
	unsigned uiCopySize1 = mELFParser.CopyELFSecStrTable((char*)(pRealELFSectionHeaderAddr + uiCopySize)) ;

	byte* bDLLImage = (byte*)DMM_AllocateForKernel(sizeof(char) * uiMemImageSize) ;

	if(!mELFParser.CopyProcessImage(bDLLImage, 0, uiMemImageSize))
	{
		DMM_DeAllocateForKernel((unsigned)bDLLImage) ;
		DMM_DeAllocateForKernel((unsigned)bDLLSectionImage) ;

		unsigned i ;
		for(i = 0; i < pProcessSharedObjectList[iProcessDLLEntryIndex].uiNoOfPages; i++)
			KC::MMemManager().DeAllocatePhysicalPage(pProcessSharedObjectList[iProcessDLLEntryIndex].uiAllocatedPageNumbers[i]) ;

		return DLLLoader_FAILURE ;
	}

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)bDLLSectionImage, MemUtil_GetDS(), (unsigned)(bDLLImage + uiDLLImageSize), uiDLLSectionSize) ;

	// Setting the Dynamic Link Loader Address in GOT
	unsigned* uiGOT = mELFParser.GetGOTAddress(bDLLImage, uiMinMemAddr) ;
	if(uiGOT != NULL)
	{
		uiGOT[1] = iProcessDLLEntryIndex ;
		uiGOT[2] = uiDLLImageSize + uiDLLLoadAddress ;
	}

	unsigned uiNoOfGOTEntries ;
	if(mELFParser.GetNoOfGOTEntries(&uiNoOfGOTEntries))
	{
		unsigned i ;
		for(i = 3; i < uiNoOfGOTEntries; i++)
			uiGOT[i] += uiDLLLoadAddress ;
	}

/* Dynamic Relocation Entries are resolved here in Global Offset Table */

	ELF32SectionHeader* pRelocationSectionHeader = NULL ;
	if(mELFParser.GetSectionHeaderByTypeAndName(SHT_REL, REL_DYN_SUB_NAME, &pRelocationSectionHeader))
	{
		ELF32SectionHeader* pDynamicSymSectiomHeader = NULL ;
		RETURN_X_IF_NOT(mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_link, &pDynamicSymSectiomHeader), true, DynamicLinkLoader_FAILURE) ;

		ELF32_Rel* pELFDynRelTable = (ELF32_Rel*)((unsigned)bDLLImage + pRelocationSectionHeader->sh_addr) ;
		unsigned uiNoOfDynRelEntries = pRelocationSectionHeader->sh_size / pRelocationSectionHeader->sh_entsize ;

		ELF32SymbolEntry* pELFDynSymTable = (ELF32SymbolEntry*)((unsigned)bDLLImage + pDynamicSymSectiomHeader->sh_addr) ;

		unsigned i ;
		unsigned uiRelType ;

		for(i = 0; i < uiNoOfDynRelEntries; i++)
		{
			uiRelType = ELF32_R_TYPE(pELFDynRelTable[i].r_info) ;

			if(uiRelType == R_386_RELATIVE)
			{
				((unsigned*)((unsigned)bDLLImage + pELFDynRelTable[i].r_offset))[0] += uiDLLLoadAddress ;
			}
			else if(uiRelType == R_386_GLOB_DAT)
			{
				((unsigned*)((unsigned)bDLLImage + pELFDynRelTable[i].r_offset))[0] = 
					pELFDynSymTable[ELF32_R_SYM(pELFDynRelTable[i].r_info)].st_value + uiDLLLoadAddress ;
			}
		}
	}

/* Enf of Dynamic Relocation Entries resolution */

	DLLLoader_CopyElfDLLImage(processAddressSpace, pProcessSharedObjectList[iProcessDLLEntryIndex].uiNoOfPages, bDLLImage, uiMemImageSize) ;

	String_Copy(pProcessSharedObjectList[iProcessDLLEntryIndex].szName, szJustDLLName) ;

	DMM_DeAllocateForKernel((unsigned)bDLLImage) ;
	DMM_DeAllocateForKernel((unsigned)bDLLSectionImage) ;

	return DLLLoader_SUCCESS ;
}

void DLLLoader_DeAllocateProcessDLLPTEPages(ProcessAddressSpace* processAddressSpace)
{
	unsigned i, uiPTEAddress, uiPresentBit;
	unsigned* uiPDEAddress = ((unsigned*)(processAddressSpace->taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE));

	for(i = 0; i < processAddressSpace->uiNoOfPagesForDLLPTE; i++)
	{
		uiPresentBit = uiPDEAddress[i + processAddressSpace->uiStartPDEForDLL] & 0x1 ;
		uiPTEAddress = uiPDEAddress[i + processAddressSpace->uiStartPDEForDLL] & 0xFFFFF000 ;

		if(uiPresentBit && uiPDEAddress != 0)
			KC::MMemManager().DeAllocatePhysicalPage(uiPTEAddress / PAGE_SIZE) ;
	}
}

unsigned DLLLoader_GetProcessDLLLoadAddress(ProcessAddressSpace* processAddressSpace, int iIndex)
{
	ProcessSharedObjectList* pProcessSharedObjectList = (ProcessSharedObjectList*)(PROCESS_DLL_PAGE_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;

	unsigned uiAllocatedPagesCount = 0 ;
	int i ;
	for(i = 0; i < iIndex; i++)
		uiAllocatedPagesCount += pProcessSharedObjectList[i].uiNoOfPages ;

	return (processAddressSpace->uiStartPDEForDLL * PAGE_SIZE * PAGE_TABLE_ENTRIES) + (uiAllocatedPagesCount * PAGE_SIZE) ;
}

byte DLLLoader_MapDLLPagesToProcess(ProcessAddressSpace* processAddressSpace, 
												ProcessSharedObjectList* pProcessSharedObjectList,
												unsigned uiAllocatedPagesCount)
{
	unsigned uiNoOfPagesAllocatedForDLL = uiAllocatedPagesCount + pProcessSharedObjectList->uiNoOfPages ;
	unsigned uiNoOfPagesForDLLPTE = KC::MMemManager().GetPTESizeInPages(uiNoOfPagesAllocatedForDLL) ;
	
	unsigned uiPDEAddress = processAddressSpace->taskState.CR3_PDBR ;
	unsigned i, uiFreePageNo, uiPDEIndex, uiPTEIndex ;

	if(uiNoOfPagesForDLLPTE > processAddressSpace->uiNoOfPagesForDLLPTE)
	{
		for(i = processAddressSpace->uiNoOfPagesForDLLPTE; i < uiNoOfPagesForDLLPTE; i++)
		{
			RETURN_X_IF_NOT(KC::MMemManager().AllocatePhysicalPage(&uiFreePageNo), MEM_SUCCESS, DLLLoader_FAILURE) ;
			
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

unsigned DLLLoader_GetProcessDLLPageAdrressForKernel(ProcessAddressSpace* processAddressSpace)
{
	unsigned uiPDEAddress = processAddressSpace->taskState.CR3_PDBR ;
	unsigned uiPDEIndex = ((PROCESS_DLL_PAGE_ADDR >> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((PROCESS_DLL_PAGE_ADDR >> 12) & 0x3FF) ;
	unsigned uiPTEAddress = ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] & 0xFFFFF000 ;
	unsigned uiPageNumber = (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] & 0xFFFFF000) / PAGE_SIZE ;

	return (uiPageNumber * PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE ;
}

