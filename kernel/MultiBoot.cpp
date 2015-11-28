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
#include <MultiBoot.h>
#include <MemConstants.h>

MultiBoot::MultiBoot()
{
	_pInfo = (multiboot_info_t*)(&MULTIBOOT_INFO_ADDR) ;
}

unsigned MultiBoot::GetRamSizeInKB()
{
	return (_pInfo->mem_lower + _pInfo->mem_upper) ;
}

unsigned MultiBoot::GetRamSizeInMB()
{
	return MultiBoot::GetRamSizeInKB() / 1024 ;
}

unsigned MultiBoot::GetRamSize()
{
	return MultiBoot::GetRamSizeInKB() * 1024 ;
}

byte MultiBoot::GetBootDeviceID()
{
	return _pInfo->boot_device[3] ;
}

byte MultiBoot::GetBootPartitionID()
{
	return _pInfo->boot_device[2] ;
}
