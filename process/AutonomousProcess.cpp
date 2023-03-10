/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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

#include <AutonomousProcess.h>
#include <Thread.h>
#include <StreamBufferDescriptor.h>
#include <ProcessManager.h>
#include <KeyboardHandler.h>
#include <GraphicsVideo.h>

AutonomousProcess::AutonomousProcess(const upan::string& name, int parentID, bool isFGProcess)
  : SchedulableProcess(name, parentID, isFGProcess), _nextThreadIt(_threadSchedulerList.begin()),
    _uiType(Process::UIType::NA), _uiKeyboardEventStreamFD(nullptr), _uiMouseEventStreamFD(nullptr),
    _isGuiBase(false) {
}

SchedulableProcess& AutonomousProcess::forSchedule() {
  if (_status == TERMINATED || _status == RELEASED) {
    return *this;
  }

  if (_nextThreadIt == _threadSchedulerList.end()) {
    _nextThreadIt = _threadSchedulerList.begin();
    return *this;
  }

  while(!_threadSchedulerList.empty()) {
    if (_nextThreadIt == _threadSchedulerList.end()) {
      _nextThreadIt = _threadSchedulerList.begin();
    }
    auto curThreadIt = _nextThreadIt++;
    Thread& thread = **curThreadIt;

    if (thread.status() == RELEASED) {
      _threadSchedulerList.erase(curThreadIt);
      ProcessManager::Instance().RemoveFromProcessMap(thread);
      delete &thread;
    } else {
      return thread;
    }
  }

  return *this;
}

//TODO: use a process level lock instead of global process switch lock
void AutonomousProcess::addToThreadScheduler(Thread& thread) {
  ProcessSwitchLock lock;
  _threadSchedulerList.push_back(&thread);
  thread.setStatus(RUN);
}

void AutonomousProcess::DestroyThreads() {
  // child threads should be destroyed
  // threads must be destroyed before dealing with child processes because
  // child processes if any of a thread will be redirected to current process (main thread)
  for(auto t : _threadSchedulerList) {
    if (t->status() != TERMINATED && t->status() != RELEASED) {
      t->Destroy();
    }
    t->Release();
    ProcessManager::Instance().RemoveFromProcessMap(*t);
    delete t;
  }
  _threadSchedulerList.clear();
}

void AutonomousProcess::dispatchKeyboardData(const upanui::KeyboardData& data) {
  switch (_uiType) {
    case Process::REDIRECT_TTY:
    case Process::TTY: {
      const auto ch = (uint8_t)KeyboardHandler::Instance().mapToTTYKey(data);
      if (ch != Keyboard_NA_CHAR) {
        iodTable().get(IODescriptorTable::STDIN).write((void*)&ch, 1);
      }
    }
    break;

    case Process::GUI:
      _uiKeyboardEventStreamFD->write((void*)&data, sizeof(upanui::KeyboardData));
      break;

    case Process::NA:
      break;
  }
}

void AutonomousProcess::dispatchMouseData(const upanui::MouseData& mouseData) {
  if (_uiType == Process::GUI) {
    _uiMouseEventStreamFD->write((void*)&mouseData, sizeof(upanui::MouseData));
  }
}

void AutonomousProcess::setupAsTtyProcess() {
  if (_uiType != Process::UIType::NA) {
    throw upan::exception(XLOC, "Process %d is already initialized with UIType %d", _processID, _uiType);
  }
  iodTable().setupStreamedStdio();
  _uiType = Process::UIType::TTY;
}

void AutonomousProcess::setupAsRedirectTtyProcess() {
  if (_uiType != Process::UIType::NA) {
    throw upan::exception(XLOC, "Process %d is already initialized with UIType %d", _processID, _uiType);
  }
  _uiType = Process::UIType::REDIRECT_TTY;
}

void AutonomousProcess::setupAsGuiProcess(int fdList[]) {
  if (_uiType != Process::UIType::NA) {
    throw upan::exception(XLOC, "Process %d is already initialized with UIType %d", _processID, _uiType);
  }

  _uiType = Process::UIType::GUI;
  _uiKeyboardEventStreamFD = &iodTable().allocate([&](int fd) {
    return new StreamBufferDescriptor(_processID, fd, 4096, O_WR_NONBLOCK | O_RD_NONBLOCK);
  });

  _uiMouseEventStreamFD = &iodTable().allocate([&](int fd) {
    return new StreamBufferDescriptor(_processID, fd, 4096, O_WR_NONBLOCK | O_RD_NONBLOCK);
  });

  fdList[0] = _uiKeyboardEventStreamFD->id();
  fdList[1] = _uiMouseEventStreamFD->id();
}

void AutonomousProcess::setGuiBase(bool val) {
  if (getGuiFrame().isEmpty()) {
    throw upan::exception(XLOC, "can not set non-GUI process as GUI base");
  }

  if (_isGuiBase == val) {
    return;
  }

  ProcessSwitchLock pLock;
  _isGuiBase = val;
  if (_isGuiBase) {
    GraphicsVideo::Instance().addGuiBase(_processID);
  } else {
    GraphicsVideo::Instance().removeGuiBase(_processID);
  }
  getGuiFrame().value().touch();
}