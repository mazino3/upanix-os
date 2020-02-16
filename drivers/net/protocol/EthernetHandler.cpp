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

#include <exception.h>

void EthernetHandler::Process(const RawNetPacket& packet) {
  if (packet.len() < MIN_ETHERNET_PACKET_LEN) {
    throw upan::exception(XLOC, "Invalid packet: Len %d < min ethernet-packet len %d", packet.len(), MIN_ETHERNET_PACKET_LEN);
  }
  const EthernetPacket ethernetPacket = reinterpret_cast<EthernetPacket*>(packet.buf());
  ethernetPacket.Print();
}

void EthernetHandler::EthernetPacket::Print() {
  printf("\n ETHERNET PACKET: D:");
  for(int i = 0; i < 6; i++) {
    printf("%x ", _destinationMAC[i]);
  }
  printf(", S:");
  for(int i = 0; i < 6; i++) {
    printf("%x ", _sourceMAC[i]);
  }
  printf(", LEN: %d", _type);
}