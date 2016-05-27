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
#ifndef _XHCI_INPUT_CONTEXT_H_
#define _XHCI_INPUT_CONTEXT_H_

#include <exception.h>

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
      _addContextFlags |= val;
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

class InputSlotContext
{
  public:
    InputSlotContext() 
    {
      memset((void*)this, 0, sizeof(InputSlotContext));
    }

    void Init(uint32_t portId, char routeString)
    {
      _rootHubPortNo = portId;
      _routeString = routeString;
      _contextEntries = 1;
    }
  private:
    uint32_t _routeString    : 20;
    uint32_t _speed          : 4;
    uint32_t _reserved1      : 1;
    uint32_t _mtt            : 1;
    uint32_t _hub            : 1;
    uint32_t _contextEntries : 5;

    uint16_t _maxExitLatency;
    uint8_t  _rootHubPortNo;
    uint8_t  _noOfPorts;

    uint8_t  _ttHubSlotID;
    uint8_t  _ttPortNo;
    uint32_t _ttThinkTime : 2;
    uint32_t _reserved2   : 4;
    uint32_t _intTarget   : 10;

    uint8_t  _usbDevAddr;
    uint32_t _reserved3  : 19;
    uint32_t _slotState  : 5;

    uint32_t _reserved4;
    uint32_t _reserved5;
    uint32_t _reserved6;
    uint32_t _reserved7;
} PACKED;

class EndPointContext
{
  public:
    EndPointContext() : _context1(0), _context2(0),
      _trDQPtr(0), _context3(0), _reserved1(0),
      _reserved2(0), _reserved3(0)
    {
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

class InputContext32
{
  private:
    InputControlContext _controlContext;
    InputSlotContext    _slotContext;
    EndPointContext     _epContext0;
    EndPointContext     _epContexts[30];
  friend class InputContext;
};

class InputContext64
{
  private:
    InputControlContext   _controlContext;
    ReservedPadding       _reserved1;
    InputSlotContext      _slotContext;
    ReservedPadding       _reserved2;
    EndPointContext64     _epContext0;
    EndPointContext64     _epContexts[30];
  friend class InputContext;
};

class InputContext
{
  public:
    InputContext(bool use64);
    ~InputContext();

    InputControlContext& Control() { return *_control; }
    InputSlotContext& Slot() { return *_slot; }
    EndPointContext& EP0() { return *_ep0; }
    EndPointContext& EP(uint32_t index) 
    { 
      if(index >= 30)
        throw upan::exception(XLOC, "Invalid EndPointContext Index: %u", index);
      return _eps[index]; 
    }
  private:
    InputContext32* _context32;
    InputContext64* _context64;

    InputControlContext* _control;
    InputSlotContext*    _slot;
    EndPointContext*     _ep0;
    EndPointContext*     _eps;
};

#endif
