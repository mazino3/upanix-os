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
#include <uniq_ptr.h>
#include <try.h>

#define INIT_NAME "_stdio_init-NOTINUSE"
#define TERM_NAME "_stdio_term-NOTINUSE"

ProcessLoader::ProcessLoader() :
  PROCESS_DLL_FILE(upan::string(OSIN_PATH) + __PROCESS_DLL_FILE),
  PROCESS_START_UP_FILE(upan::string(OSIN_PATH) + __PROCESS_START_UP_FILE)
{
}

byte* ProcessLoader::LoadInitSection(unsigned& uiSectionSize, const upan::string& szSectionName)
{
	FileSystem_DIR_Entry DirEntry ;

  if(FileOperations_GetDirEntry(szSectionName.c_str(), &DirEntry) != FileOperations_SUCCESS)
    throw upan::exception(XLOC, "failed to get dir entry for: %s", szSectionName.c_str());
	
	if((DirEntry.usAttribute & ATTR_TYPE_DIRECTORY) == ATTR_TYPE_DIRECTORY)
    throw upan::exception(XLOC, "%s is a directory!", szSectionName.c_str());

	uiSectionSize = DirEntry.uiSize ;
  upan::uniq_ptr<byte[]> bSectionImage(new byte[sizeof(char) * uiSectionSize]);

  int fd;
  unsigned n;

  if(FileOperations_Open(&fd, szSectionName.c_str(), O_RDONLY) != FileOperations_SUCCESS)
    throw upan::exception(XLOC, "failed to open file: %s", szSectionName.c_str());

  if(FileOperations_Read(fd, (char*)bSectionImage.get(), 0, &n) != FileOperations_SUCCESS)
    throw upan::exception(XLOC, "error reading file: %s", szSectionName.c_str());

  if(FileOperations_Close(fd) != FileOperations_SUCCESS)
    throw upan::exception(XLOC, "error closing file: %s", szSectionName.c_str());

  if(n != uiSectionSize)
    throw upan::exception(XLOC, "could read only %u of %u bytes of %s section", n, uiSectionSize, szSectionName.c_str());

  return bSectionImage.release();
}

byte* ProcessLoader::LoadDLLInitSection(unsigned& uiSectionSize)
{
  return LoadInitSection(uiSectionSize, PROCESS_DLL_FILE);
}

byte* ProcessLoader::LoadStartUpInitSection(unsigned& uiSectionSize)
{
  return LoadInitSection(uiSectionSize, PROCESS_START_UP_FILE);
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

void ProcessLoader::Load(const char* szProcessName, Process* pProcessAddressSpace, unsigned *uiPDEAddress,
                         unsigned* uiEntryAdddress, unsigned* uiProcessEntryStackSize, int iNumberOfParameters, char** szArgumentList)
{
	ELFParser mELFParser(szProcessName) ;

	unsigned* uiNoOfPagesForProcess = &pProcessAddressSpace->uiNoOfPagesForProcess ;
	unsigned* uiNoOfPagesForPTE = &pProcessAddressSpace->uiNoOfPagesForPTE ;
	unsigned* uiProcessBase = &pProcessAddressSpace->uiProcessBase ;
	unsigned* uiStartPDEForDLL = &pProcessAddressSpace->uiStartPDEForDLL ;

	unsigned uiMinMemAddr, uiMaxMemAddr ;

	mELFParser.GetMemImageSize(&uiMinMemAddr, &uiMaxMemAddr) ;
	if(uiMinMemAddr < PROCESS_BASE)
    throw upan::exception(XLOC, "process min load address %x is less than PROCESS_BASE %x", uiMinMemAddr, PROCESS_BASE);

	if((uiMinMemAddr % PAGE_SIZE) != 0)
    throw upan::exception(XLOC, "process min load address %x is not page aligned", uiMinMemAddr);

	unsigned uiStartUpSectionSize ;
	unsigned uiDLLSectionSize ;

  upan::uniq_ptr<byte[]> bStartUpSectionImage(ProcessLoader::Instance().LoadStartUpInitSection(uiStartUpSectionSize));
  upan::uniq_ptr<byte[]> bDLLSectionImage(ProcessLoader::Instance().LoadDLLInitSection(uiDLLSectionSize));

	unsigned uiProcessImageSize = ProcessLoader_GetCeilAlignedAddress(uiMaxMemAddr - uiMinMemAddr, 4) ;
	unsigned uiMemImageSize = uiProcessImageSize + uiStartUpSectionSize	+ uiDLLSectionSize ;

	*uiNoOfPagesForProcess = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize) + PROCESS_STACK_PAGES + PROCESS_CG_STACK_PAGES ;

	if(*uiNoOfPagesForProcess > MAX_PAGES_PER_PROCESS)
    throw upan::exception(XLOC, "process requires %u pages that's larger than supported %u", *uiNoOfPagesForProcess, MAX_PAGES_PER_PROCESS);

	*uiProcessBase = uiMinMemAddr ;
	unsigned uiPageOverlapForProcessBase = ((*uiProcessBase / PAGE_SIZE) % PAGE_TABLE_ENTRIES) ;
	*uiNoOfPagesForPTE = MemManager::Instance().GetPTESizeInPages(*uiNoOfPagesForProcess + uiPageOverlapForProcessBase) + PROCESS_SPACE_FOR_OS;

	if(ProcessAllocator_AllocateAddressSpace(*uiNoOfPagesForProcess, *uiNoOfPagesForPTE, uiPDEAddress, uiStartPDEForDLL, *uiProcessBase) != ProcessAllocator_SUCCESS)
	{
		//TODO
		// Crash the Process..... With SegFault Or OutOfMemeory Error
    printf("\n OUT OF MEMORY \n");
		__asm__ __volatile__("HLT") ;
	}

  unsigned pRealELFSectionHeadeAddr = DynamicLinkLoader_Initialize(*uiPDEAddress);
	unsigned uiCopySize = mELFParser.CopyELFSectionHeader((ELF32SectionHeader*)pRealELFSectionHeadeAddr) ;
	mELFParser.CopyELFSecStrTable((char*)(pRealELFSectionHeadeAddr + uiCopySize)) ;

  upan::uniq_ptr<byte[]> bProcessImage(new byte[sizeof(char) * uiMemImageSize]);

  upan::tryresult([&] { mELFParser.CopyProcessImage(bProcessImage.get(), *uiProcessBase, uiMemImageSize); }).badMap([&] (const upan::error& err) {
    ProcessAllocator_DeAllocateAddressSpace(pProcessAddressSpace) ;
    throw upan::exception(XLOC, err);
  });

  memcpy((void*)(bProcessImage.get() + uiProcessImageSize), (void*)bStartUpSectionImage.get(), uiStartUpSectionSize) ;
  memcpy((void*)(bProcessImage.get() + uiProcessImageSize + uiStartUpSectionSize), (void*)bDLLSectionImage.get(), uiDLLSectionSize) ;

	// Setting the Dynamic Link Loader Address in GOT
  mELFParser.GetGOTAddress(bProcessImage.get(), uiMinMemAddr).goodMap([&](uint32_t* uiGOT) {
		uiGOT[1] = -1 ;
		uiGOT[2] = uiMinMemAddr + uiProcessImageSize + uiStartUpSectionSize ;
  });

	// Initialize BSS segment to 0
  mELFParser.GetSectionHeaderByTypeAndName(ELFSectionHeader::SHT_NOBITS, BSS_SEC_NAME).goodMap([&] (ELF32SectionHeader* pBSSSectionHeader)
  {
    unsigned* pBSS = (unsigned*)(bProcessImage.get() + pBSSSectionHeader->sh_addr - uiMinMemAddr) ;
		unsigned uiBSSSize = pBSSSectionHeader->sh_size ;

		unsigned q ;
		for(q = 0; q < uiBSSSize / sizeof(unsigned); q++) pBSS[q] = 0 ;

		unsigned r ;
		for(r = q * sizeof(unsigned); r < uiBSSSize; r++) ((char*)pBSS)[r] = 0 ;
  });

  ProcessLoader_CopyElfImage(*uiPDEAddress, bProcessImage.get(), uiMemImageSize, *uiProcessBase) ;

	/* Find init and term stdio functions in libc if any */
  unsigned uiInitRelocAddress = NULL ;
  unsigned uiTermRelocAddress = NULL ;
  mELFParser.GetSectionHeaderByTypeAndName(ELFSectionHeader::SHT_REL, REL_PLT_SUB_NAME).goodMap([&] (ELF32SectionHeader* pRelocationSectionHeader)
  {
    mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_link).goodMap([&] (ELF32SectionHeader* pDynamicSymSectiomHeader)
    {
      mELFParser.GetSectionHeaderByIndex(pDynamicSymSectiomHeader->sh_link).goodMap([&] (ELF32SectionHeader* pDynamicSymStringSectionHeader)
      {
				ELFRelocSection::ELF32_Rel* pELFDynRelTable = 
          (ELFRelocSection::ELF32_Rel*)((unsigned)bProcessImage.get() + pRelocationSectionHeader->sh_addr - uiMinMemAddr) ;

				unsigned uiNoOfDynRelEntries = pRelocationSectionHeader->sh_size / pRelocationSectionHeader->sh_entsize ;

				ELFSymbolTable::ELF32SymbolEntry* pELFDynSymTable = 
          (ELFSymbolTable::ELF32SymbolEntry*)((unsigned)bProcessImage.get() + pDynamicSymSectiomHeader->sh_addr - uiMinMemAddr) ;

        const char* pDynStrTable = (const char*)((unsigned)bProcessImage.get() + pDynamicSymStringSectionHeader->sh_addr - uiMinMemAddr) ;

				for(unsigned i = 0; i < uiNoOfDynRelEntries && (uiInitRelocAddress == NULL || uiTermRelocAddress == NULL); i++)
				{
          if(ELF32_R_TYPE(pELFDynRelTable[i].r_info) == ELFRelocSection::R_386_JMP_SLOT)
					{
						int iSymIndex = ELF32_R_SYM(pELFDynRelTable[i].r_info);
						int iStrIndex = pELFDynSymTable[ iSymIndex ].st_name ;
						char* szSymName = (char*)&pDynStrTable[ iStrIndex ] ;
						
						if(strcmp(szSymName, INIT_NAME) == 0)
							uiInitRelocAddress = pELFDynRelTable[i].r_offset ;
						else if(strcmp(szSymName, TERM_NAME) == 0)
							uiTermRelocAddress = pELFDynRelTable[i].r_offset ;
					}
				}
      });
    });
  });

	/* End of Find init and term stdio functions in libc if any */

	ProcessLoader_PushProgramInitStackData(*uiPDEAddress, *uiNoOfPagesForProcess, 
				*uiProcessBase, uiProcessEntryStackSize, mELFParser.GetProgramStartAddress(), uiInitRelocAddress, uiTermRelocAddress,
				iNumberOfParameters, szArgumentList) ;

	*uiEntryAdddress = uiMinMemAddr + uiProcessImageSize ;
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
