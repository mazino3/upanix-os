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
#include <KeyboardHandler.h>
#include <PIC.h>
#include <IDT.h>
#include <ProcessManager.h>
#include <MemUtil.h>
#include <PS2KeyboardDriver.h>
#include <GraphicsVideo.h>

KeyboardHandler::KeyboardHandler() : _qBuffer(1024)
{
  KC::MConsole().LoadMessage("Keyboard Initialization", Success) ;
}

byte KeyboardHandler::GetCharInBlockMode()
{		
  byte data;
	while(!GetFromQueueBuffer(data))
		ProcessManager::Instance().WaitOnInterrupt(StdIRQ::Instance().KEYBOARD_IRQ);
  return data;
}

bool KeyboardHandler::GetCharInNonBlockMode(byte& data)
{
  return GetFromQueueBuffer(data);
}

bool KeyboardHandler::GetFromQueueBuffer(byte& data)
{
  if(_qBuffer.empty())
    return false;
  data = _qBuffer.front();
  _qBuffer.pop_front();
  return true;
}

bool KeyboardHandler::PutToQueueBuffer(byte data)
{
  return _qBuffer.push_back(data);
}

//just wait for a keyboard char input
void KeyboardHandler::Getch()
{
  byte ch;
  while(!GetCharInNonBlockMode(ch));
}

static void Keyboard_Event_Dispatcher() {
  try {
    while(true) {
      byte data = KeyboardHandler::Instance().GetCharInBlockMode();
      GraphicsVideo::Instance().getActiveFGProcess().ifPresent([&data](int pid) {
        ProcessSwitchLock pLock;
        auto process = ProcessManager::Instance().GetProcess(pid);
        process.ifPresent([&](Process& p) {
          p.dispatchKeyboardData(data);
        });
      });
    }
  } catch(upan::exception& e) {
    printf("\n Error in KB event dispatcher: %s", e.Error().Msg().c_str());
  }
  ProcessManager_Exit();
}

void KeyboardHandler::StartDispatcher() {
  static bool started = false;
  if (started) {
    return;
  }
  started = true;
  ProcessManager::Instance().CreateKernelProcess("kbed", (unsigned) &Keyboard_Event_Dispatcher,
                                                 ProcessManager::GetCurrentProcessID(), false, upan::vector<uint32_t>());

}