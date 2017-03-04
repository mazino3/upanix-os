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

#include <MemManager.h>
#include <Alloc.h>
#include <XHCIContext.h>
#include <XHCIStructures.h>
#include <TRB.h>
#include <XHCIController.h>

InputContext::InputContext(XHCIController& controller, uint32_t slotID, const XHCIPortRegister& port,
                           uint32_t portId, uint32_t routeString) : _slotID(slotID), _controller(controller)
{
  unsigned addr = KERNEL_VIRTUAL_ADDRESS(MemManager::Instance().AllocatePageForKernel() * PAGE_SIZE);
  if(_controller.CapReg().IsContextSize64())
  {
    auto context64 = new ((void*)addr)InputContext64();
    _control = &context64->_controlContext;
    _devContext = new DeviceContext(context64->_deviceContext);
  }
  else
  {
    auto context32 = new ((void*)addr)InputContext32();
    _control = &context32->_controlContext;
    _devContext = new DeviceContext(context32->_deviceContext);
  }
  Slot().Init(portId, routeString, port.PortSpeedID());
  _controlEP = new ControlEndPoint(*this, port.MaxPacketSize());
  //set A0 and A1 -> Slot and EP0 are affected by command
  Control().SetAddContextFlag(0x3);
}

InputContext::~InputContext()
{
  unsigned addr = KERNEL_REAL_ADDRESS(_control);
  MemManager::Instance().DeAllocatePageForKernel(addr / PAGE_SIZE);
  delete _devContext;
  delete _controlEP;
  for(auto inEP : _inEPs)
    delete inEP;
  for(auto outEP : _outEPs)
    delete outEP;
}

uint32_t InputContext::AddEndPoint(const USBStandardEndPt& endpoint)
{
  if(endpoint.Direction() == USBStandardEndPt::IN)
    return (new InEndPoint(*this, endpoint))->Id();
  else
    return (new OutEndPoint(*this, endpoint))->Id();
}

void InputContext::SendCommand(uint32_t bmRequestType, uint32_t bmRequest, 
                               uint32_t wValue, uint32_t wIndex, uint32_t wLength, 
                               TransferType trt, void* dataBuffer)
{
  const uint32_t trbId = _controlEP->SetupTransfer(bmRequestType, bmRequest, wValue, wIndex, wLength, trt, dataBuffer);
  _controller.InitiateTransfer(trbId, _slotID, _controlEP->Id());
}

void InputContext::SendData(uint32_t bufferAddress, uint32_t len)
{
  const uint32_t trbId = OutEP().SetupTransfer(bufferAddress, len);
  _controller.InitiateTransfer(trbId, _slotID, OutEP().Id());
}

void InputContext::ReceiveData(uint32_t bufferAddress, uint32_t len)
{
  const uint32_t trbId = InEP().SetupTransfer(bufferAddress, len);
  _controller.InitiateTransfer(trbId, _slotID, InEP().Id());
}

DeviceContext::DeviceContext(bool use64) : _allocated(true)
{
  unsigned addr = KERNEL_VIRTUAL_ADDRESS(MemManager::Instance().AllocatePageForKernel() * PAGE_SIZE);
  if(use64)
    Init(*new ((void*)addr)DeviceContext64());
  else
    Init(*new ((void*)addr)DeviceContext32());
}

DeviceContext::DeviceContext(DeviceContext32& dc32) : _allocated(false)
{
  Init(dc32);
}

DeviceContext::DeviceContext(DeviceContext64& dc64) : _allocated(false)
{
  Init(dc64);
}

template <typename DC>
void DeviceContext::Init(DC& dc)
{
  _slot = &dc._slotContext;
  _ep0 = &dc._epContext0;
  _eps = dc._epContexts;
}

DeviceContext::~DeviceContext()
{
  if(_allocated)
  {
    unsigned addr = KERNEL_REAL_ADDRESS(_slot);
    MemManager::Instance().DeAllocatePageForKernel(addr / PAGE_SIZE);
  }
}

EndPoint::EndPoint(uint32_t maxPacketSize) : _id(0), _ep(nullptr), _tRing(new TransferRing(64)), _maxPacketSize(maxPacketSize)
{
}

EndPoint::~EndPoint()
{
  delete _tRing;
}

ControlEndPoint::ControlEndPoint(InputContext& inContext, int32_t maxPacketSize) : EndPoint(maxPacketSize)
{
  _ep = &inContext.EP0();
  _ep->Init(KERNEL_REAL_ADDRESS(_tRing->RingBase()), USBStandardEndPt::BI, 0, maxPacketSize, 0);
  _id = 1;
}

uint32_t ControlEndPoint::SetupTransfer(uint32_t bmRequestType, uint32_t bmRequest, 
                                        uint32_t wValue, uint32_t wIndex, uint32_t wLength, 
                                        TransferType trt, void* dataBuffer)
{
  //Setup stage
  _tRing->AddSetupStageTRB(bmRequestType, bmRequest, wValue, wIndex, wLength, trt);
  //Data stage
  if(trt != TransferType::NO_DATA_STAGE)
  {
    const auto dir = trt == TransferType::IN_DATA_STAGE ? DataDirection::IN : DataDirection::OUT;
    _tRing->AddDataStageTRB(KERNEL_REAL_ADDRESS(dataBuffer), wLength, dir, _maxPacketSize);
  }
  //Status Stage
  uint32_t statusDir = DataDirection::IN;
  if(wLength > 0 && trt == TransferType::IN_DATA_STAGE)
    statusDir = DataDirection::OUT;
  return _tRing->AddStatusStageTRB(statusDir);
}

InOutEndPoint::InOutEndPoint(uint32_t epOffset, InputContext& inContext, const USBStandardEndPt& endpoint) : EndPoint(endpoint.wMaxPacketSize)
{
  const byte epID = endpoint.Address();
  const uint32_t epIndex = epID * 2 - epOffset;

  _ep = &inContext.EP(epIndex);
  _ep->Init(KERNEL_REAL_ADDRESS(_tRing->RingBase()), endpoint.Direction(), endpoint.bmAttributes & 0x3, endpoint.wMaxPacketSize, endpoint.bInterval);
  _id = epIndex + 2;
}

InEndPoint::InEndPoint(InputContext& inContext, const USBStandardEndPt& endpoint) : InOutEndPoint(1, inContext, endpoint)
{
  inContext.AddInEP(this);
}

uint32_t InEndPoint::SetupTransfer(uint32_t bufferAddress, uint32_t len)
{
  return _tRing->AddDataTRB(bufferAddress, len, DataDirection::IN, _maxPacketSize);
}

OutEndPoint::OutEndPoint(InputContext& inContext, const USBStandardEndPt& endpoint) : InOutEndPoint(2, inContext, endpoint)
{
  inContext.AddOutEP(this);
}

uint32_t OutEndPoint::SetupTransfer(uint32_t bufferAddress, uint32_t len)
{
  return _tRing->AddDataTRB(bufferAddress, len, DataDirection::OUT, _maxPacketSize);
}

void EndPointContext::Init(uint32_t dqPtr, USBStandardEndPt::DirectionTypes dir, byte type, int32_t maxPacketSize, byte interval)
{
  uint32_t epType = 4;

  uint32_t avgTRBLen;
  if(dir == USBStandardEndPt::BI)
    avgTRBLen = 8;
  else
  {
   avgTRBLen = maxPacketSize / 2;
   if(avgTRBLen == 0)
      avgTRBLen = 1;
  }

  switch (type)
  {
    case 0:
      epType = 4;
      avgTRBLen = 8;
      break;
    case 1:
      epType = 1;
      break;
    case 2:
      epType = 2;
      break;
    case 3:
      epType = 3;
      break;
  }

  if(dir == USBStandardEndPt::IN)
    epType += 4;

  EPType(epType);

  //Interval, MaxPStreams = 0, Mult = 0
  _context1 &= (0xFF008000 | ((interval & 0xFF) << 16));
  //Max packet size
  _context2 = (_context2 & ~(0xFFFF << 16)) | (maxPacketSize << 16);
  //Max Burst Size = 0
  _context2 = _context2 & ~(0xFF << 8);
  //Error count = 3
  _context2 = (_context2 & ~(0x7)) | 0x6;
  //Average TRB Len
  _context3 = avgTRBLen;
  //TR DQ Ptr + DCS = 1
  _trDQPtr = (uint64_t)(dqPtr | 0x1);
}
