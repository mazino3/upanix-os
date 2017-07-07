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
#include <Display.h>
#include <EHCIController.h>
#include <EHCIManager.h>

EHCIManager::EHCIManager()
{
	for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())
	{
		if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		if(pPCIEntry->bInterface == 32 && 
			pPCIEntry->bClassCode == PCI_SERIAL_BUS && 
			pPCIEntry->bSubClass == PCI_USB)
		{
      printf("\n Interface = %u, Class = %u, SubClass = %u", pPCIEntry->bInterface, pPCIEntry->bClassCode, pPCIEntry->bSubClass);
      int memMapIndex = _controllers.size();
      try
      {
        _controllers.push_back(new EHCIController(pPCIEntry, memMapIndex));
      }
      catch(const upan::exception& ex)
      {
        printf("\n Failed to add EHCI controller - Reason: %s", ex.ErrorMsg().c_str());
      }
		}
	}
	
	if(_controllers.size())
		KC::MDisplay().LoadMessage("USB EHCI Controller Found", Success) ;
	else
		KC::MDisplay().LoadMessage("No USB EHCI Controller Found", Failure) ;
}

void EHCIManager::ProbeDevice()
{
  for(auto c : _controllers)
  {
    try
    {
      c->Probe();
    }
    catch(const upan::exception& ex)
    {
      printf("\n Error probing - %s", ex.ErrorMsg().c_str());
    }
  }
}

void EHCIManager::RouteToCompanionController()
{
  for(auto c : _controllers)
	{
    try
    {
  		c->PerformBiosToOSHandoff();
	  	c->SetConfigFlag(false);
    }
    catch(const upan::exception& ex)
    {
      printf("%s", ex.ErrorMsg().c_str());
    }
	}
}
