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

#include <stdio.h>
#include <NetworkUtil.h>
#include <EthernetPacket.h>

EthernetPacket::EthernetPacket(const RawNetPacket& rawNetPacket) : 
  _rawNetPacket(rawNetPacket),
  _thisPacket(reinterpret_cast<EthernetPacket::_ThisPacket&>(*rawNetPacket.PacketData())) {
    _thisPacket._type = NetworkUtil::SwitchEndian(_thisPacket._type);
}

void EthernetPacket::Print() const {
  printf("\n ETHERNET PACKET: D ");
  for(int i = 0; i < 6; i++) {
    printf("%02x%s", _thisPacket._destinationMAC[i], i < 5 ? ":" : "");
  }
  printf(", S ");
  for(int i = 0; i < 6; i++) {
    printf("%02x%s", _thisPacket._sourceMAC[i], i < 5 ? ":" : "");
  }
  printf(", Type: %x", _thisPacket._type);
}