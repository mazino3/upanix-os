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
#include <ProcessGroup.h>
#include <ProcessManager.h>
#include <MemManager.h>
#include <Display.h>
#include <DMM.h>
#include <exception.h>

int ProcessGroup::_idSeq = 0;
ProcessGroup* ProcessGroup::_fgProcessGroup = nullptr;

ProcessGroup::ProcessGroup(bool isFGProcessGroup) 
  : _id(++_idSeq), _iProcessCount(0),
    _videoBuffer(nullptr)//KC::MConsole().CreateDisplayBuffer())
{
  if(isFGProcessGroup)
    _fgProcessGroup = this;
}

ProcessGroup::~ProcessGroup()
{
  delete _videoBuffer;
}

void ProcessGroup::PutOnFGProcessList(int iProcessID)
{
  _fgProcessList.push_front(iProcessID);
}

void ProcessGroup::RemoveFromFGProcessList(int iProcessID)
{
  _fgProcessList.erase(iProcessID);
}

void ProcessGroup::AddProcess()
{
	++_iProcessCount;
}

void ProcessGroup::RemoveProcess()
{
	--_iProcessCount;
}

void ProcessGroup::SwitchToFG()
{
  _fgProcessGroup = this;
}

bool ProcessGroup::IsFGProcessGroup() const
{
	return this == _fgProcessGroup;
}

bool ProcessGroup::IsFGProcess(int iProcessID) const
{
  if(_fgProcessList.empty())
    return false;
  return *_fgProcessList.begin() == iProcessID;
}
