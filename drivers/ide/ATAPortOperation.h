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
#ifndef _ATA_PORT_OPERATION_H_
#define _ATA_PORT_OPERATION_H_

# include <ATADeviceController.h>

#define ATAPortOperation_SUCCESS			0
#define ATAPortOperation_ERR_SIZE_TOO_BIG	1
#define ATAPortOperation_ERR_MAX_LEN		2
#define ATAPortOperation_FAILURE			3

void ATAPortOperation_PortSelect(ATAPort* pPort, byte bAddress) ;
byte ATAPortOperation_PortConfigure(ATAPort* pPort) ;
byte ATAPortOperation_PortPrepareDMARead(ATAPort* pPort, unsigned uiLength) ;
byte ATAPortOperation_PortPrepareDMAWrite(ATAPort* pPort, unsigned uiLength) ;
byte ATAPortOperation_StartDMA(ATAPort* pPort) ;
byte ATAPortOperation_PortReset(ATAPort* pPort) ;
void ATAPortOperation_CopyToUserBufferFromKernelBuffer(ATAPort* pPort, byte* pBuffer, unsigned uiLength) ;
void ATAPortOperation_CopyToKernelBufferFromUserBuffer(ATAPort* pPort, byte* pBuffer, unsigned uiLength) ;

#endif
