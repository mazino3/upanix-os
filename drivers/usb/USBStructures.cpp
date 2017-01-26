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

USBStandardDeviceDesc::USBStandardDeviceDesc() :
  bLength(0), bDescType(0), bcdUSB(0),
  bDeviceClass(0), bDeviceSubClass(0), bDeviceProtocol(0),
  bMaxPacketSize0(0), sVendorID(0), sProductID(0),
  bcdDevice(0), indexManufacturer(0), indexProduct(0),
  indexSerialNum(0), bNumConfigs(0)
{
}

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

USBStandardConfigDesc::USBStandardConfigDesc() : 
  bLength(0), 
  bDescriptorType(0),
  wTotalLength(0),
  bNumInterfaces(0),
  bConfigurationValue(0),
  iConfiguration(0),
  bmAttributes(0),
  bMaxPower(0),
  pInterfaces(NULL)
{
}

void USBStandardConfigDesc::DebugPrint()
{
  printf("\n Len=%d,DType=%d,TotLen=%d,nInterfs=%d,ConfVal=%d,Conf=%d,Attrs=%x,MaxPower=%d", 
    bLength,
    bDescriptorType,
    wTotalLength,
    bNumInterfaces,
    bConfigurationValue,
    iConfiguration,
    bmAttributes,
    bMaxPower);
}

USBStandardInterface::USBStandardInterface() :
	bLength(0),
	bDescriptorType(0),
	bInterfaceNumber(0),
	bAlternateSetting(0),
	bNumEndpoints(0),
	bInterfaceClass(0),
	bInterfaceSubClass(0),
	bInterfaceProtocol(0),
	iInterface(0),
	pEndPoints(NULL)
{
}

void USBStandardInterface::DebugPrint()
{
  printf("\n Len=%d,DType=%d,IntrfceNo=%d,AltSetting=%d,nEpts=%d,IC=%d,ISubC=%d,IProto=%d,Intrfce=%d", 
    bLength,
    bDescriptorType,
    bInterfaceNumber,
    bAlternateSetting,
    bNumEndpoints,
    bInterfaceClass,
    bInterfaceSubClass,
    bInterfaceProtocol,
    iInterface);
}

USBStandardEndPt::USBStandardEndPt() :
	bLength(0),
	bDescriptorType(0),
	bEndpointAddress(0),
	bmAttributes(0),
	wMaxPacketSize(0),
	bInterval(0)
{
}

void USBStandardEndPt::DebugPrint()
{
  printf("\n Len=%d,DType=%d,EpAddr=%x,Attrs=%d,MaxPktSize=%d,Intrvl=%d", 
    bLength, bDescriptorType, bEndpointAddress, bmAttributes, wMaxPacketSize, bInterval);
}
