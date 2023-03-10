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
  void dispatchKeyboardData(const upanui::KeyboardData& data) override;
  void dispatchMouseData(const upanui::MouseData& mouseData) override;

  UIType getUIType() override {
    return _uiType;
  }

  void setupAsTtyProcess() override;
  void setupAsRedirectTtyProcess() override;
  void setupAsGuiProcess(int fdList[]) override;

  bool isGuiBase() const override {
    return _isGuiBase;
  }

  void setGuiBase(bool v) override;

private:
  typedef upan::list<Thread*> ThreadSchedulerList;
  ThreadSchedulerList _threadSchedulerList;
  ThreadSchedulerList::iterator _nextThreadIt;
  UIType _uiType;
  IODescriptor* _uiKeyboardEventStreamFD;
  IODescriptor* _uiMouseEventStreamFD;
  bool _isGuiBase;
};