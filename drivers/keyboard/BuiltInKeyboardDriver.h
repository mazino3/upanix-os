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
#ifndef _BUILTINKEYBOARDDRIVER_H_
#define _BUILTINKEYBOARDDRIVER_H_

#include <Global.h>

#define KB_DATA_PORT 0x60
#define KB_STAT_PORT 0x64

class BuiltInKeyboardDriver
{
private:
  BuiltInKeyboardDriver();
  BuiltInKeyboardDriver(const BuiltInKeyboardDriver&) = delete;
  BuiltInKeyboardDriver& operator=(const BuiltInKeyboardDriver&) = delete;

public:
  static BuiltInKeyboardDriver& Instance()
  {
    static BuiltInKeyboardDriver instance;
    return instance;
  }
  void Process(byte rawKey);

  bool WaitForWrite();
  bool WaitForRead();
  void Reboot();

private:
  byte Decode(byte rawKey);

  bool _isShiftKey;
  bool _isCapsLock;
  bool _isCtrlKey;
};

#endif

