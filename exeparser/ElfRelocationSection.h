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
#ifndef _REL_SECTION_H_
#define _REL_SECTION_H_

#include "ElfConstants.h"

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s, t) (((s) << 8) + (unsigned char)(t))
#define ELF32_REL_ENT(p, off) ((ELF32_Rel*)((byte*)p + off))

namespace ELFRelocSection
{
	typedef struct {
		Elf32_Addr r_offset ;
		Elf32_Word r_info ;
	} PACKED ELF32_Rel ;

	typedef struct {
		Elf32_Addr r_offset ;
		Elf32_Word r_info ;
		Elf32_Sword r_addend ;
	} PACKED ELF32_Rela ;

	static const unsigned R_386_NONE = 0 ;
	static const unsigned R_386_32 = 1 ;
	static const unsigned R_386_PC32 = 2 ;
	static const unsigned R_386_GOT32  = 3 ;
	static const unsigned R_386_PLT32 = 4 ;
	static const unsigned R_386_COPY = 5 ;
	static const unsigned R_386_GLOB_DAT = 6 ;
	static const unsigned R_386_JMP_SLOT = 7 ;
	static const unsigned R_386_RELATIVE = 8 ;
	static const unsigned R_386_GOTOFF = 9 ;
	static const unsigned R_386_GOTPC = 10 ;

	static const char RelocationType[11][40] = {
		"R_386_NONE",
		"R_386_32",
		"R_386_PC32",
		"R_386_GOT32 ",
		"R_386_PLT32",
		"R_386_COPY",
		"R_386_GLOB_DAT",
		"R_386_JMP_SLOT",
		"R_386_RELATIVE",
		"R_386_GOTOFF",
		"R_386_GOTPC",
	} ;
} ;

#endif
