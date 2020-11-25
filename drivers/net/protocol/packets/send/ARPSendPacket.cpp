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

#include <ARPSendPacket.h>
#include <EtherType.h>
#include <NetworkPacketComponents.h>
#include <NetworkUtil.h>

ARPSendPacket::ARPSendPacket(uint16_t hType, EtherType pType, uint8_t hLen, uint8_t pLen, uint16_t opCode,
                             const uint8_t* sha, const uint8_t* spa, const uint8_t* tha, const uint8_t* tpa)
                             : _len(0), _buf(nullptr) {
  _len = NetworkPacket::SIZE_OF_ETHERNET_HEADER + NetworkPacket::SIZE_OF_ARP_HEADER + NetworkPacket::SIZE_OF_ARP_IPV4;
  _buf = new uint8_t[_len];

  NetworkPacket::ARP::Header* _arpHeader = reinterpret_cast<NetworkPacket::ARP::Header*>(
      _buf + NetworkPacket::SIZE_OF_ETHERNET_HEADER);
  _arpHeader->_hType = NetworkUtil::SwitchEndian(hType);
  _arpHeader->_pType = NetworkUtil::SwitchEndian((uint16_t)pType);
  _arpHeader->_hLen = hLen;
  _arpHeader->_pLen = pLen;
  _arpHeader->_opCode = NetworkUtil::SwitchEndian(opCode);

  NetworkPacket::ARP::IPV4* _arpIPV4 = reinterpret_cast<NetworkPacket::ARP::IPV4*>(
      _buf + NetworkPacket::SIZE_OF_ETHERNET_HEADER + NetworkPacket::SIZE_OF_ARP_HEADER);
  memcpy(_arpIPV4->_senderHardwareAddress, sha, NetworkPacket::MAC_ADDR_LEN);
  memcpy(_arpIPV4->_senderProtocolAddress, spa, NetworkPacket::IPV4_ADDR_LEN);
  memcpy(_arpIPV4->_targetHardwareAddress, tha, NetworkPacket::MAC_ADDR_LEN);
  memcpy(_arpIPV4->_targetProtocolAddress, tpa, NetworkPacket::IPV4_ADDR_LEN);
}

ARPSendPacket::~ARPSendPacket() {
  delete[] _buf;
}