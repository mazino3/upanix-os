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
#ifndef _USB_DEVICE_H_
#define _USB_DEVICE_H_

#include <Global.h>
#include <USBConstants.h>
#include <USBStructures.h>
#include <exception.h>
#include <string.h>

class USBInterruptDataHandler;

class USBDevice
{
  public:
    USBDevice();
    virtual ~USBDevice() {}

    virtual byte GetMaxLun() = 0;
    virtual bool CommandReset() = 0;
    virtual bool ClearHaltEndPoint(USBulkDisk* pDisk, bool bIn) = 0;
    virtual bool BulkRead(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen) = 0;
    virtual bool BulkWrite(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen) = 0;
    virtual void SetIdle() { throw upan::exception(XLOC, "SetIdle not supported"); }
    virtual void SetupInterruptReceiveData(uint32_t bufferAddress, uint32_t len, USBInterruptDataHandler* handler) { throw upan::exception(XLOC, "StartInterruptDataFeed not supported"); }

  protected:
    void SetLangId();
    void SetConfigIndex(byte bConfigValue);
    void PrintDeviceStringDetails() const;

  public:
    char _devAddr;
    int _iConfigIndex;
    int _iInterfaceIndex;
    char _bInterfaceNumber;

    unsigned short _usLangID;
    upan::string _manufacturer;
    upan::string _product;
    upan::string _serialNum;

    // Single element
    USBStandardDeviceDesc _deviceDesc;
    // Array
    USBStandardConfigDesc* _pArrConfigDesc;
    USBStringDescZero* _pStrDescZero;
      
    USB_CONTROLLER_TYPE _eControllerType;

    void* _pPrivate;
};

#endif
