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
#ifndef _ATA_INTEL_H_
#define _ATA_INTEL_H_

# include <PCIBusHandler.h>
# include <ATADeviceController.h>

#define ATAIntel_SUCCESS		0
#define ATAIntel_FAILURE		1

#define INTEL_SATA_DEVICE_ID	0x24D1
#define INTEL_VENDOR_ID			0x8086

#define INTEL_UDMA			0x00F
#define INTEL_UDMA_33		0x001
#define INTEL_UDMA_66		0x002
#define INTEL_UDMA_100		0x004
#define INTEL_UDMA_133		0x008
#define INTEL_NO_MWDMA0		0x10
#define INTEL_INIT			0x20
#define INTEL_80W			0x40

typedef struct
{
	char szName[20] ;
	unsigned short usDeviceID ;
	unsigned short usFlags ;	
} IntelIDE ;

typedef struct
{
	PCIEntry pciEntry ;
} IntelIDEInfo ;

byte ATAIntel_PortConfigure(ATAPort* pPort) ;
void ATAIntel_InitController(const PCIEntry* pPCIEntry, ATAController* pController) ;

#endif
