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
#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include <Global.h>
#include <atomic.h>

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
