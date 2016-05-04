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
  bool ScatchpadRestore() const { return (_hcsParams2 & 0x04000000) ? true : false; }

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
  bool IsAC64() const { return (_hccParams1 & 0x1) ? true : false; }
  //Bandwidth negotiation capability
  bool IsBNC() const { return (_hccParams1 & 0x2) ?  true : false; }
  //64 or 32 byte context data structure size
  unsigned ContextSize() const { return (_hccParams1 & 0x4) ? 64 : 32; }
  //Supports Port Power Control
  bool IsPPC() const { return (_hccParams1 & 0x8) ? true : false; }
  //Supports Port Indicator
  bool IsPIND() const { return (_hccParams1 & 0x10) ? true : false; }
  //Supports light HC reset
  bool IsLHRC() const { return (_hccParams1 & 0x20) ? true : false; }
  //Supports latency tolerance messaging
  bool IsLTC() const { return (_hccParams1 & 0x40) ? true : false; }
  //Supports Secondary SID
  bool IsSS() const { return (_hccParams1 & 0x80) ? false : true; }
  //Parses all event data TRBs after short packet while advancing to next TD
  bool IsPAE() const { return (_hccParams1 & 0x100) ? true : false; }
  //Supports generation of Stopped - Short Packet completion code
  bool IsSPC() const { return (_hccParams1 & 0x200) ? true : false; }
  //Supports Stopped EDTLA field
  bool IsSEC() const { return (_hccParams1 & 0x400) ? true : false; }
  //Capable of matching Frame IDs of consecutive Isoch TDs
  bool IsCFC() const { return (_hccParams1 & 0x800) ? true : false; }
  //Max Primary Stream Array Size
  unsigned MaxPSASize() const { return (_hccParams1 >> 12) & 0xF; }
  //Extended capabilities pointer offset from Base
  unsigned ECPOffset() const { return (_hccParams1 >> 14) & 0x3; }

  //DoorBell array base address from Base (cap base)
  unsigned DoorBellOffset() const { return _doorBellOffset & ~(0x3); }

  //Runtime registers offset from Base (cap base)
  unsigned RTSOffset() const { return _rtsOffset & ~(0x1F); }

  //HCCPARAMS2
  //U3 Entry Capability
  bool IsU3C() const { return _hccParams2 & 0x1 ? true : false; }
  //Configure Endpoint Command Max Exit Latency Too Large Capability
  bool IsCMC() const { return _hccParams2 & 0x2 ? true : false; }
  //Force Save Context Capability
  bool IsFSC() const { return _hccParams2 & 0x4 ? true : false; }
  //Compliance Transition Capability
  bool IsCTC() const { return _hccParams2 & 0x8 ? true : false; }
  //Large ESIT Payload Capability
  bool IsLEC() const { return _hccParams2 & 0x10 ? true : false; }
  //Configuration Information Capability
  bool IsCIC() const { return _hccParams2 & 0x20 ? true : false; }

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
  bool IsDeviceConnected() const { return _portSC & 0x1 ? true : false; }
  bool IsEnabled() const { return _portSC & 0x2 ? true : false; }
  void Reset();
  bool IsPowerOn() const { return _portSC & 0x20 ? true : false; }
  void PowerOn();
  void WakeOnConnect(bool enable)
  {
    if(enable)
      _portSC |= 0x2000000;
    else
      _portSC &= ~(0x2000000);
  }
  void WakeOnDisconnect(bool enable)
  {
    if(enable)
      _portSC |= 0x4000000;
    else
      _portSC &= ~(0x4000000);
  }
  bool IsRemovableDevice() const { return _portSC & 0x40000000 ? true : false; }
private:
  unsigned _portSC;
  unsigned _portPMSC;
  unsigned _portLI;
  unsigned _portHLPMC;
} PACKED;

class XHCIOpRegister
{
public:
  bool IsHCRunning() const { return _usbCmd & 0x1 ? true : false; }
  void Run();
  void Stop();

  bool IsHCHalted() const { return _usbStatus & 0x1 ? true : false; }
  void HCReset();

  bool IsHCInterruptEnabled() const { return _usbCmd & 0x4 ? true : false; }
  void EnableHCInterrupt()
  {
    if(!IsHCInterruptEnabled())
      _usbCmd |= 0x4;
  }
  void DisableHCInterrupt()
  {
    if(IsHCInterruptEnabled())
      _usbCmd &= ~(0x4);
  }

  bool IsHSEEnabled() const { return _usbCmd & 0x8 ? true : false; }
  void EnableHSE()
  {
    if(!IsHSEEnabled())
      _usbCmd |= 0x8;
  }
  void DisableHSE()
  {
    if(IsHSEEnabled())
      _usbCmd &= ~(0x8);
  }

  void LightHCReset(const XHCICapRegister* cr);
  bool Saving() const { return _usbStatus & 0x100 ? true : false; }
  bool Restoring() const { return _usbStatus & 0x200 ? true : false; }
  void HCSave();
  void HCRestore();

  bool IsWrapEventEnabled() { return _usbCmd & 0x400 ? true : false; }
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
  bool IsU3SEnabled() const { return _usbCmd & 0x800 ? true : false; }
  void EnableU3S()
  {
    if(!IsU3SEnabled())
      _usbCmd |= 0x800;
  }
  void DisableU3S()
  {
    if(IsU3SEnabled())
      _usbCmd &= ~(0x800);
  }

  bool IsShortPacketEnabled(const XHCICapRegister* cr) const;
  void EnableShortPacket(const XHCICapRegister* cr);
  void DisableShortPacket(const XHCICapRegister* cr);

  //Max Exit Latency Too Large Capability Error
  bool IsCEMEnable() const { return _usbCmd & 0x2000 ? true : false; }
  void EnableCEM()
  {
    if(!IsCEMEnable())
      _usbCmd |= 0x2000;
  }
  void DisableCEM()
  {
    if(IsCEMEnable())
      _usbCmd &= ~(0x2000);
  }

  //Status 
  //Host System Error
  bool IsHSError() const { return _usbStatus & 0x4 ? true : false; }
  void ClearHSError() { _usbStatus |= 0x4; }

  bool IsAnyEventPending() const { return _usbStatus & 0x8 ? true : false; }
  void ClearEventInterrupt() { _usbStatus |= 0x8; }
  
  bool IsPortChanged() const { return _usbStatus & 0x10 ? true : false; }
  void ClearPortChanged() { _usbStatus |= 0x10; }

  //Save/Restore Error
  bool IsSRError() const { return _usbStatus & 0x400 ? true : false; }
  void ClearSRError() { _usbStatus |= 0x400; }

  bool IsHCReady() const { return _usbStatus & 0x800 ? false : true; }
  //Host Controller Error
  bool IsHCError() const { return _usbStatus & 1000 ? true : false; }

  //in KB
  unsigned SupportedPageSize() const;

  bool IsCommandRingRunning() const { return _crcr & 0x8 ? true : false; }
  void StopCommandRing() { _crcr |= 0x2; }
  void AbortCommandRing() { _crcr |= 0x4; }

  void SetCommandRingPointer(uint64_t ptr)
  {
    _crcr = (_crcr & 0x3F) | (ptr & ~(0x3F));
  }

  void SetDCBaap(uint64_t ptr)
  {
    //require LSB 6 bits to be zero
    if(ptr & 0x3F)
      throw upan::exception(XLOC, "invalid DC_BAAP '%llx' not aligned to 64 byte", ptr)
    _dcBaap = ptr;
  }

  unsigned MaxSlotsEnabled() const { return _config & 0xFF; }
  void MaxSlotsEnabled(unsigned val);

  bool IsU3EntryEnabled() const { return _config & 0x100 ? true : false; }
  void EnableU3Entry()
  {
    if(!IsU3EntryEnabled())
      _config |= 0x100;
  }
  void DisableU3Entry()
  {
    if(IsU3EntryEnabled())
      _config &= ~(0x100);
  }

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
  XHCIPortRegister* _portRegs;
} PACKED;

#endif
