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
# include <FSCommand.h>
# include <DMM.h>
# include <Display.h>
# include <MemUtil.h>
# include <StringUtil.h>
# include <Directory.h>
# include <ProcFileManager.h>
# include <FileOperations.h>
# include <MultiBoot.h>
# include <MountManager.h>

static char MountManager_szRootDriveName[33] = "" ;
static int MountManager_iRootDriveID = CURRENT_DRIVE ;
static bool MountManager_bInitStatus = false ;

/*************************** static *****************************************/
static void MountManager_GetBootMountDrive(char* szBootDriveName)
{
	byte bBootDevice = MultiBoot::Instance().GetBootDeviceID() ;
	byte bBootPartitionID = MultiBoot::Instance().GetBootPartitionID() ;

	switch(bBootDevice)
	{
	case DEV_FLOPPY:
		String_Copy(szBootDriveName, "floppya") ;
		break ;
	
	case DEV_ATA_IDE:
		String_Copy(szBootDriveName, "hdda") ;
		szBootDriveName[3] += bBootPartitionID ;	
		break ;	

	case DEV_SCSI_USB_DISK:
		String_Copy(szBootDriveName, "usda") ;
		szBootDriveName[3] += bBootPartitionID ;
		break ;

	default:
		String_Copy(szBootDriveName, "floppya") ;
		break ;
	}
}

static byte MountManager_GetHomeMountDrive(char* szHomeDriveName, unsigned uiSize)
{
	int fd ;
	if(FileOperations_Open(&fd, "ROOT@/.mount.lst", O_RDONLY) != FileOperations_SUCCESS)
		return false ;

	unsigned uiBytesRead ;

	if(FileOperations_Read(fd, szHomeDriveName, uiSize, &uiBytesRead) != FileOperations_SUCCESS)
		return false ;
	
	szHomeDriveName[uiBytesRead - 1] = '\0' ; /* Junk Fix... Use ctype and trim functions
	from MOSApps library... port it to kernel using kernel coding conventions */

	return true ;
}

static byte MountManager_MountDrive(char* szDriveName)
{
	KC::MDisplay().Message("\n Mounting Drive: ", Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message(szDriveName, Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message(" ...", Display::WHITE_ON_BLACK()) ;

	// Find Drive
	DriveInfo* pDriveInfo = DeviceDrive_GetByDriveName(szDriveName, false) ;
	if(pDriveInfo == NULL)
	{
		KC::MDisplay().Message("\n Invalid Drive: ", Display::WHITE_ON_BLACK()) ;
		return false ;
	}

	// Mount Drive
	if(FSCommand_Mounter(pDriveInfo, FS_MOUNT) != FSCommand_SUCCESS)
		KC::MDisplay().Message(" Failed !!!", Display::WHITE_ON_BLACK()) ;
	else
		KC::MDisplay().Message(" Done.", Display::WHITE_ON_BLACK()) ;

	// Set Process Drive
	ProcessManager::Instance().GetCurrentPAS().iDriveID = pDriveInfo->drive.iID ;

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pDriveInfo->FSMountInfo.FSpwd), 
	MemUtil_GetDS(), 
	(unsigned)&ProcessManager::Instance().GetCurrentPAS().processPWD, 
	sizeof(FileSystem_PresentWorkingDirectory)) ;

	// Change To Root Directory
	if(FileOperations_ChangeDir(FS_ROOT_DIR) != Directory_SUCCESS)
	{
		KC::MDisplay().Message("\n Directory Change Failed !!!", Display::WHITE_ON_BLACK()) ;
		return false ;
	}

	return true ;
}
/****************************************************************************/

void MountManager_Initialize()
{
	MountManager_bInitStatus = false ;

	MountManager_GetBootMountDrive(MountManager_szRootDriveName) ;
	KC::MDisplay().Message("\n\tBoot Mount Drive: ", Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message(MountManager_szRootDriveName, ' ') ;
	
	DriveInfo* pDriveInfo = DeviceDrive_GetByDriveName(MountManager_szRootDriveName, false) ;
	
	if(pDriveInfo == NULL)
	{
		MountManager_bInitStatus = false ;
		MountManager_iRootDriveID = CURRENT_DRIVE ;
	}

	KC::MDisplay().LoadMessage("Mount Manager Initialization", MountManager_bInitStatus ? Success : Failure);
}

bool MountManager_GetInitStatus()
{
	return MountManager_bInitStatus ;
}

void MountManager_MountDrives()
{
	if(MountManager_bInitStatus == false)
	{
		KC::MDisplay().Message("\n\tMount Manager Init Failed. Not mounting any drive", '@') ;
		return ;
	}
	
	MountManager_MountDrive(MountManager_szRootDriveName) ;
	char szHomeDriveName[33] ;
	if(MountManager_GetHomeMountDrive(szHomeDriveName, 32))
	{
		if(String_Compare(MountManager_szRootDriveName, szHomeDriveName) != 0)
		{
			MountManager_MountDrive(szHomeDriveName) ;
		}	
	}
}

const char* MountManager_GetRootDriveName()
{
	return MountManager_szRootDriveName ;
}

int MountManager_GetRootDriveID()
{
	if(MountManager_iRootDriveID == CURRENT_DRIVE)
	{
		DriveInfo* pDriveInfo = DeviceDrive_GetByDriveName(MountManager_szRootDriveName, false) ;
		
		if(pDriveInfo == NULL)
		{
			if(ProcessManager::GetCurrentProcessID() != NO_PROCESS_ID)
				return ProcessManager::Instance().GetCurrentPAS().iDriveID ;

			return CURRENT_DRIVE ;
		}

		MountManager_iRootDriveID = pDriveInfo->drive.iID ;
	}
	
	return MountManager_iRootDriveID ;
}

void MountManager_SetRootDrive(DriveInfo* pDriveInfo)
{
	String_Copy(MountManager_szRootDriveName, pDriveInfo->drive.driveName) ;
	MountManager_iRootDriveID = pDriveInfo->drive.iID ;
}
