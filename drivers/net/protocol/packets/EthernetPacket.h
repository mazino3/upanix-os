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
#include <RawNetPacket.h>

class EthernetPacket {
  private:
  struct _ThisPacket {
    uint8_t _destinationMAC[6];
    uint8_t _sourceMAC[6];
    uint16_t _type;
  } PACKED;

  private:
  const RawNetPacket& _rawNetPacket;
  _ThisPacket& _thisPacket;

  public:
  EthernetPacket(const RawNetPacket& rawNetPacket);
  void Print() const;

  EtherType Type() const {
    return static_cast<EtherType>(_thisPacket._type);
  }

  uint8_t* PacketData() const {
    return _rawNetPacket.PacketData() + sizeof(_ThisPacket);
  }
};