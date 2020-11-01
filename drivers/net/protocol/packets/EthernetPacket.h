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
#include <EtherType.h>

class EthernetPacket {
  private:
  struct _RawPacket {
    const uint8_t _destinationMAC[6];
    const uint8_t _sourceMAC[6];
    const uint16_t _type;
    const uint8_t* _payload;
  } PACKED;

  private:
  const _RawPacket& _rawPacket;
  EtherType _type;

  public:
  EthernetPacket(const uint8_t* packetBuf);
  void Print() const;

  EtherType Type() const {
    return _type;
  }

  const uint8_t* Payload() {
    return _rawPacket._payload;
  }
};