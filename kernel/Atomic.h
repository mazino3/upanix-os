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
#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include <Global.h>

class Mutex
{
	private:
		__volatile__ int m_iLock ;
		__volatile__ int m_iID ;
		
		static const int FREE_MUTEX = -999 ;

	public:
		Mutex() ;
		~Mutex() ;

		bool Lock(bool bBlock = true) ;
		bool UnLock() ;

	private:

		void Acquire() ;
		void Release() ;

	friend class TestSuite ;
} ;

class Atomic
{
	public:
		static int Swap(__volatile__ int& iLock, int val) ;
} ;

class MutexGuard
{
  public:
    MutexGuard(Mutex& m) : _m(m)
    {
      _m.Lock();
    }
    ~MutexGuard()
    {
      _m.UnLock();
    }
  private:
    Mutex& _m;
};

enum RESOURCE_KEYS
{
	RESOURCE_NIL = 999,
	RESOURCE_FDD = 0,
	RESOURCE_HDD,
	RESOURCE_UHCI_FRM_BUF,
	RESOURCE_USD,
	RESOURCE_GENERIC_DISK,
	MAX_RESOURCE,
};

class ResourceMutex
{
  public:
    ResourceMutex(RESOURCE_KEYS resourceKey) : _resourceKey(resourceKey)
    {
    }
  
    bool Lock(bool blocking = true);
    void UnLock();

  private:
    RESOURCE_KEYS _resourceKey;
};

class ResourceMutexGuard
{
  public:
    ResourceMutexGuard(RESOURCE_KEYS k) : _m(k)
    {
      _m.Lock();
    }
    ~ResourceMutexGuard()
    {
      _m.UnLock();
    }
  private:
    ResourceMutex _m;
};

#endif
