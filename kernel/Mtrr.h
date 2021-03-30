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

#include <stdlib.h>
#include <vector.h>
#include <Cpu.h>

class Mtrr {
private:
  static const int IA32_MTRR_CAP = 0xFE;
  static const int IA32_MTRR_DEF_TYPE = 0x2FF;
  static const int IA32_MTRR_PHY_BASEn = 0x200;
  static const int IA32_MTRR_PHY_MASKn = 0x201;

  Mtrr();
  Mtrr(const Mtrr&) = delete;

  class CapReg {
  public:
    CapReg() : _val(0) {
    }
    explicit CapReg(uint64_t val) {
      _val = val;
    }

    uint32_t noOfVariableSizeRangeRegisters() {
      return _val & 0xFF;
    }

    bool isFixRangeRegistersSupported() {
      return _val & 0x100;
    }

    bool isWriteCombiningMemTypeSupported() {
      return _val & 0x400;
    }

    bool isSysManagementRangeRegistersSupported() {
      return _val & 0x800;
    }

    void Print();
  private:
    uint64_t _val;
  };

  class DefType {
  public:
    DefType() : _val(0) {
    }
    explicit DefType(uint64_t val) {
      _val = val;
    }

    Cpu::MEM_TYPE defaultMemoryType() {
      return (Cpu::MEM_TYPE)(_val & 0xFF);
    }

    bool isFixedRangeMTRREnabled() {
      return _val & 0x400;
    }

    bool isMTRREnabled() {
      return _val & 0x800;
    }

    void Print();
  private:
    uint64_t _val;
  };

  // Variable Sized Range Registers
  class VarRR {
  public:
    VarRR() : _base(0), _mask(0) {}
    VarRR(uint64_t base, uint64_t mask) : _base(base), _mask(mask) {}

    Cpu::MEM_TYPE memType() {
      return (Cpu::MEM_TYPE)(_base & 0xFF);
    }

    uint32_t base() {
      //assume 32bit
      return (_base & 0xFFFFF000) >> 12;
    }

    uint32_t mask() {
      //assume 32bit
      return (_mask & 0xFFFFF000) >> 12;
    }

    bool isValid() {
      return (_mask & 0x800);
    }

    void Print();
  private:
    uint64_t _base;
    uint64_t _mask;
  };
public:
  static Mtrr& Instance() {
    static Mtrr mtrr;
    return mtrr;
  }

  bool isSupported() {
    return _isSupported;
  }

private:
  bool _isSupported;
  CapReg _capReg;
  DefType _defType;
  upan::vector<VarRR> _varRRs;
};