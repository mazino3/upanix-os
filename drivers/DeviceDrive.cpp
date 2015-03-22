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
# include <DeviceDrive.h>
# include <Floppy.h>
# include <ATADrive.h>
# include <ATADeviceController.h>
# include <PartitionManager.h>
# include <DMM.h>
# include <Display.h>
# include <String.h>
# include <MemUtil.h>
# include <ProcessEnv.h>
# include <FSCommand.h>
# include <ResourceManager.h>
# include <ProcessManager.h>
# include <SCSIHandler.h>
# include <stdio.h>
# include <MountManager.h>
# include <DiskCache.h>

DriveInfo* DeviceDrive_pCurrentDrive ;

static Mutex mDriveListMutex ;
static DriveInfo* DeviceDrive_pList ;

RawDiskDrive* DeviceDrive_pRawDiskList ;

int DeviceDrive_iID ;
unsigned DeviceDrive_uiCount ;

void DeviceDrive_Initialize()
{
	DeviceDrive_pCurrentDrive = NULL ;
	new (&mDriveListMutex)Mutex() ;
	DeviceDrive_pList = NULL ;
	DeviceDrive_iID = 0 ;
	DeviceDrive_uiCount = 0 ;

	DeviceDrive_pRawDiskList = NULL ;
}

RawDiskDrive* DeviceDrive_GetFirstRawDiskEntry()
{
	return DeviceDrive_pRawDiskList ;
}

RawDiskDrive* DeviceDrive_CreateRawDisk(const char* szName, RAW_DISK_TYPES iType, void* pDevice)
{
	RawDiskDrive* pDisk = (RawDiskDrive*)DMM_AllocateForKernel(sizeof(RawDiskDrive)) ;

	if(pDisk)
	{
		pDisk->pNext = NULL ;
		pDisk->iType = iType ;
		pDisk->pDevice = pDevice ;
		String_Copy(pDisk->szNameID, szName) ;
		new (&(pDisk->mDiskMutex))Mutex() ;

		switch(iType)
		{
			case ATA_HARD_DISK:
				pDisk->uiSectorSize = 512 ;
				pDisk->uiSizeInSectors = ATADeviceController_GetDeviceSectorLimit((ATAPort*)pDevice) ;
				break ;

			case USB_SCSI_DISK:
				pDisk->uiSectorSize = ((SCSIDevice*)pDevice)->uiSectorSize ;
				pDisk->uiSizeInSectors = ((SCSIDevice*)pDevice)->uiSectors ;
				break ;

			case FLOPPY_DISK:
				pDisk->uiSectorSize = 512 ;
				pDisk->uiSizeInSectors = 2880 ;
				break;

			default:
				printf("\n Unsupported Raw Disk Type: %d", iType) ;
				return NULL ;
		}
	}

	return pDisk ;
}

byte DeviceDrive_AddRawDiskEntry(RawDiskDrive* pRawDisk)
{
	RawDiskDrive* pCur = DeviceDrive_pRawDiskList ;

	for(; pCur != NULL; pCur = pCur->pNext)
	{
		if(String_Compare(pCur->szNameID, pRawDisk->szNameID) == 0)
		{
			printf("\n Raw Drive '%s' already in the list", pRawDisk->szNameID) ;
			return DeviceDrive_ERR_DUPLICATE ;
		}
	}

	pCur = DeviceDrive_pRawDiskList ;
	DeviceDrive_pRawDiskList = pRawDisk ;
	DeviceDrive_pRawDiskList->pNext = pCur ;

	return DeviceDrive_SUCCESS ;
}

byte DeviceDrive_RemoveRawDiskEntry(const char* szName)
{
	RawDiskDrive* pCur = DeviceDrive_pRawDiskList ;
	RawDiskDrive* pPrev = NULL ;

	for(; pCur != NULL; pCur = pCur->pNext)
	{
		if(String_Compare(pCur->szNameID, szName) == 0)
		{
			if(pPrev == NULL)
				DeviceDrive_pRawDiskList = pCur->pNext ;
			else
				pPrev->pNext = pCur->pNext ;

			DMM_DeAllocateForKernel((unsigned)pCur) ;
			return DeviceDrive_SUCCESS ;
		}

		pPrev = pCur ;
	}

	return DeviceDrive_ERR_NOTFOUND ;
}

RawDiskDrive* DeviceDrive_GetRawDiskByName(const char* szName)
{
	RawDiskDrive* pCur = DeviceDrive_pRawDiskList ;

	for(; pCur != NULL; pCur = pCur->pNext)
	{
		if(String_Compare(pCur->szNameID, szName) == 0)
			return pCur ;
	}

	return NULL ;
}

void DeviceDrive_AddEntry(DriveInfo* pDriveInfo)
{
	mDriveListMutex.Lock() ;

	pDriveInfo->pNext = DeviceDrive_pList ;
	DeviceDrive_pList = pDriveInfo ;
	DeviceDrive_uiCount++ ;
	DiskCache_StartReleaseCacheTask(pDriveInfo) ;

	mDriveListMutex.UnLock() ;
}

void DeviceDrive_RemoveEntryByCondition(const DriveRemoveClause& removeClause)
{
	mDriveListMutex.Lock() ;

	DriveInfo* pCur = DeviceDrive_pList ;
	DriveInfo* pPrev = NULL ;

	while(pCur != NULL)
	{
		if(removeClause(pCur))
		{
			if(pPrev != NULL)
				pPrev->pNext = pCur->pNext ;
			else
				DeviceDrive_pList = pCur->pNext ;

			DriveInfo* pTemp = pCur->pNext ;

			if(pCur->drive.bMounted)
				FSCommand_Mounter(pCur, FS_UNMOUNT) ;

			DMM_DeAllocateForKernel((unsigned)pCur) ;
			DeviceDrive_uiCount-- ;

			pCur = pTemp ;
		}
		else
		{
			pPrev = pCur ;
			pCur = pCur->pNext ;
		}
	}

	mDriveListMutex.UnLock() ;
}

DriveInfo* DeviceDrive_CreateDriveInfo(bool bEnableDiskCache)
{
	DriveInfo* pDriveInfo = (DriveInfo*)DMM_AllocateForKernel(sizeof(DriveInfo)) ;

	if(!pDriveInfo)
		return pDriveInfo ;

	pDriveInfo->pNext = NULL ;
	pDriveInfo->drive.iID = DeviceDrive_iID++ ;
	pDriveInfo->drive.bMounted = false ;
	pDriveInfo->pRawDisk = NULL ;
	pDriveInfo->bEnableDiskCache = false ;

	if(bEnableDiskCache)
		DiskCache_Setup(pDriveInfo) ;

	new (&(pDriveInfo->mDriveMutex))Mutex() ;

	//TODO:Temp Sol
	DeviceDrive_pCurrentDrive = pDriveInfo ;

	return pDriveInfo ;
}

byte DeviceDrive_Read(DriveInfo* pDriveInfo, unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
	byte bStatus = DeviceDrive_SUCCESS ;

	pDriveInfo->mDriveMutex.Lock() ;

	bStatus = DiskCache_Read(const_cast<DriveInfo*>(pDriveInfo), uiStartSector, uiNoOfSectors, bDataBuffer) ;

	pDriveInfo->mDriveMutex.UnLock() ;

	return bStatus ;
}

byte DeviceDrive_Write(DriveInfo* pDriveInfo, unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
	byte bStatus = DeviceDrive_SUCCESS ;
	
	pDriveInfo->mDriveMutex.Lock() ;

	bStatus = DiskCache_Write(const_cast<DriveInfo*>(pDriveInfo), uiStartSector, uiNoOfSectors, bDataBuffer) ;

	pDriveInfo->mDriveMutex.UnLock() ;

	return bStatus ;
}

DriveInfo* DeviceDrive_GetByDriveName(const char* szDriveName, bool bCheckMount)
{	
	mDriveListMutex.Lock() ;

	DriveInfo* pCur = DeviceDrive_pList ;

	while(pCur != NULL)
	{
		if(String_Compare(pCur->drive.driveName, szDriveName) == 0)
		{
			if(bCheckMount)
				pCur = (pCur->drive.bMounted) ? pCur : NULL ;

			break ;
		}

		pCur = pCur->pNext ;
	}

	mDriveListMutex.UnLock() ;

	return pCur ;
}

DriveInfo* DeviceDrive_GetByID(int iID, bool bCheckMount)
{	
	if(iID == ROOT_DRIVE)
	{
		iID = ROOT_DRIVE_ID ;
	}
	else if(iID == CURRENT_DRIVE)
	{
		iID = ProcessManager_processAddressSpace [ProcessManager_iCurrentProcessID].iDriveID ;
	}

	mDriveListMutex.Lock() ;

	DriveInfo* pCur = DeviceDrive_pList ;

	while(pCur != NULL)
	{
		if(pCur->drive.iID == iID)
		{
			if(bCheckMount)
				pCur = (pCur->drive.bMounted) ? pCur : NULL ;

			break ;
		}

		pCur = pCur->pNext ;
	}

	mDriveListMutex.UnLock() ;

	return pCur ;
}

void DeviceDrive_DisplayList()
{	
	DriveInfo* pCur = DeviceDrive_pList ;

	while(pCur != NULL)
	{
		KC::MDisplay().Message("\n", Display::WHITE_ON_BLACK()) ;
		KC::MDisplay().Message(pCur->drive.driveName, Display::WHITE_ON_BLACK()) ;
		KC::MDisplay().Number(" (", pCur->drive.uiSizeInSectors) ;
		DDWORD i = (DDWORD)pCur->drive.uiSizeInSectors * (DDWORD)512 ;
		printf(" - %llu", i) ;
		KC::MDisplay().DDNumberInHex(" - ", i) ;
		KC::MDisplay().Message(")", Display::WHITE_ON_BLACK()) ;

		pCur = pCur->pNext ;
	}
}

DriveInfo* DeviceDrive_GetNextDrive(const DriveInfo* pDriveInfo)
{
	if(pDriveInfo == NULL)
		return DeviceDrive_pList ;
	return pDriveInfo->pNext ;
}

byte DeviceDrive_Change(const char* szDriveName)
{
	DriveInfo* pDriveInfo = DeviceDrive_GetByDriveName(szDriveName, false) ;
	if(pDriveInfo == NULL)
		return DeviceDrive_ERR_INVALID_DRIVE_NAME ;

	ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].iDriveID = pDriveInfo->drive.iID ;

	MemUtil_CopyMemory(MemUtil_GetDS(), 
	(unsigned)&(pDriveInfo->FSMountInfo.FSpwd), 
	MemUtil_GetDS(), 
	(unsigned)&ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].processPWD, 
	sizeof(FileSystem_PresentWorkingDirectory)) ;

	ProcessEnv_Set("PWD", (const char*)pDriveInfo->FSMountInfo.FSpwd.DirEntry.Name) ;

	return DeviceDrive_SUCCESS ;
}

byte DeviceDrive_GetList(DriveStat** pDriveList, int* iListSize)
{	
	mDriveListMutex.Lock() ;

	*pDriveList = NULL ;
	*iListSize = DeviceDrive_uiCount ;
	
	if(DeviceDrive_uiCount == 0)
		return DeviceDrive_SUCCESS ;

	ProcessAddressSpace* pAddrSpace = &ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID] ;
	DriveStat* pAddress = NULL ;
	
	if(pAddrSpace->bIsKernelProcess == true)
	{
		*pDriveList = (DriveStat*)DMM_AllocateForKernel(sizeof(DriveStat) * DeviceDrive_uiCount) ;
		pAddress = *pDriveList ;
	}
	else
	{
		*pDriveList = (DriveStat*)DMM_Allocate(pAddrSpace, sizeof(DriveStat) * DeviceDrive_uiCount) ;
		pAddress = (DriveStat*)(((unsigned)*pDriveList + PROCESS_BASE) - GLOBAL_DATA_SEGMENT_BASE) ;
	}

	if(pAddress == NULL)
	{
		mDriveListMutex.UnLock() ;
		return DeviceDrive_FAILURE ;
	}

	DriveInfo* pCur = DeviceDrive_pList ;

	int i = 0 ;
	while(pCur != NULL)
	{
		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&pCur->drive, MemUtil_GetDS(), 
							(unsigned)&(pAddress[i].drive), sizeof(Drive)) ;

		pAddress[i].driveSpace.ulTotalSize = 0 ;
		pAddress[i].driveSpace.ulUsedSize = 0 ;

		if(pCur->drive.bMounted)
			FSCommand_GetDriveSpace(pCur, &(pAddress[i].driveSpace)) ;
		
		pCur = pCur->pNext ;
		i++ ;
	}

	mDriveListMutex.UnLock() ;
	return DeviceDrive_SUCCESS ;
}

byte DeviceDrive_MountDrive(const char* szDriveName)
{
	DriveInfo* pDriveInfo = DeviceDrive_GetByDriveName(szDriveName, false) ;
	if(pDriveInfo == NULL)
		return DeviceDrive_ERR_INVALID_DRIVE_NAME ;

	RETURN_X_IF_NOT(FSCommand_Mounter(pDriveInfo, FS_MOUNT), FSCommand_SUCCESS, DeviceDrive_ERR_MOUNT) ;
	
	return DeviceDrive_SUCCESS ;
}

byte DeviceDrive_UnMountDrive(const char* szDriveName)
{
	DriveInfo* pDriveInfo = DeviceDrive_GetByDriveName(szDriveName, false) ;

	if(pDriveInfo == NULL)
		return DeviceDrive_ERR_INVALID_DRIVE_NAME ;

	bool bKernel = IS_KERNEL() ? true : IS_KERNEL_PROCESS(ProcessManager_iCurrentProcessID) ;
	if(!bKernel)
	{
		if(pDriveInfo->drive.iID == ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].iDriveID)
			return DeviceDrive_ERR_CURR_DRIVE_UMOUNT ;
	}

	RETURN_X_IF_NOT(FSCommand_Mounter(pDriveInfo, FS_UNMOUNT), FSCommand_SUCCESS, DeviceDrive_ERR_UNMOUNT) ;
	
	return DeviceDrive_SUCCESS ;
}

byte DeviceDrive_FormatDrive(const char* szDriveName)
{
	DriveInfo* pDriveInfo = DeviceDrive_GetByDriveName(szDriveName, false) ;
	if(pDriveInfo == NULL)
		return DeviceDrive_ERR_INVALID_DRIVE_NAME ;

	if(FSCommand_Format(pDriveInfo) != FSCommand_SUCCESS)
		return DeviceDrive_ERR_FORMAT ;

	switch((int)pDriveInfo->drive.deviceType)
	{
		case DEV_ATA_IDE:
		case DEV_SCSI_USB_DISK:
			if(PartitionManager_UpdateSystemIndicator(pDriveInfo->pRawDisk, pDriveInfo->drive.uiLBAStartSector, 0x93) != PartitionManager_SUCCESS)
				return DeviceDrive_ERR_PARTITION_UPDATE ;
			break ;
	}

	return DeviceDrive_SUCCESS ;
}

byte DeviceDrive_GetCurrentDrive(Drive* pDrive)
{
	DriveInfo* pDriveInfo = DeviceDrive_GetByID(ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].iDriveID, true) ;

	if(pDriveInfo == NULL)
		return DeviceDrive_FAILURE ;
	
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pDriveInfo->drive), MemUtil_GetDS(), (unsigned)pDrive, sizeof(Drive)) ;

	return DeviceDrive_SUCCESS ;
}

byte DeviceDrive_RawDiskRead(RawDiskDrive* pDisk, unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer)
{
	byte bStatus = DeviceDrive_SUCCESS ;

//	unsigned uiResourceType ;
//	RETURN_X_IF_NOT(ResourceManager_GetDiskResourceType(pDisk->iType, &uiResourceType), ResourceManager_SUCCESS, DeviceDrive_FAILURE) ;
//	ResourceManager_Acquire(uiResourceType, RESOURCE_ACQUIRE_BLOCK) ;

	pDisk->mDiskMutex.Lock() ;

	switch(pDisk->iType)
	{
		case ATA_HARD_DISK:
			bStatus = ATADrive_Read((ATAPort*)pDisk->pDevice, uiStartSector, pDataBuffer, uiNoOfSectors) ;
			break ;

		case USB_SCSI_DISK:
			bStatus = SCSIHandler_GenericRead((SCSIDevice*)pDisk->pDevice, uiStartSector, uiNoOfSectors, pDataBuffer) ;
			break ;

		default:
			bStatus = DeviceDrive_ERR_UNKNOWN_DEVICE_TYPE ;
	}

//	ResourceManager_Release(uiResourceType) ;

	pDisk->mDiskMutex.UnLock() ;

	return bStatus ;
}

byte DeviceDrive_RawDiskWrite(RawDiskDrive* pDisk, unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer)
{
	byte bStatus = DeviceDrive_SUCCESS ;

//	unsigned uiResourceType ;
//	RETURN_X_IF_NOT(ResourceManager_GetDiskResourceType(pDisk->iType, &uiResourceType), ResourceManager_SUCCESS, DeviceDrive_FAILURE) ;
//	ResourceManager_Acquire(uiResourceType, RESOURCE_ACQUIRE_BLOCK) ;

	pDisk->mDiskMutex.Lock() ;

	switch(pDisk->iType)
	{
		case ATA_HARD_DISK:
			bStatus = ATADrive_Write((ATAPort*)pDisk->pDevice, uiStartSector, pDataBuffer, uiNoOfSectors) ;
			break ;

		case USB_SCSI_DISK:
			bStatus = SCSIHandler_GenericWrite((SCSIDevice*)pDisk->pDevice, uiStartSector, uiNoOfSectors, pDataBuffer) ;
			break ;

		default:
			bStatus = DeviceDrive_ERR_UNKNOWN_DEVICE_TYPE ;
	}

//	ResourceManager_Release(uiResourceType) ;

	pDisk->mDiskMutex.UnLock() ;

	return bStatus ;
}

