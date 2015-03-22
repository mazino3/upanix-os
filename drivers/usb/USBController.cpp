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
# include <USBStructures.h>
# include <USBController.h>
# include <DMM.h>

static int USBController_seqDevAddr ;

static int USBController_seqDriverId ;
static USBDriver* USBController_pDriverList[ MAX_USB_DEVICES ] ;

byte USBController_Initialize()
{
	USBController_seqDevAddr = 1 ;
	USBController_seqDriverId = 0 ;
	return USBController_SUCCESS ;
}

int USBController_RegisterDriver(USBDriver* pDriver)
{
	int iDriverId = USBController_seqDriverId++ ;

	if(iDriverId > MAX_USB_DEVICES)
		return -1 ;

	USBController_pDriverList[ iDriverId ] = pDriver ;
	USBController_pDriverList[ iDriverId ]->bDeviceAdded = false ;

	return iDriverId ;
}

char USBController_GetNextDevNum()
{
	int iDevAddr = USBController_seqDevAddr++ ;
	if(iDevAddr > MAX_USB_DEVICES)
		return -1 ;
	return (char)iDevAddr ;
}

USBDriver* USBController_FindDriver(USBDevice* pUSBDevice)
{
	int i ;
	for(i = 0; i < USBController_seqDevAddr; i++)
	{
		USBDriver* pDriver = USBController_pDriverList[ i ] ;

		if(pDriver != NULL)
		{
			if(!pDriver->bDeviceAdded)
			{
				if(pDriver->AddDevice(pUSBDevice))
				{
					pDriver->bDeviceAdded = true ;
					return pDriver ;
				}
			}
		}
	}

	return NULL ;
}

