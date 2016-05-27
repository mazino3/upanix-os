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

#include <MemManager.h>
#include <Alloc.h>
#include <XHCIInputContext.h>

InputContext::InputContext(bool use64) : _context32(nullptr), _context64(nullptr)
{
  unsigned addr = KERNEL_VIRTUAL_ADDRESS(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);
  if(use64)
  {
    _context64 = new ((void*)addr)InputContext64();
    _control = &_context64->_controlContext;
    _slot = &_context64->_slotContext;
    _ep0 = &_context64->_epContext0;
    _eps = _context64->_epContexts;
  }
  else
  {
    _context32 = new ((void*)addr)InputContext32();
    _control = &_context32->_controlContext;
    _slot = &_context32->_slotContext;
    _ep0 = &_context32->_epContext0;
    _eps = _context32->_epContexts;
  }
}

InputContext::~InputContext()
{
  unsigned addr = KERNEL_REAL_ADDRESS(_context32 ? (unsigned)_context32 : (unsigned)_context64);
  MemManager::Instance().DeAllocatePhysicalPage(addr / PAGE_SIZE);
}
