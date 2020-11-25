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

#include <NetworkPacketComponents.h>
#include <EtherType.h>

class ARPSendPacket {
private:
  uint32_t _len;
  uint8_t* _buf;

public:
  ARPSendPacket(uint16_t hType, EtherType _pType, uint8_t hLen, uint8_t pLen, uint16_t opCode,
                const uint8_t* sha, const uint8_t* spa, const uint8_t* tha, const uint8_t* tpa);
  ~ARPSendPacket();

  uint8_t* buf() { return _buf; }
  uint32_t len() { return _len; }
};