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

#include <IPV4RecvPacket.h>
#include <stdio.h>
#include <NetworkUtil.h>

IPV4RecvPacket::IPV4RecvPacket(const EthernetRecvPacket& ethernetPacket) :
    _ethernetPacket(ethernetPacket),
    _ipv4Header(reinterpret_cast<NetworkPacket::IPV4::Header&>(*ethernetPacket.PacketData())) {
  _ipv4Header._totalLen = NetworkUtil::SwitchEndian(_ipv4Header._totalLen);
  _ipv4Header._identification = NetworkUtil::SwitchEndian(_ipv4Header._identification);
  //_ipv4Header._fragmentOffset = NetworkUtil::SwitchEndian(_ipv4Header._fragmentOffset);
}

void IPV4RecvPacket::Print() const {
  printf("\n Version: %d, IHL: %d, TOS: %d, TotalLen: %d",
         _ipv4Header._version, _ipv4Header._ihl, _ipv4Header._tos, _ipv4Header._totalLen);

  printf("\nIdentification: %d, Flags: 0x%x, FragmentOffset: 0x%x, TTL: %d, Protocol: 0x%x",
         _ipv4Header._identification, _ipv4Header._flags, _ipv4Header._fragmentOffset, _ipv4Header._ttl, _ipv4Header._protocol);

  printf("\nChecksum: 0x%x", _ipv4Header._checksum);

  printf("\nSource Addr: %d.%d.%d.%d, Dest Addr: %d.%d.%d.%d",
         _ipv4Header._srcAddr[0], _ipv4Header._srcAddr[1], _ipv4Header._srcAddr[2], _ipv4Header._srcAddr[3],
         _ipv4Header._destAddr[0], _ipv4Header._destAddr[1], _ipv4Header._destAddr[2], _ipv4Header._destAddr[3]);
}