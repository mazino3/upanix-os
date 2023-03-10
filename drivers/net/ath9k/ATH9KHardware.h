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

#include <stdlib.h>

#define AR_SREV_ID_MASK 0x000000FF

class PCIEntry;

class ATH9KHardware
{
public:
  enum REG_OFFSET
  {
    SREV = 0x4020
  };

  ATH9KHardware(const PCIEntry& pciEntry);
  uint32_t Read(const REG_OFFSET regOffset);
  void MultiRead(REG_OFFSET* addr, uint32_t* val, uint16_t count);
  void Write(uint32_t value, const REG_OFFSET regOffset);
	uint32_t ReadMofidyWrite(const REG_OFFSET regOffset, const uint32_t set, const uint32_t clr);

private:
  void ReadCacheLineSize();
  void ReadRevisions();

  const PCIEntry& _pciEntry;
  uint32_t  _regBase;
  int       _cacheLineSize;
};
