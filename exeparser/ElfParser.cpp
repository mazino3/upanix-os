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
#include <ElfParser.h>
#include <ElfConstants.h>
#include <ElfHeader.h>
#include <ElfProgHeader.h>
#include <ElfSectionHeader.h>
#include <ElfSymbolTable.h>
#include <MemUtil.h>
#include <DMM.h>
#include <StringUtil.h>
#include <Display.h>
#include <DynamicLinkLoader.h>
#include <ProcFileManager.h>
#include <FileOperations.h>
#include <BufferedReader.h>

ELFParser::ELFParser(ELF32Header* pELFHeader, ELF32SectionHeader* pELFSectionHeader, char* pSecHeaderStrTable) :
	m_bReleaseMem(false),
	m_bObjectState(true),
	m_uiSymTabCount(0),
	m_pBR(NULL),
	m_pHeader(pELFHeader),
	m_pSectionHeader(pELFSectionHeader),
	m_pSecHeaderStrTable(pSecHeaderStrTable),
	m_pProgramHeader(NULL),
	m_pSectionTableMap(NULL),
	m_pSymbolTable(NULL)
{
}

ELFParser::ELFParser(const char* szFileName) : 
	m_bReleaseMem(true),
	m_bObjectState(false),
	m_uiSymTabCount(0),
	m_pBR(NULL),
	m_pHeader(NULL),
	m_pSectionHeader(NULL),
	m_pSecHeaderStrTable(NULL),
	m_pProgramHeader(NULL),
	m_pSectionTableMap(NULL),
	m_pSymbolTable(NULL)
{
	m_pBR = new BufferedReader(szFileName, 0, .5 * 1024 * 1024) ;

	if(!m_pBR)
		return ;

	if(!m_pBR->GetState())
		return ;

	if(!ReadHeader())
		return ;

	if(!ReadProgramHeaders())
		return ;

	if(!ReadSectionHeaders())
		return ;

	if(!ReadSecHeaderStrTable())
		return ;

	if(!ReadSymbolTables())
		return ;

	m_bObjectState = true ;
}

ELFParser::~ELFParser()
{
	// The cleanup/destruction is safe even if construction as failed. 
	if(m_bReleaseMem)
		ReleaseMemory() ;
}

void ELFParser::AllocateHeader()
{
	m_pHeader = new ELF32Header ;
}

void ELFParser::DeAllocateHeader()
{
	delete m_pHeader ;
}

void ELFParser::AllocateProgramHeader()
{
	m_pProgramHeader = new ELF32ProgramHeader[ m_pHeader->e_phnum ] ;
}

void ELFParser::DeAllocateProgramHeader()
{
	delete[] m_pProgramHeader ;
}

void ELFParser::AllocateSectionHeader()
{
	m_pSectionHeader = new ELF32SectionHeader[ m_pHeader->e_shnum ] ;
	m_pSectionTableMap = new int[ m_pHeader->e_shnum ] ;
	
	for(int i = 0; i < m_pHeader->e_shnum; i++)
		m_pSectionTableMap[i] = -1 ;
}

void ELFParser::DeAllocateSectionHeader()
{
	delete[] m_pSectionHeader ;
	delete[] m_pSectionTableMap ;
}

void ELFParser::AllocateSecHeaderStrTable(int iSecSize)
{
	m_pSecHeaderStrTable = new char[ iSecSize ] ;
}

void ELFParser::DeAllocateSecHeaderStrTable()
{
	delete[] m_pSecHeaderStrTable ;
}

void ELFParser::AllocateSymbolTable()
{
	m_uiSymTabCount = 0 ;

	for(int i = 0; i < m_pHeader->e_shnum; i++)
	{
		if(m_pSectionHeader[i].sh_type == ELFSectionHeader::SHT_SYMTAB)
			m_uiSymTabCount++ ;
	}

	m_pSymbolTable = new ELFSymbolTableList[ m_uiSymTabCount ] ;

	unsigned uiSymTabIndex = 0 ;
	for(int i = 0; i < m_pHeader->e_shnum; i++)
	{
		if(m_pSectionHeader[i].sh_type == ELFSectionHeader::SHT_SYMTAB)
		{
			m_pSymbolTable[uiSymTabIndex].uiTableSize = (m_pSectionHeader[i].sh_size / m_pSectionHeader[i].sh_entsize) ;

			m_pSymbolTable[uiSymTabIndex].SymTabEntries = new ELFSymbolTable::ELF32SymbolEntry[ m_pSymbolTable[uiSymTabIndex].uiTableSize ] ;

			uiSymTabIndex++ ;
		}
	}
}

void ELFParser::DeAllocateSymbolTable()
{
	for(unsigned i = 0; i < m_uiSymTabCount; i++)
		delete[] (m_pSymbolTable[i].SymTabEntries) ;
	
	delete[] m_pSymbolTable ;
}

void ELFParser::ReleaseMemory()
{
	delete m_pBR ;

	DeAllocateSymbolTable() ;
	DeAllocateSecHeaderStrTable() ;
	DeAllocateSectionHeader() ;
	DeAllocateProgramHeader() ;
	DeAllocateHeader() ;
}

bool ELFParser::ReadHeader()
{
	unsigned n ;

	AllocateHeader() ;

	if(!m_pBR->Seek(0))
		return false ;

	if(!m_pBR->Read((char*)m_pHeader, sizeof(ELF32Header), &n))
		return false ;

	if(n < sizeof(ELF32Header))
		return false ;

	if(!CheckMagicSignature(m_pHeader))
		return false ;

	return true ;
}

bool ELFParser::ReadProgramHeaders()
{
	/* e_phentsize is not used as this Parser is only for 32 bit elf files.
	   and ElfProgHeader size is 32 bytes */

	AllocateProgramHeader() ;
	
	unsigned n ;

	if(!m_pBR->Seek(m_pHeader->e_phoff))
		return false ;

	if(!m_pBR->Read((char*)m_pProgramHeader, sizeof(ELF32ProgramHeader) * m_pHeader->e_phnum, &n))
		return false ;

	if(n < sizeof(ELF32ProgramHeader) * m_pHeader->e_phnum)
		return false ;
	
	return true ;
}

bool ELFParser::ReadSectionHeaders()
{
	/* e_shentsize if not used as this Parser is only for 32 bit elf files.
	   and ElfSectionHeader size is 40 bytes */
	   
	AllocateSectionHeader() ;
	
	unsigned n ;

	if(!m_pBR->Seek(m_pHeader->e_shoff))
		return false ;

	if(!m_pBR->Read((char*)m_pSectionHeader, sizeof(ELF32SectionHeader) * m_pHeader->e_shnum, &n))
		return false ;

	if(n < sizeof(ELF32SectionHeader) * m_pHeader->e_shnum)
		return false ;
	
	return true ;
}

bool ELFParser::ReadSecHeaderStrTable()
{
	unsigned uiSecSize = m_pSectionHeader[m_pHeader->e_shstrndx].sh_size ;
	unsigned uiSecOffset = m_pSectionHeader[m_pHeader->e_shstrndx].sh_offset ;
	unsigned n ;

	AllocateSecHeaderStrTable(uiSecSize) ;

	if(!m_pBR->Seek(uiSecOffset))
		return false ;

	if(!m_pBR->Read((char*)m_pSecHeaderStrTable, uiSecSize, &n))
		return false ;

	if(n < uiSecSize)
		return false ;

	return true ;
}

bool ELFParser::ReadSymbolTables()
{
	unsigned i, n ;

	AllocateSymbolTable() ;

	unsigned uiSymTabIndex = 0 ;
	for(i = 0; i < m_pHeader->e_shnum; i++)
	{
		if(m_pSectionHeader[i].sh_type == ELFSectionHeader::SHT_SYMTAB)
		{
			if(!m_pBR->Seek(m_pSectionHeader[i].sh_offset))
				return false ;

			if(!m_pBR->Read((char*)(m_pSymbolTable[uiSymTabIndex].SymTabEntries), 
						sizeof(ELFSymbolTable::ELF32SymbolEntry) * m_pSymbolTable[uiSymTabIndex].uiTableSize,
						&n))
			{
				return false ;
			}

			if(n < sizeof(ELFSymbolTable::ELF32SymbolEntry) * m_pSymbolTable[uiSymTabIndex].uiTableSize)
				return false ;

			m_pSectionTableMap[i] = uiSymTabIndex ;
			uiSymTabIndex++ ;
		}
	}

	return true ;
}

unsigned* ELFParser::GetAddressBySectionName(byte* bProcessImage, unsigned uiMinMemAddr, const char* szSectionName)
{
	for(int i = 0; i < m_pHeader->e_shnum; i++)
	{
		if(strcmp(ELFSectionHeader::GetSectionName(m_pSecHeaderStrTable, m_pSectionHeader[i].sh_name), szSectionName) == 0)
		{
			return (unsigned*)(bProcessImage + m_pSectionHeader[i].sh_addr - uiMinMemAddr) ;
		}
	}

	return NULL ;
}

bool ELFParser::GetNoOfGOTEntriesBySectionName(unsigned* uiNoOfGOTEntries, const char* szSectionName)
{
	for(int i = 0; i < m_pHeader->e_shnum; i++)
	{
		if(strcmp(ELFSectionHeader::GetSectionName(m_pSecHeaderStrTable, m_pSectionHeader[i].sh_name), szSectionName) == 0)
		{
			*uiNoOfGOTEntries = (m_pSectionHeader[i].sh_size / m_pSectionHeader[i].sh_entsize) ;
			return true ;
		}
	}

	return false ;
}

void ELFParser::GetMemImageSize(unsigned* uiMinMemAddr, unsigned* uiMaxMemAddr)
{
	*uiMinMemAddr = 0 ;
	*uiMaxMemAddr = 0 ;

	bool bFirstTime = true ;

	for(unsigned i = 0; i < m_pHeader->e_phnum; i++)
	{
		if(m_pProgramHeader[i].p_type == ELFProgramHeader::PT_LOAD)
		{
			if(bFirstTime == true)
			{
				bFirstTime = false ;
				*uiMinMemAddr = m_pProgramHeader[i].p_vaddr ;
			}

			*uiMaxMemAddr = m_pProgramHeader[i].p_vaddr + m_pProgramHeader[i].p_memsz ;
		}
	}
}

unsigned ELFParser::GetProgramStartAddress() 
{
	return m_pHeader->e_entry ;
}

unsigned* ELFParser::GetGOTAddress(byte* bProcessImage, unsigned uiMinMemAddr)
{
	unsigned* uiValue = GetAddressBySectionName(bProcessImage, uiMinMemAddr, ".got.plt") ;

	if(uiValue == NULL)
		uiValue = GetAddressBySectionName(bProcessImage, uiMinMemAddr, ".got") ;

	return uiValue ;
}

bool ELFParser::GetNoOfGOTEntries(unsigned* uiNoOfGOTEntries)
{
	if(!GetNoOfGOTEntriesBySectionName(uiNoOfGOTEntries, ".got.plt"))
		return GetNoOfGOTEntriesBySectionName(uiNoOfGOTEntries, ".got") ;

	return true ;
}

bool ELFParser::GetSectionHeaderByType(unsigned uiType, ELF32SectionHeader** pSectionHeader)
{
	for(int i = 0; i < m_pHeader->e_shnum; i++)
	{
		if(m_pSectionHeader[i].sh_type == uiType)
		{
			*pSectionHeader = &m_pSectionHeader[i] ;
			return true ;
		}
	}

	return false ;
}

bool ELFParser::GetSectionHeaderByTypeAndName(unsigned uiType, const char* szLikeName, ELF32SectionHeader** pSectionHeader)
{
	for(int i = 0; i < m_pHeader->e_shnum; i++)
	{
		if(strstr((m_pSecHeaderStrTable + m_pSectionHeader[i].sh_name), szLikeName) && m_pSectionHeader[i].sh_type == uiType)
		{
			*pSectionHeader = &m_pSectionHeader[i] ;
			return true ;	
		}
	}

	return false ;
}

bool ELFParser::GetSectionHeaderByIndex(unsigned uiIndex, ELF32SectionHeader** pSectionHeader)
{
	if(uiIndex >= m_pHeader->e_shnum)
		return false ;

	*pSectionHeader = &m_pSectionHeader[uiIndex] ;
	return true ;
}

bool ELFParser::CopyProcessImage(byte* bProcessImage, unsigned uiProcessBase, unsigned uiMaxImageSize)
{
	unsigned n, uiOffset ;

	for(unsigned i = 0; i < m_pHeader->e_phnum; i++)
	{
		if(m_pProgramHeader[i].p_type == ELFProgramHeader::PT_LOAD)
		{
			if(!m_pBR->Seek(m_pProgramHeader[i].p_offset))
				return false ;

			uiOffset = m_pProgramHeader[i].p_vaddr - uiProcessBase ;

			if(uiOffset >= uiMaxImageSize)
				return false ;

			if(!m_pBR->Read((char*)bProcessImage + uiOffset, m_pProgramHeader[i].p_filesz, &n))
				return false ;

			if(n < m_pProgramHeader[i].p_filesz)
				return false ;
		}
	}

	return true ;
}

unsigned ELFParser::CopyELFSecStrTable(char* szSecStrTable)
{
	unsigned uiSize = m_pSectionHeader[ m_pHeader->e_shstrndx ].sh_size ;
	memcpy(szSecStrTable, m_pSecHeaderStrTable, uiSize) ;
	return uiSize ;
}

unsigned ELFParser::CopyELFSectionHeader(ELF32SectionHeader* pSectionHeader)
{
	unsigned uiSize = sizeof(ELF32SectionHeader) * m_pHeader->e_shnum ;
	memcpy(pSectionHeader, m_pSectionHeader, uiSize) ;
	return uiSize ;
}

bool ELFParser::CheckMagicSignature(const ELF32Header* pELFHeader)
{
	return (pELFHeader->e_ident[ELFHeader::EI_MAG0] == 0x7F 
		&& pELFHeader->e_ident[ELFHeader::EI_MAG1] == 'E'
		&& pELFHeader->e_ident[ELFHeader::EI_MAG2] == 'L'
		&& pELFHeader->e_ident[ELFHeader::EI_MAG3] == 'F'
		&& (pELFHeader->e_type == ELFHeader::ET_EXEC || pELFHeader->e_type == ELFHeader::ET_DYN)) ;
}

/*
static void ElfHacker_KC::MDisplay().e_ident_Field(const Elf_Header* elfHeader)
{
	printf("\n e_ident:") ;
	printf("\n\t\tEI_MAG0 = 0x%X (%d)", elfHeader->e_ident[EI_MAG0], elfHeader->e_ident[EI_MAG0]) ;
	printf("\n\t\tEI_MAG1 = %c", elfHeader->e_ident[EI_MAG1]) ; 
	printf("\n\t\tEI_MAG2 = %c", elfHeader->e_ident[EI_MAG2]) ;
	printf("\n\t\tEI_MAG3 = %c", elfHeader->e_ident[EI_MAG3]) ;
	printf("\n\t\tEI_CLASS = %s", Elf_Classes[elfHeader->e_ident[EI_CLASS]]) ; 
	printf("\n\t\tEI_DATA = %s", Elf_DataEncoding[elfHeader->e_ident[EI_DATA]]) ;
	printf("\n\t\tEI_VERSION = %s", Elf_Versions[elfHeader->e_ident[EI_VERSION]]) ;
	printf("\n\t\tEI_PAD = %d\n", elfHeader->e_ident[EI_PAD]) ;
}
{
	ElfHacker_KC::MDisplay().e_ident_Field(&ElfHacker_elfHeader) ;
	printf("\n\te_tpye = %s", Elf_eType[ElfHacker_elfHeader.e_type]) ;
	printf("\n\te_machine = %s", Elf_eMachine[ElfHacker_elfHeader.e_machine]) ;
	printf("\n\te_version = %d", ElfHacker_elfHeader.e_version) ;
	printf("\n\te_entry = 0x%X (%d)", ElfHacker_elfHeader.e_entry, ElfHacker_elfHeader.e_entry) ;
	printf("\n\te_phoff = %d", ElfHacker_elfHeader.e_phoff) ;
	printf("\n\te_shoff = %d", ElfHacker_elfHeader.e_shoff) ;
	printf("\n\te_flags = %d", ElfHacker_elfHeader.e_flags) ;
	printf("\n\te_ehsize = %d", ElfHacker_elfHeader.e_ehsize) ;
	printf("\n\te_phentsize = %d", ElfHacker_elfHeader.e_phentsize) ;
	printf("\n\te_phnum = %d", ElfHacker_elfHeader.e_phnum) ;
	printf("\n\te_shentsize = %d", ElfHacker_elfHeader.e_shentsize) ;
	printf("\n\te_shnum = %d", ElfHacker_elfHeader.e_shnum) ;
	printf("\n\te_shstrndx = %d", ElfHacker_elfHeader.e_shstrndx) ;

}*/

namespace ELFProgramHeader
{
	const char* GetProgHeaderType(unsigned uiPType)
	{
		if(0 <= uiPType && uiPType < MAX_PROG_TYPES)
			return ProgramHeaderType[uiPType] ;

		return (uiPType <= PT_LOPROC || uiPType >= PT_HIPROC) ? "Processor-Specific" : "Invalid Program Header Type" ;
	}
} ;

namespace ELFSectionHeader
{
	const char* GetSecHeaderType(unsigned uiSType)
	{
		if(0 <= uiSType && uiSType < MAX_SEC_HEADER_TYPES)
			return SectionHeaderType[ uiSType ] ;

		return (uiSType > SHT_LOPROC && uiSType <= SHT_HIPROC) ? "Processor-Specific" : "Invalid Section Header Type" ;
	}

	const char* GetSectionName(const char* szStrTable, int iIndex)
	{
		return szStrTable + iIndex ;
	}
} ;
