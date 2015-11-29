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

int Atomic::Swap(__volatile__ int& iLock, int val)
{
	__asm__ __volatile__ ("lock xchgl %0, %1"
		: "=r" ( val )
		: "m"( iLock ), "0" (val) 
		: "memory" );

	return val ;
}

Mutex::Mutex() : m_iLock(0), m_iID(FREE_MUTEX)
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
		val = Atomic::Swap(m_iLock, 1) ;
		if(val == 0)
			break ;

		ProcessManager::Instance().Sleep(10) ;
	}
}

void Mutex::Release()
{
	Atomic::Swap(m_iLock, 0) ;
}

bool Mutex::Lock(bool bBlock)
{
	__volatile__ int val ;

	while(true)
	{
		Acquire() ;

		val = ProcessManager::Instance().GetCurProcId() ;

		if(m_iID != FREE_MUTEX && m_iID != val)
		{
			Release() ;

			if(!bBlock)
				return false ;

			ProcessManager::Instance().Sleep(10) ;
			continue ;
		}

		if(m_iID == FREE_MUTEX)
			m_iID = val ;

		Release() ;
		break ;
	}

	return true ;
}

bool Mutex::UnLock()
{
	Acquire() ;

	__volatile__ int val = ProcessManager::Instance().GetCurProcId() ;

	if(m_iID != val)
	{
		Release() ;
		return false ;
	}

	m_iID = FREE_MUTEX ;

	Release() ;

	return true ;
}

