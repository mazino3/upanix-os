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
#include <ProcessLoader.h>
#include <ProcessManager.h>
#include <ProcessAllocator.h>
#include <PIC.h>
#include <FileSystem.h>
#include <Directory.h>
#include <Display.h>
#include <MemManager.h>
#include <ElfParser.h>
#include <ElfProgHeader.h>
#include <DMM.h>
#include <MemUtil.h>
#include <DynamicLinkLoader.h>
#include <StringUtil.h>
#include <ProcFileManager.h>
#include <FileOperations.h>
#include <ElfRelocationSection.h>
#include <ElfSymbolTable.h>
#include <ElfDynamicSection.h>
#include <stdio.h>
#include <string.h>
#include <exception.h>

#define INIT_NAME "_stdio_init-NOTINUSE"
#define TERM_NAME "_stdio_term-NOTINUSE"

ProcessLoader::ProcessLoader() :
  PROCESS_DLL_FILE(upan::string(OSIN_PATH) + __PROCESS_DLL_FILE),
  PROCESS_START_UP_FILE(upan::string(OSIN_PATH) + __PROCESS_START_UP_FILE)
{
}

byte* ProcessLoader::LoadInitSection(ProcessAddressSpace& pas, unsigned& uiSectionSize, const upan::string& szSectionName)
{
	FileSystem_DIR_Entry DirEntry ;

  if(FileOperations_GetDirEntry(szSectionName.c_str(), &DirEntry) != FileOperations_SUCCESS)
    throw upan::exception(XLOC, "failed to get dir entry for: %s", szSectionName.c_str());
	
	if((DirEntry.usAttribute & ATTR_TYPE_DIRECTORY) == ATTR_TYPE_DIRECTORY)
    throw upan::exception(XLOC, "%s is a directory!", szSectionName.c_str());

	uiSectionSize = DirEntry.uiSize ;
	byte* bSectionImage = (byte*)DMM_AllocateForKernel(sizeof(char) * (uiSectionSize));

  try
  {
    int fd;
    unsigned n;

    if(FileOperations_Open(&fd, szSectionName.c_str(), O_RDONLY) != FileOperations_SUCCESS)
      throw upan::exception(XLOC, "failed to open file: %s", szSectionName.c_str());

    if(FileOperations_Read(fd, (char*)bSectionImage, 0, &n) != FileOperations_SUCCESS)
      throw upan::exception(XLOC, "error reading file: %s", szSectionName.c_str());

    if(FileOperations_Close(fd) != FileOperations_SUCCESS)
      throw upan::exception(XLOC, "error closing file: %s", szSectionName.c_str());

    if(n != uiSectionSize)
      throw upan::exception(XLOC, "could read only %u of %u bytes of %s section", n, uiSectionSize, szSectionName.c_str());
  }
  catch(...)
  {
    DMM_DeAllocateForKernel((unsigned)bSectionImage);
    throw;
  }
  return bSectionImage;
}

byte* ProcessLoader::LoadDLLInitSection(ProcessAddressSpace& pas, unsigned& uiSectionSize)
{
  return LoadInitSection(pas, uiSectionSize, PROCESS_DLL_FILE);
}

byte* ProcessLoader::LoadStartUpInitSection(ProcessAddressSpace& pas, unsigned& uiSectionSize)
{
  return LoadInitSection(pas, uiSectionSize, PROCESS_START_UP_FILE);
}

using namespace ELFSectionHeader ;

unsigned ProcessLoader_GetCeilAlignedAddress(unsigned uiAddress, unsigned uiAlign)
{
	while(true)
	{
		if((uiAddress % uiAlign) == 0)
			return uiAddress ;
	
		uiAddress++ ;
	}
	return 0 ;
}

unsigned ProcessLoader_GetFloorAlignedAddress(unsigned uiAddress, unsigned uiAlign)
{
	while(true)
	{
		if((uiAddress % uiAlign) == 0)
			return uiAddress ;
	
		uiAddress-- ;
	}
	return 0 ;
}

byte ProcessLoader_Load(const char* szProcessName, ProcessAddressSpace* pProcessAddressSpace, unsigned *uiPDEAddress,
		unsigned* uiEntryAdddress, unsigned* uiProcessEntryStackSize, int iNumberOfParameters,
		char** szArgumentList)
{
	return ProcessLoader_LoadELFExe(szProcessName, pProcessAddressSpace, uiPDEAddress, uiEntryAdddress, uiProcessEntryStackSize, iNumberOfParameters, szArgumentList) ;
}

byte ProcessLoader_LoadELFExe(const char* szProcessName, ProcessAddressSpace* pProcessAddressSpace, unsigned *uiPDEAddress,
	   	unsigned *uiEntryAdddress, unsigned* uiProcessEntryStackSize, int iNumberOfParameters, 
		char** szArgumentList)
{
	ELFParser mELFParser(szProcessName) ;

	if(!mELFParser.GetState())
		return ProcessLoader_FAILURE ;

	unsigned* uiNoOfPagesForProcess = &pProcessAddressSpace->uiNoOfPagesForProcess ;
	unsigned* uiNoOfPagesForPTE = &pProcessAddressSpace->uiNoOfPagesForPTE ;
	unsigned* uiProcessBase = &pProcessAddressSpace->uiProcessBase ;
	unsigned* uiStartPDEForDLL = &pProcessAddressSpace->uiStartPDEForDLL ;

	unsigned uiMinMemAddr, uiMaxMemAddr ;

	mELFParser.GetMemImageSize(&uiMinMemAddr, &uiMaxMemAddr) ;
	if(uiMinMemAddr < PROCESS_BASE)
		return ProcessLoader_ERR_TEXT_ADDR_LESS_THAN_LIMIT ;

	if((uiMinMemAddr % PAGE_SIZE) != 0)
		return ProcessLoader_ERR_NOT_ALIGNED_TO_PAGE ;

	unsigned uiStartUpSectionSize ;
	byte* bStartUpSectionImage = NULL ;
	unsigned uiDLLSectionSize ;
	byte* bDLLSectionImage = NULL ;

  try
  {
    bStartUpSectionImage = ProcessLoader::Instance().LoadStartUpInitSection(*pProcessAddressSpace, uiStartUpSectionSize);
    bDLLSectionImage = ProcessLoader::Instance().LoadDLLInitSection(*pProcessAddressSpace, uiDLLSectionSize);
  }
  catch(const upan::exception& ex)
  {
    ex.Print();
    return ProcessLoader_FAILURE;
  }

	unsigned uiProcessImageSize = ProcessLoader_GetCeilAlignedAddress(uiMaxMemAddr - uiMinMemAddr, 4) ;
	unsigned uiMemImageSize = uiProcessImageSize + uiStartUpSectionSize	+ uiDLLSectionSize ;

	*uiNoOfPagesForProcess = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize) + PROCESS_STACK_PAGES + PROCESS_CG_STACK_PAGES ;

	if(*uiNoOfPagesForProcess > MAX_PAGES_PER_PROCESS)
		return ProcessLoader_ERR_HUGE_PROCESS_SIZE ;

	*uiProcessBase = uiMinMemAddr ;
	unsigned uiPageOverlapForProcessBase = ((*uiProcessBase / PAGE_SIZE) % PAGE_TABLE_ENTRIES) ;
	*uiNoOfPagesForPTE = MemManager::Instance().GetPTESizeInPages(*uiNoOfPagesForProcess + uiPageOverlapForProcessBase) + PROCESS_SPACE_FOR_OS;

	if(ProcessAllocator_AllocateAddressSpace(*uiNoOfPagesForProcess, *uiNoOfPagesForPTE, uiPDEAddress, uiStartPDEForDLL, *uiProcessBase) != ProcessAllocator_SUCCESS)
	{
		//TODO
		// Crash the Process..... With SegFault Or OutOfMemeory Error
		KC::MDisplay().Message("\n Out Of Memory1\n", Display::WHITE_ON_BLACK()) ;
		__asm__ __volatile__("HLT") ;
	}

	unsigned pRealELFSectionHeadeAddr ;
	if(DynamicLinkLoader_Initialize(&pRealELFSectionHeadeAddr, *uiPDEAddress) != DynamicLinkLoader_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)bDLLSectionImage) ;
		DMM_DeAllocateForKernel((unsigned)bStartUpSectionImage) ;

		ProcessAllocator_DeAllocateAddressSpace(pProcessAddressSpace) ;
		return ProcessLoader_FAILURE ;	
	}
	
	unsigned uiCopySize = mELFParser.CopyELFSectionHeader((ELF32SectionHeader*)pRealELFSectionHeadeAddr) ;
	mELFParser.CopyELFSecStrTable((char*)(pRealELFSectionHeadeAddr + uiCopySize)) ;

	byte* bProcessImage = (byte*)DMM_AllocateForKernel(sizeof(char) * uiMemImageSize) ;

	if(!mELFParser.CopyProcessImage(bProcessImage, *uiProcessBase, uiMemImageSize))
	{
		DMM_DeAllocateForKernel((unsigned)bDLLSectionImage) ;
		DMM_DeAllocateForKernel((unsigned)bStartUpSectionImage) ;
		DMM_DeAllocateForKernel((unsigned)bProcessImage) ;

		ProcessAllocator_DeAllocateAddressSpace(pProcessAddressSpace) ;

		return ProcessLoader_FAILURE ;
	}

	memcpy((void*)(bProcessImage + uiProcessImageSize), (void*)bStartUpSectionImage, uiStartUpSectionSize) ;
	memcpy((void*)(bProcessImage + uiProcessImageSize + uiStartUpSectionSize), (void*)bDLLSectionImage, uiDLLSectionSize) ;

	// Setting the Dynamic Link Loader Address in GOT
	unsigned* uiGOT = mELFParser.GetGOTAddress(bProcessImage, uiMinMemAddr) ;
	if(uiGOT != NULL)
	{
		uiGOT[1] = -1 ;
		uiGOT[2] = uiMinMemAddr + uiProcessImageSize + uiStartUpSectionSize ;
	}

	// Initialize BSS segment to 0
	ELF32SectionHeader* pBSSSectionHeader = NULL ;
	if(mELFParser.GetSectionHeaderByTypeAndName(ELFSectionHeader::SHT_NOBITS, BSS_SEC_NAME, &pBSSSectionHeader))
	{
		unsigned* pBSS = (unsigned*)(bProcessImage + pBSSSectionHeader->sh_addr - uiMinMemAddr) ;
		unsigned uiBSSSize = pBSSSectionHeader->sh_size ;

		unsigned q ;
		for(q = 0; q < uiBSSSize / sizeof(unsigned); q++) pBSS[q] = 0 ;

		unsigned r ;
		for(r = q * sizeof(unsigned); r < uiBSSSize; r++) ((char*)pBSS)[r] = 0 ;

		/*
		int size = (uiBSSSize > 200) ? 200 : uiBSSSize ;

		int ii ;
		KC::MDisplay().Message("\n BSS:- ", ' ');
		for(ii = 0; ii < size; ii++)
			KC::MDisplay().Number("", (unsigned)((char*)pBSS)[ii]);
		KC::MDisplay().NextLine() ;
		*/
	}

	ProcessLoader_CopyElfImage(*uiPDEAddress, bProcessImage, uiMemImageSize, *uiProcessBase) ;

	/* Find init and term stdio functions in libc if any */

	unsigned uiInitRelocAddress = NULL ;
	unsigned uiTermRelocAddress = NULL ;

	ELF32SectionHeader* pRelocationSectionHeader = NULL ;
	if(mELFParser.GetSectionHeaderByTypeAndName(ELFSectionHeader::SHT_REL, REL_PLT_SUB_NAME, &pRelocationSectionHeader))
	{
		ELF32SectionHeader* pDynamicSymSectiomHeader = NULL ;
		if(mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_link, &pDynamicSymSectiomHeader))
		{
			ELF32SectionHeader* pDynamicSymStringSectionHeader = NULL ;
			if(mELFParser.GetSectionHeaderByIndex(pDynamicSymSectiomHeader->sh_link, &pDynamicSymStringSectionHeader))
			{
				ELFRelocSection::ELF32_Rel* pELFDynRelTable = 
					(ELFRelocSection::ELF32_Rel*)((unsigned)bProcessImage + pRelocationSectionHeader->sh_addr - uiMinMemAddr) ;

				unsigned uiNoOfDynRelEntries = pRelocationSectionHeader->sh_size / pRelocationSectionHeader->sh_entsize ;

				ELFSymbolTable::ELF32SymbolEntry* pELFDynSymTable = 
					(ELFSymbolTable::ELF32SymbolEntry*)((unsigned)bProcessImage + pDynamicSymSectiomHeader->sh_addr - uiMinMemAddr) ;

				const char* pDynStrTable = (const char*)((unsigned)bProcessImage + pDynamicSymStringSectionHeader->sh_addr - uiMinMemAddr) ;

				unsigned uiRelType ;

				for(unsigned i = 0; i < uiNoOfDynRelEntries && (uiInitRelocAddress == NULL || uiTermRelocAddress == NULL); i++)
				{
					uiRelType = ELF32_R_TYPE(pELFDynRelTable[i].r_info) ;

					if(uiRelType == ELFRelocSection::R_386_JMP_SLOT)
					{
						int iSymIndex = ELF32_R_SYM(pELFDynRelTable[i].r_info);
						int iStrIndex = pELFDynSymTable[ iSymIndex ].st_name ;
						char* szSymName = (char*)&pDynStrTable[ iStrIndex ] ;
						
						if(String_Compare(szSymName, INIT_NAME) == 0)
							uiInitRelocAddress = pELFDynRelTable[i].r_offset ;
						else if(String_Compare(szSymName, TERM_NAME) == 0)
							uiTermRelocAddress = pELFDynRelTable[i].r_offset ;
					}
				}
			}
		}
	}

	/* End of Find init and term stdio functions in libc if any */

	ProcessLoader_PushProgramInitStackData(*uiPDEAddress, *uiNoOfPagesForProcess, 
				*uiProcessBase, uiProcessEntryStackSize, mELFParser.GetProgramStartAddress(), uiInitRelocAddress, uiTermRelocAddress,
				iNumberOfParameters, szArgumentList) ;

	*uiEntryAdddress = uiMinMemAddr + uiProcessImageSize ;

	DMM_DeAllocateForKernel((unsigned)bDLLSectionImage) ;
	DMM_DeAllocateForKernel((unsigned)bStartUpSectionImage) ;
	DMM_DeAllocateForKernel((unsigned)bProcessImage) ;

	return ProcessLoader_SUCCESS ;
}

void ProcessLoader_CopyElfImage(unsigned uiPDEAddr, byte* bProcessImage, unsigned uiMemImageSize, unsigned uiProcessBase)
{
	unsigned i ;
	unsigned uiPDEAddress = uiPDEAddr ;
	unsigned uiPDEIndex, uiPTEIndex, uiPTEAddress, uiPageAddress ;
	unsigned uiOffset = 0 ;
	unsigned uiProcessPDEBase = uiProcessBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;
	unsigned uiProcessPageBase = ( uiProcessBase / PAGE_SIZE ) % PAGE_TABLE_ENTRIES ;

	unsigned uiCopySize = uiMemImageSize ;
	unsigned uiNoOfPagesForProcess = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize) ;

	for(i = 0; i < uiNoOfPagesForProcess; i++)
	{
		uiPDEIndex = (uiProcessPageBase + i) / PAGE_TABLE_ENTRIES + uiProcessPDEBase + PROCESS_SPACE_FOR_OS ;
		uiPTEIndex = (uiProcessPageBase + i) % PAGE_TABLE_ENTRIES ;
		//Kernel Heap PTE and PROCESS_SPACE_FOR_OS need not be part of uiPTEIndex calculation as they shall
		//be aligned at PAGE BOUNDARY

		uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;
		uiPageAddress = (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex]) & 0xFFFFF000 ;

		if(uiCopySize <= PAGE_SIZE)
		{
			MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)bProcessImage + uiOffset, MemUtil_GetDS(), 
						uiPageAddress - GLOBAL_DATA_SEGMENT_BASE, uiCopySize) ;
			return ;
		}
		else
		{
			MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)bProcessImage + uiOffset, MemUtil_GetDS(), 
						uiPageAddress - GLOBAL_DATA_SEGMENT_BASE, PAGE_SIZE) ;
		}
		
		uiOffset += PAGE_SIZE ;
		uiCopySize -= PAGE_SIZE ;
	}

	return ;
}

void ProcessLoader_PushProgramInitStackData(unsigned uiPDEAddr, unsigned uiNoOfPagesForProcess, 
				unsigned uiProcessBase, unsigned* uiProcessEntryStackSize,
				unsigned uiProgramStartAddress, unsigned uiInitRelocAddress, unsigned uiTermRelocAddress, int iNumberOfParameters, 
				char** szArgumentList)
{
	unsigned uiPDEAddress = uiPDEAddr ;
	unsigned uiPDEIndex, uiPTEIndex, uiPTEAddress, uiPageAddress ;
	unsigned uiProcessPDEBase = uiProcessBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;
	unsigned uiProcessPageBase = ( uiProcessBase / PAGE_SIZE ) % PAGE_TABLE_ENTRIES ;

	unsigned uiLastProcessStackPage = uiNoOfPagesForProcess - PROCESS_CG_STACK_PAGES - 1 ;

	uiPDEIndex = (uiProcessPageBase + uiLastProcessStackPage) / PAGE_TABLE_ENTRIES + uiProcessPDEBase + PROCESS_SPACE_FOR_OS ;
	uiPTEIndex = (uiProcessPageBase + uiLastProcessStackPage) % PAGE_TABLE_ENTRIES ;

	uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;
	uiPageAddress = (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex]) & 0xFFFFF000 ;

	const unsigned uiEntryAdddressSize = 4 ;
	const unsigned uiInitRelocAddressSize = 4 ;
	const unsigned uiTermRelocAddressSize = 4 ;
	const unsigned argc = 4 ;
	const unsigned argv = 4 ;
	const unsigned uiArgumentAddressListSize = iNumberOfParameters * 4 ;

	unsigned uiStackTopAddress = uiProcessBase + (uiNoOfPagesForProcess - PROCESS_CG_STACK_PAGES) * PAGE_SIZE ;

	unsigned uiArgumentListSize = 0 ;
	int i ;
	for(i = 0; i < iNumberOfParameters; i++)
		uiArgumentListSize += (strlen(szArgumentList[i]) + 1) ;

	*uiProcessEntryStackSize = uiTermRelocAddressSize + uiInitRelocAddressSize + uiEntryAdddressSize + argc + argv + uiArgumentAddressListSize + uiArgumentListSize ;

	uiPageAddress = uiPageAddress + PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE - *uiProcessEntryStackSize ;
	
	int iStackIndex = 0 ;
	((unsigned*)(uiPageAddress))[iStackIndex++] = uiProgramStartAddress;
	((unsigned*)(uiPageAddress))[iStackIndex++] = uiInitRelocAddress;
	((unsigned*)(uiPageAddress))[iStackIndex++] = uiTermRelocAddress;
	((unsigned*)(uiPageAddress))[iStackIndex++] = iNumberOfParameters;
	int argLength = (iStackIndex + 1) * 4;
	((unsigned*)(uiPageAddress))[iStackIndex++] = uiStackTopAddress - *uiProcessEntryStackSize + argLength;
	
	uiArgumentListSize = 0 ;
	for(i = 0; i < iNumberOfParameters; i++)
	{
		((unsigned*)(uiPageAddress))[iStackIndex + i] = ((unsigned*)(uiPageAddress))[iStackIndex - 1] + uiArgumentAddressListSize + uiArgumentListSize ;

		strcpy((char*)&((unsigned*)(uiPageAddress))[iStackIndex + iNumberOfParameters] + uiArgumentListSize,	szArgumentList[i]) ;

		uiArgumentListSize += (strlen(szArgumentList[i]) + 1) ;
	}
}
