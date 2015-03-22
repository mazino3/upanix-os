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
[BITS 32]

ENTRY EQU 0x400000 ; 4 MB + PROCESS BASE = 16 MB ==> 20MB 
INIT EQU 0x400004 ; 
TERM EQU 0x400008 ; 

POP DWORD [ENTRY] ; Entry Address
POP DWORD [INIT] ; Init Address
POP DWORD [TERM] ; TERM Address

MOV DWORD EDX, [INIT]
CMP EDX, 0x00
JZ MAIN
CALL [EDX] ; Call Init

MAIN:

;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 613 ; SYS_CALL_PROC_INIT
;call 0x78:0x00

MOV DWORD EAX, [ENTRY]
CALL EAX ; Call Main

;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 0x20
;push dword 614 ; SYS_CALL_PROC_UINIT
;call 0x78:0x00

TERMINATE:

MOV DWORD EDX, [TERM]
CMP EDX, 0x00
JZ END
CALL [EDX] ; Call Term

END:

PUSHA
PUSHF
POP EAX
MOV EBX, 0x4000
OR EAX, EBX
PUSH EAX
POPF
POPA
IRET
