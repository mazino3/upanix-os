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

[EXTERN MEM_PDBR]
[EXTERN CR0_CONTENT]
[GLOBAL Mem_EnablePaging]
[GLOBAL Mem_FlushTLB]
[GLOBAL Mem_FlushTLBPage]

Mem_EnablePaging:
	cli

	mov dword eax, [MEM_PDBR]
	mov dword cr3, eax

	mov dword eax, cr0
	or eax, 0x80000000
	mov dword cr0, eax
	mov dword [CR0_CONTENT], eax
	
	jmp refresh_mem
	
refresh_mem:

	sti
	ret

Mem_FlushTLB:
	push dword	eax
	mov dword	eax, cr3
	mov dword	cr3, eax
	pop dword	eax
	ret

Mem_FlushTLBPage:
	push dword	ebp
	mov dword	ebp, esp
	push dword	ebx
	mov dword	ebx, [ebp]
	invlpg		[ebx]
	pop dword	ebx
	pop dword	ebp
	ret

