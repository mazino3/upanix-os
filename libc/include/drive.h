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
#ifndef _DRIVE_H_
#define _DRIVE_H_

#include <DeviceDrive.h>

/*
typedef enum
{
	FS_UNKNOWN = 0x0,
	MOSFS = 0x83
} FS_TYPE ;

typedef enum
{
	DEV_FLOPPY = 0x0,
	DEV_CD,
	DEV_ATA_IDE = 0x80,
	DEV_ATAPI,
	DEV_SCSI_USB_DISK = 0x90
} DEVICE_TYPE ;

typedef enum
{
	FD_DRIVE0 = 0,
	FD_DRIVE1,
	FD_DRIVE2,
	FD_DRIVE3,
	CD_DRIVE0,
	CD_DRIVE1,
	HDD_DRIVE0,
	HDD_DRIVE1,
	USD_DRIVE0,
	USD_DRIVE1,
	USD_DRIVE2,
	USD_DRIVE3,
	USD_DRIVE4,
	USD_DRIVE5
} DRIVE_NO ;

typedef struct 
{
	char			driveName[33] ;
	unsigned		uiID ;
	byte			bMounted ;

	DEVICE_TYPE		deviceType ;
	FS_TYPE			fsType ;
	DRIVE_NO		driveNumber ;
	
	unsigned		uiLBAStartSector ;
	unsigned		uiSizeInSectors ;

	unsigned		uiStartCynlider ;
	unsigned		uiStartHead ;
	unsigned		uiStartSector ;

	unsigned		uiEndCynlider ;
	unsigned		uiEndHead ;
	unsigned		uiEndSector ;

	unsigned		uiSectorsPerTrack ;
	unsigned		uiTracksPerHead ;
	unsigned		uiNoOfHeads ;
} Drive ;

typedef struct
{
	unsigned long	ulTotalSize ;
	unsigned long	ulUsedSize ;
} DriveSpace ;

typedef struct
{
	Drive		drive ;
	DriveSpace	driveSpace ;
} DriveStat ;

*/

extern int SysDrive_ChangeDrive(const char* szDriveName) ;
extern int SysDrive_ShowDrives(DriveStat** pDriveList, int* iListSize) ;
extern int SysDrive_Mount(const char* szDriveName) ;
extern int SysDrive_UnMount(const char* szDriveName) ;
extern int SysDrive_Format(const char* szDriveName) ;
extern int SysDrive_GetCurrentDrive(Drive* pDrive) ;

#define chdrive(drive) SysDrive_ChangeDrive(drive)
#define get_drive_list(drive_list, list_size) SysDrive_ShowDrives(drive_list, list_size)
#define mount(drive) SysDrive_Mount(drive) 
#define umount(drive) SysDrive_UnMount(drive) 
#define format(drive) SysDrive_Format(drive) 
#define getcurdrive(pdrive) SysDrive_GetCurrentDrive(pdrive)

#endif
