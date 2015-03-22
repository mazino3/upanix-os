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
#include <FSCommand.h>
#include <MemUtil.h>
#include <SystemUtil.h>
#include <DMM.h>
#include <Display.h>

byte FSCommand_Mounter(DriveInfo* pDriveInfo, MOUNT_TYPE mountType)
{
	byte bStatus ;

	if(pDriveInfo == NULL)
		return FSCommand_FAILURE ;

	if(!KERNEL_MOUNT_DRIVE)
	{
		KC::MDisplay().Message("\n KERNEL_MOUNT_DRIVE Parameter is OFF\n", ' ') ;
		return FSCommand_FAILURE ;
	}

	if(mountType == FS_MOUNT)
	{
		RETURN_IF_NOT(bStatus, FileSystem_Mount(pDriveInfo), FileSystem_SUCCESS) ;
		RETURN_IF_NOT(bStatus, FileSystem_VerifyBootBlock(pDriveInfo), FileSystem_SUCCESS) ;
	}
	else
	{
		RETURN_IF_NOT(bStatus, FileSystem_UnMount(pDriveInfo), FileSystem_SUCCESS) ;
	}

	return FSCommand_SUCCESS ;
}

byte FSCommand_Format(DriveInfo* pDriveInfo)
{	
	byte bStatus ;

	if(pDriveInfo == NULL)
		return FSCommand_FAILURE ;

	RETURN_IF_NOT(bStatus, FileSystem_Format(pDriveInfo), FileSystem_SUCCESS) ;

	DiskCache_FlushDirtyCacheSectors(pDriveInfo) ;

	return FSCommand_SUCCESS ;
}

void FSCommand_GetDriveSpace(DriveInfo* pDriveInfo, DriveSpace* pDriveSpace)
{
	FileSystem_BootBlock* pFSBootBlock = &pDriveInfo->FSMountInfo.FSBootBlock ;
	pDriveSpace->ulTotalSize = pFSBootBlock->BPB_FSTableSize * ENTRIES_PER_TABLE_SECTOR * 512 ;
	pDriveSpace->ulUsedSize = pFSBootBlock->uiUsedSectors * 512 ;
}
