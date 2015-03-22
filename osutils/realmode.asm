;	Mother Operating System - An x86 based Operating System
;	Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa_prajwal@yahoo.co.in'
;	                                                                        
;	This program is free software: you can redistribute it and/or modify
;	it under the terms of the GNU General Public License as published by
;	the Free Software Foundation, either version 3 of the License, or
;	(at your option) any later version.
;	                                                                        
;	This program is distributed in the hope that it will be useful,
;	but WITHOUT ANY WARRANTY; without even the implied warranty of
;	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;	GNU General Public License for more details.
;	                                                                        
;	You should have received a copy of the GNU General Public License
;	along with this program.  If not, see <http://www.gnu.org/licenses/
[ORG 0x8000]
[BITS 32]

MEM_PDBR EQU 0x620000
GDT_DESC_SIZE	EQU		8
SYS_LINEAR_SEL	EQU		GDT_DESC_SIZE * 1
SYS_CODE_SEL	EQU		GDT_DESC_SIZE * 2
SYS_DATA_SEL	EQU		GDT_DESC_SIZE * 3

_start_realmode:

	jmp SYS_CODE_SEL:_entry

	save_idtr:
		dw 0
		dd 0

	pe_stack:
		dd 0

	idt_real:
		dw 0x3ff		; 256 entries, 4b each = 1K
		dd 0			; Real Mode IVT @ 0x0000
	 
	savcr0:
		dd 0			; Storage location for pmode CR0.

_entry
	cli

	mov dword [pe_stack], esp
	sidt [save_idtr]

	mov dword eax, cr0
	mov dword [savcr0], eax
	and eax, 0x7FFFFFFF
	mov dword cr0, eax

	mov dword eax, 0x0
	mov dword cr3, eax

	jmp 0x88:_Entry16BitPM
	
[BITS 16]
_Entry16BitPM:
	mov sp, 0x1000
	mov ax, 0x90
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	and eax, 0xFFFFFFFE
	mov dword cr0, eax

	jmp 0x0:_EntryRealMode
	
_EntryRealMode:

	mov sp, 0x1000
	mov ax, 0
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	jmp 0x0:_EntryRealMode1

_EntryRealMode1:
	LIDT [idt_real]
	;sti
	mov ax, 0x13
	int 0x10 ; this is causing a crash

	mov dword eax, MEM_PDBR
	mov dword cr3, eax

	mov dword eax, [savcr0]
	mov dword cr0, eax

	JMP SYS_CODE_SEL:_ReturnToPE

[BITS 32]
_ReturnToPE:

	mov ax, SYS_DATA_SEL
	mov ds, ax
	mov ss, ax
	mov fs, ax
	mov gs, ax
	mov ax, SYS_LINEAR_SEL
	mov es, ax
	mov esp, [pe_stack]
	
	LIDT [save_idtr]
	
	sti
ret
