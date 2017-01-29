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

InputContext::InputContext(bool use64, const XHCIPortRegister& port, uint32_t portId, uint32_t routeString)
{
  unsigned addr = KERNEL_VIRTUAL_ADDRESS(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);
  if(use64)
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
  _ctRing = new TransferRing(64);
  EP0().Init(KERNEL_REAL_ADDRESS(_ctRing->RingBase()), USBStandardEndPt::BI, 0, port.MaxPacketSize(), 0);
  //set A0 and A1 -> Slot and EP0 are affected by command
  Control().SetAddContextFlag(0x3);
}

InputContext::~InputContext()
{
  unsigned addr = KERNEL_REAL_ADDRESS(_control);
  MemManager::Instance().DeAllocatePhysicalPage(addr / PAGE_SIZE);
  delete _devContext;
  delete _ctRing;
  for(auto itring : _itRings)
    delete itring;
  for(auto otring : _otRings)
    delete otring;
}

uint32_t InputContext::InitEP(const USBStandardEndPt& endpoint)
{
  const byte epID = endpoint.Address();
  const uint32_t epIndex = endpoint.Direction() == USBStandardEndPt::IN ? (epID * 2 - 1) : (epID * 2 - 2);

  TransferRing* tRing = new TransferRing(64);
  EP(epIndex).Init(KERNEL_REAL_ADDRESS(tRing->RingBase()), endpoint.Direction(), endpoint.bmAttributes & 0x3, endpoint.wMaxPacketSize, endpoint.bInterval);

  if(endpoint.Direction() == USBStandardEndPt::IN)
    _itRings.push_back(tRing);
  else
    _otRings.push_back(tRing);

  return epIndex;
}

DeviceContext::DeviceContext(bool use64) : _allocated(true)
{
  unsigned addr = KERNEL_VIRTUAL_ADDRESS(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);
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
    MemManager::Instance().DeAllocatePhysicalPage(addr / PAGE_SIZE);
  }
}

void EndPointContext::Init(uint32_t dqPtr, USBStandardEndPt::DirectionTypes dir, byte type, int32_t maxPacketSize, byte interval)
{
  uint32_t epType = 4;
  uint32_t avgTRBLen = maxPacketSize / 2;
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
