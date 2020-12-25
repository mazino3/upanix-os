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
# include <DMM.h>
# include <Display.h>
# include <MemUtil.h>
# include <StringUtil.h>
# include <Directory.h>
# include <ProcFileManager.h>
# include <FileOperations.h>
# include <MultiBoot.h>
# include <MountManager.h>
# include <try.h>
# include <drive.h>

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
		strcpy(szBootDriveName, "floppya") ;
		break ;
	
	case DEV_ATA_IDE:
		strcpy(szBootDriveName, "hdda") ;
		szBootDriveName[3] += bBootPartitionID ;	
		break ;	

	case DEV_SCSI_USB_DISK:
		strcpy(szBootDriveName, "usda") ;
		szBootDriveName[3] += bBootPartitionID ;
		break ;

	default:
		strcpy(szBootDriveName, "floppya") ;
		break ;
	}
}

static bool MountManager_GetHomeMountDrive(char* szHomeDriveName, unsigned uiSize)
{
  auto result = upan::tryreturn([&]() {
    const int fd = FileOperations_Open("ROOT@/.mount.lst", O_RDONLY);
    return FileOperations_Read(fd, szHomeDriveName, uiSize);
  });

  if(result.isBad())
    return false;

  int bytesRead = result.goodValue();
	
  szHomeDriveName[bytesRead - 1] = '\0' ; /* Junk Fix... Use ctype and trim functions
	from UPANIXApps library... port it to kernel using kernel coding conventions */

	return true ;
}

static void MountManager_MountDrive(char* szDriveName)
{
  printf("\n Mounting Drive: %s ...", szDriveName);

	// Find Drive
  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByDriveName(szDriveName, false).goodValueOrThrow(XLOC);

	// Mount Drive
  pDiskDrive->Mount();

	// Set Process Drive
	ProcessManager::Instance().GetCurrentPAS().iDriveID = pDiskDrive->Id();

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pDiskDrive->_fileSystem.FSpwd), 
	MemUtil_GetDS(), 
	(unsigned)&ProcessManager::Instance().GetCurrentPAS().processPWD, 
	sizeof(FileSystem::PresentWorkingDirectory)) ;

	// Change To Root Directory
  FileOperations_ChangeDir(FS_ROOT_DIR);
}
/****************************************************************************/

void MountManager_Initialize()
{
	MountManager_bInitStatus = false ;

	MountManager_GetBootMountDrive(MountManager_szRootDriveName) ;
	KC::MDisplay().Message("\n\tBoot Mount Drive: ", Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message(MountManager_szRootDriveName, ' ') ;
	
  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByDriveName(MountManager_szRootDriveName, false).goodValueOrElse(nullptr);
	
	if(pDiskDrive == NULL)
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
		if(strcmp(MountManager_szRootDriveName, szHomeDriveName) != 0)
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
    DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByDriveName(MountManager_szRootDriveName, false).goodValueOrElse(nullptr);
		
		if(pDiskDrive == NULL)
		{
			if(ProcessManager::GetCurrentProcessID() != NO_PROCESS_ID)
				return ProcessManager::Instance().GetCurrentPAS().iDriveID ;

			return CURRENT_DRIVE ;
		}

		MountManager_iRootDriveID = pDiskDrive->Id();
	}
	
	return MountManager_iRootDriveID ;
}

void MountManager_SetRootDrive(DiskDrive* pDiskDrive)
{
	strcpy(MountManager_szRootDriveName, pDiskDrive->DriveName().c_str());
	MountManager_iRootDriveID = pDiskDrive->Id();
}
