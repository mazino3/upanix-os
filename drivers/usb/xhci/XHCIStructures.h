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
#ifndef _XHCI_STRUCTURES_H_
#define _XHCI_STRUCTURES_H_

#include <Global.h>
#include <Bit.h>
#include <exception.h>

enum USB_PROTOCOL
{
  USB1,
  USB2,
  USB3,
  UNKNOWN
};

class XHCICapRegister
{
  public:
    enum ISTFrameTypes { Microframes, Frames };

    void Print() const;
    byte CapLength() const { return _capLength; }
    
    //HCSPARAMS1
    unsigned MaxSlots() const { return _hcsParams1 & 0xFF; }
    unsigned MaxIntrs() const { return (_hcsParams1 >> 8) & 0x3FF; }
    unsigned MaxPorts() const { return (_hcsParams1 >> 24) & 0xFF; }

    //HCSPARAMS2
    ISTFrameTypes ISTFrameType() const { return _hcsParams2 & 0x8 ? Frames : Microframes; }
    unsigned ISTValue() const { return _hcsParams2 & 0x7; }
    unsigned ERSTMax() const { return (_hcsParams2 >> 4) & 0xF; }
    //in terms of pages
    unsigned MaxScratchpadBufSize() const
    {
      unsigned hi = (_hcsParams2 >> 21) & 0x1F;
      unsigned lo = (_hcsParams2 >> 27) & 0x1F;
      return (hi << 5) | lo;
    }
    bool ScatchpadRestore() const { return Bit::IsSet(_hcsParams2, 0x04000000); }

    //HCSPARAMS3
    //U1 -> U0 in micro seconds
    unsigned U1ExitLatency() const
    {
      unsigned l = _hcsParams3 & 0xF;
      return l > 10 ? 10 : l;
    }
    //U2 -> U0 in micro seconds
    unsigned U2ExitLatency() const
    {
      unsigned l = (_hcsParams3 >> 16);
      return l > 0x7FF ? 0x7FF : l;
    }

    //HCCPARAMS1
    //64bit addressing capability
    bool IsAC64() const { return Bit::IsSet(_hccParams1, 0x1); }
    //Bandwidth negotiation capability
    bool IsBNC() const { return Bit::IsSet(_hccParams1, 0x2); }
    //64 or 32 byte context data structure size
    unsigned ContextSize() const { return Bit::IsSet(_hccParams1, 0x4) ? 64 : 32; }
    //Supports Port Power Control
    bool IsPPC() const { return Bit::IsSet(_hccParams1, 0x8); }
    //Supports Port Indicator
    bool IsPIND() const { return Bit::IsSet(_hccParams1, 0x10); }
    //Supports light HC reset
    bool IsLHRC() const { return Bit::IsSet(_hccParams1, 0x20); }
    //Supports latency tolerance messaging
    bool IsLTC() const { return Bit::IsSet(_hccParams1, 0x40); }
    //Supports Secondary SID
    bool IsSS() const { return !Bit::IsSet(_hccParams1, 0x80); }
    //Parses all event data TRBs after short packet while advancing to next TD
    bool IsPAE() const { return Bit::IsSet(_hccParams1, 0x100); }
    //Supports generation of Stopped - Short Packet completion code
    bool IsSPC() const { return Bit::IsSet(_hccParams1, 0x200); }
    //Supports Stopped EDTLA field
    bool IsSEC() const { return Bit::IsSet(_hccParams1, 0x400); }
    //Capable of matching Frame IDs of consecutive Isoch TDs
    bool IsCFC() const { return Bit::IsSet(_hccParams1, 0x800); }
    //Max Primary Stream Array Size
    unsigned MaxPSASize() const { return (_hccParams1 >> 12) & 0xF; }
    //Extended capabilities pointer offset from Base
    unsigned ECPOffset() const { return (_hccParams1 >> 14) & 0x3FFFC; }

    //DoorBell array base address from Base (cap base)
    unsigned DoorBellOffset() const { return _doorBellOffset & ~(0x3); }

    //Runtime registers offset from Base (cap base)
    unsigned RTSOffset() const { return _rtsOffset & ~(0x1F); }

    //HCCPARAMS2
    //U3 Entry Capability
    bool IsU3C() const { return Bit::IsSet(_hccParams2, 0x1); }
    //Configure Endpoint Command Max Exit Latency Too Large Capability
    bool IsCMC() const { return Bit::IsSet(_hccParams2, 0x2); }
    //Force Save Context Capability
    bool IsFSC() const { return Bit::IsSet(_hccParams2, 0x4); }
    //Compliance Transition Capability
    bool IsCTC() const { return Bit::IsSet(_hccParams2, 0x8); }
    //Large ESIT Payload Capability
    bool IsLEC() const { return Bit::IsSet(_hccParams2, 0x10); }
    //Configuration Information Capability
    bool IsCIC() const { return Bit::IsSet(_hccParams2, 0x20); }

  private:
    byte _capLength;
    byte _reserved;
    unsigned short _hciVersion;
    unsigned _hcsParams1;
    unsigned _hcsParams2;
    unsigned _hcsParams3;
    unsigned _hccParams1;
    unsigned _doorBellOffset;
    unsigned _rtsOffset;
    unsigned _hccParams2;
} PACKED;

class XHCIPortRegister
{
  public:
    bool IsDeviceConnected() const { return Bit::IsSet(_sc, 0x1); }
    bool IsEnabled() const { return Bit::IsSet(_sc, 0x2); }
    void Reset();
    void WarmReset();
    bool IsPowerOn() const { return Bit::IsSet(_sc, 0x200); }
    void PowerOn();
    void PowerOff();
    void WakeOnConnect(bool enable)
    {
      _sc = Bit::Set(_sc, 0x2000000, enable);
    }
    void WakeOnDisconnect(bool enable)
    {
      _sc = Bit::Set(_sc, 0x4000000, enable);
    }
    bool IsRemovableDevice() const { return Bit::IsSet(_sc, 0x40000000); }
    void Print();
  private:
    unsigned _sc;
    unsigned _pmsc;
    unsigned _li;
    unsigned _hlpmc;
} PACKED;

class XHCIOpRegister
{
  public:
    void Print() const;

    bool IsHCRunning() const { return Bit::IsSet(_usbCmd, 0x1); }
    void Run();
    void Stop();

    bool IsHCHalted() const { return Bit::IsSet(_usbStatus, 0x1); }
    void HCReset();

    bool IsHCInterruptEnabled() const { return Bit::IsSet(_usbCmd, 0x4); }
    void EnableHCInterrupt()
    {
      if(!IsHCInterruptEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x4, true);
    }
    void DisableHCInterrupt()
    {
      if(IsHCInterruptEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x4, false);
    }

    bool IsHSEEnabled() const { return Bit::IsSet(_usbCmd, 0x8); }
    void EnableHSE()
    {
      if(!IsHSEEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x8, true);
    }
    void DisableHSE()
    {
      if(IsHSEEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x8, false);
    }

    void LightHCReset(const XHCICapRegister* cr);
    bool Saving() const { return Bit::IsSet(_usbStatus, 0x100); }
    bool Restoring() const { return Bit::IsSet(_usbStatus, 0x200); }
    void HCSave();
    void HCRestore();

    bool IsWrapEventEnabled() { return Bit::IsSet(_usbCmd, 0x400); }
    void EnableWrapEvent()
    {
      if(!IsWrapEventEnabled())
        _usbCmd |= 0x400;
    }
    void DisableWrapEvent()
    {
      if(IsWrapEventEnabled())
        _usbCmd &= ~(0x400);
    }

    //U3 MFINDEX STOP
    bool IsU3SEnabled() const { return Bit::IsSet(_usbCmd, 0x800); }
    void EnableU3S()
    {
      if(!IsU3SEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x800, true);
    }
    void DisableU3S()
    {
      if(IsU3SEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x800, false);
    }

    bool IsShortPacketEnabled(const XHCICapRegister* cr) const;
    void EnableShortPacket(const XHCICapRegister* cr);
    void DisableShortPacket(const XHCICapRegister* cr);

    //Max Exit Latency Too Large Capability Error
    bool IsCEMEnable() const { return Bit::IsSet(_usbCmd, 0x2000); }
    void EnableCEM()
    {
      if(!IsCEMEnable())
        _usbCmd = Bit::Set(_usbCmd, 0x2000, true);
    }
    void DisableCEM()
    {
      if(IsCEMEnable())
        _usbCmd = Bit::Set(_usbCmd, 0x2000, false);
    }

    void SetDNCTRL(unsigned val) { _dnCtrl |= val; }
    //Status 
    //Host System Error
    bool IsHSError() const { return Bit::IsSet(_usbStatus, 0x4); }
    void ClearHSError() { _usbStatus = Bit::Set(_usbStatus, 0x4, true); }

    bool IsAnyEventPending() const { return Bit::IsSet(_usbStatus, 0x8); }
    void ClearEventInterrupt() { _usbStatus = Bit::Set(_usbStatus, 0x8, true); }
    
    bool IsPortChanged() const { return Bit::IsSet(_usbStatus, 0x10); }
    void ClearPortChanged() { _usbStatus = Bit::Set(_usbStatus, 0x10, true); }

    //Save/Restore Error
    bool IsSRError() const { return Bit::IsSet(_usbStatus, 0x400); }
    void ClearSRError() { _usbStatus = Bit::Set(_usbStatus, 0x400, true); }

    bool IsHCReady() const;
    //Host Controller Error
    bool IsHCError() const { return Bit::IsSet(_usbStatus, 0x1000); }

    //in KB
    unsigned SupportedPageSize() const;

    bool IsCommandRingRunning() const { return Bit::IsSet(_crcr, 0x8); }
    //crcr is 64 bit
    //void StopCommandRing() { _crcr = Bit::Set(_crcr, 0x2, true); }
    //void AbortCommandRing() { _crcr = Bit::Set(_crcr, 0x4, true); }

    void SetCommandRingPointer(uint64_t ptr)
    {
      _crcr = (_crcr & (uint64_t)0x3F) | (ptr & ~((uint64_t)0x3F));
    }

    void SetDCBaap(uint64_t ptr)
    {
      //require LSB 6 bits to be zero
      if(ptr & 0x3F)
        throw upan::exception(XLOC, "invalid DC_BAAP '%llx' not aligned to 64 byte", ptr);
      _dcBaap = ptr;
    }

    unsigned MaxSlotsEnabled() const { return _config & 0xFF; }
    void MaxSlotsEnabled(unsigned val);

    bool IsU3EntryEnabled() const { return Bit::IsSet(_config, 0x100); }
    void EnableU3Entry()
    {
      if(!IsU3EntryEnabled())
        _config = Bit::Set(_config, 0x100, true);
    }
    void DisableU3Entry()
    {
      if(IsU3EntryEnabled())
        _config = Bit::Set(_config, 0x100, false);
    }

    //Port Registers is an array starting at 0x400
    //So, it's address is same as address of _portRegs;
    XHCIPortRegister* PortRegisters() { return &_portRegs; }
  private:
    unsigned _usbCmd;
    unsigned _usbStatus;
    unsigned _pageSize;
    uint64_t _reserved1;
    unsigned _dnCtrl; //Device Notification Control
    uint64_t _crcr; //Command Ring Control
    uint64_t _reserved2;
    uint64_t _reserved3;
    uint64_t _dcBaap; //Device Context Base Address Array Pointer
    unsigned _config;
    byte     _reserved4[964];
    XHCIPortRegister _portRegs;
} PACKED;

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
    void SetType(unsigned type)
    {
      type &= 0x3F;
      _b4 = (_b4 & ~(0x3F << 10)) | (type << 10);
    }
    unsigned _b1;
    unsigned _b2;
    unsigned _b3;
    unsigned _b4;
} PACKED;

//Wrappers for TRBs
//
class LinkTRB
{
  public:
    LinkTRB(TRB& trb, bool reset) : _trb(trb)
    {
      if(reset)
        _trb.Clear();
      _trb.SetType(6);
    }

    void SetLinkAddr(uint64_t la)
    {
      if(la & 0x3F)
        throw upan::exception(XLOC, "link address is not 64 byte aligned");
      _trb._b1 = la & 0xFFFFFFFF;
      _trb._b2 = (la >> 32);
    }
    uint64_t GetLinkAddr() const
    {
      uint64_t la = _trb._b2;
      return la << 32 | _trb._b1;
    }

    void SetInterrupterTarget(unsigned itarget)
    {
      itarget &= 0x3FF;
      _trb._b3 = (_trb._b3 & ~(0x3FF << 22)) | (itarget << 22);
    }
    unsigned GetInterrupterTarget() const
    {
      return (_trb._b3 >> 22) & 0x3FF;
    }

    void SetCycleBit(bool val) { _trb._b4 = Bit::Set(_trb._b4, 0x1, val); }
    bool IsCycleBitSet() const { return Bit::IsSet(_trb._b4, 0x1); }

    void SetToggleBit(bool val) { _trb._b4 = Bit::Set(_trb._b4, 0x2, val); }
    bool IsToggleBitSet() const { return Bit::IsSet(_trb._b4, 0x2); }

    void SetChainBit(bool val) { _trb._b4 = Bit::Set(_trb._b4, 0x10, val); }
    bool IsChainBitSet() const { return Bit::IsSet(_trb._b4, 0x10); }

    void SetIOC(bool val) { _trb._b4 = Bit::Set(_trb._b4, 0x20, val); }
    bool IsIOC() const { return Bit::IsSet(_trb._b4, 0x20); }

  private:
    TRB& _trb;
};

class LegSupXCap
{
  public:
    bool IsBiosOwned() const { return Bit::IsSet(_usbLegSup, 0x10000); }
    bool IsOSOwned() const { return Bit::IsSet(_usbLegSup, 0x1000000); }
    void BiosToOSHandoff();
  private:
    unsigned _usbLegSup;
    unsigned _usbLegCtlSts; 
} PACKED;

class SupProtocolXCap
{
  public:
    unsigned MajorVersion() const { return (_revision >> 24) & 0xFF; }
    unsigned MinorVersion() const { return (_revision >> 16) & 0xFF; }
    
    USB_PROTOCOL Protocol() const
    {
      switch(MajorVersion())
      {
        case 3: return USB_PROTOCOL::USB3;
        case 2: return USB_PROTOCOL::USB2;
        case 1: return USB_PROTOCOL::USB1;
      }
      return USB_PROTOCOL::UNKNOWN;
    }

    bool HasPort(unsigned portId) const
    {
      unsigned startPortNo = _portDetails & 0xFF;
      unsigned endPortNo = startPortNo + ((_portDetails >> 8) & 0xFF);
      return portId >= startPortNo && portId < endPortNo;
    }

    void Print() const
    {
      printf("\n B1: %x, B2: %x, B3: %x", _revision, _name, _portDetails);
    }
  private:
    unsigned _revision;
    unsigned _name;
    unsigned _portDetails;

} PACKED;

#endif
