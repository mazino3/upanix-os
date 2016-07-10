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
#ifndef _XHCI_CONTEXT_H_
#define _XHCI_CONTEXT_H_

#include <exception.h>

class XHCIPortRegister;

class ReservedPadding
{
  public:
    ReservedPadding() : _reserved1(0), _reserved2(0),
      _reserved3(0), _reserved4(0), _reserved5(0),
      _reserved6(0), _reserved7(0), _reserved8(0)
    {
    }
  private:
    uint32_t _reserved1;
    uint32_t _reserved2;
    uint32_t _reserved3;
    uint32_t _reserved4;
    uint32_t _reserved5;
    uint32_t _reserved6;
    uint32_t _reserved7;
    uint32_t _reserved8;
} PACKED;

class InputControlContext
{
  public:
    InputControlContext() : _dropContextFlags(0), _addContextFlags(0),
      _reserved1(0), _reserved2(0), _reserved3(0), _reserved4(0),
      _reserved5(0), _context(0)
    {
    }
    void SetAddContextFlag(uint32_t val)
    {
      _addContextFlags = val;
    }
    void DebugPrint()
    {
      printf("\n ICC DC: %x, AC: %x, C: %x", _dropContextFlags, _addContextFlags, _context);
    }
  private:
    uint32_t _dropContextFlags;
    uint32_t _addContextFlags;
    uint32_t _reserved1;
    uint32_t _reserved2;
    uint32_t _reserved3;
    uint32_t _reserved4;
    uint32_t _reserved5;
    uint32_t _context;
} PACKED;

class SlotContext
{
  public:
    enum State { Disabled_Enabled, Default, Addressed, Configured, Undefined };

    SlotContext() : _context1(0), _context2(0), _context3(0), _context4(0),
      _reserved1(0), _reserved2(0), _reserved3(0), _reserved4(0)
    {
    }

    void Init(uint32_t portId, uint32_t routeString, uint32_t speed)
    {
      _context1 = (_context1 & ~(0xFFFFF)) | routeString;
      _context1 = (_context1 & ~(0x00F00000)) | (speed << 20);
      //context entries
      _context1 = (_context1 & ~(0x1F000000)) | (1 << 27);
      //root hub port number
      _context2 = (_context2 & ~(0x00FF0000)) | (portId << 16);
    }

    State SlotState() const
    {
      unsigned state = _context4 >> 27;
      switch(state)
      {
        case 0:
        case 1:
        case 2:
        case 3:
          return static_cast<State>(state);
      }
      return State::Undefined;
    }
  
    unsigned DevAddress()
    {
      return _context4 & 0xFF;
    }

    void DebugPrint()
    {
      printf("\n Slot C1: %x, C2: %x, C3: %x, C4: %x", _context1, _context2, _context3, _context4);
    }
  private:
    uint32_t _context1;
    uint32_t _context2;
    uint32_t _context3;
    uint32_t _context4;

    uint32_t _reserved1;
    uint32_t _reserved2;
    uint32_t _reserved3;
    uint32_t _reserved4;
} PACKED;

class EndPointContext
{
  public:
    enum State { Disables, Running, Halted, Stopped, Error, Undefined };

    EndPointContext() : _context1(0), _context2(0),
      _trDQPtr(0), _context3(0), _reserved1(0),
      _reserved2(0), _reserved3(0)
    {
    } 

    void EP0Init(unsigned dqPtr, int32_t maxPacketSize)
    {
      //Control EP
      EPType(4);
      //Interval = 0, MaxPStreams = 0, Mult = 0
      _context1 &= 0xFF008000;
      //Max packet size
      _context2 = (_context2 & ~(0xFFFF << 16)) | (maxPacketSize << 16);
      //Max Burst Size = 0
      _context2 = _context2 & ~(0xFF << 8);
      //Error count = 3
      _context2 = (_context2 & ~(0x7)) | 0x6;
      //Average TRB Len
      _context3 = 8;
      //TR DQ Ptr + DCS = 1
      _trDQPtr = (uint64_t)(dqPtr | 0x1);
    }

    uint32_t MaxPacketSize() const
    {
      return (_context2 >> 16) & 0xFFFF;
    }

    State EPState() const
    {
      unsigned state =_context1 & 0x7;
      switch(state)
      {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4: 
          return static_cast<State>(state);
      }
      return State::Undefined;
    }

    void EPType(unsigned value)
    {
      _context2 = (_context2 & ~(0x7 << 3)) | ((value & 0x7) << 3);
    }

    unsigned EPType() const
    {
      return (_context2 >> 3) & 0x7;
    }

    void DebugPrint()
    {
      printf("\n EP C1: %x, C2: %x, TDQPTR: %x, C3: %x", _context1, _context2, (unsigned)_trDQPtr, _context3);
    }
  private:

    uint32_t _context1;
    uint32_t _context2;
    uint64_t _trDQPtr;
    uint32_t _context3;
    uint32_t _reserved1;
    uint32_t _reserved2;
    uint32_t _reserved3;
} PACKED;

class EndPointContext64 : public EndPointContext
{
  private:
    ReservedPadding _reserved;
} PACKED;

class DeviceContext32
{
  private:
    SlotContext     _slotContext;
    EndPointContext _epContext0;
    EndPointContext _epContexts[30];
  friend class DeviceContext;
} PACKED;

class InputContext32
{
  private:
    InputControlContext _controlContext;
    DeviceContext32     _deviceContext;
  friend class InputContext;
} PACKED;

class DeviceContext64
{
  private:
    SlotContext       _slotContext;
    ReservedPadding   _reserved2;
    EndPointContext64 _epContext0;
    EndPointContext64 _epContexts[30];
  friend class DeviceContext;
} PACKED;

class InputContext64
{
  private:
    InputControlContext   _controlContext;
    ReservedPadding       _reserved1;
    DeviceContext64       _deviceContext;
  friend class InputContext;
} PACKED;

class DeviceContext
{
  public:
    DeviceContext(bool use64);
    DeviceContext(DeviceContext32& dc32);
    DeviceContext(DeviceContext64& dc64);
    ~DeviceContext();
    SlotContext& Slot() { return *_slot; }
    EndPointContext& EP0() { return *_ep0; }
    EndPointContext& EP(uint32_t index)
    { 
      if(index >= 30)
        throw upan::exception(XLOC, "Invalid EndPointContext Index: %u", index);
      return _eps[index]; 
    }

  private:
    template <typename DC> void Init(DC& dc);

    bool             _allocated;
    SlotContext*     _slot;
    EndPointContext* _ep0;
    EndPointContext* _eps;
};

class InputContext
{
  public:
    InputContext(bool use64, const XHCIPortRegister& port, uint32_t portId, uint32_t routeString, uint32_t trRingPtr);
    ~InputContext();
    InputControlContext& Control() { return *_control; }
    SlotContext& Slot() { return _devContext->Slot(); }
    EndPointContext& EP0() { return _devContext->EP0(); }
    EndPointContext& EP(uint32_t index) { return _devContext->EP(index); }

  private:
    InputControlContext* _control;
    DeviceContext*       _devContext;
};

#endif
