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
#include <ProcessManager.h>
#include <DynamicLinkLoader.h>
#include <MemManager.h>
#include <ElfSectionHeader.h>
#include <ElfConstants.h>
#include <ElfParser.h>
#include <ElfRelocationSection.h>
#include <ElfSymbolTable.h>
#include <ElfDynamicSection.h>
#include <Display.h>
#include <DMM.h>
#include <DLLLoader.h>
#include <ProcessLoader.h>
#include <StringUtil.h>
#include <MemUtil.h>
#include <ProcFileManager.h>
#include <FileOperations.h>
#include <BufferedReader.h>
#include <ProcessEnv.h>
#include <MountManager.h>
#include <GenericUtil.h>
#include <uniq_ptr.h>

using namespace ELFSectionHeader ;
using namespace ELFHeader ;
using namespace ELFRelocSection ;
using namespace ELFSymbolTable ;
using namespace ELFDynamicSection ;

/****************************** Static Functions *******************************************/
static unsigned DynamicLinkLoader_GetHashValue(const char* name)
{
    unsigned h = 0, g ;

    while(*name)
    {
        h = (h << 4) + *name++ ;

        if((g = (h & 0xf0000000)))
            h ^= g >> 24 ;

        h &= ~g ;
    }

    return h ;
}

static byte DynamicLinkLoader_LoadDLL(const char* szJustDLLName, ProcessAddressSpace* processAddressSpace)
{
	ProcessSharedObjectList* pProcessSharedObjectList = DLLLoader_GetProcessSharedObjectListByName(szJustDLLName) ;

	if(pProcessSharedObjectList == NULL)
	{
		char szDLLFullName[128] ;
		char szLibPath[128] = "" ;

		if(!GenericUtil_GetFullFilePathFromEnv(LD_LIBRARY_PATH_ENV, LIB_PATH, szJustDLLName, szLibPath))
		{
			return DynamicLinkLoader_ERR_LIB_EFOUND ;
		}

		strcpy(szDLLFullName, szLibPath) ;
		strcat(szDLLFullName, szJustDLLName) ;
		
		byte bStatus = DLLLoader_LoadELFDLL(szDLLFullName, szJustDLLName, processAddressSpace) ;
		return (bStatus != DLLLoader_SUCCESS) ? DynamicLinkLoader_FAILURE : DynamicLinkLoader_SUCCESS ;
	}

	return DynamicLinkLoader_SUCCESS ;
}

static byte* DynamicLinkLoader_LoadDLLFileIntoMemory(const ELFParser& elfParser)
{
	unsigned uiMinMemAddr, uiMaxMemAddr ;
  elfParser.GetMemImageSize(&uiMinMemAddr, &uiMaxMemAddr) ;
	if(uiMinMemAddr != 0)
    throw upan::exception(XLOC, "Not a PIC");

	unsigned uiMemImageSize = ProcessLoader_GetCeilAlignedAddress(uiMaxMemAddr - uiMinMemAddr, 4) ;

  upan::uniq_ptr<byte[]> dllImage(new byte[uiMemImageSize]);

  if(!elfParser.CopyProcessImage(dllImage.get(), 0, uiMemImageSize))
    throw upan::exception(XLOC, "Failed to load/copy dll file into memory");

  return dllImage.release();
}

/**********************************************************************************************/

byte DynamicLinkLoader_Initialize(unsigned* pRealELFSectionHeadeAddr, unsigned uiPDEAddress)
{
	unsigned uiFreePageNo ;

  uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
	ProcessSharedObjectList* pSharedObjectList = (ProcessSharedObjectList*)(uiFreePageNo * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE) ;

	for(int i = 0; i < DLLLoader_iNoOfProcessSharedObjectList; i++)
		pSharedObjectList[i].szName[0] = '\0' ;

	unsigned uiPDEIndex = ((PROCESS_DLL_PAGE_ADDR >> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((PROCESS_DLL_PAGE_ADDR >> 12) & 0x3FF) ;

	unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;

	((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x3 ;

	uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
	*pRealELFSectionHeadeAddr = (uiFreePageNo * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE) ;

	uiPDEIndex = ((PROCESS_SEC_HEADER_ADDR >> 22) & 0x3FF) ;
	uiPTEIndex = ((PROCESS_SEC_HEADER_ADDR >> 12) & 0x3FF) ;

	uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;

	((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x3 ;

	Mem_FlushTLB();
	return DynamicLinkLoader_SUCCESS ;
}

void DynamicLinkLoader_UnInitialize(ProcessAddressSpace* processAddressSpace)
{
	unsigned uiPDEAddress = processAddressSpace->taskState.CR3_PDBR ;
	unsigned uiPDEIndex = ((PROCESS_DLL_PAGE_ADDR >> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((PROCESS_DLL_PAGE_ADDR >> 12) & 0x3FF) ;
	unsigned uiPTEAddress = ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] & 0xFFFFF000 ;
	unsigned uiPageNumber = (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] & 0xFFFFF000) / PAGE_SIZE ;

	ProcessSharedObjectList* pSharedObjectList = (ProcessSharedObjectList*)((uiPageNumber * PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE) ;

	for(int i = 0; i < DLLLoader_iNoOfProcessSharedObjectList; i++)
	{
		if(pSharedObjectList[i].szName[0] != '\0')
		{
			for(unsigned j = 0; j < pSharedObjectList[i].uiNoOfPages; j++)
			{
				MemManager::Instance().DeAllocatePhysicalPage(pSharedObjectList[i].uiAllocatedPageNumbers[j]) ;
			}
			DMM_DeAllocateForKernel((unsigned)pSharedObjectList[i].uiAllocatedPageNumbers) ;
		}
		else
			break ;
	}
	MemManager::Instance().DeAllocatePhysicalPage(uiPageNumber) ;

	uiPDEIndex = ((PROCESS_SEC_HEADER_ADDR >> 22) & 0x3FF) ;
	uiPTEIndex = ((PROCESS_SEC_HEADER_ADDR >> 12) & 0x3FF) ;
	uiPTEAddress = ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] & 0xFFFFF000 ;
	uiPageNumber = (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] & 0xFFFFF000) / PAGE_SIZE ;
	MemManager::Instance().DeAllocatePhysicalPage(uiPageNumber) ;
}

byte DynamicLinkLoader_DoRelocation(ProcessAddressSpace* processAddressSpace, int iID, unsigned uiRelocationOffset, __volatile__ int* iDynamicSymAddress)
{
	ELF32Header* pELFHeader ;
	ELF32SectionHeader* pELFSectionHeader ;
	char* pSecHeaderStrTable ;
	
	ELF32Header* pProcessELFHeader ;
	ELF32SectionHeader* pProcessELFSectionHeader ;
	char* pProcessSecHeaderStrTable ;

	unsigned uiBaseAddress ;

	if(iID >= 0)
	{
		ProcessSharedObjectList* pProcessSharedObjectList = DLLLoader_GetProcessSharedObjectListByIndex(iID) ;
		uiBaseAddress = DLLLoader_GetProcessDLLLoadAddress(processAddressSpace, iID) ;

		pELFHeader = (ELF32Header*)(uiBaseAddress - GLOBAL_DATA_SEGMENT_BASE) ;
		pELFSectionHeader = (ELF32SectionHeader*)(uiBaseAddress - GLOBAL_DATA_SEGMENT_BASE + (pProcessSharedObjectList->uiNoOfPages - 1) * PAGE_SIZE) ;
	}
	else
	{
		uiBaseAddress = PROCESS_BASE ;
		pELFHeader = (ELF32Header*)(GLOBAL_REL_ADDR(processAddressSpace->uiProcessBase, uiBaseAddress)) ;
		pELFSectionHeader = (ELF32SectionHeader*)(PROCESS_SEC_HEADER_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;
	}

	pSecHeaderStrTable = (char*)((unsigned)pELFSectionHeader + (pELFHeader->e_shnum * sizeof(ELF32SectionHeader))) ;

	ELFParser mELFParser(pELFHeader, pELFSectionHeader, pSecHeaderStrTable) ;

  ELF32SectionHeader* pRelocationSectionHeader = mELFParser.GetSectionHeaderByTypeAndName(SHT_REL, REL_PLT_SUB_NAME).goodValueOrThrow(XLOC);
  ELF32SectionHeader* pDynamicSymSectionHeader = mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_link).goodValueOrThrow(XLOC);
  ELF32SectionHeader* pProcedureLinkSectionHeader = mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_info).goodValueOrThrow(XLOC);
  ELF32SectionHeader* pDynamicSymStringSectionHeader = mELFParser.GetSectionHeaderByIndex(pDynamicSymSectionHeader->sh_link).goodValueOrThrow(XLOC);

	ELF32_Rel* pELFRelTable = (ELF32_Rel*)(GLOBAL_REL_ADDR(pRelocationSectionHeader->sh_addr, uiBaseAddress)) ;

	ELF32SymbolEntry* pELFDynSymTable = (ELF32SymbolEntry*)(GLOBAL_REL_ADDR(pDynamicSymSectionHeader->sh_addr, uiBaseAddress)) ;
	const char* pDynStrTable = (const char*)(GLOBAL_REL_ADDR(pDynamicSymStringSectionHeader->sh_addr, uiBaseAddress)) ;

	unsigned uiSymIndex = ELF32_R_SYM(ELF32_REL_ENT(pELFRelTable, uiRelocationOffset)->r_info) ;
	unsigned uiSymStrIndex = pELFDynSymTable[uiSymIndex].st_name ;
	char* szSymName = (char*)&pDynStrTable[uiSymStrIndex] ;

	const char* pProcessDynStrTable ;

	pProcessELFHeader = (ELF32Header*)(GLOBAL_REL_ADDR(processAddressSpace->uiProcessBase, PROCESS_BASE)) ;
	pProcessELFSectionHeader = (ELF32SectionHeader*)(PROCESS_SEC_HEADER_ADDR - GLOBAL_DATA_SEGMENT_BASE) ;
	pProcessSecHeaderStrTable = (char*)((unsigned)pProcessELFSectionHeader + (pProcessELFHeader->e_shnum * sizeof(ELF32SectionHeader))) ;

	ELFParser mProgELFParser(pProcessELFHeader, pProcessELFSectionHeader, pProcessSecHeaderStrTable) ;

	if(iID >= 0)
	{
    ELF32SectionHeader* pProcRelocSectionHeader = mProgELFParser.GetSectionHeaderByTypeAndName(SHT_REL, REL_PLT_SUB_NAME).goodValueOrThrow(XLOC);
    ELF32SectionHeader* pProcDynamicSymSecHeader = mProgELFParser.GetSectionHeaderByIndex(pProcRelocSectionHeader->sh_link).goodValueOrThrow(XLOC);
    ELF32SectionHeader* pProcDynamicSymStringSecHeader = mProgELFParser.GetSectionHeaderByIndex(pProcDynamicSymSecHeader->sh_link).goodValueOrThrow(XLOC);
		pProcessDynStrTable = (const char*)(GLOBAL_REL_ADDR(pProcDynamicSymStringSecHeader->sh_addr, PROCESS_BASE)) ;
	}
	else
	{
		pProcessDynStrTable = pDynStrTable ;
	}

	ELF32SectionHeader* pDynamicSectionHeader = NULL ;
	RETURN_X_IF_NOT(mProgELFParser.GetSectionHeaderByType(SHT_DYNAMIC, &pDynamicSectionHeader), true, DynamicLinkLoader_FAILURE) ;
	
	unsigned uiIndex, uiNoOfEntries = pDynamicSectionHeader->sh_size / pDynamicSectionHeader->sh_entsize ;
	Elf32_Dyn* pELFDynSection = (Elf32_Dyn*)(GLOBAL_REL_ADDR(pDynamicSectionHeader->sh_addr, PROCESS_BASE)) ;

  for(uiIndex = 0; uiIndex < uiNoOfEntries; uiIndex++)
	{
		if(pELFDynSection[uiIndex].d_tag == DT_NEEDED) // TODO: Maintain a Map of tagID and tagType
		{
			unsigned uiDynSymOffset ;
			char* szDLLName = (char*)&pProcessDynStrTable[ pELFDynSection[uiIndex].d_un.d_val ] ;

			if(DynamicLinkLoader_GetSymbolOffset(szDLLName, szSymName, &uiDynSymOffset, processAddressSpace) == DynamicLinkLoader_SUCCESS) 
			{	
				if(DynamicLinkLoader_LoadDLL(szDLLName, processAddressSpace) == DynamicLinkLoader_SUCCESS)
				{
					int index = DLLLoader_GetProcessSharedObjectListIndexByName(szDLLName) ;
					if(index == -1)
					{
						KC::MDisplay().Message("\n SynName = ", ' ') ;
						KC::MDisplay().Message(szSymName, ' ') ;
						return DynamicLinkLoader_FAILURE ;
					}

					unsigned uiDLLLoadAddress = DLLLoader_GetProcessDLLLoadAddress(processAddressSpace, index) ;
					unsigned* uiGOTAddress = (unsigned*)GLOBAL_REL_ADDR(ELF32_REL_ENT(pELFRelTable, uiRelocationOffset)->r_offset, uiBaseAddress) ;
					unsigned uiDynSymAddress = uiDLLLoadAddress + uiDynSymOffset - PROCESS_BASE ;

					uiGOTAddress[0] = uiDynSymAddress ;
					*iDynamicSymAddress = uiDynSymAddress ;
          return DynamicLinkLoader_SUCCESS ;
				}
				else
				{
					//TRACE_LINE ;
				}
			}
			else
			{
				//TRACE_LINE ;
			}
		}
	}

	KC::MDisplay().Message("\n Dynamic Symbol Look Up Failed: ", ' ') ;
	KC::MDisplay().Message(szSymName, ' ') ;
	return DynamicLinkLoader_FAILURE ;
}

byte DynamicLinkLoader_GetSymbolOffset(const char* szJustDLLName, const char* szSymName, unsigned* uiDynSymOffset, ProcessAddressSpace* processAddressSpace)
{
	ProcessSharedObjectList* pProcessSharedObjectList = DLLLoader_GetProcessSharedObjectListByName(szJustDLLName) ;

  upan::uniq_ptr<byte[]> dllImage(nullptr);
  upan::uniq_ptr<ELFParser> pELFParser(nullptr);

	if(pProcessSharedObjectList == NULL)
	{
		char szDLLFullName[128] ;
		char szLibPath[128] = "" ;

		if(!GenericUtil_GetFullFilePathFromEnv(LD_LIBRARY_PATH_ENV, LIB_PATH, szJustDLLName, szLibPath))
			return DynamicLinkLoader_ERR_LIB_EFOUND ;

		strcpy(szDLLFullName, szLibPath) ;
		strcat(szDLLFullName, szJustDLLName) ;

    pELFParser.reset(new ELFParser(szDLLFullName));
		if(!pELFParser->GetState())
			return DynamicLinkLoader_FAILURE ;

    dllImage.reset(DynamicLinkLoader_LoadDLLFileIntoMemory(*pELFParser));
	}
	else
	{
		int index = DLLLoader_GetProcessSharedObjectListIndexByName(szJustDLLName) ;
		if(index == -1)
			return DynamicLinkLoader_FAILURE ;

    dllImage.disown();
    dllImage.reset((byte*)(DLLLoader_GetProcessDLLLoadAddress(processAddressSpace, index) - GLOBAL_DATA_SEGMENT_BASE));

    ELF32Header* pELFHeader = (ELF32Header*)(dllImage.get()) ;
    ELF32SectionHeader* pELFSectionHeader = (ELF32SectionHeader*)(dllImage.get() + (pProcessSharedObjectList->uiNoOfPages - 1) * PAGE_SIZE) ;
    pELFParser.reset(new ELFParser(pELFHeader, pELFSectionHeader, NULL));
	}

	ELF32SectionHeader* pHashSectionHeader = NULL ;
	if(!pELFParser->GetSectionHeaderByType(SHT_HASH, &pHashSectionHeader))
	{
		return DynamicLinkLoader_FAILURE ;
	}

  ELF32SectionHeader* pDynamicSymSectionHeader = pELFParser->GetSectionHeaderByIndex(pHashSectionHeader->sh_link).goodValueOrThrow(XLOC);
  ELF32SectionHeader* pDynamicSymStringSectionHeader = pELFParser->GetSectionHeaderByIndex(pDynamicSymSectionHeader->sh_link).goodValueOrThrow(XLOC);

  ELF32SymbolEntry* pELFDynSymTable = (ELF32SymbolEntry*)(dllImage.get() + pDynamicSymSectionHeader->sh_addr) ;
  const char* pDynStrTable = (const char*)(dllImage.get() + pDynamicSymStringSectionHeader->sh_addr) ;

  unsigned* pHashTable = (unsigned*)(dllImage.get() + pHashSectionHeader->sh_addr) ;
	unsigned uiNoOfBuckets = pHashTable[0] ;
	__attribute__((unused)) unsigned uiNoOfChains = pHashTable[1] ;
	unsigned* pBucket = (unsigned*)((unsigned*)pHashTable + 2) ;
	unsigned* pChain = (unsigned*)((unsigned*)pHashTable + 2 + uiNoOfBuckets) ;
	
	unsigned uiHashValue = DynamicLinkLoader_GetHashValue(szSymName) ;
	unsigned uiSymTabIndex = pBucket[uiHashValue % uiNoOfBuckets] ;
	unsigned uiSymStrIndex ;

	for(; uiSymTabIndex != STN_UNDEF;)
	{
		uiSymStrIndex = pELFDynSymTable[uiSymTabIndex].st_name ;
		if(strcmp(&pDynStrTable[uiSymStrIndex], szSymName) == 0)
		{
			if(pELFDynSymTable[uiSymTabIndex].st_value != 0)
			{
				*uiDynSymOffset = pELFDynSymTable[uiSymTabIndex].st_value ;
				return DynamicLinkLoader_SUCCESS ;
			}
		}

		uiSymTabIndex = pChain[uiSymTabIndex] ;
	}

	return DynamicLinkLoader_ERR_SYM_NOT_FOUND ;
}

