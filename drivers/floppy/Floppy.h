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
#ifndef _FLOPPY_H_
#define _FLOPPY_H_

#include <Global.h>
#include <DeviceDrive.h>

#define Floppy_SUCCESS							0
#define Floppy_ERR_TIMEOUT						1
#define Floppy_ERR_UNDERRUN						2
#define Floppy_ERR_OVERRUN						3
#define Floppy_INIT_FAILURE						4
#define Floppy_FAILURE							5
#define Floppy_ERR_INVALID_NO_OF_RESULT_REPLIES	6
#define Floppy_ERR_SEEK							7
#define Floppy_INVALID_SECTOR					8
#define Floppy_ERR_RECALIBERATE					9
#define Floppy_ERR_READ							10
#define Floppy_ERR_DMA							11
#define Floppy_ERR_FORMAT						12
#define Floppy_ERR_INVALID_DRIVE				13

#define MAX_RESULT_PHASE_REPLIES 7

typedef enum
{
	FD_HEAD0 = 0x00,
	FD_HEAD1 = 0x01
} Floppy_HEAD_NO ;

typedef enum
{
	FD_READ,
	FD_WRITE
} Floppy_MODE ;

void Floppy_Initialize() ;
bool Floppy_GetInitStatus() ;
void Floppy_Handler() ;
byte Floppy_IsEnhancedController(bool* boolEnhanced) ;
byte Floppy_Read(const Drive* pDriveInfo, unsigned uiStartSectorNo, unsigned uiEndSectorNo, byte* bSectorBuffer) ;
byte Floppy_Write(const Drive* pDriveInfo, unsigned uiStartSectorNo, unsigned uiEndSectorNo, byte* bSectorBuffer) ;
byte Floppy_Format(const Drive* pDriveInfo) ;

#endif
