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

typedef struct 
{
	char			    driveName[33];
	byte			    bMounted;
	unsigned		  uiSizeInSectors ;
	unsigned long	ulTotalSize ;
	unsigned long	ulUsedSize ;
} DriveStat;

extern int SysDrive_ChangeDrive(const char* szDriveName) ;
extern int SysDrive_ShowDrives(DriveStat** pDriveList, int* iListSize) ;
extern int SysDrive_Mount(const char* szDriveName) ;
extern int SysDrive_UnMount(const char* szDriveName) ;
extern int SysDrive_Format(const char* szDriveName) ;
extern int SysDrive_GetCurrentDriveStat(DriveStat* pDriveStat) ;

#define chdrive(drive) SysDrive_ChangeDrive(drive)
#define get_drive_list(drive_list, list_size) SysDrive_ShowDrives(drive_list, list_size)
#define mount(drive) SysDrive_Mount(drive) 
#define umount(drive) SysDrive_UnMount(drive) 
#define format(drive) SysDrive_Format(drive) 
#define getcurdrive(pdrivestat) SysDrive_GetCurrentDriveStat(pdrivestat)

#endif
