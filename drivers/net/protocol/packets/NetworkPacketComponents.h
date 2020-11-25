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

namespace NetworkPacket {
  constexpr uint32_t MAC_ADDR_LEN = 6;
  constexpr uint32_t IPV4_ADDR_LEN = 4;

  namespace ARP {
    struct Header {
      uint16_t _hType;
      uint16_t _pType;
      uint8_t _hLen;
      uint8_t _pLen;
      uint16_t _opCode;
    } PACKED;

    struct IPV4 {
      uint8_t _senderHardwareAddress[MAC_ADDR_LEN];
      uint8_t _senderProtocolAddress[IPV4_ADDR_LEN];
      uint8_t _targetHardwareAddress[MAC_ADDR_LEN];
      uint8_t _targetProtocolAddress[IPV4_ADDR_LEN];
    } PACKED;
  }

  namespace Ethernet {
    struct Header {
      uint8_t _destinationMAC[MAC_ADDR_LEN];
      uint8_t _sourceMAC[MAC_ADDR_LEN];
      uint16_t _type;
    } PACKED;
  }

  constexpr uint32_t SIZE_OF_ARP_HEADER = sizeof(ARP::Header);
  constexpr uint32_t SIZE_OF_ARP_IPV4 = sizeof(ARP::IPV4);
  constexpr uint32_t SIZE_OF_ETHERNET_HEADER = sizeof(Ethernet::Header);
};