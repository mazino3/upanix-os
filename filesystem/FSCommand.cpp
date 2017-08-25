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
#include <FSCommand.h>
#include <MemUtil.h>
#include <SystemUtil.h>
#include <DMM.h>
#include <Display.h>

void FSCommand_Mounter(DiskDrive* pDiskDrive, MOUNT_TYPE mountType)
{
	if(pDiskDrive == NULL)
    throw upan::exception(XLOC, "drive can't be null");

	if(!KERNEL_MOUNT_DRIVE)
    throw upan::exception(XLOC, "KERNEL_MOUNT_DRIVE Parameter is OFF");

	if(mountType == FS_MOUNT)
  {
    pDiskDrive->Mount();
  }
	else
  {
    pDiskDrive->UnMount();
  }
}

void FSCommand_GetDriveSpace(DiskDrive* pDiskDrive, DriveStat* pDriveStat)
{
  const FSBootBlock& fsBootBlock = pDiskDrive->FSMountInfo.GetBootBlock();
  pDriveStat->ulTotalSize = fsBootBlock.BPB_FSTableSize * ENTRIES_PER_TABLE_SECTOR * 512 ;
  pDriveStat->ulUsedSize = fsBootBlock.uiUsedSectors * 512 ;
}
