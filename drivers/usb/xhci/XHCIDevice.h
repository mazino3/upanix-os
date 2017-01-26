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
#ifndef _XHCI_DEVICE_H_
#define _XHCI_DEVICE_H_

#include <Global.h>
#include <USBDevice.h>

class XHCIController;
class XHCIPortRegister;
class TransferRing;
class InputContext;
class DeviceContext;

class XHCIDevice final : public USBDevice
{
  public:
    XHCIDevice(XHCIController& controller, XHCIPortRegister& port, unsigned portId, unsigned slotType);
    ~XHCIDevice();

    bool GetMaxLun(byte* bLUN);
    bool CommandReset();
    bool ClearHaltEndPoint(USBulkDisk* pDisk, bool bIn);
    bool BulkRead(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen);
    bool BulkWrite(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen);

  private:
    void GetDeviceDescriptor();
    void GetConfigDescriptor();
    void GetStringDescriptorZero();
    void GetDeviceStringDesc(upan::string& desc, int descIndex);
    void GetDescriptor(uint16_t descValue, uint16_t index, int len, void* dataBuffer);
    char GetConfigValue();

    uint32_t                _slotID;
    uint32_t                _portId;
    XHCIPortRegister&       _port;
    XHCIController&         _controller;
    TransferRing*           _tRing;
    InputContext*           _inputContext;
    DeviceContext*          _devContext;
};

#endif
