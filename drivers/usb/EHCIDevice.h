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
#ifndef _EHCI_DEVICE_H_
#define _EHCI_DEVICE_H_

#include <EHCIStructures.h>

class EHCIDevice final : public USBDevice
{
  public:
    EHCIDevice(EHCIController&);

    bool GetMaxLun(byte* bLUN);
    bool CommandReset();
    bool ClearHaltEndPoint(USBulkDisk* pDisk, bool bIn);
    bool BulkRead(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen);
    bool BulkWrite(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen);

  private:
    bool SetAddress();
    bool GetDeviceDescriptor(USBStandardDeviceDesc* pDevDesc);
    bool GetDescriptor(unsigned short usDescValue, unsigned short usIndex, int iLen, void* pDestDesc);
    bool GetConfigValue(char& bConfigValue);
    bool GetDeviceStringDetails();
    bool SetConfiguration(char bConfigValue);
    bool CheckConfiguration(char& bConfigValue, char bNumConfigs);
    bool GetConfigDescriptor(char bNumConfigs, USBStandardConfigDesc** pConfigDesc);
    bool GetStringDescriptorZero(USBStringDescZero** ppStrDescZero);
    byte SetupBuffer(EHCIQTransferDesc* pTD, unsigned uiAddress, unsigned uiSize);
    byte SetupAllocBuffer(EHCIQTransferDesc* pTD, unsigned uiSize);
    void DisplayTransactionState(EHCIQueueHead* pQH, EHCIQTransferDesc* pTDStart);

    EHCIController& _controller;
    EHCIQueueHead* _pControlQH;
    EHCIQueueHead* _pBulkInEndPt;
    EHCIQueueHead* _pBulkOutEndPt;

    bool _bFirstBulkRead;
    bool _bFirstBulkWrite;
    EHCIQTransferDesc* _ppBulkReadTDs[ MAX_EHCI_TD_PER_BULK_RW ];
    EHCIQTransferDesc* _ppBulkWriteTDs[ MAX_EHCI_TD_PER_BULK_RW ];
};

#endif
