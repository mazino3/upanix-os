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
#ifndef _ATA_SIS_H_
#define _ATA_SIS_H_

# include <PCIBusHandler.h>
# include <ATADeviceController.h>

#define ATASIS_SUCCESS	0
#define ATASIS_FAILURE	1

#define SIS_VENDOR_ID	0x1039

#define SIS_16			0x01
#define SIS_UDMA_33		0x02
#define SIS_UDMA_66		0x03
#define SIS_UDMA_100a	0x04
#define SIS_UDMA_100	0x05
#define SIS_UDMA_133a	0x06
#define SIS_UDMA_133	0x07
#define SIS_UDMA		0x07

typedef struct 
{
	char szName[40] ;
	unsigned short usDeviceID ;
	unsigned short usFlags ;
} SISIDE ;

typedef struct
{
	PCIEntry pciEntry ;
	unsigned uiSpeed ;
} SISIDEInfo ;

byte ATASIS_PortConfigure(ATAPort* pPort) ;
void ATASIS_InitController(const PCIEntry* pPCIEntry, ATAController* pController) ;

#endif
