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
# include <DeviceDrive.h>
# include <Floppy.h>
# include <ATADrive.h>
# include <ATADeviceController.h>
# include <PartitionManager.h>
# include <DMM.h>
# include <Display.h>
# include <StringUtil.h>
# include <MemUtil.h>
# include <ProcessEnv.h>
# include <FSCommand.h>
# include <ResourceManager.h>
# include <ProcessManager.h>
# include <SCSIHandler.h>
# include <stdio.h>
# include <MountManager.h>
# include <DiskCache.h>

static unsigned uiTotalFloppyDiskReads = 0;
static unsigned uiTotalATADiskReads = 0;
static unsigned uiTotalUSBDiskReads = 0;

void DiskCache_ShowTotalDiskReads()
{
  printf("\n Total Floppy Disk Reads: %u", uiTotalFloppyDiskReads) ;
  printf("\n Total ATA Disk Reads: %u", uiTotalATADiskReads) ;
  printf("\n Total USB Disk Reads: %u", uiTotalUSBDiskReads) ;
}

static void DiskCache_TaskFlushCache(DiskDrive* pDiskDrive, unsigned uiParam2)
{
	do
	{
	//	if(pDiskDrive->Mounted())
    pDiskDrive->FlushDirtyCacheSectors(10) ;
		ProcessManager::Instance().Sleep(200) ;
	} while(!pDiskDrive->StopReleaseCacheTask());

	ProcessManager_EXIT() ;
}

static void DiskCache_TaskReleaseCache(DiskDrive* pDiskDrive, unsigned uiParam2)
{
	do
	{
    pDiskDrive->ReleaseCache();
		ProcessManager::Instance().Sleep(50) ;
	} while(!pDiskDrive->StopReleaseCacheTask());

	ProcessManager_EXIT() ;
}

DiskDrive::DiskDrive(int id,
  const upan::string& driveName, 
  DEVICE_TYPE deviceType,
  DRIVE_NO driveNumber,
  unsigned uiLBAStartSector,
  unsigned uiSizeInSectors,
  unsigned uiStartCynlider,
  unsigned uiStartHead,
  unsigned uiStartSector,
  unsigned uiEndCynlider,
  unsigned uiEndHead,
  unsigned uiEndSector,
  unsigned uiSectorsPerTrack,
  unsigned uiTracksPerHead,
  unsigned uiNoOfHeads,
  void* device,
  RawDiskDrive* rawDisk,
  unsigned uiMaxSectorsInFreePoolCache,
  unsigned uiNoOfSectorsInTableCache,
  unsigned uiMountPointStart,
  unsigned uiMountPointEnd) : _id(id),
    _driveName(driveName),
    _deviceType(deviceType),
    _driveNumber(driveNumber),
    _uiLBAStartSector(uiLBAStartSector),
    _uiSizeInSectors(uiSizeInSectors),
    _uiStartCynlider(uiStartCynlider),
    _uiStartHead(uiStartHead),
    _uiStartSector(uiStartSector),
    _uiEndCynlider(uiEndCynlider),
    _uiEndHead(uiEndHead),
    _uiEndSector(uiEndSector),
    _uiSectorsPerTrack(uiSectorsPerTrack),
    _uiTracksPerHead(uiTracksPerHead),
    _uiNoOfHeads(uiNoOfHeads),
    _bEnableDiskCache(true),
    _device(device),
    _rawDisk(rawDisk),
    _uiMaxSectorsInFreePoolCache(uiMaxSectorsInFreePoolCache),
    _uiNoOfSectorsInTableCache(uiNoOfSectorsInTableCache),
    _uiMountPointStart(uiMountPointStart),
    _uiMountPointEnd(uiMountPointEnd),
    _fsType(FS_UNKNOWN),
    _mounted(false),
    _next(nullptr),
    _mCache(*this)
{
	StartReleaseCacheTask();
}

byte DiskDrive::Read(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
  MutexGuard g(_driveMutex);
	uiStartSector += LBAStartSector();

	if(!_bEnableDiskCache)
		return RawRead(uiStartSector, uiNoOfSectors, bDataBuffer) ;

	DiskCacheValue* pCacheValue[ uiNoOfSectors ] ;

	unsigned uiEndSector = uiStartSector + uiNoOfSectors ;
	unsigned uiSectorIndex ;
	unsigned uiFirstBreak, uiLastBreak ;
	uiFirstBreak = uiLastBreak = uiEndSector ;

	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		unsigned uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ] = static_cast<DiskCacheValue*>(_mCache.m_pTree->Find(DiskCacheKey(uiSectorIndex))) ;
		if(!pCacheValue[ uiIndex ])
		{
			if(uiFirstBreak == uiEndSector)
				uiFirstBreak = uiSectorIndex ;

			uiLastBreak = uiSectorIndex ;
		}
	}

	__volatile__ unsigned uiIndex ;
	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiFirstBreak; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Read(bDataBuffer + (uiIndex * 512)) ;
	}

	for(uiSectorIndex = uiLastBreak + 1; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Read(bDataBuffer + (uiIndex * 512)) ;
	}

	if(uiFirstBreak < uiEndSector)
	{
		uiIndex = uiFirstBreak - uiStartSector ;

		byte bStatus ;
		RETURN_IF_NOT(bStatus, RawRead(uiFirstBreak, uiLastBreak - uiFirstBreak + 1, (bDataBuffer + uiIndex * 512)), DiskCache_SUCCESS) ;

		for(uiSectorIndex = uiFirstBreak; uiSectorIndex <= uiLastBreak; uiSectorIndex++)
		{
			uiIndex = uiSectorIndex - uiStartSector ;

			if(!pCacheValue[ uiIndex ])
			{
				if(_mCache.m_pTree->GetTotalElements() >= _mCache.iMaxCacheSectors)
				{
					if(!_mCache.m_pLFUSectorManager->ReplaceCache(uiSectorIndex, bDataBuffer + (uiIndex * 512)))
					{
						printf("\n Cache Error: Disabling Disk Cache") ;
						_bEnableDiskCache = false;
						return DiskCache_SUCCESS ;
					}

					continue ;
				}

				DiskCacheKey* pKey = _mCache.CreateKey(uiSectorIndex) ;
				DiskCacheValue* pVal = _mCache.CreateValue(bDataBuffer + (uiIndex * 512)) ;

				if(!_mCache.m_pTree->Insert(pKey, pVal))
				{
					printf("\n Disk Cache failed Insert for SectorID: %d", uiSectorIndex) ;
					printf("\n Disabling Disk Cache!!! %s:%d", __FILE__, __LINE__) ;
          _bEnableDiskCache = false;
					return DiskCache_SUCCESS ;
				}
			}
			else
			{
				pCacheValue[ uiIndex ]->Read(bDataBuffer + (uiIndex * 512)) ;
			}
		}
	}
	
	return DiskCache_SUCCESS ;
}

byte DiskDrive::RawRead(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
	switch(DeviceType())
	{
    case DEV_FLOPPY:
      uiTotalFloppyDiskReads++ ;
      return Floppy_Read(this, uiStartSector, uiStartSector + uiNoOfSectors, bDataBuffer) ;

    case DEV_ATA_IDE:
      uiTotalATADiskReads++ ;
      return ATADrive_Read((ATAPort*)Device(), uiStartSector, bDataBuffer, uiNoOfSectors) ;

    case DEV_SCSI_USB_DISK:
      uiTotalUSBDiskReads++ ;
      return SCSIHandler_GenericRead((SCSIDevice*)Device(), uiStartSector, uiNoOfSectors, bDataBuffer) ;
	}
  return DeviceDrive_ERR_UNKNOWN_DEVICE_TYPE ;
}

byte DiskDrive::Write(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
	uiStartSector += LBAStartSector();
	
	if(!_bEnableDiskCache)
	{
		return RawWrite(uiStartSector, uiNoOfSectors, bDataBuffer) ;
	}

	DiskCacheValue* pCacheValue[ uiNoOfSectors ] ;

	unsigned uiEndSector = uiStartSector + uiNoOfSectors ;
	unsigned uiSectorIndex ;
	unsigned uiFirstBreak, uiLastBreak ;
	uiFirstBreak = uiLastBreak = uiEndSector ;

	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		unsigned uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ] = static_cast<DiskCacheValue*>(_mCache.m_pTree->Find(DiskCacheKey(uiSectorIndex))) ;

		if(!pCacheValue[ uiIndex ])
		{
			if(uiFirstBreak == uiEndSector)
				uiFirstBreak = uiSectorIndex ;

			uiLastBreak = uiSectorIndex ;
		}
	}

	__volatile__ unsigned uiIndex ;
	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiFirstBreak; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Write(bDataBuffer + (uiIndex * 512)) ;
		_mCache.InsertToDirtyList(DiskCache::SecKeyCacheValue(uiSectorIndex, pCacheValue[ uiIndex ]->GetSectorBuffer())) ;
	}

	for(uiSectorIndex = uiLastBreak + 1; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Write(bDataBuffer + (uiIndex * 512)) ;
		_mCache.InsertToDirtyList(DiskCache::SecKeyCacheValue(uiSectorIndex, pCacheValue[ uiIndex ]->GetSectorBuffer())) ;
	}

	if(uiFirstBreak < uiEndSector)
	{
		uiIndex = uiFirstBreak - uiStartSector ;

		//byte bStatus ;
		//RETURN_IF_NOT(bStatus, RawWrite(uiFirstBreak, uiLastBreak - uiFirstBreak + 1, (bDataBuffer + uiIndex * 512)), DiskCache_SUCCESS) ;

		for(uiSectorIndex = uiFirstBreak; uiSectorIndex <= uiLastBreak; uiSectorIndex++)
		{
			uiIndex = uiSectorIndex - uiStartSector ;

			if(!pCacheValue[ uiIndex ])
			{
				if(_mCache.m_pTree->GetTotalElements() >= _mCache.iMaxCacheSectors)
				{
					byte bStatus ;
					RETURN_IF_NOT(bStatus, RawWrite(uiSectorIndex, 1, (bDataBuffer + uiIndex * 512)), DiskCache_SUCCESS) ;

					if(!_mCache.m_pLFUSectorManager->ReplaceCache(uiSectorIndex, bDataBuffer + (uiIndex * 512)))
					{
						printf("\n Cache Error: Disabling Disk Cache") ;
						_bEnableDiskCache = false;
						return RawWrite(uiFirstBreak, uiLastBreak - uiFirstBreak + 1, (bDataBuffer + uiIndex * 512)) ;
					}
					continue ;
				}
			
				DiskCacheKey* pKey = _mCache.CreateKey(uiSectorIndex) ;
				DiskCacheValue* pVal = _mCache.CreateValue(bDataBuffer + (uiIndex * 512)) ;

				if(!_mCache.m_pTree->Insert(pKey, pVal))
				{
					printf("\n Disk Cache failed Insert. Disabling Disk Cache!!! %s:%d", __FILE__, __LINE__) ;
          _bEnableDiskCache = false;
					return DiskCache_SUCCESS ;
				}

				_mCache.InsertToDirtyList(DiskCache::SecKeyCacheValue(uiSectorIndex, pVal->GetSectorBuffer())) ;
			}
			else
			{
				pCacheValue[ uiIndex ]->Write(bDataBuffer + (uiIndex * 512)) ;
				_mCache.InsertToDirtyList(DiskCache::SecKeyCacheValue(uiSectorIndex, pCacheValue[ uiIndex ]->GetSectorBuffer())) ;
			}
		}
	}
	
	return DiskCache_SUCCESS ;
}

byte DiskDrive::RawWrite(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
	switch(DeviceType())
	{
	case DEV_FLOPPY:
		return Floppy_Write(this, uiStartSector, uiStartSector + uiNoOfSectors, bDataBuffer) ;

	case DEV_ATA_IDE:
		return ATADrive_Write((ATAPort*)Device(), uiStartSector, bDataBuffer, uiNoOfSectors) ;

	case DEV_SCSI_USB_DISK:
		return SCSIHandler_GenericWrite((SCSIDevice*)Device(), uiStartSector, uiNoOfSectors, bDataBuffer) ;
	}

  return DeviceDrive_ERR_UNKNOWN_DEVICE_TYPE ;
}

byte DiskDrive::FlushDirtyCacheSectors(int iCount)
{
	if(!_bEnableDiskCache)
		return DiskCache_SUCCESS ;

  MutexGuard g(_driveMutex);

  while(iCount != 0)
  {
    if(_mCache.m_pDirtyCacheList->empty())
      break;
	  const DiskCache::SecKeyCacheValue& v = _mCache.m_pDirtyCacheList->front();
	  _mCache.m_pDirtyCacheList->pop_front();
    if(!FlushSector(v.m_uiSectorID, v.m_pSectorBuffer))
    {
      printf("\n Flushing Sector %u to Drive %s failed !!", v.m_uiSectorID, DriveName().c_str()) ;
      return DiskCache_FAILURE ;
    }
    --iCount ;
	}
	return DiskCache_SUCCESS ;
}

bool DiskDrive::FlushSector(unsigned uiSectorID, const byte* pBuffer)
{
	if(!pBuffer)
		return false;
	if(RawWrite(uiSectorID, 1, (byte*)pBuffer) != DiskCache_SUCCESS)
		return false;
	return true;
}

void DiskDrive::StartReleaseCacheTask()
{
  StopReleaseCacheTask(false);

	int pid;
	char szDCFName[64] = "dcf-";
	String_CanCat(szDCFName, DriveName().c_str());
	ProcessManager::Instance().CreateKernelImage((unsigned)&DiskCache_TaskFlushCache, ProcessManager::Instance().GetCurProcId(), false, (unsigned)this, 0, &pid, szDCFName);

	char szDCRName[64] = "dcr-";
	String_CanCat(szDCRName, DriveName().c_str());
	ProcessManager::Instance().CreateKernelImage((unsigned)&DiskCache_TaskReleaseCache, ProcessManager::Instance().GetCurProcId(), false, (unsigned)this, 0, &pid, szDCRName);
}

void DiskDrive::ReleaseCache()
{
  if(Mounted())
    _mCache.m_pLFUSectorManager->Run();
}

DiskDriveManager::DiskDriveManager() : _idSequence(0)
{
}

void DiskDriveManager::Create(const upan::string& driveName, 
  DEVICE_TYPE deviceType, DRIVE_NO driveNumber,
  unsigned uiLBAStartSector, unsigned uiSizeInSectors,
  unsigned uiStartCynlider, unsigned uiStartHead, unsigned uiStartSector,
  unsigned uiEndCynlider, unsigned uiEndHead, unsigned uiEndSector,
  unsigned uiSectorsPerTrack, unsigned uiTracksPerHead, unsigned uiNoOfHeads,
  void* device, RawDiskDrive* rawDisk,
  unsigned uiMaxSectorsInFreePoolCache, unsigned uiNoOfSectorsInTableCache,
  unsigned uiMountPointStart, unsigned uiMountPointEnd)
{
  MutexGuard g(_driveListMutex);
  DiskDrive* pDiskDrive = new DiskDrive(_idSequence++, driveName,
    deviceType, driveNumber,
    uiLBAStartSector, uiSizeInSectors,
    uiStartCynlider, uiStartHead, uiStartSector,
    uiEndCynlider, uiEndHead, uiEndSector,
    uiSectorsPerTrack, uiTracksPerHead, uiNoOfHeads,
    device, rawDisk,
    uiMaxSectorsInFreePoolCache, uiNoOfSectorsInTableCache,
    uiMountPointStart, uiMountPointEnd);
  _driveList.push_back(pDiskDrive);
}

RawDiskDrive* DiskDriveManager::CreateRawDisk(const char* szName, RAW_DISK_TYPES iType, void* pDevice)
{
  for(auto d : _rawDiskList)
  {
		if(String_Compare(d->szNameID, szName) == 0)
      throw upan::exception(XLOC, "\n Raw Drive '%s' already in the list", szName);
	}

	RawDiskDrive* pDisk = (RawDiskDrive*)DMM_AllocateForKernel(sizeof(RawDiskDrive)) ;

	if(pDisk)
	{
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

  _rawDiskList.push_back(pDisk);
	return pDisk ;
}

byte DiskDriveManager::RemoveRawDiskEntry(const char* szName)
{
  for(auto it = _rawDiskList.begin(); it != _rawDiskList.end(); ++it)
  {
		if(String_Compare((*it)->szNameID, szName) == 0)
    {
      _rawDiskList.erase(it);
			return DeviceDrive_SUCCESS;
		}
  }
	return DeviceDrive_ERR_NOTFOUND;
}

RawDiskDrive* DiskDriveManager::GetRawDiskByName(const char* szName)
{
  for(auto d : _rawDiskList)
		if(String_Compare(d->szNameID, szName) == 0)
			return d;
	return nullptr;
}

void DiskDriveManager::RemoveEntryByCondition(const DriveRemoveClause& removeClause)
{
  MutexGuard g(_driveListMutex);

  for(auto it = _driveList.begin(); it != _driveList.end();)
  {
		if(removeClause(*it))
		{
			if((*it)->Mounted())
				FSCommand_Mounter(*it, FS_UNMOUNT) ;
      delete *it;
      _driveList.erase(it++);
		}
    else
      ++it;
	}
}

DiskDrive* DiskDriveManager::GetByDriveName(const upan::string& driveName, bool bCheckMount)
{
  MutexGuard g(_driveListMutex);
  for(auto d : _driveList)
    if(d->DriveName() == driveName)
      return d;
  return nullptr;
}

DiskDrive* DiskDriveManager::GetByID(int iID, bool bCheckMount)
{	
  MutexGuard g(_driveListMutex);
	if(iID == ROOT_DRIVE)
		iID = ROOT_DRIVE_ID ;
	else if(iID == CURRENT_DRIVE)
		iID = ProcessManager::Instance().GetCurrentPAS().iDriveID ;
  for(auto d : _driveList)
  {
    if(d->Id() == iID)
    {
      if(bCheckMount && d->Mounted())
        return d;
      return nullptr;
    }
  }
  return nullptr;
}

void DiskDriveManager::DisplayList()
{	
  for(auto d : _driveList)
  {
		DDWORD i = (DDWORD)d->SizeInSectors() * (DDWORD)512;
    printf("\n %s (%u - %llu - %x)", d->DriveName().c_str(), d->SizeInSectors(), i, i);
	}
}

byte DiskDriveManager::Change(const upan::string& szDriveName)
{
	DiskDrive* pDiskDrive = GetByDriveName(szDriveName, false) ;
	if(pDiskDrive == NULL)
		return DeviceDrive_ERR_INVALID_DRIVE_NAME ;

	ProcessManager::Instance().GetCurrentPAS().iDriveID = pDiskDrive->Id();

	MemUtil_CopyMemory(MemUtil_GetDS(), 
	(unsigned)&(pDiskDrive->FSMountInfo.FSpwd), 
	MemUtil_GetDS(), 
	(unsigned)&ProcessManager::Instance().GetCurrentPAS().processPWD, 
	sizeof(FileSystem_PresentWorkingDirectory)) ;

	ProcessEnv_Set("PWD", (const char*)pDiskDrive->FSMountInfo.FSpwd.DirEntry.Name) ;

	return DeviceDrive_SUCCESS ;
}

byte DiskDriveManager::GetList(DriveStat** pDriveList, int* iListSize)
{	
  MutexGuard g(_driveListMutex);

	*pDriveList = NULL ;
	*iListSize = _driveList.size();
	
	if(_driveList.empty())
		return DeviceDrive_SUCCESS ;

	ProcessAddressSpace* pAddrSpace = &ProcessManager::Instance().GetCurrentPAS() ;
	DriveStat* pAddress = NULL ;
	
	if(pAddrSpace->bIsKernelProcess == true)
	{
		*pDriveList = (DriveStat*)DMM_AllocateForKernel(sizeof(DriveStat) * _driveList.size());
		pAddress = *pDriveList ;
	}
	else
	{
		*pDriveList = (DriveStat*)DMM_Allocate(pAddrSpace, sizeof(DriveStat) * _driveList.size());
		pAddress = (DriveStat*)(((unsigned)*pDriveList + PROCESS_BASE) - GLOBAL_DATA_SEGMENT_BASE) ;
	}

	if(pAddress == NULL)
		return DeviceDrive_FAILURE ;

	int i = 0;
  for(auto d : _driveList)
	{
    strncpy(pAddress[i].driveName, d->DriveName().c_str(), 32);
    pAddress[i].bMounted = d->Mounted();
    pAddress[i].uiSizeInSectors = d->SizeInSectors();
		pAddress[i].ulTotalSize = 0;
		pAddress[i].ulUsedSize = 0;

		if(d->Mounted())
			FSCommand_GetDriveSpace(d, &(pAddress[i])) ;
		
		++i;
	}

	return DeviceDrive_SUCCESS ;
}

byte DiskDriveManager::MountDrive(const upan::string& szDriveName)
{
	DiskDrive* pDiskDrive = GetByDriveName(szDriveName, false) ;
	if(pDiskDrive == NULL)
		return DeviceDrive_ERR_INVALID_DRIVE_NAME ;

	RETURN_X_IF_NOT(FSCommand_Mounter(pDiskDrive, FS_MOUNT), FSCommand_SUCCESS, DeviceDrive_ERR_MOUNT) ;
	
	return DeviceDrive_SUCCESS ;
}

byte DiskDriveManager::UnMountDrive(const upan::string& szDriveName)
{
	DiskDrive* pDiskDrive = GetByDriveName(szDriveName, false) ;

	if(pDiskDrive == NULL)
		return DeviceDrive_ERR_INVALID_DRIVE_NAME ;

	bool bKernel = IS_KERNEL() ? true : IS_KERNEL_PROCESS(ProcessManager::GetCurrentProcessID()) ;
	if(!bKernel)
	{
		if(pDiskDrive->Id() == ProcessManager::Instance().GetCurrentPAS().iDriveID)
			return DeviceDrive_ERR_CURR_DRIVE_UMOUNT ;
	}

	RETURN_X_IF_NOT(FSCommand_Mounter(pDiskDrive, FS_UNMOUNT), FSCommand_SUCCESS, DeviceDrive_ERR_UNMOUNT) ;
	
	return DeviceDrive_SUCCESS ;
}

byte DiskDriveManager::FormatDrive(const upan::string& szDriveName)
{
	DiskDrive* pDiskDrive = GetByDriveName(szDriveName, false) ;
	if(pDiskDrive == NULL)
		return DeviceDrive_ERR_INVALID_DRIVE_NAME ;

	if(FSCommand_Format(pDiskDrive) != FSCommand_SUCCESS)
		return DeviceDrive_ERR_FORMAT ;

	switch((int)pDiskDrive->DeviceType())
	{
		case DEV_ATA_IDE:
		case DEV_SCSI_USB_DISK:
			if(PartitionManager_UpdateSystemIndicator(pDiskDrive->RawDisk(), pDiskDrive->LBAStartSector(), 0x93) != PartitionManager_SUCCESS)
				return DeviceDrive_ERR_PARTITION_UPDATE ;
			break ;
	}

	return DeviceDrive_SUCCESS ;
}

byte DiskDriveManager::GetCurrentDriveStat(DriveStat* pDriveStat)
{
	DiskDrive* pDiskDrive = GetByID(ProcessManager::Instance().GetCurrentPAS().iDriveID, true) ;

	if(pDiskDrive == NULL)
		return DeviceDrive_FAILURE ;

  strncpy(pDriveStat->driveName, pDiskDrive->DriveName().c_str(), 32);
  pDriveStat->bMounted = pDiskDrive->Mounted();
  pDriveStat->uiSizeInSectors = pDiskDrive->SizeInSectors();
  pDriveStat->ulTotalSize = 0;
  pDriveStat->ulUsedSize = 0;

	return DeviceDrive_SUCCESS ;
}

byte DiskDriveManager::RawDiskRead(RawDiskDrive* pDisk, unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer)
{
//	unsigned uiResourceType ;
//	RETURN_X_IF_NOT(ResourceManager_GetDiskResourceType(pDisk->iType, &uiResourceType), ResourceManager_SUCCESS, DeviceDrive_FAILURE) ;
//	ResourceManager_Acquire(uiResourceType, RESOURCE_ACQUIRE_BLOCK) ;

  MutexGuard g(pDisk->mDiskMutex);

	switch(pDisk->iType)
	{
		case ATA_HARD_DISK:
			return ATADrive_Read((ATAPort*)pDisk->pDevice, uiStartSector, pDataBuffer, uiNoOfSectors) ;

		case USB_SCSI_DISK:
			return SCSIHandler_GenericRead((SCSIDevice*)pDisk->pDevice, uiStartSector, uiNoOfSectors, pDataBuffer) ;
	}

//	ResourceManager_Release(uiResourceType) ;

  return DeviceDrive_ERR_UNKNOWN_DEVICE_TYPE ;
}

byte DiskDriveManager::RawDiskWrite(RawDiskDrive* pDisk, unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer)
{
//	unsigned uiResourceType ;
//	RETURN_X_IF_NOT(ResourceManager_GetDiskResourceType(pDisk->iType, &uiResourceType), ResourceManager_SUCCESS, DeviceDrive_FAILURE) ;
//	ResourceManager_Acquire(uiResourceType, RESOURCE_ACQUIRE_BLOCK) ;

	MutexGuard g(pDisk->mDiskMutex);

	switch(pDisk->iType)
	{
		case ATA_HARD_DISK:
			return ATADrive_Write((ATAPort*)pDisk->pDevice, uiStartSector, pDataBuffer, uiNoOfSectors) ;

		case USB_SCSI_DISK:
			return SCSIHandler_GenericWrite((SCSIDevice*)pDisk->pDevice, uiStartSector, uiNoOfSectors, pDataBuffer) ;
	}

//	ResourceManager_Release(uiResourceType) ;
  return DeviceDrive_ERR_UNKNOWN_DEVICE_TYPE ;
}

