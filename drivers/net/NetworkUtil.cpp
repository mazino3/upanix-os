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

#include <NetworkUtil.h>
#include <StringUtil.h>

uint8_t NetworkUtil::SwitchEndian(const uint8_t val) {
  return (val >> 4) | (val << 4);
}

uint16_t NetworkUtil::SwitchEndian(const uint16_t val) {
    return (val >> 8) | (val << 8);
}

uint32_t NetworkUtil::SwitchEndian(const uint32_t val) {
    return ((val >> 24) & 0xFF) | ((val >> 8) & 0xFF00) | ((val << 8) & 0xFF0000) | ((val << 24) & 0xFF000000);
}

uint16_t NetworkUtil::CalculateChecksum(const uint16_t* buf, uint32_t lengthInBytes, uint32_t initSum) {
  uint32_t sum = AddForChecksum(buf, lengthInBytes, initSum);
  while((sum >> 16)) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }
  return sum & 0xFFFF;
}

uint32_t NetworkUtil::AddForChecksum(const uint16_t* buf, uint32_t lengthInBytes, uint32_t initSum) {
  const uint32_t lengthInWords = lengthInBytes / 2;
  uint32_t sum = initSum;
  for(uint32_t i = 0; i < lengthInWords; ++i) {
    sum += buf[i];
  }
  if (lengthInBytes % 2) {
    sum += ((uint8_t*)buf)[lengthInBytes - 1];
  }
  return sum;
}

MACAddress::MACAddress(const upan::string &macAddr) : _macAddrStr(macAddr) {
  upan::vector<upan::string> tokens = tokenize(macAddr.c_str(), ':');
  if (tokens.size() != NetworkPacket::MAC_ADDR_LEN) {
    throw upan::exception(XLOC, "Invalid MAC Address: %s", macAddr.c_str());
  }
  for(int i = 0; i < tokens.size(); ++i) {
    _macAddr[i] = atoi(tokens[i].c_str());
  }
}

MACAddress::MACAddress(const upan::vector<uint8_t>& macAddr) {
  if (macAddr.size() != NetworkPacket::MAC_ADDR_LEN) {
    throw upan::exception(XLOC, "Invalid MAC Address Len: %d", macAddr.size());
  }
  char c[5];
  for(uint32_t i = 0; i < NetworkPacket::MAC_ADDR_LEN; ++i) {
    sprintf(c, "%02x%s", macAddr[i], i < NetworkPacket::MAC_ADDR_LEN - 1 ? ":" : "");
    _macAddrStr += c;
    _macAddr[i] = macAddr[i];
  }
}

MACAddress::MACAddress(const MACAddress& r) {
  copy(r);
}

MACAddress& MACAddress::operator=(const MACAddress& r) {
  copy(r);
  return *this;
}

MACAddress& MACAddress::operator=(const MACAddress&& r) {
  copy(r);
  return *this;
}

void MACAddress::copy(const MACAddress& r) {
  this->_macAddrStr = r._macAddrStr;
  memcpy(this->_macAddr, r._macAddr, NetworkPacket::MAC_ADDR_LEN);
}

IPAddress::IPAddress(const upan::string &ipAddr) : _ipAddrStr(ipAddr) {
  upan::vector<upan::string> tokens = tokenize(ipAddr.c_str(), '.');
  if (tokens.size() != NetworkPacket::IPV4_ADDR_LEN) {
    throw upan::exception(XLOC, "Invalid IP Address: %s", ipAddr.c_str());
  }
  for(int i = 0; i < tokens.size(); ++i) {
    _ipAddr[i] = atoi(tokens[i].c_str());
  }
}

IPAddress::IPAddress(const upan::vector<uint8_t>& ipAddr) {
  if (ipAddr.size() != NetworkPacket::IPV4_ADDR_LEN) {
    throw upan::exception(XLOC, "Invalid IP Address Len: %d", ipAddr.size());
  }
  char c[5];
  for(uint32_t i = 0; i < NetworkPacket::IPV4_ADDR_LEN; ++i) {
    sprintf(c, "%02x%s", ipAddr[i], i < NetworkPacket::IPV4_ADDR_LEN - 1 ? "." : "");
    _ipAddrStr += c;
    _ipAddr[i] = ipAddr[i];
  }
}

IPAddress::IPAddress(const IPAddress& r) {
  copy(r);
}

IPAddress& IPAddress::operator=(const IPAddress& r) {
  copy(r);
  return *this;
}

void IPAddress::copy(const IPAddress& r) {
  this->_ipAddrStr = r._ipAddrStr;
  memcpy(this->_ipAddr, r._ipAddr, NetworkPacket::IPV4_ADDR_LEN);
}
