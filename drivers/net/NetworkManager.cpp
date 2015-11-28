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
#include <NetworkManager.h>
#include <RealtekNIC.h>
#include <PCIBusHandler.h>
#include <Display.h>
#include <PortCom.h>
#include <stdio.h>

NetworkManager::NetworkManager()
{
	PCIEntry* pPCIEntry ;
	byte bControllerFound = false ;

	unsigned uiPCIIndex ;
	for(uiPCIIndex = 0; uiPCIIndex < PCIBusHandler_uiDeviceCount; uiPCIIndex++)
	{
		if(PCIBusHandler_GetPCIEntry(&pPCIEntry, uiPCIIndex) != Success)
			break ;
	
		if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		NIC* pNIC = NIC::Create(pPCIEntry) ;
		if(pNIC)
		{
			bControllerFound = true ;
			m_NICList.PushBack(pNIC) ;
		}
	}
	
	KC::MDisplay().LoadMessage("Network controller setup", bControllerFound ? Success : Failure);
}

