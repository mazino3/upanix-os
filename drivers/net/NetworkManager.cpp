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
#include <E1000NICDevice.h>
#include <NetworkManager.h>

NetworkManager::NetworkManager()
{
  //Initialize();
}

void NetworkManager::Initialize()
{
  for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())
  {
    if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE)
      continue;
    Probe(*pPCIEntry);
  }
}

void NetworkManager::Probe(const PCIEntry& pciEntry)
{
  try
  {
    if(pciEntry.usVendorID == 0x168C && pciEntry.usDeviceID == 0x36)
    {
      printf("ATH9K network-card detected");
      //return new ATH9KDevice(pciEntry);
    }
    else if(pciEntry.usVendorID == INTEL_VENDOR_ID && pciEntry.usDeviceID == 0x100E)
    {
      E1000NICDevice::Create(pciEntry);
    }
    else if(pciEntry.usVendorID == INTEL_VENDOR_ID && pciEntry.usDeviceID == 0x153A)
    {
      printf("Ethernet i217-v network-card detected");
    }
  }
  catch(const upan::exception& e)
  {
    e.Print();
  }
}
