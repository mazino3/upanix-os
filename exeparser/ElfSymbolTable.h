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
#ifndef _ELF_SYM_TAB_HEADER_H_
#define _ELF_SYM_TAB_HEADER_H_

#include <ElfConstants.h>

#define ELF32_ST_BIND(st_info) ( (st_info) >> 4 )
#define ELF32_ST_TYPE(st_info) ( (st_info) & 0xf )
#define ELF32_ST_INFO(bind, type) ( ((bind) << 4) + ((type) & 0xf) )

namespace ELFSymbolTable
{
	typedef	struct	{
		Elf32_Word		st_name;
		Elf32_Addr		st_value;
		Elf32_Word		st_size;
		unsigned char	st_info;
		unsigned char	st_other;
		Elf32_Half		st_shndx;
	} PACKED ELF32SymbolEntry ;

	typedef struct {
		unsigned uiTableSize ;
		ELF32SymbolEntry* SymTabEntries ;
	} PACKED ELFSymbolTableList ;

	/**** Symbol Binding Types *******/
	static const unsigned STB_LOCAL = 0 ;
	static const unsigned STB_GLOBAL = 1 ;
	static const unsigned STB_WEAK = 2 ;
	static const unsigned STB_LOPROC = 13 ;
	static const unsigned STB_HIPROC = 15 ;

	/***** Symbol Types *******/
	static const unsigned STT_NOTYPE = 0 ;
	static const unsigned STT_OBJECT = 1 ;
	static const unsigned STT_FUNC = 2 ;
	static const unsigned STT_SECTION = 3 ;
	static const unsigned STT_FILE = 4 ;
	static const unsigned STT_LOPROC = 13 ;
	static const unsigned STT_HIPROC = 15 ;

	static const unsigned STN_UNDEF = 0 ;

} ;

#endif
