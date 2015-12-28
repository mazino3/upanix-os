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
#ifndef _USB_STRUCTURES_H_
#define _USB_STRUCTURES_H_

#include <Global.h>
#include <USBConstants.h>
#include <SCSIHandler.h>
#include <string.h>

typedef enum
{
	UHCI_CONTROLLER = 100,
	EHCI_CONTROLLER = 101,
} USB_CONTROLLER_TYPE ;

typedef struct
{
	byte bRequestType;
	byte bRequest ;
	unsigned short usWValue ;
	unsigned short usWIndex ;
	unsigned short usWLength ;
} PACKED USBDevRequest ;

typedef struct
{
	byte bLength ;
	byte bDescType ;
	unsigned short bcdUSB ;
	byte bDeviceClass ;
	byte bDeviceSubClass ;
	byte bDeviceProtocol ;
	byte bMaxPacketSize0 ;

	short sVendorID ;
	short sProductID ;
	short bcdDevice ;
	char indexManufacturer ;
	char indexProduct ;
	char indexSerialNum ;
	char bNumConfigs ;
		
} PACKED USBStandardDeviceDesc ;

typedef struct
{
	char bLength ;
	char bDescriptorType ;
	byte bEndpointAddress ;
	char bmAttributes ;
	unsigned short wMaxPacketSize ;
	char bInterval ;
} PACKED USBStandardEndPt ;

typedef struct
{
	char bLength ;
	char bDescriptorType ;
	char bInterfaceNumber ;
	char bAlternateSetting ;
	char bNumEndpoints ;
	byte bInterfaceClass ;
	byte bInterfaceSubClass ;
	byte bInterfaceProtocol ;
	char iInterface ;
	USBStandardEndPt* pEndPoints ;
} PACKED USBStandardInterface ;

typedef struct
{
	byte bLength ;
	byte bDescriptorType ;
	unsigned short* usLangID ;
} USBStringDescZero ;

typedef struct
{
	char bLength ;
	char bDescriptorType ;
	unsigned short wTotalLength ;
	char bNumInterfaces ;
	char bConfigurationValue ;
	char iConfiguration ;
	byte bmAttributes ;
	char bMaxPower ;
	USBStandardInterface* pInterfaces ;
} PACKED USBStandardConfigDesc ;

class USBDevice;
typedef struct USBulkDisk USBulkDisk ;

class USBDevice
{
  public:
    USBDevice();
    virtual ~USBDevice() {}

    virtual bool GetMaxLun(byte* bLUN) = 0;
    virtual bool CommandReset() = 0;
    virtual bool ClearHaltEndPoint(USBulkDisk* pDisk, bool bIn) = 0;
    virtual bool BulkRead(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen) = 0;
    virtual bool BulkWrite(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen) = 0;

	char devAddr ;
	int iConfigIndex ;
	int iInterfaceIndex ;
	char bInterfaceNumber ;

	unsigned short usLangID ;
	char szManufacturer[ USB_MAX_STR_LEN + 1 ] ;
	char szProduct[ USB_MAX_STR_LEN + 1 ] ;
	char szSerialNum[ USB_MAX_STR_LEN + 1 ] ;

	// Single element
	USBStandardDeviceDesc deviceDesc ;
	// Array
	USBStandardConfigDesc* pArrConfigDesc ;
	USBStringDescZero* pStrDescZero ;
		
	USB_CONTROLLER_TYPE eControllerType ;

	void* pPrivate ;
};

struct USBulkDisk
{
	USBDevice* pUSBDevice ;

	byte bDeviceSubClass ;
	byte bDeviceProtocol ;

	unsigned uiEndPointIn ;
	unsigned uiEndPointOut ;
	unsigned short usInMaxPacketSize ;
	unsigned short usOutMaxPacketSize ;

	byte bEndPointInToggle ;
	byte bEndPointOutToggle ;

	USBStandardEndPt* pEndPointInt ;

	byte bMaxLun ;

	upan::string szProtocolName ;
	SCSIHost* pHost ;
	SCSIDevice** pSCSIDeviceList ;
	byte* pRawAlignedBuffer ;
};

class USBDriver
{
  public:
    USBDriver(const upan::string& name) : _name(name), _deviceAdded(false) {}

    const upan::string& Name() const { return _name; }

    bool AddDevice(USBDevice* d)
    {
      if(_deviceAdded)
        return false;
      _deviceAdded = DoAddDevice(d);
      return _deviceAdded;
    }

    void RemoveDevice(USBDevice* d)
    {
      DoRemoveDevice(d);
      _deviceAdded = false;
    }

  protected:
    virtual bool DoAddDevice(USBDevice*) = 0;
    virtual void DoRemoveDevice(USBDevice*) = 0;

    const upan::string _name;
    bool _deviceAdded;
};

#endif
