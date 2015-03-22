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
#ifndef _ELF_PARSER_H_
#define _ELF_PARSER_H_

#include <ElfHeader.h>
#include <ElfProgHeader.h>
#include <ElfSectionHeader.h>
#include <ElfSymbolTable.h>

class BufferedReader ;
using ELFHeader::ELF32Header ;
using ELFSectionHeader::ELF32SectionHeader ;
using ELFSymbolTable::ELFSymbolTableList ;
using ELFProgramHeader::ELF32ProgramHeader ;

class ELFParser
{
	private:
		const bool m_bReleaseMem ;
		bool m_bObjectState ;
		unsigned m_uiSymTabCount ;

		BufferedReader* m_pBR ;

		ELF32Header* m_pHeader ;
		ELF32SectionHeader* m_pSectionHeader ;
		char* m_pSecHeaderStrTable ;

		ELF32ProgramHeader* m_pProgramHeader ;
		int* m_pSectionTableMap ;
		ELFSymbolTableList* m_pSymbolTable ;

	public:
		ELFParser(ELF32Header* pELFHeader, ELF32SectionHeader* pELFSectionHeader, char* pSecHeaderStrTable) ;
		ELFParser(const char* szFileName) ;
		~ELFParser() ;

		inline bool GetState() { return m_bObjectState ; }

		bool CopyProcessImage(byte* bProcessImage, unsigned uiProcessBase, unsigned uiMaxImageSize) ;
		unsigned CopyELFSecStrTable(char* szSecStrTable) ;
		unsigned CopyELFSectionHeader(ELF32SectionHeader* pSectionHeader) ;

		unsigned* GetGOTAddress(byte* bProcessImage, unsigned uiMinMemAddr) ;
		bool GetNoOfGOTEntries(unsigned* uiNoOfGOTEntries) ;

		void GetMemImageSize(unsigned* uiMinMemAddr, unsigned* uiMaxMemAddr) ;
		unsigned GetProgramStartAddress() ;

		bool GetSectionHeaderByType(unsigned uiType, ELF32SectionHeader** pSectionHeader) ;
		bool GetSectionHeaderByTypeAndName(unsigned uiType, const char* szLikeName, ELF32SectionHeader** pSectionHeader) ;
		bool GetSectionHeaderByIndex(unsigned uiIndex, ELF32SectionHeader** pSectionHeader) ;

		inline const ELF32Header* GetHeader() const { return m_pHeader ; }
		inline ELF32Header* GetHeader() { return m_pHeader ; }

		inline const ELF32SectionHeader* GetSectionHeader() const { return m_pSectionHeader ; }
		inline ELF32SectionHeader* GetSectionHeader() { return m_pSectionHeader ; }

		inline const char* GetSecHeaderStrTable() const { return m_pSecHeaderStrTable ; }
		inline char* GetSecHeaderStrTable() { return m_pSecHeaderStrTable ; }

	private:
		void AllocateHeader() ;
		void DeAllocateHeader() ;
		void AllocateProgramHeader() ;
		void DeAllocateProgramHeader() ;
		void AllocateSectionHeader() ;
		void DeAllocateSectionHeader() ;
		void AllocateSecHeaderStrTable(int iSecSize) ;
		void DeAllocateSecHeaderStrTable() ;
		void AllocateSymbolTable() ;
		void DeAllocateSymbolTable() ;

		void ReleaseMemory() ;

		bool ReadHeader() ;
		bool ReadProgramHeaders() ;
		bool ReadSectionHeaders() ;
		bool ReadSecHeaderStrTable() ;
		bool ReadSymbolTables() ;

		unsigned* GetAddressBySectionName(byte* bProcessImage, unsigned uiMinMemAddr, const char* szSectionName) ;
		bool GetNoOfGOTEntriesBySectionName(unsigned* uiNoOfGOTEntries, const char* szSectionName) ;

		bool CheckMagicSignature(const ELF32Header* pELFHeader) ;
} ;

#endif 
