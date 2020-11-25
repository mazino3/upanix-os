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

#include <drivers/net/protocol/packets/recv/ARPRecvPacket.h>
#include <stdio.h>
#include <NetworkUtil.h>

ARPRecvPacket::ARPRecvPacket(const EthernetRecvPacket& ethernetPacket) :
    _ethernetPacket(ethernetPacket),
    _arpHeader(reinterpret_cast<NetworkPacket::ARP::Header&>(*ethernetPacket.PacketData())),
    _arpIPV4(nullptr) {
  _arpHeader._hType = NetworkUtil::SwitchEndian(_arpHeader._hType);
  _arpHeader._pType = NetworkUtil::SwitchEndian(_arpHeader._pType);
  _arpHeader._opCode = NetworkUtil::SwitchEndian(_arpHeader._opCode);

  if (ARPRecvPacket::Type() == EtherType::IPV4) {
    _arpIPV4 = reinterpret_cast<NetworkPacket::ARP::IPV4*>(_ethernetPacket.PacketData() + sizeof(NetworkPacket::ARP::Header));
  }
}

void ARPRecvPacket::Print() const {
  printf("\n HType: %x, PType: %x, HLen: %d, PLen: %d, OpCode: %d",
         _arpHeader._hType, _arpHeader._pType, _arpHeader._hLen, _arpHeader._pLen, _arpHeader._opCode);

  if (_arpIPV4) {
    printf("\n SENDER HW ADDR: ");
    for (uint32_t  i = 0; i < NetworkPacket::MAC_ADDR_LEN; i++) {
      printf("%02x%s", _arpIPV4->_senderHardwareAddress[i], i < NetworkPacket::MAC_ADDR_LEN - 1 ? ":" : "");
    }
    printf("\n TARGET HW ADDRESS: ");
    for (uint32_t  i = 0; i < NetworkPacket::MAC_ADDR_LEN; i++) {
      printf("%02x%s", _arpIPV4->_targetHardwareAddress[i], i < NetworkPacket::MAC_ADDR_LEN - 1 ? ":" : "");
    }
  }
}