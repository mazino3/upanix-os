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
# include <SysCall.h>
# include <drive.h>

int SysDrive_ChangeDrive(const char* szDriveName)
{
	__volatile__ int iRetStatus ;
	SysCallFile_Handle(&iRetStatus, SYS_CALL_CHANGE_DRIVE, false, (unsigned)szDriveName, 2, 3, 4, 5, 6, 7, 8, 9) ;
	return iRetStatus ;
}

int SysDrive_ShowDrives(DriveStat** pDriveList, int* iListSize)
{
	__volatile__ int iRetStatus ;
	SysCallFile_Handle(&iRetStatus, SYS_CALL_SHOW_DRIVES, false, (unsigned)pDriveList, (unsigned)iListSize, 3, 4, 5, 6, 7, 8, 9) ;
	return iRetStatus ;
}

int SysDrive_Mount(const char* szDriveName)
{
	__volatile__ int iRetStatus ;
	SysCallFile_Handle(&iRetStatus, SYS_CALL_MOUNT_DRIVE, false, (unsigned)szDriveName, 2, 3, 4, 5, 6, 7, 8, 9) ;
	return iRetStatus ;
}

int SysDrive_UnMount(const char* szDriveName)
{
	__volatile__ int iRetStatus ;
	SysCallFile_Handle(&iRetStatus, SYS_CALL_UNMOUNT_DRIVE, false, (unsigned)szDriveName, 2, 3, 4, 5, 6, 7, 8, 9) ;
	return iRetStatus ;
}

int SysDrive_Format(const char* szDriveName)
{
	__volatile__ int iRetStatus ;
	SysCallFile_Handle(&iRetStatus, SYS_CALL_FORMAT_DRIVE, false, (unsigned)szDriveName, 2, 3, 4, 5, 6, 7, 8, 9) ;
	return iRetStatus ;
}

int SysDrive_GetCurrentDrive(Drive* pDrive)
{
	__volatile__ int iRetStatus ;
	SysCallFile_Handle(&iRetStatus, SYS_CALL_CURRENT_DRIVE, false, (unsigned)pDrive, 2, 3, 4, 5, 6, 7, 8, 9) ;
	return iRetStatus ;
}
