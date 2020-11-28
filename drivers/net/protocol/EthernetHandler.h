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
#include <map.h>
#include <EtherType.h>
#include <drivers/net/protocol/packets/NetworkPacketComponents.h>
#include <option.h>

class RawNetPacket;
class EtherPacketHandler;
class NetworkDevice;
class ARPSendPacket;

class EthernetHandler {
public:
  EthernetHandler(NetworkDevice& networkDevice);
  void Process(const RawNetPacket& packet);
  NetworkDevice& GetNetworkDevice() {
    return _networkDevice;
  }
  template <typename T>
  upan::option<T&> GetHandler() {
    auto i = _etherPacketHandlers.find(T::HandlerType());
    if (i == _etherPacketHandlers.end()) {
      return upan::option<T&>::empty();
    }
    return upan::option<T&>(dynamic_cast<T&>(*i->second));
  }

  void SendPacket(ARPSendPacket& arpPacket, EtherType pType, const uint8_t* destMac);

  private:
    typedef upan::map<EtherType, EtherPacketHandler*> EtherPacketHandlerMap;
    EtherPacketHandlerMap _etherPacketHandlers;
    NetworkDevice& _networkDevice;

    const static uint32_t MIN_ETHERNET_PACKET_LEN = NetworkPacket::MAC_ADDR_LEN /*dmac*/ + NetworkPacket::MAC_ADDR_LEN /*smac*/ + 2 /*eType*/ + 1 /*payload*/;
};
