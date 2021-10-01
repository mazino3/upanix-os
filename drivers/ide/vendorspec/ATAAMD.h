/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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
#ifndef _ATA_AMD_H_
#define _ATA_AMD_H_

# include <PCIBusHandler.h>
# include <ATADeviceController.h>

#define ATAAMD_SUCCESS		0
#define ATAAMD_FAILURE		1

#define AMD_CLOCK		33333
#define AMD_UDMA		0x00F
#define AMD_UDMA_NONE	0x000
#define AMD_UDMA_33		0x001
#define AMD_UDMA_66		0x002
#define AMD_UDMA_100	0x004
#define AMD_UDMA_133	0x008
#define AMD_BAD_FIFO	0x010
#define AMD_SATA		0x080

typedef struct
{
	char szName[40] ;
	unsigned short usVendorID ;
	unsigned short usDeviceID ;
	unsigned short usBase ;
	unsigned short usFlags ;
} AMDIDE ;

typedef struct
{
	PCIEntry pciEntry ;
	AMDIDE* pAMDIDE ;
} AMDIDEInfo ;

void ATAAMD_PortConfigure(ATAPort* pPort) ;
void ATAAMD_InitController(const PCIEntry* pPCIEntry, ATAController* pController) ;

#endif
