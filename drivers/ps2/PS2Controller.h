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

#include <option.h>

class PS2Controller {
public:
  static const int DATA_PORT = 0x60;
  static const int COMMAND_PORT = 0x64;

private:
  PS2Controller();

public:
  static PS2Controller& Instance() {
    static PS2Controller instance;
    return instance;
  }

  bool WaitForWrite();
  bool WaitForRead();
  void Reboot();

private:
  void Initialize();
  void InitDriver(uint32_t devCode);
  void SendCommand(uint16_t port, uint8_t cmd, const upan::string& opName);
  void SendCommand2(byte command, const upan::string& opName);
  void WaitForAck();
  upan::option<uint8_t> ReceiveData();
  void ClearOutputBuffer();

  friend class KC;
};

