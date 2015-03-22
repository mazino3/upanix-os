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
#include <NIC.h>
#include <RealtekNIC.h>
#include <stdio.h>

NIC* NIC::Create(const PCIEntry* pPCIEntry)
{
	if(pPCIEntry->usVendorID == 0x10EC)
	{
		RealtekNIC* pNIC = new RealtekNIC(pPCIEntry) ;
		printf("\n Found Realtek Device ID: 0x%x\n", pPCIEntry->usDeviceID) ;
		if(!pNIC->IsSetup())
		{
			delete pNIC ;
			printf("\n --> Failed to initialize\n") ;
			return NULL ;
		}
		return pNIC ;
	}
	return NULL ;
}

