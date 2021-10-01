/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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
#ifndef _UHCI_DEVICE_H_
#define _UHCI_DEVICE_H_

#include <Global.h>
#include <queue.h>
#include <UHCIStructures.h>
#include <USBController.h>
#include <USBDevice.h>
#include <USBMassBulkStorageDisk.h>

class UHCIController;

class UHCIDevice final : public USBDevice
{
  public:
    UHCIDevice(UHCIController&, unsigned uiPortAddr);

    byte GetMaxLun();
    bool CommandReset();
    bool ClearHaltEndPoint(USBulkDisk* pDisk, bool bIn);
    bool BulkRead(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen);
    bool BulkWrite(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen);

  private:
    void SetAddress();
    void GetDescriptor(unsigned short usDescValue, unsigned short usIndex, int iLen, void* pDestDesc);
    void GetDeviceStringDesc(upan::string& desc, int descIndex);
    void GetDeviceDescriptor(USBStandardDeviceDesc* pDevDesc);
    void CheckConfiguration(byte* bConfigValue);
    void GetConfigDescriptor(USBStandardConfigDesc** pConfigDesc);
    void GetStringDescriptorZero(USBStringDescZero** ppStrDescZero);
    void GetConfiguration(byte* bConfigValue);
    void SetConfiguration(byte bConfigValue);

    unsigned uiIOSize ;
    int iIRQ ;
    int iNumPorts ;

    UHCIController& _controller;
    UHCITransferDesc* _ppBulkReadTDs[ MAX_UHCI_TD_PER_BULK_RW ] ;
    UHCITransferDesc* _ppBulkWriteTDs[ MAX_UHCI_TD_PER_BULK_RW ] ;
    bool _bFirstBulkRead ;
    bool _bFirstBulkWrite ;
    unsigned _portAddr;
};

#endif
