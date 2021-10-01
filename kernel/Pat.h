/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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

#include <stdint.h>
#include <Cpu.h>

class Pat {
private:
  static const int IA32_PAT = 0x277;
  static const int NO_OF_PAT_ENTRIES = 8;

  Pat();
  Pat(const Pat&) = delete;

public:
  static Pat& Instance() {
    static Pat pat;
    return pat;
  }

  bool isSupported() {
    return _isSupported;
  }

  int writeCombiningPageTableFlag();

  void print();
  Cpu::MEM_TYPE get(int i);
  void set(int i, Cpu::MEM_TYPE memType);

private:
  bool _isSupported;
  uint64_t _patReg;
};