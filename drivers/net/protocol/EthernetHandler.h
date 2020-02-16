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

class EthernetHandler {
public:
  void Process(const RawNetPacket& packet);
private:
  struct EthernetPacket {
    void Print();
    
    const uint8_t _destinationMAC[6];
    const uint8_t _sourceMAC[6];
    const uint16_t _type;
    const uint8_t* _payload;
  } PACKED;

  const static uint32_t MIN_ETHERNET_PACKET_LEN = 6 /*dmac*/ + 6 /*smac*/ + 2 /*eType*/ + 1 /*payload*/;
};