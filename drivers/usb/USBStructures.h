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
#ifndef _USB_STRUCTURES_H_
#define _USB_STRUCTURES_H_

#include <Global.h>
#include <USBConstants.h>
#include <SCSIHandler.h>

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

typedef struct USBDevice USBDevice ;
typedef struct USBulkDisk USBulkDisk ;
typedef bool (*FuncPtr_GetMaxLun)(USBDevice* pDevice, byte* bLUN) ;
typedef bool (*FuncPtr_CommandReset)(USBDevice* pDevice) ;
typedef bool (*FuncPtr_ClearHaltEP)(USBulkDisk* pDisk, bool bIn) ;
typedef bool (*FuncPtr_BulkReadWrite)(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen) ;

struct USBDevice
{
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
	void* pRawDevice ;

	FuncPtr_GetMaxLun GetMaxLun ;
	FuncPtr_CommandReset CommandReset ;
	FuncPtr_ClearHaltEP ClearHaltEndPoint ;

	FuncPtr_BulkReadWrite BulkRead ;
	FuncPtr_BulkReadWrite BulkWrite ;

	void* pPrivate ;
} ;

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

	char* szProtocolName ;
	SCSIHost* pHost ;
	SCSIDevice** pSCSIDeviceList ;
	byte* pRawAlignedBuffer ;

//	sint32 nIpWaitq;
//	sint32 nIrqUrbSem;
//	struct sUsbPacket *psIrqUrb;
//	uint8 anIrqBuf[ 2 ];
//	uint8 anIrqData[ 2 ];
//	sint32 nCurrentUrbSem;
//	struct sUsbPacket *psCurrentUrb;
//	sint32 nQueueExclusion;
//	atomic nIpWanted;
//	sint32 nHostNumber;
//	sint32 nId;

//	SCSICommand* pCommand ;
} ;

typedef struct
{
	char szName[ 256 ] ;
	bool bDeviceAdded ;

	bool (*AddDevice)(USBDevice* pDevice) ;
	void (*RemoveDevice)(USBDevice* pDevice) ;
} USBDriver ;

#endif
