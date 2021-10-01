/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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
# include <SysCallDrive.h>
# include <DeviceDrive.h>
# include <try.h>

byte SysCallDrive_IsPresent(unsigned uiSysCallID)
{
	return (uiSysCallID > SYS_CALL_DRIVE_START && uiSysCallID < SYS_CALL_DRIVE_END) ;
}

void SysCallDrive_Handle(
__volatile__ int* piRetVal,
__volatile__ unsigned uiSysCallID, 
__volatile__ bool bDoAddrTranslation,
__volatile__ unsigned uiP1, 
__volatile__ unsigned uiP2, 
__volatile__ unsigned uiP3, 
__volatile__ unsigned uiP4, 
__volatile__ unsigned uiP5, 
__volatile__ unsigned uiP6, 
__volatile__ unsigned uiP7, 
__volatile__ unsigned uiP8, 
__volatile__ unsigned uiP9)
{
	switch(uiSysCallID)
	{
		case SYS_CALL_CHANGE_DRIVE : //Change Drive
			//P1 => Drive Name
			{
				char* szDriveName = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;

				*piRetVal = 0 ;
				if(DiskDriveManager::Instance().Change(szDriveName) != DeviceDrive_SUCCESS)
					*piRetVal = -1 ;
			}
			break ;

		case SYS_CALL_SHOW_DRIVES :
			// P1 => Ret Drive Stat List
			// P2 => Ret Drive Stat List Size
			{
				DriveStat** pDriveList = KERNEL_ADDR(bDoAddrTranslation, DriveStat**, uiP1) ;
				int* iListSize = KERNEL_ADDR(bDoAddrTranslation, int*, uiP2) ;

				*piRetVal = 0 ;
				if(DiskDriveManager::Instance().GetList(pDriveList, iListSize) != DeviceDrive_SUCCESS)
					*piRetVal = -1 ;
			}
			break ;

		case SYS_CALL_MOUNT_DRIVE : //Mount Drive
			//P1 => Drive Name
			{
				char* szDriveName = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;
        *piRetVal = 0;
        try
        {
          DiskDriveManager::Instance().MountDrive(szDriveName);
        }
        catch(...)
        {
          *piRetVal = -1;
        }
			}
			break ;

		case SYS_CALL_UNMOUNT_DRIVE : //UnMount Drive
			//P1 => Drive Name
			{
				char* szDriveName = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;
        *piRetVal = 0;
        try
        {
          DiskDriveManager::Instance().UnMountDrive(szDriveName);
        }
        catch(...)
        {
          *piRetVal = -1;
        }
			}
			break ;

		case SYS_CALL_FORMAT_DRIVE : //Format Drive
			//P1 => Drive Name
			{
				char* szDriveName = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;
        *piRetVal = 0;
        try
        {
          DiskDriveManager::Instance().FormatDrive(szDriveName);
        }
        catch(...)
        {
          *piRetVal = -1;
        }
			}
			break ;

		case SYS_CALL_CURRENT_DRIVE_STAT : //Current Drive
			//P1 => Ret Drive
			{
        *piRetVal = 0;
        try
        {
          DriveStat* pDriveStat = KERNEL_ADDR(bDoAddrTranslation, DriveStat*, uiP1) ;
          DiskDriveManager::Instance().GetCurrentDriveStat(pDriveStat);
        }
        catch(...)
        {
          *piRetVal = -1;
        }
			}
			break ;
	}
}
