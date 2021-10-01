;	Upanix - An x86 based Operating System
;	Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
;
; I am making my contributions/submissions to this project solely in
; my personal capacity and am not conveying any rights to any
; intellectual property of any third parties.
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
[GLOBAL _start]

[GLOBAL GLOBAL_DATA_SEGMENT_BASE]
[GLOBAL SYS_CODE_SELECTOR]
[GLOBAL SYS_LINEAR_SELECTOR]
[GLOBAL SYS_DATA_SELECTOR]
[GLOBAL SYS_TSS_SELECTOR]
[GLOBAL USER_TSS_SELECTOR]
[GLOBAL INT_TSS_SELECTOR_SV]
[GLOBAL INT_TSS_SELECTOR_PF]
[GLOBAL CALL_GATE_SELECTOR]
[GLOBAL SYS_TSS_LINEAR_ADDRESS]
[GLOBAL USER_TSS_LINEAR_ADDRESS]
[GLOBAL INT_TSS_LINEAR_ADDRESS_SV]
[GLOBAL INT_TSS_LINEAR_ADDRESS_PF]
[GLOBAL CR0_CONTENT]

[EXTERN UpanixMain]

SYS_TSS_BASE	EQU 533 * 1024
USER_TSS_BASE	EQU 534 * 1024
INT_TSS_BASE_SV	EQU 535 * 1024
INT_TSS_BASE_PF	EQU 535 * 1024 + 120
KERNEL_STACK	EQU 1 * 1024 * 1024

GDT_DESC_SIZE	EQU	8

UPANIX_KERNEL_BASE_ADDRESS	EQU	1 * 1024 * 1024

SYS_LINEAR_SEL	EQU		GDT_DESC_SIZE * 1
SYS_CODE_SEL	EQU		GDT_DESC_SIZE * 2
SYS_DATA_SEL	EQU		GDT_DESC_SIZE * 3
SYS_TSS_SEL		EQU		GDT_DESC_SIZE * 4
USER_TSS_SEL	EQU		GDT_DESC_SIZE * 5
INT_TSS_SEL_SV	EQU		GDT_DESC_SIZE * 13
INT_TSS_SEL_PF	EQU		GDT_DESC_SIZE * 14
CALL_GATE_SEL	EQU		GDT_DESC_SIZE * 15

[BITS 32]

_start:

	CLI

	MOV AX, SYS_DATA_SEL
	MOV DS, AX
	MOV SS, AX
	MOV FS, AX
	MOV GS, AX
	MOV ESP, KERNEL_STACK
	MOV AX, SYS_LINEAR_SEL
	MOV ES, AX

	MOV DWORD [GLOBAL_DATA_SEGMENT_BASE], UPANIX_KERNEL_BASE_ADDRESS

	MOV WORD [SYS_CODE_SELECTOR], SYS_CODE_SEL
	MOV WORD [SYS_LINEAR_SELECTOR], SYS_LINEAR_SEL
	MOV WORD [SYS_DATA_SELECTOR], SYS_DATA_SEL
	MOV WORD [SYS_TSS_SELECTOR], SYS_TSS_SEL
	MOV WORD [USER_TSS_SELECTOR], USER_TSS_SEL
	MOV WORD [INT_TSS_SELECTOR_SV], INT_TSS_SEL_SV
	MOV WORD [INT_TSS_SELECTOR_PF], INT_TSS_SEL_PF
	MOV WORD [CALL_GATE_SELECTOR], CALL_GATE_SEL

	MOV DWORD [SYS_TSS_LINEAR_ADDRESS], SYS_TSS_BASE
	MOV DWORD [USER_TSS_LINEAR_ADDRESS], USER_TSS_BASE
	MOV DWORD [INT_TSS_LINEAR_ADDRESS_SV], INT_TSS_BASE_SV
	MOV DWORD [INT_TSS_LINEAR_ADDRESS_PF], INT_TSS_BASE_PF

	MOV EAX,CR0
	MOV DWORD [CR0_CONTENT], EAX

	MOV AX, SYS_TSS_SEL
	LTR AX

	;***********************************
	CALL UpanixMain
	;***********************************

	HLT

GLOBAL_DATA_SEGMENT_BASE:
		DD 0
		
SYS_CODE_SELECTOR:
		DD 0
SYS_LINEAR_SELECTOR:
		DD 0
SYS_DATA_SELECTOR:
		DD 0
SYS_TSS_SELECTOR:
		DD 0
USER_TSS_SELECTOR:
		DD 0
INT_TSS_SELECTOR_SV:
		DD 0
INT_TSS_SELECTOR_PF:
		DD 0
CALL_GATE_SELECTOR:
		DD 0

SYS_TSS_LINEAR_ADDRESS:
		DD 0
USER_TSS_LINEAR_ADDRESS:
		DD 0
INT_TSS_LINEAR_ADDRESS_SV:
		DD 0
INT_TSS_LINEAR_ADDRESS_PF:
		DD 0
CR0_CONTENT:
		DD 0
