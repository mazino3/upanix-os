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
#include <EthernetRecvPacket.h>
#include <IPV4Handler.h>
#include <EthernetHandler.h>
#include <IPV4RecvPacket.h>

IPV4Handler::IPV4Handler(EthernetHandler &ethernetHandler) : _ethernetHandler(ethernetHandler) {
}

void IPV4Handler::Process(const EthernetRecvPacket& packet) {
  printf("\n Handling IPV4 Packet");
  IPV4RecvPacket ipv4Packet(packet);
  ipv4Packet.Print();
}