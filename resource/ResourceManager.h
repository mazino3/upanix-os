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
#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_

#define ResourceManager_SUCCESS					0
#define ResourceManager_ERR_BUSY				1
#define ResourceManager_ERR_UNKNOWN_RESOURCE	2
#define ResourceManager_FAILURE					3

# include <Global.h>
# include <DeviceDrive.h>

typedef enum
{
	RESOURCE_NIL = 999,
	RESOURCE_FDD = 0,
	RESOURCE_HDD,
	RESOURCE_UHCI_FRM_BUF,
	RESOURCE_USD,
	RESOURCE_GENERIC_DISK,
	MAX_RESOURCE,
} RESOURCE_KEYS ;

typedef enum
{
	RESOURCE_ACQUIRE_BLOCK,
	RESOURCE_ACQUIRE_NON_BLOCK
} RESOURCE_ACQUIRE_MODE ;

byte ResourceManager_Acquire(unsigned uiType, unsigned uiMode) ;
void ResourceManager_Release(unsigned uiType) ;
byte ResourceManager_GetDiskResourceTypeByDeviceType(DEVICE_TYPE deviceType, unsigned* uiResourceType) ;
byte ResourceManager_GetDiskResourceType(RAW_DISK_TYPES iType, unsigned* uiResourceType) ;

#endif
