/*
 *	Mother Operating System - An x86 based Operating System
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
#include <Display.h>

static multiboot_info_t* MultiBoot_pInfo ;

void MultiBoot_Initialize()
{
	MultiBoot_pInfo = (multiboot_info_t*)(&MULTIBOOT_INFO_ADDR) ;
}

unsigned MultiBoot_GetRamSizeInKB()
{
	return (MultiBoot_pInfo->mem_lower + MultiBoot_pInfo->mem_upper) ;
}

unsigned MultiBoot_GetRamSizeInMB()
{
	return MultiBoot_GetRamSizeInKB() / 1024 ;
}

unsigned MultiBoot_GetRamSize()
{
	return MultiBoot_GetRamSizeInKB() * 1024 ;
}

byte MultiBoot_GetBootDeviceID()
{
	return MultiBoot_pInfo->boot_device[3] ;
}

byte MultiBoot_GetBootPartitionID()
{
	return MultiBoot_pInfo->boot_device[2] ;
}
