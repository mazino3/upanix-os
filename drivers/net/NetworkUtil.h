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
#include <libmcpp/ds/ustring.h>
#include <drivers/net/protocol/packets/NetworkPacketComponents.h>
#include <libmcpp/ds/vector.h>

#define LITTLE_ENDIAN 1

class NetworkUtil {
public:
  static uint8_t SwitchEndian(const uint8_t val);
  static uint16_t SwitchEndian(const uint16_t val);
  static uint32_t SwitchEndian(const uint32_t val);
  static uint16_t CalculateChecksum(const uint16_t* buf, uint32_t lengthInBytes, uint32_t initSum);
  static uint32_t AddForChecksum(const uint16_t* buf, uint32_t lengthInBytes, uint32_t initSum);
};

class MACAddress {
public:
  MACAddress() {}
  MACAddress(const upan::string& macAddr);
  MACAddress(const upan::vector<uint8_t>& macAddr);
  MACAddress(const uint8_t* macAddr);
  MACAddress(const MACAddress&);
  MACAddress& operator=(const MACAddress&);

  MACAddress(const MACAddress&&) = delete;
  MACAddress& operator=(const MACAddress&&);

  const upan::string str() const {
    return _macAddrStr;
  }
  const uint8_t* get() const {
    return _macAddr;
  }
private:
  template <typename MACAddr>
  void convert(const MACAddr& macAddr) {
    char c[5];
    for(uint32_t i = 0; i < NetworkPacket::MAC_ADDR_LEN; ++i) {
      sprintf(c, "%02x%s", macAddr[i], i < NetworkPacket::MAC_ADDR_LEN - 1 ? ":" : "");
      _macAddrStr += c;
      _macAddr[i] = macAddr[i];
    }
  }
  void copy(const MACAddress&);

  upan::string _macAddrStr;
  uint8_t _macAddr[NetworkPacket::MAC_ADDR_LEN];
};

class IPAddress {
public:
  IPAddress(const upan::string& ipAddr);
  IPAddress(const upan::vector<uint8_t>& ipAddr);
  IPAddress(const uint8_t* ipAddr);
  IPAddress(const IPAddress&);
  IPAddress& operator=(const IPAddress&);

  IPAddress(const IPAddress&&) = delete;
  IPAddress& operator=(const IPAddress&&) = delete;

  const upan::string str() const {
    return _ipAddrStr;
  }
  const uint8_t* get() const {
    return _ipAddr;
  }
private:
  template <typename IPAddr>
  void convert(const IPAddr& ipAddr) {
    char c[5];
    for(uint32_t i = 0; i < NetworkPacket::IPV4_ADDR_LEN; ++i) {
      sprintf(c, "%u%s", ipAddr[i], i < NetworkPacket::IPV4_ADDR_LEN - 1 ? "." : "");
      _ipAddrStr += c;
      _ipAddr[i] = ipAddr[i];
    }
  }

  void copy(const IPAddress&);

  upan::string _ipAddrStr;
  uint8_t _ipAddr[NetworkPacket::IPV4_ADDR_LEN];
};