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
OUTPUT_FORMAT("elf32-i386", "elf32-i386","elf32-i386")
OUTPUT_ARCH(i386)

phys	= 0x100000;

ENTRY(_start)
SECTIONS
{
	. = phys;
	.text phys : AT(phys) { 
		h1 = .;
		*(.text)
	}

	.data : AT(phys + ( h2 - h1 ) ) {
		h2 = .;
		*(data*)
	}

	.bss : AT(phys + (  h3 - h1 ) ) {
		h3 = .;
		*(bss*)
	}

	.ksymtab :  AT(phys + ( h4 - h1 ) ) {
		h4 = .;
		*(ksymtab*)
	}
	
	.rodata : AT(phys + ( h7 - h1 ) ) {
		h7 = .;
		*(rodata);
	}
	. = ALIGN(4096);
	_end = .;
	end =.;

	/DISCARD/ : { *(.comment .note .eh_frame .dtors) }
}
