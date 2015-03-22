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
#ifndef _ATA_VIA_H_
#define _ATA_VIA_H_

# include <PCIBusHandler.h>
# include <ATADeviceController.h>

#define ATAVIA_SUCCESS		0
#define ATAVIA_FAILURE		1

#define VIA_VENDOR_ID		0x1106

#define VIA_DRIVE_TIMING	0x48
#define VIA_8BIT_TIMING		0x4E
#define VIA_ADDRESS_SETUP	0x4C
#define VIA_CLOCK			33333
#define VIA_UDMA_TIMING		0x50
#define VIA_UDMA_NONE		0x000
#define VIA_UDMA			0x00F
#define VIA_UDMA_33			0x001
#define VIA_UDMA_66			0x002
#define VIA_UDMA_100		0x004
#define VIA_UDMA_133		0x008
#define VIA_BAD_PREQ		01010
#define VIA_BAD_CLK66		0x020
#define VIA_BAD_AST			0x200
#define VIA_IDE_ENABLE		0x40
#define VIA_SET_FIFO		0x40
#define VIA_FIFO_CONFIG		0x43
#define VIA_NO_UNMASK		0x080

typedef struct
{
	char szName[40] ;
	unsigned short usDeviceID ;
	byte bMinRevision ;
	byte bMaxRevision ;
	unsigned short usFlags ;
} VIAIDE ;

typedef struct
{
	PCIEntry pciEntry ;
	VIAIDE* pVIAIDE ;	
} VIAIDEInfo ;

byte ATAVIA_PortConfigure(ATAPort* pPort) ;
void ATAVIA_InitController(const PCIEntry* pPCIEntry, ATAController* pController) ;

#endif
