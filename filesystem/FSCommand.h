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
#ifndef _FSCOMMAND_H_
#define _FSCOMMAND_H_

#include <FileSystem.h>
#include <DeviceDrive.h>
#include <Global.h>

#define FSCommand_SUCCESS 		0
#define FSCommand_FAILURE		1

typedef enum
{
	FS_MOUNT,
	FS_UNMOUNT
} MOUNT_TYPE ;

byte FSCommand_Mounter(DiskDrive* pDiskDrive, MOUNT_TYPE mountType) ;
byte FSCommand_Format(DiskDrive* pDiskDrive) ;
void FSCommand_GetDriveSpace(DiskDrive* pDiskDrive, DriveStat* pDriveStat) ;

#endif
