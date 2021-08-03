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
#include <ProcessManager.h>
#include <PCIBusHandler.h>
#include <UHCIManager.h>
#include <UHCIController.h>

UNUSED static unsigned short lSupportedLandIds[] = { 0x409, 0 } ;

UHCIManager::UHCIManager()
{
	for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())
	{
		if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		if(pPCIEntry->bInterface == 0x0 && 
			pPCIEntry->bClassCode == PCI_SERIAL_BUS && 
			pPCIEntry->bSubClass == PCI_USB)
		{
      printf("\n Interface = %u, Class = %u, SubClass = %u", pPCIEntry->bInterface, pPCIEntry->bClassCode, pPCIEntry->bSubClass);
      if(!pPCIEntry->BusEntity.NonBridge.bInterruptLine)
      {
        printf("\nUHCI device with no IRQ. Check BIOS/PCI settings!");
        continue ;
        //return UHCIController_FAILURE ;
      }

      /* Enable busmaster */
      unsigned short usCommand ;
      pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
      pPCIEntry->WritePCIConfig(PCI_COMMAND, 2, usCommand | PCI_COMMAND_IO | PCI_COMMAND_MASTER) ;
      
      /* Search for the IO base address */
      unsigned* pBaseAddress = &(pPCIEntry->BusEntity.NonBridge.uiBaseAddress0) ;
      for(int i = 0; i < 6; i++)
      {
        unsigned uiIOBase = pBaseAddress[i] ;
        if(!(uiIOBase & PCI_IO_ADDRESS_SPACE))
          continue ;
			    unsigned uiIOSize = pPCIEntry->GetPCIMemSize(i) ;
        _uhciControllers.push_back(new UHCIController(pPCIEntry, uiIOBase, uiIOSize));
        break;
      }
		}
	}
	
	if(_uhciControllers.size())
    KC::MConsole().LoadMessage("USB UHCI Controller Found", Success) ;
	else
    KC::MConsole().LoadMessage("No USB UHCI Controller Found", Failure) ;
}

void UHCIManager::Probe()
{
	if(_uhciControllers.empty())
    printf("\n No USB UHCI Controller Found on PCI Bus");
	
	for(auto c : _uhciControllers)
    c->Probe();
}

bool UHCIManager::PollWait(unsigned* pPoleAddr, unsigned uiValue)
{
	// 50 ms * 200 = 10 seconds
	int iTotalAttempts = 200 ;

	while(true)
	{
		if(iTotalAttempts <= 0)
			return false ;

		if((*pPoleAddr) == uiValue)
			break ;

		ProcessManager::Instance().Sleep(50) ;
	
		iTotalAttempts-- ;
	}

	return true ;
}

