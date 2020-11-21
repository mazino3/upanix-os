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

#include <ARPPacket.h>
#include <stdio.h>
#include <NetworkUtil.h>

ARPPacket::ARPPacket(const EthernetPacket& ethernetPacket) :
  _ethernetPacket(ethernetPacket),
  _thisPacket(reinterpret_cast<ARPPacket::_ThisPacket&>(*ethernetPacket.PacketData())),
  _thisIPV4PacketData(nullptr) {
  _thisPacket._hType = NetworkUtil::SwitchEndian(_thisPacket._hType);
  _thisPacket._pType = NetworkUtil::SwitchEndian(_thisPacket._pType);
  _thisPacket._opCode = NetworkUtil::SwitchEndian(_thisPacket._opCode);

  if (Type() == EtherType::IPV4) {
    _thisIPV4PacketData = reinterpret_cast<_ThisIPV4PacketData*>(_ethernetPacket.PacketData() + sizeof(_ThisPacket));
  }
}

void ARPPacket::Print() const {
  printf("\n HType: %x, PType: %x, HLen: %d, PLen: %d, OpCode: %d",
         _thisPacket._hType, _thisPacket._pType, _thisPacket._hLen, _thisPacket._pLen, _thisPacket._opCode);

  if (_thisIPV4PacketData) {
    printf("\n SENDER HW ADDR: ");
    for (int i = 0; i < 6; i++) {
      printf("%02x%s", _thisIPV4PacketData->_senderHardwareAddress[i], i < 5 ? ":" : "");
    }
    printf("\n TARGET HW ADDRESS: ");
    for (int i = 0; i < 6; i++) {
      printf("%02x%s", _thisIPV4PacketData->_targetHardwareAddress[i], i < 5 ? ":" : "");
    }
  }
}