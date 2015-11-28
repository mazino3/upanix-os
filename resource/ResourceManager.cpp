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
# include <ResourceManager.h>
# include <DMM.h>
# include <ProcessManager.h>

byte ResourceManager_Acquire(unsigned uiType, unsigned uiMode)
{
	ProcessManager_DisableTaskSwitch() ;

	byte bStatus = ResourceManager_SUCCESS ;

	if(!ProcessManager_IsResourceBusy(uiType))
		ProcessManager_SetResourceBusy(uiType, true) ;
	else
	{
		if(uiMode == RESOURCE_ACQUIRE_NON_BLOCK)
			bStatus = ResourceManager_ERR_BUSY ;
		else
			ProcessManager_WaitOnResource(uiType) ;
	}

	ProcessManager_EnableTaskSwitch() ;
	return bStatus ;
}

void ResourceManager_Release(unsigned uiType)
{
	ProcessManager_DisableTaskSwitch() ;

	ProcessManager_SetResourceBusy(uiType, false) ;

	ProcessManager_EnableTaskSwitch() ;
}

byte ResourceManager_GetDiskResourceType(RAW_DISK_TYPES iType, unsigned* uiResourceType)
{
	switch(iType)
	{
		case ATA_HARD_DISK:
			*uiResourceType = RESOURCE_HDD ;
			break ;

		case USB_SCSI_DISK:
			*uiResourceType = RESOURCE_USD ;
			break ;

		//TODO: FLOPPY falls under this category which should be changed to particular disk type
		default:
			*uiResourceType = RESOURCE_GENERIC_DISK ;
			break ;
	}

	return ResourceManager_SUCCESS ;
	
	/*
	ProcessAddressSpace* pAddrSpace = &ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID] ;

	DriveInfo* pDriveInfo = DeviceDrive_GetByID(pAddrSpace->iDriveID, false) ;
	if(pDriveInfo == NULL)
		return ResourceManager_FAILURE ;

	switch(pDriveInfo->drive.deviceType)
	{
		case DEV_FLOPPY:
			*uiResourceType = RESOURCE_FDD ;
			break ;
			
		case DEV_ATA_IDE:
			*uiResourceType = RESOURCE_HDD ;
			break ;

		case DEV_SCSI_USB_DISK:
			*uiResourceType = RESOURCE_USD ;
			break ;

		default:
			return ResourceManager_ERR_UNKNOWN_RESOURCE ;
	}

	return ResourceManager_SUCCESS ;
	*/
}

byte ResourceManager_GetDiskResourceTypeByDeviceType(DEVICE_TYPE deviceType, unsigned* uiResourceType)
{
	switch(deviceType)
	{
		case DEV_FLOPPY:
			*uiResourceType = RESOURCE_FDD ;
			break ;
			
		case DEV_ATA_IDE:
			*uiResourceType = RESOURCE_HDD ;
			break ;

		case DEV_SCSI_USB_DISK:
			*uiResourceType = RESOURCE_USD ;
			break ;

		default:
			*uiResourceType = RESOURCE_GENERIC_DISK ;
			break ;
	}

	return ResourceManager_SUCCESS ;
}
