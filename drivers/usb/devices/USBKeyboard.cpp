/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2Keyboard_NA_CHAR11 'Prajwala Prabhakar' 'srinivasa_prajwal@yahoo.co.in'
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
#include <USBKeyboard.h>
#include <stdio.h>
#include <ustring.h>
#include <USBController.h>
#include <USBDevice.h>
#include <KeyboardHandler.h>
#include <IrqManager.h>
#include <KBInputHandler.h>
#include <KernelUtil.h>
#include <MemManager.h>
#include <Alloc.h>
#include <Bit.h>

static const byte Keyboard_USB_GENERIC_KEY_MAP[] =
{
  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',

  'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',

  '3', '4', '5', '6', '7', '8', '9', '0', Keyboard_ENTER, Keyboard_ESC, Keyboard_BACKSPACE,

  '\t', ' ', '-', '=', '[', ']', '\\', Keyboard_NA_CHAR, ';', '\'', '`', ',', '.', '/', Keyboard_CAPS_LOCK,

  Keyboard_F1, Keyboard_F2, Keyboard_F3, Keyboard_F4, Keyboard_F5, Keyboard_F6, Keyboard_F7,

  Keyboard_F8, Keyboard_F9, Keyboard_F10, Keyboard_F11, Keyboard_F12, Keyboard_NA_CHAR/*print*/, Keyboard_NA_CHAR/*scroll*/, Keyboard_NA_CHAR/*pause*/,

  Keyboard_KEY_INST, Keyboard_KEY_HOME, Keyboard_KEY_PG_UP, Keyboard_KEY_DEL, Keyboard_KEY_END,

  Keyboard_KEY_PG_DOWN, Keyboard_KEY_RIGHT, Keyboard_KEY_LEFT, Keyboard_KEY_DOWN, Keyboard_KEY_UP, Keyboard_KEY_NUM,

  '/', '*', '-', '+', Keyboard_ENTER, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '.',

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_LEFT_CTRL, Keyboard_LEFT_SHIFT, Keyboard_LEFT_ALT, Keyboard_NA_CHAR, Keyboard_RIGHT_CTRL, Keyboard_RIGHT_SHIFT, Keyboard_RIGHT_ALT,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR
};

static const byte Keyboard_USB_SHIFTED_KEY_MAP[] =
{
  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',

  'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',

  '#', '$', '%', '^', '&', '*', '(', ')', Keyboard_ENTER, Keyboard_ESC, Keyboard_BACKSPACE,

  '\t', ' ', '_', '+', '{', '}', '|', Keyboard_NA_CHAR, ':', '"', '~', '<', '>', '?', Keyboard_CAPS_LOCK,

  Keyboard_F1, Keyboard_F2, Keyboard_F3, Keyboard_F4, Keyboard_F5, Keyboard_F6, Keyboard_F7,

  Keyboard_F8, Keyboard_F9, Keyboard_F10, Keyboard_F11, Keyboard_F12, Keyboard_NA_CHAR/*print*/, Keyboard_NA_CHAR/*scroll*/, Keyboard_NA_CHAR/*pause*/,

  Keyboard_KEY_INST, Keyboard_KEY_HOME, Keyboard_KEY_PG_UP, Keyboard_KEY_DEL, Keyboard_KEY_END,

  Keyboard_KEY_PG_DOWN, Keyboard_KEY_RIGHT, Keyboard_KEY_LEFT, Keyboard_KEY_DOWN, Keyboard_KEY_UP, Keyboard_KEY_NUM,

  '/', '*', '-', '+', Keyboard_ENTER, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '.',

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_LEFT_CTRL, Keyboard_LEFT_SHIFT, Keyboard_LEFT_ALT, Keyboard_NA_CHAR, Keyboard_RIGHT_CTRL, Keyboard_RIGHT_SHIFT, Keyboard_RIGHT_ALT,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR,

  Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_NA_CHAR
};


USBKeyboardDriver* USBKeyboardDriver::_instance = nullptr;

USBKeyboardDriver::USBKeyboardDriver(const upan::string& name) : USBDriver(name), _isShiftKey(false), _isCapsLock(false), _isCtrlKey(false)
{
  if(_instance)
    throw upan::exception(XLOC, "USB Keyboard driver is already registered!");
  _instance = this;
}

void USBKeyboardDriver::Register()
{
  USBController::Instance().RegisterDriver(new USBKeyboardDriver("USB Keyboard"));
}

USBKeyboardDriver& USBKeyboardDriver::Instance()
{
  if(!_instance)
    throw upan::exception(XLOC, "USB Keyboard driver is not initialized yet!");
  return *_instance;
}

int USBKeyboardDriver::MatchingInterfaceIndex(USBDevice* pUSBDevice)
{
  USBStandardConfigDesc* pConfig = &(pUSBDevice->_pArrConfigDesc[ pUSBDevice->_iConfigIndex ]) ;
  for(byte i = 0; i < pConfig->bNumInterfaces; ++i)
  {
    const auto& interface = pConfig->pInterfaces[i];
    //for now, support only keyboard with boot interface (which uses default interrupt report)
    if(interface.bInterfaceClass == CLASS_CODE && interface.bInterfaceProtocol == PROTOCOL)
      return i;
  }
  return -1;
}

bool USBKeyboardDriver::AddDevice(USBDevice* pUSBDevice)
{
  int interfaceIndex = MatchingInterfaceIndex(pUSBDevice);
  if(interfaceIndex < 0)
    return false;

  const USBStandardInterface& interface = pUSBDevice->_pArrConfigDesc[ pUSBDevice->_iConfigIndex ].pInterfaces[ interfaceIndex ];
  if(interface.bInterfaceSubClass != SUB_CLASS_CODE_BOOT)
    throw upan::exception(XLOC, "Non boot-device USB keyboard not supported - SubClass: %d", interface.bInterfaceSubClass);

  _devices.push_back(new USBKeyboard(*pUSBDevice, interfaceIndex));

  return true;
}

void USBKeyboardDriver::RemoveDevice(USBDevice* pUSBDevice)
{
  for(auto device : _devices)
  {
    if(pUSBDevice == &device->GetUSBDevice())
    {
      _devices.erase(device);
      delete device;
      break;
    }
  }
}

void USBKeyboardDriver::Process(byte rawKey)
{
  byte kbKey = Decode(rawKey);
  if (kbKey == Keyboard_NA_CHAR)
    return;

  if(_isCtrlKey)
    kbKey += CTRL_VALUE;

  if(!KBInputHandler_Process(kbKey))
  {
    KeyboardHandler::Instance().PutToQueueBuffer(kbKey);
    StdIRQ::Instance().KEYBOARD_IRQ.Signal();
  }
}

void USBKeyboardDriver::ProcessSpecialKeys(byte report)
{
  _isShiftKey = Bit::IsSet(report, 0x2) || Bit::IsSet(report, 0x20);
  _isCtrlKey = Bit::IsSet(report, 0x1) || Bit::IsSet(report, 0x10);
}

byte USBKeyboardDriver::Decode(byte rawKey)
{
  byte mappedKey = Keyboard_USB_GENERIC_KEY_MAP[rawKey];

  if(mappedKey == Keyboard_CAPS_LOCK)
  {
    _isCapsLock = !_isCapsLock;
    return Keyboard_NA_CHAR;
  }

  if(_isShiftKey)
  {
    if(mappedKey >= 'a' && mappedKey <= 'z' && _isCapsLock)
      return mappedKey;
    return Keyboard_USB_SHIFTED_KEY_MAP[rawKey];
  }
  else
  {
    if(mappedKey >= 'a' && mappedKey <= 'z' && _isCapsLock)
      return Keyboard_USB_SHIFTED_KEY_MAP[rawKey];
    return mappedKey;
  }
}

USBKeyboard::USBKeyboard(USBDevice& device, int interfaceIndex) : _device(device), _currentReportIndex(0)
{
  if (_device.GetInterruptQueueSize() * STD_USB_KB_REPORT_LEN > PAGE_SIZE)
    throw upan::exception(XLOC, "The Interrupt Queue buffer size is larger than PAGE_SIZE");

  const USBStandardInterface& interface = _device._pArrConfigDesc[ _device._iConfigIndex ].pInterfaces[ interfaceIndex ];
  _device._iInterfaceIndex = interfaceIndex;
  _device._bInterfaceNumber = interface.bInterfaceNumber;
  _device._pPrivate = this;

  _device.SetIdle();
  //TODO: To set this upfront or use scheduler ??
  for(int i = 0; i < 0; ++i)
  {
    byte* report = new byte[STD_USB_KB_REPORT_LEN];
    _device.SetupInterruptReceiveData((uint32_t)report, STD_USB_KB_REPORT_LEN, this);
  }

  uint32_t reportAddress = KERNEL_VIRTUAL_ADDRESS(MemManager::Instance().AllocatePageForKernel() * PAGE_SIZE);
  _reports = new ((void*)reportAddress) byte*[_device.GetInterruptQueueSize()];

  reportAddress += sizeof(byte*) * _device.GetInterruptQueueSize();
  for(uint32_t i = 0; i < _device.GetInterruptQueueSize(); ++i)
    _reports[i] = new ((void*)(reportAddress + i * STD_USB_KB_REPORT_LEN)) byte[STD_USB_KB_REPORT_LEN];

  KernelUtil::ScheduleTimedTask("USB KB Poller", 50, *this);
}

USBKeyboard::~USBKeyboard()
{
  MemManager::Instance().DeAllocatePageForKernel(KERNEL_REAL_ADDRESS(_reports) / PAGE_SIZE);
}

bool USBKeyboard::TimerTrigger()
{
  if(_device.SetupInterruptReceiveData((uint32_t)&_reports[_currentReportIndex], STD_USB_KB_REPORT_LEN, this))
    _currentReportIndex = (_currentReportIndex + 1) % _device.GetInterruptQueueSize();
  return true;
}

void USBKeyboard::Handle(uint32_t data)
{
  byte* report = (byte*)data;
  //printf("\n%d %d %d %d %d %d", report[2], report[3], report[4], report[5], report[6], report[7]);

  USBKeyboardDriver::Instance().ProcessSpecialKeys(report[0]);

  for(auto k = _pressedKeys.begin(); k != _pressedKeys.end();)
  {
    bool found = false;
    for(int i = 2; i < 8 && report[i] != 0; ++i)
    {
      found = report[i] == *k;
      if(found)
        break;
    }
    if(!found)
      _pressedKeys.erase(k++);
    else
      ++k;
  }

  for(int i = 2; i < 8 && report[i] != 0; ++i)
  {
    //TODO: it's a set - so a check for exists may not be required. try insert and check if it is a success instead
    if(_pressedKeys.exists(report[i]))
      continue;
    _pressedKeys.insert(report[i]);
    USBKeyboardDriver::Instance().Process(report[i]);
  }
}
