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
#include <EthernetPacket.h>

class ARPPacket {
  public:
  struct _ThisPacket {
    uint16_t _hType;
    uint16_t _pType;
    uint8_t _hLen;
    uint8_t _pLen;
    uint16_t _opCode;
  } PACKED;

  struct _ThisIPV4PacketData {
      uint8_t _senderHardwareAddress[6];
      uint8_t _senderProtocolAddress[4];
      uint8_t _targetHardwareAddress[6];
      uint8_t _targetProtocolAddress[4];
  } PACKED;

  private:
  const EthernetPacket& _ethernetPacket;
  _ThisPacket& _thisPacket;
  _ThisIPV4PacketData* _thisIPV4PacketData;

  public:
  ARPPacket(const EthernetPacket& ethernetPacket);

  EtherType Type() const {
      return static_cast<EtherType>(_thisPacket._pType);
  }

  void Print() const;
};