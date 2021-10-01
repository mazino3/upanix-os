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
[ORG 0x7C00]
[BITS 16]

ENTRY:
	JMP	SHORT BEGIN

; --------------------------------------------------
; data portion of the "DOS BOOT RECORD"
; ----------------------------------------------------------------------
brINT13Flag     DB      90H             ; 0002h - 0EH for INT13 AH=42 READ
brOEM           DB      'MSDOS5.0'      ; 0003h - OEM ID - Windows 95B
brBPS           DW      512             ; 000Bh - Bytes per sector
brSPC           DB      1               ; 000Dh - Sector per cluster
brSc_b4_fat	DW      1               ; 000Eh - Reserved sectors
brFATs          DB      2               ; 0010h - FAT copies
brRootEntries   DW      0E0H		; 0011h - Root directory entries
brSectorCount   DW      2880		; 0013h - Sectors in volume, < 32MB
brMedia         DB      240		; 0015h - Media descriptor
brSPF           DW      9               ; 0016h - Sectors per FAT
brSc_p_trk	DW      18              ; 0018h - Sectors per head/track
brHPC           DW      2		; 001Ah - Heads per cylinder
brSc_b4_prt	DD      0               ; 001Ch - Hidden sectors
brSectors       DD      0	        ; 0020h - Total number of sectors
brDrive		DB      0               ; 0024h - Physical drive no.
		DB      0               ; 0025h - Reserved (FAT32)
		DB      29H             ; 0026h - Extended boot record sig (FAT32)
brSerialNum     DD      404418EAH       ; 0027h - Volume serial number
brLabel         DB      'Joels disk '   ; 002Bh - Volume label
brFSID          DB      'FAT12   '      ; 0036h - File System ID
;------------------------------------------------------------------------

; --- DS ---
MSG DB 'WELCOME TO UPANIX',13,10
    DB 'LOADING......',0

ERR_MSG DB 'READ ERR 1',0
ERR_MSG1 DB 'READ ERR 2',0

BEGIN:

;SET DATA SEGMENT
PUSH CS
POP DS

push 0x0
pop fs
mov dx, 0x0
;PRINT WELCOME MESSAGE
MOV SI,MSG ;LOAD ADDRESS OF MSG
CALL PRINTSTR ;PRINT THE MSG
;mov ax, 0x13
;int 0x10
HLT

PRINTSTR:
LODSB     ;AL = [DS:SI]
OR AL, AL ;SET ZERO FLAG IF AL = 0 -- END OF STRING
JZ PRINTEND
MOV AH, 0EH ;VIDEO FUNCTION TO PRINT A CHAR
MOV BX, 07H ;COLOR
;INT 10H
mov [dx], AL
inc dx
mov [dx], AL
inc dx
JMP PRINTSTR

PRINTEND:
RETN
