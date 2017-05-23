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
#ifndef _USB_KEYBOARD_H_
#define _USB_KEYBOARD_H_

#include <Global.h>
#include <SCSIHandler.h>
#include <USBStructures.h>
#include <USBDataHandler.h>
#include <list.h>
#include <set.h>

class USBKeyboard final : public USBInterruptDataHandler
{
public:
  USBKeyboard(USBDevice& device, int interfaceIndex);
  ~USBKeyboard();
  USBDevice& GetUSBDevice() { return _device; }

private:
  virtual void Handle(uint32_t data);

  const int STD_USB_KB_REPORT_LEN = 8;
  USBDevice& _device;
  byte*      _report;
  upan::set<byte> _pressedKeys;
};

class USBKeyboardDriver final : public USBDriver
{
private:
  static USBKeyboardDriver* _instance;
public:
  USBKeyboardDriver(const upan::string& name);
  static void Register();
  static USBKeyboardDriver& Instance();
  void Process(byte rawKey);

private:
  byte Decode(byte rawKey);
  bool AddDevice(USBDevice*);
  void RemoveDevice(USBDevice*);
  int MatchingInterfaceIndex(USBDevice*);

private:
  const int CLASS_CODE = 0x3;
  const int SUB_CLASS_CODE_BOOT = 0x01;
  const int PROTOCOL = 0x1;

  bool _isShiftKey;
  bool _isCapsLock;
  bool _isCtrlKey;

  upan::list<USBKeyboard*> _devices;
};

#endif
