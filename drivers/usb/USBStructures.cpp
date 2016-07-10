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
#include <DMM.h>
#include <USBStructures.h>
#include <stdio.h>

/***********************************************************************************************/

void USBStandardDeviceDesc::DebugPrint() const
{
	printf("\n Len=%d,DType=%d,bcd=%d", 
    (int)bLength, 
    (int)bDescType, 
    (int)bcdUSB);

	printf(",Dev C=%d,SubC=%d,Proto=%d,MaxPkSize=%d", 
    (int)bDeviceClass, 
    (int)bDeviceSubClass, 
    (int)bDeviceProtocol, 
    (int)bMaxPacketSize0);

	printf(",VenID=%d,ProdID=%d,bcdDev=%d,iManufac=%d,iProd=%d,iSlNo=%d,nConfigs=%d", 
    (int)sVendorID,
    (int)sProductID,
    (int)bcdDevice,
    (int)indexManufacturer,
    (int)indexProduct,
    (int)indexSerialNum,
    (int)bNumConfigs);
}

