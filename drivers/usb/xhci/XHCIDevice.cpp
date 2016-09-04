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

#include <XHCIDevice.h>
#include <XHCIController.h>
#include <XHCIContext.h>
#include <TRB.h>
#include <USBStructures.h>
#include <USBDataHandler.h>

XHCIDevice::XHCIDevice(XHCIController& controller, 
                       XHCIPortRegister& port, 
                       unsigned portId, 
                       unsigned slotType) : _portId(portId), _port(port), _controller(controller),
                       _tRing(nullptr), _inputContext(nullptr), _devContext(nullptr)
{
  _slotID = _controller.EnableSlot(slotType);
  if(!_slotID)
    throw upan::exception(XLOC, "Failed to get SlotID");

  _tRing = new TransferRing(64);
  uint32_t trRingPtr = KERNEL_REAL_ADDRESS(_tRing->RingBase());

  _inputContext = new InputContext(_controller.CapReg().IsContextSize64(), port, portId, 0, trRingPtr);
  _devContext = new DeviceContext(_controller.CapReg().IsContextSize64());
  //set A0 and A1 -> Slot and EP0 are affected by command
  _inputContext->Control().SetAddContextFlag(0x3);

  _controller.SetDeviceContext(_slotID, _devContext->Slot());

//TODO: to deal with older devices - you may have to send 2 AddressDevice twice
//first, with block bit set and then with block bit cleared
//With first request, address should be set and slot stat is default (1)
//With second request, slot state should change to Addressed (2)
  _controller.AddressDevice((unsigned)&_inputContext->Control(), _slotID, false);

  if(_devContext->EP0().EPState() != EndPointContext::Running)
    throw upan::exception(XLOC, "After AddressDevice, EndPoint0 is in %d state", _devContext->EP0().EPState());

  if(_devContext->Slot().SlotState() != SlotContext::Addressed)
    throw upan::exception(XLOC, "After AddressDevice, Slot is in %d state", _devContext->Slot().SlotState());

  printf("\n USB Device Address: %x, PortMaxPkSz: %u, ControlEP MaxPktSz: %u", 
    _devContext->Slot().DevAddress(),
    port.MaxPacketSize(),
    _devContext->EP0().MaxPacketSize());

  GetDeviceDescriptor();
  GetStringDescriptorZero();
	SetLangId();
  GetDeviceStringDesc(_manufacturer, _deviceDesc.indexManufacturer);
  GetDeviceStringDesc(_product, _deviceDesc.indexProduct);
  GetDeviceStringDesc(_serialNum, _deviceDesc.indexSerialNum);

  PrintDeviceStringDetails();
//  inputContext->Control().SetAddContextFlag(0x1);
//  ConfigureEndPoint((unsigned)&inputContext->Control(), slotID);
}

XHCIDevice::~XHCIDevice()
{
  delete _devContext;
  delete _inputContext;
  delete _tRing;
}

void XHCIDevice::GetDeviceDescriptor()
{
  //the buffer has to be on kernel heap - a mem area that is 1-1 mapped b/w virtual (page) and physical
  //as it's used by XHCI controller to transfer data
  GetDescriptor(0x100, 0, sizeof(USBStandardDeviceDesc), KERNEL_REAL_ADDRESS(&_deviceDesc));
  _deviceDesc.DebugPrint();
}

void XHCIDevice::GetStringDescriptorZero()
{
	unsigned short usDescValue = (0x3 << 8) ;
	byte* buffer = new byte[8];

	GetDescriptor(usDescValue, 0, -1, KERNEL_REAL_ADDRESS(buffer));

	int iLen = reinterpret_cast<USBStringDescZero*>(buffer)->bLength ;
	printf("\n String Desc Zero Len: %d", iLen) ;

  delete[] buffer;
  buffer = new byte[iLen];

  GetDescriptor(usDescValue, 0, iLen, KERNEL_REAL_ADDRESS(buffer));

  _pStrDescZero = new USBStringDescZero();

	USBDataHandler_CopyStrDescZero(_pStrDescZero, buffer);
	USBDataHandler_DisplayStrDescZero(_pStrDescZero);

  delete[] buffer;
}

void XHCIDevice::GetDeviceStringDesc(upan::string& desc, int descIndex)
{
  desc = "Unknown";
	if(_usLangID == 0)
    return;

	unsigned short usDescValue = (0x3 << 8);
	char* buffer = new char[8];

  GetDescriptor(usDescValue | descIndex, _usLangID, -1, KERNEL_REAL_ADDRESS(buffer));

	int iLen = reinterpret_cast<USBStringDescZero*>(buffer)->bLength ;
  if(iLen == 0)
    return;

  delete buffer;
  buffer = new char[iLen];
  GetDescriptor(usDescValue | descIndex, _usLangID, iLen, KERNEL_REAL_ADDRESS(buffer));

  int j, k;
  for(j = 0, k = 0; j < (iLen - 2); k++, j += 2)
  {
    //TODO: Ignoring Unicode 2nd byte for the time.
    buffer[k] = buffer[j + 2];
  }
  buffer[k] = '\0';

  desc = buffer;
  delete[] buffer;
}

void XHCIDevice::GetDescriptor(uint16_t descValue, uint16_t index, int len, uint32_t dataBuffer)
{
  if(len < 0)
    len = DEF_DESC_LEN;
  //Setup stage
  _tRing->AddSetupStageTRB(0x80, 6, descValue, index, len, TransferType::IN_DATA_STAGE);
  //Data stage
  _tRing->AddDataStageTRB(dataBuffer, len, DataDirection::IN, _port.MaxPacketSize());
  //Status Stage
  const uint32_t trbId = _tRing->AddStatusStageTRB();
  _controller.InitiateTransfer(trbId, _slotID, 1);
}

bool XHCIDevice::GetMaxLun(byte* bLUN)
{
  return false;
}

bool XHCIDevice::CommandReset()
{
  return false;
}

bool XHCIDevice::ClearHaltEndPoint(USBulkDisk* pDisk, bool bIn)
{
  return false;
}

bool XHCIDevice::BulkRead(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
  return false;
}

bool XHCIDevice::BulkWrite(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
  return false;
}

