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
#ifndef _ELF_HEADER_H_
#define _ELF_HEADER_H_

#include <ElfConstants.h>

namespace ELFHeader
{
	static const int EI_NIDENT = 16 ;

	typedef struct 
	{
		unsigned char  e_ident[EI_NIDENT];
		uint16_t       e_type;
		uint16_t       e_machine;
		uint32_t       e_version;
		Elf32_Addr     e_entry;
		Elf32_Off      e_phoff;
		Elf32_Off      e_shoff;
		uint32_t       e_flags;
		uint16_t       e_ehsize;
		uint16_t       e_phentsize;
		uint16_t       e_phnum;
		uint16_t       e_shentsize;
		uint16_t       e_shnum;
		uint16_t       e_shstrndx;
	} PACKED ELF32Header ;

	/************ Elf Header Interpretation ******************/
	/****** Index for ei_ident field of Elf Headers ******/

	static const unsigned EI_MAG0 = 0 ;
	static const unsigned EI_MAG1 = 1 ;
	static const unsigned EI_MAG2 = 2 ;
	static const unsigned EI_MAG3 = 3 ;
	static const unsigned EI_CLASS = 4 ;
	static const unsigned EI_DATA = 5 ;
	static const unsigned EI_VERSION = 6 ;
	static const unsigned EI_PAD = 7 ;

	static const char Classes[3][20] = {
		"Invalid class",
		"32-bit objects",
		"64-bit objects"
	} ;

	static const unsigned ELFCLASSNONE = 0 ;
	static const unsigned ELFCLASS32 = 1 ;
	static const unsigned ELFCLASS64 = 2 ;

	static const char Versions[2][20] = {
		"Invalid Version",
		"Current Version"
	} ;

	static const unsigned EV_NONE = 0 ;
	static const unsigned EV_CURRENT = 1 ;

	static const char DataEncoding[3][30] = {
		"Invalid Data Encoding",
		"2's Comp LSB",
		"2's Comp MSB"
	} ;

	static const unsigned ELFDATANONE = 0 ;
	static const unsigned ELFDATA2LSB = 1 ;
	static const unsigned ELFDATA2MSB = 2 ;

	static const char eType[7][50] = {
		"No file type",
		"Relocatable file",
		"Executable file",
		"Shared object file",
		"Core file",
		"0xff00 Processor-specific",
		"0xffff Processor-specific"
	} ;

	static const unsigned ET_NONE = 0 ;
	static const unsigned ET_REL = 1 ;
	static const unsigned ET_EXEC = 2 ;
	static const unsigned ET_DYN = 3 ;
	static const unsigned ET_CORE = 4 ;
	static const unsigned ET_LOPROC = 5 ;
	static const unsigned ET_HIPROC = 6 ;

	static const char Machine[13][30] = {
		"No machine",
		"AT&T WE 32100",
		"SPARC",
		"Intel 80386",
		"Motorola 68000",
		"Motorola 88000",
		UNKNOWN,
		"Intel 80860",
		"MIPS RS3000",
		UNKNOWN,
		UNKNOWN,
		UNKNOWN,
		UNKNOWN
	} ;

	static const unsigned EM_NONE = 0 ;
	static const unsigned EM_M32 = 1 ;
	static const unsigned EM_SPARC = 2 ;
	static const unsigned EM_386 = 3 ;
	static const unsigned EM_68K = 4 ;
	static const unsigned EM_88K = 5 ;
	static const unsigned EM_860 = 7 ;
	static const unsigned EM_MIPS = 8 ;
} ;

#endif
