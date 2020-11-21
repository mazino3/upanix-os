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

#include <RawNetPacket.h>
#include <EthernetPacket.h>
#include <ARPHandler.h>
#include <EthernetHandler.h>

EthernetHandler::EthernetHandler() {
  _etherPacketHandlers.insert(EtherPacketHandlerMap::value_type(EtherType::ARP, new ARPHandler()));
}

void EthernetHandler::Process(const RawNetPacket& packet) {
  if (packet.len() < MIN_ETHERNET_PACKET_LEN) {
    throw upan::exception(XLOC, "Invalid packet: Len %d < min ethernet-packet len %d", packet.len(), MIN_ETHERNET_PACKET_LEN);
  }
  const EthernetPacket ethernetPacket(packet);
  ethernetPacket.Print();
  EtherPacketHandlerMap::const_iterator it = _etherPacketHandlers.find(ethernetPacket.Type());
  if (it == _etherPacketHandlers.end()) {
    throw upan::exception(XLOC, "Unhandled Ethernet Packet Type: %x", ethernetPacket.Type());
  }
  it->second->Process(ethernetPacket);
}
