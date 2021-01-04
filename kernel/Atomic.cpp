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
#include <Global.h>
#include <Display.h>
#include <Atomic.h>
#include <ProcessManager.h>

uint32_t Atomic::Swap(__volatile__ uint32_t& iLock, uint32_t val)
{
	__asm__ __volatile__ ("lock xchgl %0, %1"
		: "=r" ( val )
		: "m"( iLock ), "0" (val) 
		: "memory" );

	return val ;
}

void Atomic::Add(__volatile__ uint32_t& var, uint32_t val)
{
  __asm__ __volatile__ ("lock xaddl %0, %1"
    : "=r"(val)
    : "m"( var ), "0" (val)
    : "memory", "cc");
}

Mutex::Mutex() : _lock(0), _processID(FREE_MUTEX)
{
}

Mutex::~Mutex()
{
}

void Mutex::Acquire()
{
	__volatile__ int val ;

	while(true)
	{
		val = Atomic::Swap(_lock, 1) ;
		if(val == 0)
			break ;

		ProcessManager::Instance().Sleep(10) ;
	}
}

void Mutex::Release()
{
	Atomic::Swap(_lock, 0) ;
}

bool Mutex::Lock(bool bBlock)
{
	__volatile__ int val ;

	while(true)
	{
		Acquire() ;

		val = ProcessManager::Instance().GetCurProcId() ;

		if(_processID != FREE_MUTEX && _processID != val)
		{
			Release() ;

			if(!bBlock)
				return false ;

			ProcessManager::Instance().Sleep(10) ;
			continue ;
		}

		if(_processID == FREE_MUTEX)
      _processID = val ;

		Release() ;
		break ;
	}

	return true ;
}

bool Mutex::UnLock()
{
  Acquire() ;

  __volatile__ int pid = ProcessManager::Instance().GetCurProcId() ;

  if(_processID != pid)
  {
    Release() ;
    return false ;
  }

  _processID = FREE_MUTEX ;

  Release() ;

  return true ;
}

bool Mutex::UnLock(int pid)
{
	Acquire() ;

	if(_processID != pid)
	{
		Release() ;
		return false ;
	}

  _processID = FREE_MUTEX ;

	Release() ;

	return true ;
}

bool ResourceMutex::Lock(bool blocking)
{
	ProcessManager::DisableTaskSwitch();

	if(!ProcessManager::Instance().IsResourceBusy(_resourceKey))
		ProcessManager::Instance().SetResourceBusy(_resourceKey, true);
	else
	{
    if(!blocking)
      return false;  
    ProcessManager::Instance().WaitOnResource(_resourceKey);
	}

	ProcessManager::EnableTaskSwitch();
	return true;
}

void ResourceMutex::UnLock()
{
  ProcessSwitchLock pLock;
	ProcessManager::Instance().SetResourceBusy(_resourceKey, false);
}
