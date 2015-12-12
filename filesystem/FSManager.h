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
#ifndef _FS_MANAGER_H_
#define _FS_MANAGER_H_

#define FSManager_SUCCESS	0
#define FSManager_FAILURE	1

#define ENABLE_TABLE_CAHCE 0x01
#define ENABLE_FREE_POOL_CACHE 0x02

# include <DeviceDrive.h>

byte FSManager_Mount(DiskDrive* pDiskDrive) ;
byte FSManager_UnMount(DiskDrive* pDiskDrive) ;
byte FSManager_GetSectorEntryValue(DiskDrive* pDiskDrive, const unsigned uiSectorID, unsigned* uiSectorEntryValue, byte bFromCahceOnly) ;
byte FSManager_SetSectorEntryValue(DiskDrive* pDiskDrive, const unsigned uiSectorID, unsigned uiSectorEntryValue, byte bFromCahceOnly) ;
byte FSManager_AllocateSector(DiskDrive* pDiskDrive, unsigned* uiFreeSectorID) ;

#endif
