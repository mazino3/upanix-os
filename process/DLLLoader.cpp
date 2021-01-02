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

/************************ static functions *******************************************/

static void DLLLoader_CopyElfDLLImage(uint32_t uiStartAddress, uint32_t uiNoOfPagesForDLL, byte* bDLLImage, uint32_t uiMemImageSize)
{
	unsigned uiOffset = 0;
	unsigned uiCopySize = uiMemImageSize;

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

void DLLLoader_Initialize() {
}

void DLLLoader_LoadELFDLL(const char* szDLLName, const char* szJustDLLName, Process* processAddressSpace)
{
	ELFParser mELFParser(szDLLName) ;

	unsigned uiMinMemAddr, uiMaxMemAddr ;

	mELFParser.GetMemImageSize(&uiMinMemAddr, &uiMaxMemAddr) ;
	if(uiMinMemAddr != 0)
    throw upan::exception(XLOC, "Not a PIC - DLL Min Address: %x", uiMinMemAddr);

	unsigned uiDLLSectionSize ;
  upan::uniq_ptr<byte[]> bDLLSectionImage(ProcessLoader::Instance().LoadDLLInitSection(uiDLLSectionSize));

  const uint32_t uiDLLImageSize = ProcessLoader_GetCeilAlignedAddress(uiMaxMemAddr - uiMinMemAddr, 4) ;
  const uint32_t uiMemImageSize = uiDLLImageSize + uiDLLSectionSize ;
	const uint32_t uiNoOfPagesForDLL = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize) + DLL_ELF_SEC_HEADER_PAGE ;

	if(uiNoOfPagesForDLL > MAX_PAGES_PER_PROCESS)
    throw upan::exception(XLOC, "No. of pages for DLL %u exceeds max limit per dll %u", uiNoOfPagesForDLL, MAX_PAGES_PER_PROCESS);

	if(!KC::MKernelService().RequestDLLAlloCopy(uiNoOfPagesForDLL, szJustDLLName)) {
    throw upan::exception(XLOC, "Failed to allocate memory for DLL via kernal service");
	}

	const ProcessDLLInfo& dllInfo = processAddressSpace->getDLLInfo(szJustDLLName).value();
  const uint32_t uiDLLLoadAddress = dllInfo.loadAddressForProcess();
	const uint32_t pRealELFSectionHeaderAddr = dllInfo.elfSectionHeaderAddress();

	unsigned uiCopySize = mELFParser.CopyELFSectionHeader((ELF32SectionHeader*)pRealELFSectionHeaderAddr) ;
	mELFParser.CopyELFSecStrTable((char*)(pRealELFSectionHeaderAddr + uiCopySize)) ;

  upan::uniq_ptr<byte[]> bDLLImage(new byte[sizeof(char) * uiMemImageSize]);

  upan::trycall([&] () { mELFParser.CopyProcessImage(bDLLImage.get(), 0, uiMemImageSize); }).onBad([&] (const upan::error& err) {
    throw upan::exception(XLOC, err);
  });

  MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)bDLLSectionImage.get(), MemUtil_GetDS(), (unsigned)(bDLLImage.get() + uiDLLImageSize), uiDLLSectionSize) ;

	// Setting the Dynamic Link Loader Address in GOT
  mELFParser.GetGOTAddress(bDLLImage.get(), uiMinMemAddr).onGood([&](uint32_t* uiGOT) {
    uiGOT[1] = dllInfo.id();
    uiGOT[2] = uiDLLImageSize + uiDLLLoadAddress;

    mELFParser.GetNoOfGOTEntries().onGood([&](uint32_t uiNoOfGOTEntries) {
      for(uint32_t i = 3; i < uiNoOfGOTEntries; i++)
        uiGOT[i] += uiDLLLoadAddress ;
    });
  });

/* Dynamic Relocation Entries are resolved here in Global Offset Table */
  mELFParser.GetSectionHeaderByTypeAndName(SHT_REL, REL_DYN_SUB_NAME).onGood([&] (ELF32SectionHeader* pRelocationSectionHeader)
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

/* End of Dynamic Relocation Entries resolution */
  DLLLoader_CopyElfDLLImage(dllInfo.loadAddressForKernel(), uiNoOfPagesForDLL, bDLLImage.get(), uiMemImageSize) ;
}