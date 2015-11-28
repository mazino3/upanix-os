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
#ifndef _KB_DRIVER_H_
#define _KB_DRIVER_H_

#include <Global.h>

#define KBDriver_SUCCESS 0
#define KBDriver_ERR_BUFFER_EMPTY 1
#define KBDriver_ERR_BUFFER_FULL 2

#define KB_DATA_PORT 0x60
#define KB_STAT_PORT 0x64

void KBDriver_Initialize() ;
void KBDriver_Handler() ;
byte KBDriver_GetCharInBlockMode(byte *data) ;
byte KBDriver_GetCharInNonBlockMode(byte *data) ;
byte KBDriver_GetFromQueueBuffer(byte *data) ;
byte KBDriver_PutToQueueBuffer(byte data) ;
byte KBDriver_WaitForWrite() ;
byte KBDriver_WaitForRead() ;
void KBDriver_Reboot() ;

#endif

