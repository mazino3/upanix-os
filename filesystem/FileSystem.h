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
#ifndef _FileSystem_H_
#define _FileSystem_H_

#include <Global.h>
#include <FSStructures.h>
#include <DeviceDrive.h>

#define FileSystem_SUCCESS								0
#define FileSystem_ERR_INVALID_BPB_SIGNATURE			1
#define FileSystem_ERR_INVALID_BOOT_SIGNATURE			2
#define FileSystem_ERR_INVALID_SECTORS_PER_TRACK		3
#define FileSystem_ERR_INVALID_NO_OF_HEADS				4
#define FileSystem_ERR_INVALID_DRIVE_SIZE_IN_SECTORS	5
#define FileSystem_ERR_INVALID_CLUSTER_ID				6
#define FileSystem_ERR_ZERO_FATSZ32						7

#define FileSystem_ERR_UNSUPPORTED_SECTOR_SIZE			9
#define FileSystem_ERR_NO_FREE_CLUSTER					10
#define FileSystem_ERR_BPB_JMP							11
#define FileSystem_ERR_UNSUPPORTED_MEDIA				12
#define FileSystem_ERR_INVALID_EXTFLAG					13
#define FileSystem_ERR_FS_VERSION						14
#define FileSystem_ERR_FSINFO_SECTOR					15
#define FileSystem_ERR_INVALID_VOL_ID					16
#define FileSystem_ERR_ALREADY_MOUNTED					17
#define FileSystem_ERR_NOT_MOUNTED						18
#define FileSystem_ERR_INSUF_MOUNT_SPACE				19
#define FileSystem_FAILURE								20

#define MEDIA_REMOVABLE	0xF0
#define MEDIA_FIXED		0xF8

#define EOC		0x0FFFFFFF
#define EOC_B	0xFF
#define DIR_ENTRIES_PER_SECTOR 7

#define FS_ROOT_DIR "/"

 unsigned FileSystem_GetRealSectorNumber(const unsigned uiSectorID, const DriveInfo* pDriveInfo) ;
 byte FileSystem_GetSectorEntryValue(DriveInfo* pDriveInfo, const unsigned uiSectorID,
		unsigned* uiSectorEntryValue) ;
 byte FileSystem_SetSectorEntryValue(DriveInfo* pDriveInfo, const unsigned uiSectorID,
		unsigned uiSectorEntryValue) ;

byte FileSystem_Format(DriveInfo* pDriveInfo) ;
byte FileSystem_Mount(DriveInfo* pDriveInfo) ;
byte FileSystem_UnMount(DriveInfo* pDriveInfo) ;
byte FileSystem_AllocateSector(DriveInfo* pDriveInfo, unsigned* uiFreeSectorID) ;
byte FileSystem_DeAllocateSector(DriveInfo* pDriveInfo, unsigned uiCurrentSectorID, unsigned* uiNextSectorID) ;

byte FileSystem_VerifyBootBlock(DriveInfo* pDriveInfo) ;
//void FileSystem_UpdateTime(FileSystem_Time* pTime) ;
byte FileSystem_IsFreePoolCacheEnabled(const DriveInfo* pDriveInfo) ;
byte FileSystem_IsTableCacheEnabled(const DriveInfo* pDriveInfo) ;
unsigned FileSystem_GetSizeForTableCache(unsigned uiNoOfSectorsInTableCache) ;

#endif

