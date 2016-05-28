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
#include <XHCIContext.h>

InputContext::InputContext(bool use64)
{
  unsigned addr = KERNEL_VIRTUAL_ADDRESS(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);
  if(use64)
  {
    auto context64 = new ((void*)addr)InputContext64();
    _control = &context64->_controlContext;
    _devContext = new DeviceContext(context64->_deviceContext);
  }
  else
  {
    auto context32 = new ((void*)addr)InputContext32();
    _control = &context32->_controlContext;
    _devContext = new DeviceContext(context32->_deviceContext);
  }
}

InputContext::~InputContext()
{
  unsigned addr = KERNEL_REAL_ADDRESS(_control);
  MemManager::Instance().DeAllocatePhysicalPage(addr / PAGE_SIZE);
  delete _devContext;
}

DeviceContext::DeviceContext(bool use64) : _allocated(true)
{
  unsigned addr = KERNEL_VIRTUAL_ADDRESS(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);
  if(use64)
    Init(*new ((void*)addr)DeviceContext64());
  else
    Init(*new ((void*)addr)DeviceContext32());
}

DeviceContext::DeviceContext(DeviceContext32& dc32) : _allocated(false)
{
  Init(dc32);
}

DeviceContext::DeviceContext(DeviceContext64& dc64) : _allocated(false)
{
  Init(dc64);
}

template <typename DC>
void DeviceContext::Init(DC& dc)
{
  _slot = &dc._slotContext;
  _ep0 = &dc._epContext0;
  _eps = dc._epContexts;
}

DeviceContext::~DeviceContext()
{
  if(_allocated)
  {
    unsigned addr = KERNEL_REAL_ADDRESS(_slot);
    MemManager::Instance().DeAllocatePhysicalPage(addr / PAGE_SIZE);
  }
}
