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
#ifndef _KB_DRIVER_H_
#define _KB_DRIVER_H_

#include <Global.h>
#include <queue.h>
#include <KeyboardKeyProcessor.h>

#define KBDriver_SUCCESS 0
#define KBDriver_ERR_BUFFER_EMPTY 1
#define KBDriver_ERR_BUFFER_FULL 2

#define KB_DATA_PORT 0x60
#define KB_STAT_PORT 0x64

class KeyboardKey
{
public:
  KeyboardKey(byte rawKey, KeyboardKeyProcessor& processor) : _rawKey(rawKey), _processor(&processor)
  {
  }

  KeyboardKey() : _rawKey(-1), _processor(nullptr)
  {
  }

  byte Data()
  {
    return _processor->Data(_rawKey);
  }

  byte MapChar()
  {
    return _processor->MapChar(_rawKey);
  }

  byte ShiftedMapChar()
  {
    return _processor->ShiftedMapChar(_rawKey);
  }

  bool IsKeyRelease()
  {
    return _processor->IsKeyRelease(_rawKey);
  }

  bool IsExtraKey()
  {
    return _processor->IsExtraKey(_rawKey);
  }

  byte ExtraKey()
  {
    return _processor->ExtraKey(_rawKey);
  }

  bool IsKey(byte val)
  {
    return _processor->IsKey(_rawKey, val);
  }

  bool IsNormal()
  {
    return _processor->IsNormal(_rawKey);
  }

  bool IsSpecial()
  {
    return _processor->IsSpecial(_rawKey);
  }

private:
  byte _rawKey;
  KeyboardKeyProcessor* _processor;
};

class KBDriver
{
  private:
    KBDriver();
  public:
    static KBDriver& Instance()
    {
      static KBDriver instance;
      return instance;
    }

    bool Process(KeyboardKey key);
    KeyboardKey GetCharInBlockMode();
    bool GetCharInNonBlockMode(KeyboardKey& data);
    bool PutToQueueBuffer(KeyboardKey data);

    bool WaitForWrite();
    bool WaitForRead();
    void Reboot();
    void Getch();
  private:
    bool GetFromQueueBuffer(KeyboardKey& data);

    upan::queue<KeyboardKey> _qBuffer;
};

#endif

