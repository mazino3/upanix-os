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
#ifndef _XHCI_TRB_H_
#define _XHCI_TRB_H_

#include <Global.h>
#include <stdio.h>
#include <exception.h>
#include <Bit.h>

class TRB
{
  public:
    TRB() : _b1(0), _b2(0), _b3(0), _b4(0)
    {
    }

    void Clear()
    {
      _b1 = 0;
      _b2 = 0;
      _b3 = 0;
      _b4 = 0;
    }

    void Type(unsigned type)
    {
      type &= 0x3F;
      _b4 = (_b4 & ~(0x3F << 10)) | (type << 10);
    }

    unsigned Type() const
    {
      return (_b4 >> 10) & 0x3F;
    }

    void SetCycleBit(bool val) { _b4 = Bit::Set(_b4, 0x1, val); }
    bool IsCycleBitSet() const { return Bit::IsSet(_b4, 0x1); }

    void Print()
    {
      printf("\n TRB0: %x, TRB1: %x, TRB2: %x, TRB3: %x", _b1, _b2, _b3, _b4);
    }

  protected:
    unsigned _b1;
    unsigned _b2;
    unsigned _b3;
    unsigned _b4;
} PACKED;

class LinkTRB : public TRB
{
  public:
    LinkTRB()
    {
      Type(6);
    }

    void SetLinkAddr(uint64_t la)
    {
      if(la & 0x3F)
        throw upan::exception(XLOC, "link address is not 64 byte aligned");
      _b1 = la & 0xFFFFFFFF;
      _b2 = (la >> 32);
    }
    uint64_t GetLinkAddr() const
    {
      uint64_t la = _b2;
      return la << 32 | _b1;
    }

    void SetInterrupterTarget(unsigned itarget)
    {
      itarget &= 0x3FF;
      _b3 = (_b3 & ~(0x3FF << 22)) | (itarget << 22);
    }
    unsigned GetInterrupterTarget() const
    {
      return (_b3 >> 22) & 0x3FF;
    }

    void SetToggleBit(bool val) { _b4 = Bit::Set(_b4, 0x2, val); }
    bool IsToggleBitSet() const { return Bit::IsSet(_b4, 0x2); }

    void SetChainBit(bool val) { _b4 = Bit::Set(_b4, 0x10, val); }
    bool IsChainBitSet() const { return Bit::IsSet(_b4, 0x10); }

    void SetIOC(bool val) { _b4 = Bit::Set(_b4, 0x20, val); }
    bool IsIOC() const { return Bit::IsSet(_b4, 0x20); }
} PACKED;

class EnableSlotTRB : public TRB
{
  public:
    EnableSlotTRB(unsigned slotType)
    {
      Type(9);
      _b4 = _b4 | (slotType << 16);
    }
} PACKED;

class DisableSlotTRB : public TRB
{
  public:
    DisableSlotTRB(unsigned slotId)
    {
      Type(10);
      _b4 = _b4 | (slotId << 24);
    }
} PACKED;

class EventTRB : public TRB
{
  public:
    uint64_t CommandPointer() const
    {
      uint64_t addr = _b2;
      return (addr << 32 | (uint64_t)_b1);
    }

    unsigned CompletionCode() const
    {
      return _b3 >> 24;
    }

    bool IsCommandSuccess() const
    {
      return CompletionCode() == 1;
    }

    unsigned SlotID() const
    {
      return _b4 >> 24;
    }
} PACKED;

class TransferTRB : public TRB
{
} PACKED;

class TransferRing
{
  public:
    TransferRing(unsigned size);
  private:
    uint32_t _size;
    TRB*     _trbs;
};

#endif
