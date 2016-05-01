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
#include <Global.h>
#include <Display.h>
#include <MemConstants.h>
#include <XHCIController.h>
#include <XHCIManager.h>

XHCIManager::XHCIManager()
{
	for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())
	{
		if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		if(pPCIEntry->bInterface == 48 && 
			pPCIEntry->bClassCode == PCI_SERIAL_BUS && 
			pPCIEntry->bSubClass == PCI_USB)
		{
      printf("\n Interface = %u, Class = %u, SubClass = %u", pPCIEntry->bInterface, pPCIEntry->bClassCode, pPCIEntry->bSubClass);
      try
      {
        _controllers.push_back(new XHCIController(pPCIEntry));
      }
      catch(const upan::exception& ex)
      {
        printf("\n Failed to add XHCI controller - Reason: %s", ex.Error().c_str());
      }
		}
	}
	
	if(_controllers.size())
		KC::MDisplay().LoadMessage("USB XHCI Controller Found", Success) ;
	else
		KC::MDisplay().LoadMessage("No USB XHCI Controller Found", Failure) ;
}
