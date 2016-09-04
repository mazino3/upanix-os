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
#include <USBStructures.h>
#include <USBController.h>
#include <DMM.h>
#include <set.h>

USBController::USBController() : _seqDevAddr(1)
{
}

void USBController::RegisterDriver(USBDriver* pDriver)
{
  _drivers.push_back(pDriver);
}

int USBController::GetNextDevNum()
{
  return _seqDevAddr++;
}

USBDriver* USBController::FindDriver(USBDevice* pUSBDevice)
{
	for(auto pDriver : _drivers)
	{
    if(pDriver->AddDevice(pUSBDevice))
      return pDriver;
	}

	return nullptr;
}
