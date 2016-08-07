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
#include <IrqManager.h>
#include <KBDriver.h>

static const IRQ* XHCI_IRQ = nullptr;

static void XHCI_IRQHandler()
{
	unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	AsmUtil_SET_KERNEL_DATA_SEGMENTS

  printf("\n XHCI IRQ");
	//ProcessManager_SignalInterruptOccured(HD_PRIMARY_IRQ) ;
  for(auto c : XHCIManager::Instance().Controllers())
    c->NotifyEvent();

	IrqManager::Instance().SendEOI(*XHCI_IRQ);
	
	AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
	AsmUtil_RESTORE_GPR(GPRStack) ;
	
	asm("leave") ;
	asm("IRET") ;
}

XHCIManager::XHCIManager()
{
  XHCI_IRQ = IrqManager::Instance().RegisterIRQ(XHCI_IRQ_NO, (unsigned)XHCI_IRQHandler);
  if(XHCI_IRQ)
  {
    IrqManager::Instance().DisableIRQ(*XHCI_IRQ);

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

    IrqManager::Instance().EnableIRQ(*XHCI_IRQ);
  }
  else
    printf("Failed to register XHCI IRQ: %d", XHCI_IRQ_NO);

	if(_controllers.size())
		KC::MDisplay().LoadMessage("USB XHCI Controller Found", Success) ;
	else
		KC::MDisplay().LoadMessage("No USB XHCI Controller Found", Failure) ;
}

void XHCIManager::ProbeDevice()
{
  for(auto c : _controllers)
  {
    try
    {
      c->Probe();
    }
    catch(const upan::exception& ex)
    {
      printf("\n Error probing - %s", ex.Error().c_str());
    }
  }
}
