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
#ifndef _ELF_SEC_HEADER_H_
#define _ELF_SEC_HEADER_H_

#include <Global.h>
#include <ElfConstants.h>

namespace ELFSectionHeader
{
	typedef struct {
		uint32_t		sh_name;
		uint32_t		sh_type;
		uint32_t		sh_flags;
		Elf32_Addr		sh_addr;
		Elf32_Off		sh_offset;
		uint32_t		sh_size;
		uint32_t		sh_link;
		uint32_t		sh_info;
		uint32_t		sh_addralign;
		uint32_t		sh_entsize;
	} PACKED ELF32SectionHeader ;

	/* Reserved Section Header Table Indexes */

	static const unsigned SHN_UNDEF	= 0 ;
	static const unsigned SHN_LORESERVE	= 0xff00 ;
	static const unsigned SHN_LOPROC = 0xff00 ;
	static const unsigned SHN_HIPROC = 0xff1f ;
	static const unsigned SHN_ABS = 0xfff1 ;
	static const unsigned SHN_COMMON = 0xfff2 ;
	static const unsigned SHN_HIRESERVE	= 0xffff ;

	/* Section Types */
	static const unsigned SHT_NULL = 0 ;
	static const unsigned SHT_PROGBITS = 1 ;
	static const unsigned SHT_SYMTAB = 2 ;
	static const unsigned SHT_STRTAB = 3 ;
	static const unsigned SHT_RELA = 4 ;
	static const unsigned SHT_HASH = 5 ;
	static const unsigned SHT_DYNAMIC = 6 ;
	static const unsigned SHT_NOTE = 7 ;
	static const unsigned SHT_NOBITS = 8 ;
	static const unsigned SHT_REL = 9 ;
	static const unsigned SHT_SHLIB = 10 ;
	static const unsigned SHT_DYNSYM = 11 ;
	static const unsigned SHT_LOPROC = 0x70000000 ;
	static const unsigned SHT_HIPROC = 0x7fffffff ;
	static const unsigned SHT_LOUSER = 0x80000000 ;
	static const unsigned SHT_HIUSER = 0xffffffff ;

	static const unsigned MAX_SEC_HEADER_TYPES = 12 ;
	namespace
	{
		static const char SectionHeaderType[MAX_SEC_HEADER_TYPES][40] = {
			"Null Section",
			"PROGBITS",
			"Symbol Table Section",
			"String Table Section",
			"Relocation Info With addends",
			"Hash Table Section",
			"Dynamic Link Info",
			"Note Section",
			"NO BITS",
			"Relocation Info WithOut addends",
			"SHLIB Unspecified",
			"Dynamic Symbol Table Section",
		} ;
	} ;

	/* Section Flags */
	static const unsigned SHF_WRITE = 0x1 ;
	static const unsigned SHF_ALLOC = 0x2 ;
	static const unsigned SHF_EXECINSTR = 0x4 ;
	static const unsigned SHF_MASKPROC = 0xf0000000 ;

	const char* GetSecHeaderType(unsigned uiSType) ;
	inline const char* GetSectionName(const char* szStrTable, int iIndex) ;
} ;

#endif
