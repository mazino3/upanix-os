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
#include <stdio.h>
#include <AsmUtil.h>
#include <IrqManager.h>
#include <PCIBusHandler.h>
#include <ATH9KDevice.h>
#include <NetworkManager.h>

static void WIFINET_IRQHandler()
{
  unsigned GPRStack[NO_OF_GPR];
  AsmUtil_STORE_GPR(GPRStack);
  AsmUtil_SET_KERNEL_DATA_SEGMENTS

  printf("\n WIFINET IRQ");
  if(NetworkManager::Instance().Initialized())
  {
    for(auto d : NetworkManager::Instance().Devices())
      d->NotifyEvent();
  }

  IrqManager::Instance().SendEOI(NetworkManager::Instance().WifiIrq());

  AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
  AsmUtil_RESTORE_GPR(GPRStack);

  asm("leave");
  asm("IRET");
}

NetworkManager::NetworkManager() : _initialized(false), _wifiIrq(nullptr)
{
  _wifiIrq = IrqManager::Instance().RegisterIRQ(PCI_IRQ::WIFINET_IRQ_NO, (unsigned)WIFINET_IRQHandler);
  if(!_wifiIrq)
    throw upan::exception(XLOC, "Failed to register wifi-net irq: %d", WIFINET_IRQ_NO);
  IrqManager::Instance().DisableIRQ(*_wifiIrq);
  //Initialize();
}

void NetworkManager::Initialize()
{
  for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())
  {
    if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE)
      continue ;

    auto device = Probe(*pPCIEntry);
    if(device)
      _devices.push_back(device);
  }
  IrqManager::Instance().EnableIRQ(*_wifiIrq);
  _initialized = true;
}

NetworkDevice* NetworkManager::Probe(PCIEntry& pciEntry)
{
  try
  {
    if(pciEntry.usVendorID == 0x168C && pciEntry.usDeviceID == 0x36)
      return new ATH9KDevice(pciEntry);
  }
  catch(const upan::exception& e)
  {
    e.Print();
  }
  return nullptr;
}
