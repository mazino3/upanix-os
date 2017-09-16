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
#include <PCIBusHandler.h>
#include <ATH9K.h>
#include <NetworkManager.h>

NetworkManager::NetworkManager()
{
  //Probe();
}

void NetworkManager::Probe()
{
  for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())
  {
    if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE)
      continue ;

    auto device = Probe(*pPCIEntry);
    if(device)
      _devices.push_back(device);
  }
}

NetworkDevice* NetworkManager::Probe(PCIEntry& pciEntry)
{
  try
  {
    if(pciEntry.usVendorID == 0x168C && pciEntry.usDeviceID == 0x36)
      return new ATH9K(pciEntry);
  }
  catch(const upan::exception& e)
  {
    e.Print();
  }
  return nullptr;
}
