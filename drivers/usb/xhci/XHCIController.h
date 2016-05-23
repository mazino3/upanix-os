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
#ifndef _XHCI_CONTROLLER_H_
#define _XHCI_CONTROLLER_H_

#include <USBController.h>
#include <PCIBusHandler.h>
#include <XHCIStructures.h>

class CommandManager;
class EventManager;
class SupProtocolXCap;

class XHCIController
{
  public:
    XHCIController(PCIEntry*);
    void Probe();

  private:
    void LoadXCaps(unsigned base);
    void PerformBiosToOSHandoff();
    void RingDoorBell(unsigned index, unsigned value);
    void InitializeDevice(XHCIPortRegister& port, unsigned slotType);

    SupProtocolXCap* GetSupportedProtocol(unsigned portId) const;
    const char* PortProtocolName(USB_PROTOCOL) const;
    const char* PortSpeedName(DEVICE_SPEED speed) const;

    static unsigned  _memMapBaseAddress;
    PCIEntry*        _pPCIEntry;
    XHCICapRegister* _capReg;
    XHCIOpRegister*  _opReg;
    CommandManager*  _cmdManager;
    EventManager*    _eventManager;
    LegSupXCap*      _legSupXCap;
    unsigned*        _doorBellRegs;
    upan::list<SupProtocolXCap*> _supProtoXCaps;

    friend class XHCIManager;
};

class CommandManager
{
  private:
    CommandManager(XHCICapRegister&, XHCIOpRegister&, EventManager& eventManager);
    void EnableSlot(unsigned slotType);
    void DebugPrint();

  private:
    struct Ring
    {
      TRB _cmd;
      TRB _link;
    } PACKED;

    bool             _pcs;
    Ring*            _ring;
    XHCICapRegister& _capReg;
    XHCIOpRegister&  _opReg;
    EventManager&    _eventManager;
  friend class XHCIController;
};

class EventManager
{
  public:
    void DebugPrint() const;

  private:
    EventManager(XHCICapRegister&, XHCIOpRegister&);

  private:
    //Event Ring Segment Table
    struct ERSTEntry
    {
      ERSTEntry();
      uint64_t _ersAddr;
      unsigned _size;
      unsigned _reserved;
    } PACKED;

    struct InterrupterRegister
    {
      InterrupterRegister();

      TRB* ERSegment(unsigned index)
      {
        if(index >= _erstSize)
          throw upan::exception(XLOC, "\n Invalid ERST index %d - ERST Size is %d", index, _erstSize);
        ERSTEntry* erst = (ERSTEntry*)KERNEL_VIRTUAL_ADDRESS(_erstBA);
        return (TRB*)KERNEL_VIRTUAL_ADDRESS(erst[index]._ersAddr);
      }

      unsigned _iman;
      unsigned _imod;
      unsigned _erstSize;
      unsigned _reserved;
      uint64_t _erstBA;
      uint64_t _erdqPtr;
    } PACKED;

    InterrupterRegister* _iregs;
    XHCICapRegister& _capReg;
    XHCIOpRegister& _opReg;
  friend class XHCIController;
};

#endif
