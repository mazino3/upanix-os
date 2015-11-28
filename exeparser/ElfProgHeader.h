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
#ifndef _ELF_PROG_HEADER_H_
#define _ELF_PROG_HEADER_H_

#include <ElfConstants.h>

namespace ELFProgramHeader
{
	typedef struct {
		uint32_t	p_type ;
		Elf32_Off	p_offset ;
		Elf32_Addr	p_vaddr ;
		Elf32_Addr	p_paddr ;
		uint32_t	p_filesz ;
		uint32_t	p_memsz ;
		uint32_t	p_flags ;
		uint32_t	p_align ;
	} PACKED ELF32ProgramHeader ;

	static const unsigned PT_NULL = 0 ;
	static const unsigned PT_LOAD = 1 ;
	static const unsigned PT_DYNAMIC = 2 ;
	static const unsigned PT_INTERP = 3 ;
	static const unsigned PT_NOTE = 4 ;
	static const unsigned PT_SHLIB = 5 ;
	static const unsigned PT_PHDR = 6 ;
	static const unsigned PT_LOPROC = 0x70000000 ;
	static const unsigned PT_HIPROC = 0x7fffffff  ;

	static const unsigned PF_R = 0x4  ;
	static const unsigned PF_W = 0x2 ;
	static const unsigned PF_X = 0x1 ;

	static const unsigned MAX_PROG_TYPES = 7 ;
	namespace {
		static const char ProgramHeaderType[MAX_PROG_TYPES][30] = {
				"Null Program Header",
				"Loadable Segment",
				"Dynamic Linking Info",
				"Interpreter Info",
				"Auxiliary Info",
				"SHLIB Unspecified",
				"Program Header Info"
		} ;
	} ;

	const char* GetProgHeaderType(unsigned uiPType) ;
} ;

#endif
