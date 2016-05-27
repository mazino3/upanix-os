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

class ReservedPadding
{
  public:
    ReservedPadding() : _reserved1(0), _reserved2(0),
      _reserved3(0), _reserved4(0), _reserved5(0),
      _reserved6(0), _reserved7(0), _reserved8(0)
    {
    }
  private:
    unsigned _reserved1;
    unsigned _reserved2;
    unsigned _reserved3;
    unsigned _reserved4;
    unsigned _reserved5;
    unsigned _reserved6;
    unsigned _reserved7;
    unsigned _reserved8;
} PACKED;

class InputControlContext
{
  public:
    InputControlContext() : _dropContextFlags(0), _addContextFlags(0),
      _reserved1(0), _reserved2(0), _reserved3(0), _reserved4(0),
      _reserved5(0), _context(0)
    {
    }
  private:
    unsigned _dropContextFlags;
    unsigned _addContextFlags;
    unsigned _reserved1;
    unsigned _reserved2;
    unsigned _reserved3;
    unsigned _reserved4;
    unsigned _reserved5;
    unsigned _context;
} PACKED;

class InputSlotContext
{
  public:
    InputSlotContext() : _context1(0), _context2(0), _context3(0), _context4(0),
      _reserved1(0), _reserved2(0), _reserved3(0), _reserved4(0)
    {
    }

  private:
    unsigned _context1;
    unsigned _context2;
    unsigned _context3;
    unsigned _context4;
    unsigned _reserved1;
    unsigned _reserved2;
    unsigned _reserved3;
    unsigned _reserved4;
} PACKED;

class EndPointContext
{
  public:
    EndPointContext() : _context1(0), _context2(0), _trDQPtr(0), _context3(0),
      _reserved1(0), _reserved2(0), _reserved3(0)
   {
   }

  private:
    unsigned _context1;
    unsigned _context2;
    uint64_t _trDQPtr;
    unsigned _context3;
    unsigned _reserved1;
    unsigned _reserved2;
    unsigned _reserved3;
} PACKED;

#endif
