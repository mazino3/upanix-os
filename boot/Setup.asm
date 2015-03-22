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

GDT_BASE EQU 520 * 1024
LDT_BASE EQU 522 * 1024

SYS_TSS_BASE	EQU 533 * 1024
USER_TSS_BASE	EQU 534 * 1024
INT_TSS_BASE_SV	EQU 535 * 1024
INT_TSS_BASE_PF	EQU 535 * 1024 + 120

GDT_DESC_SIZE	EQU	8

MOS_SYS_CODE_SELECTOR EQU GDT_DESC_SIZE * 2

MOS_KERNEL_BASE_ADDRESS	EQU	1 * 1024 * 1024
KERNEL_STACK	EQU 1 * 1024 * 1024

MOS_KERNEL_SIZE			EQU	63 * 512	; 33 Sectors (Sector Size = 512)
CURRENT_KERNEL_OFFSET	EQU	2 * 512

[BITS 16]

_start:
	PUSH CS
	POP DS

	CLI

	; ENABLE A20 LINE
	; USED LINUX ARCH/I386/BOOT/SETUP.S
	JMP A20_CHK
	
KBDW0:	JMP SHORT $+2
	IN AL,0x60
KBDWAIT:JMP SHORT $+2
	IN AL,0x64
	TEST AL,1
	JNZ KBDW0
	TEST AL,2
	JNZ KBDWAIT
	RET

A20_CHK:
	CALL KBDWAIT
	MOV AL,0xD1
	OUT 0x64,AL
	CALL KBDWAIT
	MOV AL,0xDF
	OUT 0x60,AL
	CALL KBDWAIT

	XOR EBX,EBX
	MOV BX,DS                       ; BX=SEGMENT
	SHL EBX,4                       ; BX="LINEAR" ADDRESS OF SEGMENT BASE
	
	MOV EAX,EBX
	
	MOV EDX, EAX
	ADD EDX, CURRENT_KERNEL_OFFSET
	MOV [CURRENT_KERNEL_LOCATION], EDX
	
	MOV [GDT_TEMP_CS + 2],AX               ; SET BASE ADDRESS OF 32-BIT SEGMENTS
	MOV [GDT_TEMP_DS + 2],AX               ; SET BASE ADDRESS OF 32-BIT SEGMENTS
	
	SHR EAX,16
	
	MOV [GDT_TEMP_CS + 4],AL
	MOV [GDT_TEMP_DS + 4],AL               ; SET BASE ADDRESS OF 32-BIT SEGMENTS
	MOV [GDT_TEMP_CS + 7],AH
	MOV [GDT_TEMP_DS + 7],AH               ; SET BASE ADDRESS OF 32-BIT SEGMENTS

    LEA EAX,[GDT_TEMP + EBX]             ; EAX=PHYSICAL ADDRESS OF GDT
    MOV [GDTR_TEMP + 2],EAX

	LGDT [GDTR_TEMP]

	MOV AL, 11111111B               ; SELECT TO MASK OF ALL IRQ'S
	OUT 0X21, AL                    ; WRITE IT TO THE PIC CONTROLLER

	MOV EAX,CR0
	OR AL,1
	MOV CR0,EAX

	JMP SYS_CODE_SEL_TEMP:DO_PM          ; JUMPS TO DO_PM
	
[BITS 32]
DO_PM:

	MOV AX, SYS_DATA_SEL_TEMP
	MOV DS, AX
	MOV FS, AX
	MOV GS, AX
	MOV SS, AX
	MOV ESP, KERNEL_STACK
	MOV AX, SYS_LINEAR_SEL_TEMP
	MOV ES, AX
	
	LEA ESI, [LOADMSG]                  	; -> "LOADING DONE"
	MOV EDI, 0xB8000 + (80 * 3 + 4) * 2  ; ROW 3, COLUMN 4
	MOV ECX, 52
	CLD
	REP MOVSB

	; SET UP REAL GDT
	
	XOR EBX,EBX
	MOV EBX,MOS_KERNEL_BASE_ADDRESS ; BX=SEGMENT
	
	MOV EAX,EBX
	
	MOV WORD [GDT_CS + 2],AX               ; SET BASE ADDRESS OF 32-BIT SEGMENTS
	MOV WORD [GDT_CS1 + 2],AX               ; SET BASE ADDRESS OF 32-BIT SEGMENTS
	MOV WORD [GDT_DS + 2],AX
	MOV WORD [GDT_DS1 + 2],AX
	
	SHR EAX,16
	
	MOV BYTE [GDT_CS + 4],AL
	MOV BYTE [GDT_CS1 + 4],AL
	MOV BYTE [GDT_DS + 4],AL
	MOV WORD [GDT_DS1 + 2],AX

	MOV BYTE [GDT_CS + 7],AH
	MOV BYTE [GDT_CS1 + 7],AH
	MOV BYTE [GDT_DS + 7],AH
	MOV WORD [GDT_DS1 + 2],AX

	; SYS_TSS INIT
	MOV EAX, SYS_TSS_BASE
	MOV WORD [GDT_SYS_TSS + 2], AX
	SHR EAX, 16
	MOV BYTE [GDT_SYS_TSS + 4], AL
	MOV BYTE [GDT_SYS_TSS + 7], AH

	; USER_TSS INIT
	MOV EAX, USER_TSS_BASE
	MOV WORD [GDT_USER_TSS + 2], AX
	SHR EAX, 16
	MOV BYTE [GDT_USER_TSS + 4], AL
	MOV BYTE [GDT_USER_TSS + 7], AH

	; INT_TSS INIT FOR INVALID TSS EX
	MOV EAX, INT_TSS_BASE_SV
	MOV WORD [GDT_INT_TSS_SV + 2], AX
	SHR EAX, 16
	MOV BYTE [GDT_INT_TSS_SV + 4], AL
	MOV BYTE [GDT_INT_TSS_SV + 7], AH

	; INT_TSS INIT FOR PAGE FAULT EX
	MOV EAX, INT_TSS_BASE_PF
	MOV WORD [GDT_INT_TSS_PF + 2], AX
	SHR EAX, 16
	MOV BYTE [GDT_INT_TSS_PF + 4], AL
	MOV BYTE [GDT_INT_TSS_PF + 7], AH

	XOR EBX,EBX
	MOV EBX,LDT_BASE
	MOV EAX,EBX
	MOV WORD [GDT_LDT_DESC + 2],AX               ; SET BASE ADDRESS OF 32-BIT SEGMENTS
	SHR EAX,16
	MOV BYTE [GDT_LDT_DESC + 4],AL
	MOV BYTE [GDT_LDT_DESC + 7],AH

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; COPY REAL GDT TO PROPER POSITION
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	MOV ESI, GDT
	MOV EDI, GDT_BASE
	MOV ECX, GDT_LENGTH
	CLD
	REP MOVSB

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; LOAD MOS KERNEL TO 1MB LOCATION IN RAM
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LGDT [GDTR]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	MOV ESI, [CURRENT_KERNEL_LOCATION]

	MOV AX, SYS_LINEAR_SEL_TEMP
	MOV DS, AX

	MOV EDI, MOS_KERNEL_BASE_ADDRESS
	MOV ECX, MOS_KERNEL_SIZE
	CLD
	REP MOVSB

	JMP MOS_SYS_CODE_SELECTOR:0x0000
	HLT 

LOADMSG:   DB "L O A D I N G   M O S   K E R N E L   I S   D O N E "

GDTR_TEMP:	DW GDT_END_TEMP - GDT_TEMP - 1		; GDT LIMIT
			DD 0								; (GDT BASE GETS SET ABOVE)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	TEMP GLOBAL DESCRIPTOR TABLE (GDT)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; NULL DESCRIPTOR
GDT_TEMP:	
		DW 0			; LIMIT 15:0
		DW 0			; BASE 15:0
		DB 0			; BASE 23:16
		DB 0			; TYPE
		DB 0			; LIMIT 19:16, FLAGS
		DB 0			; BASE 31:24

; LINEAR DATA SEGMENT DESCRIPTOR
SYS_LINEAR_SEL_TEMP	EQU	$-GDT_TEMP
	DW 0XFFFF		; LIMIT 0XFFFFF
	DW 0			; BASE 0
	DB 0
	DB 0X92			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0XCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

; CODE SEGMENT DESCRIPTOR
SYS_CODE_SEL_TEMP	EQU	$-GDT_TEMP
GDT_TEMP_CS:
	DW 0XFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0X9A			; PRESENT, RING 0, CODE, NON-CONFORMING, READABLE
	DB 0XCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

; DATA SEGMENT DESCRIPTOR
SYS_DATA_SEL_TEMP	EQU	$-GDT_TEMP
GDT_TEMP_DS:
	DW 0XFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0X92			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0XCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

GDT_END_TEMP:

CURRENT_KERNEL_LOCATION:
	DD 0

;;;;;;;;;;;;;;;;;;;;
; REAL GDT 
;;;;;;;;;;;;;;;;;;;;


GDTR:	DW GDT_END - GDT - 1		; GDT LIMIT
		DD GDT_BASE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	GLOBAL DESCRIPTOR TABLE (GDT)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; NULL DESCRIPTOR
GDT:	DW 0			; LIMIT 15:0
		DW 0			; BASE 15:0
		DB 0			; BASE 23:16
		DB 0			; TYPE
		DB 0			; LIMIT 19:16, FLAGS
		DB 0			; BASE 31:24

; LINEAR DATA SEGMENT DESCRIPTOR
	DW 0xFFFF		; LIMIT 0XFFFFF
	DW 0			; BASE 0
	DB 0
	DB 0x92			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

; CODE SEGMENT DESCRIPTOR
GDT_CS:  
	DW 0xFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0x9A			; PRESENT, RING 0, CODE, NON-CONFORMING, READABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

; DATA SEGMENT DESCRIPTOR
GDT_DS:
	DW 0xFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0x92			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

GDT_SYS_TSS:
	DW 103
	DW 0			; SET ABOVE
	DB 0
	DB 0x89			; PRESENT, RING 0, 32-BIT AVAILABLE TSS
	DB 0
	DB 0

GDT_USER_TSS:		;0x28
	DW 103
	DW 0			; SET ABOVE
	DB 0
	DB 10001001b	; PRESENT, RING 0, 32-BIT AVAILABLE TSS
	DB 0
	DB 0

GDT_TASK_GATE1:		;0x30
	DW 0
	DW 0x68			; SET ABOVE
	DB 0
	DB 11100101b	; PRESENT, RING 0, 32-BIT AVAILABLE TSS
	DB 0
	DB 0

; CODE SEGMENT DESCRIPTOR
GDT_CS1:  
	DW 0xFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0xFA			; PRESENT, RING 0, CODE, NON-CONFORMING, READABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

; DATA SEGMENT DESCRIPTOR
GDT_DS1:
	DW 0xFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0xF2			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

; LINEAR DATA SEGMENT DESCRIPTOR
	DW 0xFFFF		; LIMIT 0XFFFFF
	DW 0			; BASE 0
	DB 0
	DB 0xF2			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

GDT_LDT_DESC:
	DW 0xFF		; LIMIT 0XFFFFF
	DW 0			; BASE 0
	DB 0
	DB 0x82			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0xC0                ; PAGE-GRANULAR, 32-BIT
	DB 0

; DATA SEGMENT DESCRIPTOR
GDT_DS_P1:
	DW 0xFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0xB2			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

; DATA SEGMENT DESCRIPTOR
GDT_DS_P2:
	DW 0xFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0xD2			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

GDT_INT_TSS_SV:		;
	DW 103
	DW 0			; SET ABOVE
	DB 0
	DB 10001001b	; PRESENT, RING 0, 32-BIT AVAILABLE TSS
	DB 0
	DB 0

GDT_INT_TSS_PF:		;
	DW 103
	DW 0			; SET ABOVE
	DB 0
	DB 10001001b	; PRESENT, RING 0, 32-BIT AVAILABLE TSS
	DB 0
	DB 0

GDT_CALL_GATE:
	DW 0
	DW 0			; SET ABOVE
	DB 0x00
	DB 11101100b	; PRESENT, RING 0, 32-BIT AVAILABLE TSS
	DB 0
	DB 0

GDT_END:

GDT_LENGTH EQU GDT_END - GDT

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	LOCAL DESCRIPTOR TABLE (LDT)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; NULL DESCRIPTOR
LDT:	DW 0			; LIMIT 15:0
		DW 0			; BASE 15:0
		DB 0			; BASE 23:16
		DB 0			; TYPE
		DB 0			; LIMIT 19:16, FLAGS
		DB 0			; BASE 31:24

; LINEAR DATA SEGMENT DESCRIPTOR
	DW 0xFFFF		; LIMIT 0XFFFFF
	DW 0			; BASE 0
	DB 0
	DB 0xF2			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

; CODE SEGMENT DESCRIPTOR
LDT_CS:  
	DW 0xFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0xFA			; PRESENT, RING 0, CODE, NON-CONFORMING, READABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

; DATA SEGMENT DESCRIPTOR
LDT_DS:
	DW 0xFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0xF2			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

; DATA SEGMENT DESCRIPTOR
LDT_SS:
	DW 0xFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0xF2			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

; DATA SEGMENT DESCRIPTOR
LDT_CG_SS:
	DW 0xFFFF               ; LIMIT 0XFFFFF
	DW 0			; (BASE GETS SET ABOVE)
	DB 0
	DB 0x92			; PRESENT, RING 0, DATA, EXPAND-UP, WRITABLE
	DB 0xCF                 ; PAGE-GRANULAR, 32-BIT
	DB 0

LDT_END:

LDT_LENGTH EQU LDT_END - LDT 

