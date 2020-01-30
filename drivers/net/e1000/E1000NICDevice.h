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
#pragma once

#include <vector.h>

class E1000NICDevice : public NetworkDevice {
public:
  E1000NICDevice(const PCIEntry&);
  virtual void NotifyEvent();

private:
  class RegEEPROM {
  public:
    RegEEPROM(const uint32_t memIOBase);
    const upan::vector<uint8_t>& getMacAddress() const {
      return _macAddress;
    }
    void print() const;
  private:
      uint32_t readEEPROM(const int wordPos);
  private:
      const uint32_t REG_EEPROM = 0x14;
      volatile uint32_t* _eeprom;
      upan::vector<uint8_t> _macAddress;
  };

  class RegIntControl {
  public:
    RegIntControl(const uint32_t memIOBase);
    void disable();
    void enable();
  private:
    const static uint32_t REG_ICR = 0x00C0; // Interrupt Cause Read
    const static uint32_t REG_ITR	= 0x00C4; // Interrupt Throttling Register
    const static uint32_t REG_ICS	= 0x00C8; // Interrupt Cause Set Register
    const static uint32_t REG_IMS	= 0x00D0; // Interrupt Mask Set/Read Register
    const static uint32_t REG_IMC	= 0x00D8; // Interrupt Mask Clear Register
    const static uint32_t REG_RDTR = 0x2820; // RX Delay Timer Register
    const static uint32_t REG_RADV = 0x282C; // RX Int. Absolute Delay Timer
    const static uint32_t REG_RSRPD = 0x2C00; // RX Small Packet Detect Interrupt

    volatile uint32_t* _icr;
    volatile uint32_t* _itr;
    volatile uint32_t* _ics;
    volatile uint32_t* _ims;
    volatile uint32_t* _imc;
    volatile uint32_t* _rdtr;
    volatile uint32_t* _radv;
    volatile uint32_t* _rsrpd;
    uint32_t x;
  };

  class RegControl {
  public:
    RegControl(const uint32_t memIOBase);
    
  private:
    const static uint32_t REG_PBA = 0x1000; // Packet Buffer Allocation
    const static uint32_t REG_TXCW = 0x0178; // Transmit Configuration Word
    const static uint32_t REG_CTRL = 0x0000; // Control Register
    const static uint32_t REG_MTA = 0x5200; // MTA

    volatile uint32_t* _pba;
    volatile uint32_t* _txcw;
    volatile uint32_t* _ctrl;
    volatile uint32_t* _mta;
  };

  class RXDescriptor {
  public:
    RXDescriptor();
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint16_t checksum;
    volatile uint8_t status;
    volatile uint8_t errors;
    volatile uint16_t special;
  } PACKED;

  class RegRXDescriptor {
  public:
    RegRXDescriptor(const uint32_t memIOBase);
    
  private:
    const static uint32_t REG_RDBAL = 0x2800; // RX Descriptor Base Address Low
    const static uint32_t REG_RDBAH = 0x2804; // RX Descriptor Base Address High
    const static uint32_t REG_RDLEN = 0x2808; // RX Descriptor Length
    const static uint32_t REG_RDH = 0x2810; // RX Descriptor Head
    const static uint32_t REG_RDT = 0x2818; // RX Descriptor Tail
    const static uint32_t REG_RCTL = 0x0100; // Receive Control Register

    volatile uint32_t* _alow;
    volatile uint32_t* _ahigh;
    volatile uint32_t* _len;
    volatile uint32_t* _head;
    volatile uint32_t* _tail;
    volatile uint32_t* _rxctrl;

    const uint32_t NUM_OF_DESC = 32;
    const RXDescriptor* _rxDescriptors;
  };

  class TXDescriptor {
  public:
    TXDescriptor();
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint8_t cso;
    volatile uint8_t cmd;
    volatile uint8_t status;
    volatile uint8_t css;
    volatile uint16_t special;

    const static uint32_t TSTA_DD = (1 << 0); // Descriptor Done
    const static uint32_t TSTA_EC = (1 << 1); // Excess Collisions
    const static uint32_t TSTA_LC = (1 << 2); // Late Collision
    const static uint32_t LSTA_TU = (1 << 3); // Transmit Underrun
  } PACKED;

  class RegTXDescriptor {
  public:
    RegTXDescriptor(const uint32_t memIOBase);
    
  private:
    const static uint32_t REG_TDBAL = 0x3800; // TX Descriptor Base Address Low
    const static uint32_t REG_TDBAH = 0x3804; // TX Descriptor Base Address High
    const static uint32_t REG_TDLEN = 0x3808; // TX Descriptor Length
    const static uint32_t REG_TDH = 0x3810; // TX Descriptor Head
    const static uint32_t REG_TDT = 0x3818; // TX Descriptor Tail
    const static uint32_t REG_TCTL = 0x0400; // Transmit Control Register
    const static uint32_t REG_TIPG = 0x0410; // Transmit Inter Packet Gap

    volatile uint32_t* _alow;
    volatile uint32_t* _ahigh;
    volatile uint32_t* _len;
    volatile uint32_t* _head;
    volatile uint32_t* _tail;
    volatile uint32_t* _txctrl;
    volatile uint32_t* _tipg;

    const uint32_t NUM_OF_DESC = 16;
    const TXDescriptor* _txDescriptors;
  };

  private:
    uint32_t _memIOBase;
};