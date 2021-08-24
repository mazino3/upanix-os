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

#include <SchedulableProcess.h>

class Thread;

class AutonomousProcess : public SchedulableProcess {
public:
  AutonomousProcess(const upan::string& name, int parentID, bool isFGProcess);

  virtual Thread& CreateThread(uint32_t threadCaller, uint32_t entryAddress, void* arg) = 0;

  SchedulableProcess& forSchedule() override;
  void DestroyThreads() override;
  void addToThreadScheduler(Thread& thread);
  void dispatchKeyboardData(byte data) override;

  UIType getUIType() override {
    return _uiType;
  }

  void setupAsTtyProcess() override;
  int setupAsGuiProcess() override;

private:
  typedef upan::list<Thread*> ThreadSchedulerList;
  ThreadSchedulerList _threadSchedulerList;
  ThreadSchedulerList::iterator _nextThreadIt;
  UIType _uiType;
};