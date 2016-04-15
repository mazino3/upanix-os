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
# include <ProcessManager.h>
# include <SCSIHandler.h>
# include <stdio.h>
# include <MountManager.h>
# include <DiskCache.h>
# include <KernelUtil.h>
# include <FSManager.h>

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
    _enableFreePoolCache(true),
    _enableTableCache(true)
{
	StartReleaseCacheTask();

	unsigned uiMountSpaceLimit = _uiMountPointEnd - _uiMountPointStart;
	unsigned uiSizeOfTableCahce = FileSystem_GetSizeForTableCache(NoOfSectorsInTableCache());
	if(uiSizeOfTableCahce > uiMountSpaceLimit)
	{
		KC::MDisplay().Message("\n Critical Error: FS Mount Cache size insufficient. ", ' ') ;
    _enableTableCache = false;
		KernelUtil::Wait(3000) ;
	}
	FSMountInfo.FSTableCache.iSize = 0;
  FSMountInfo.pFreePoolQueue = nullptr;
	FSMountInfo.FSTableCache.pSectorBlockEntryList = (SectorBlockEntry*)(_uiMountPointStart);
}

byte DiskDrive::Mount()
{
	byte bStatus;
	
	if(Mounted())
		return FileSystem_ERR_ALREADY_MOUNTED;

	FSMountInfo.FSTableCache.iSize = 0;
  FSMountInfo.pFreePoolQueue = new upan::queue<unsigned>(MaxSectorsInFreePoolCache());

	RETURN_IF_NOT(bStatus, ReadFSBootBlock(), DeviceDrive_SUCCESS);
	RETURN_IF_NOT(bStatus, LoadFreeSectors(), DeviceDrive_SUCCESS);
	RETURN_IF_NOT(bStatus, ReadRootDirectory(), DeviceDrive_SUCCESS);
	RETURN_IF_NOT(bStatus, VerifyBootBlock(), DeviceDrive_SUCCESS);

	Mounted(true);

	return DeviceDrive_SUCCESS;
}

byte DiskDrive::UnMount()
{
	byte bStatus ;

	if(!Mounted())
		return FileSystem_ERR_NOT_MOUNTED ;

	byte bSectorBuffer[512];

	bSectorBuffer[510] = 0x55; /* BootSector Signature */
	bSectorBuffer[511] = 0xAA;

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&FSMountInfo.FSBootBlock, MemUtil_GetDS(), (unsigned)bSectorBuffer, sizeof(FileSystem_BootBlock));

	RETURN_IF_NOT(bStatus, Write(1, 1, bSectorBuffer), DeviceDrive_SUCCESS) ;
	RETURN_IF_NOT(bStatus, FlushTableCache(NoOfSectorsInTableCache()), DeviceDrive_SUCCESS);
	FlushDirtyCacheSectors();
	Mounted(false);

	return FileSystem_SUCCESS ;
}


byte DiskDrive::VerifyBootBlock()
{
	FileSystem_BootBlock& fsBootBlock = FSMountInfo.FSBootBlock;

	if(fsBootBlock.BPB_jmpBoot[0] != 0xEB || fsBootBlock.BPB_jmpBoot[1] != 0xFE || fsBootBlock.BPB_jmpBoot[2] != 0x90)
		return FileSystem_ERR_BPB_JMP;

	if(fsBootBlock.BPB_BytesPerSec != 0x200)
		return FileSystem_ERR_UNSUPPORTED_SECTOR_SIZE;
	
	if(DeviceType() == DEV_FLOPPY)
		if(fsBootBlock.BPB_Media  != 0xF0)
			return FileSystem_ERR_UNSUPPORTED_MEDIA;
		
	if(fsBootBlock.BPB_ExtFlags  != 0x0080)
		return FileSystem_ERR_INVALID_EXTFLAG;
		
	if(fsBootBlock.BPB_FSVer != 0x0100)
		return FileSystem_ERR_FS_VERSION;
		
	if(fsBootBlock.BPB_FSInfo  != 1)
		return FileSystem_ERR_FSINFO_SECTOR;
		
	if(fsBootBlock.BPB_VolID != 0x01)
		return FileSystem_ERR_INVALID_VOL_ID;

	return DeviceDrive_SUCCESS;
}

byte DiskDrive::ReadFSBootBlock()
{
	byte bArrFSBootBlock[512];
	byte bStatus;
	
	RETURN_IF_NOT(bStatus, Read(1, 1, bArrFSBootBlock), DeviceDrive_SUCCESS);

	if(bArrFSBootBlock[510] != 0x55 || bArrFSBootBlock[511] != 0xAA)
		return FileSystem_ERR_INVALID_BPB_SIGNATURE;

	FileSystem_BootBlock& fsBootBlock = FSMountInfo.FSBootBlock;

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&bArrFSBootBlock, MemUtil_GetDS(), 
						(unsigned)&fsBootBlock, sizeof(FileSystem_BootBlock));
	
	if(fsBootBlock.BPB_BootSig != 0x29)
		return FileSystem_ERR_INVALID_BOOT_SIGNATURE;

	// TODO: A write to HD image file from mos fs util is changing the CHS value !!
	// Needs to be fixed. So, this check is skipped for the time being

	/*
	if(fsBootBlock.BPB_SecPerTrk != pDiskDrive->uiSectorsPerTrack)
		return FileSystem_ERR_INVALID_SECTORS_PER_TRACK;

	if(fsBootBlock.BPB_NumHeads != pDiskDrive->uiNoOfHeads)
		return FileSystem_ERR_INVALID_NO_OF_HEADS;
	*/
	
	if(fsBootBlock.BPB_TotSec32 != SizeInSectors())
		return FileSystem_ERR_INVALID_DRIVE_SIZE_IN_SECTORS;
	
	if(fsBootBlock.BPB_FSTableSize == 0)
		return FileSystem_ERR_ZERO_FATSZ32;

	if(fsBootBlock.BPB_BytesPerSec != 0x200)
		return FileSystem_ERR_UNSUPPORTED_SECTOR_SIZE;
		
	return DeviceDrive_SUCCESS;
}

byte DiskDrive::LoadFreeSectors()
{ 
	if(!IsFreePoolCacheEnabled())
		return DeviceDrive_SUCCESS;

  auto& freePoolQueue = *FSMountInfo.pFreePoolQueue;
  if(freePoolQueue.full())
    return DeviceDrive_SUCCESS;

	FileSystem_BootBlock& fsBootBlock = FSMountInfo.FSBootBlock;
	FileSystem_TableCache& fsTableCache = FSMountInfo.FSTableCache;
	SectorBlockEntry* pSectorBlockEntryList = fsTableCache.pSectorBlockEntryList;

	bool bStop = false;

	if(IsTableCacheEnabled())
	{
		// First do Cache Lookup
		for(int i = 0; i < fsTableCache.iSize; ++i)
		{
			if(bStop)
				break;
			unsigned* uiSectorBlock = pSectorBlockEntryList[i].uiSectorBlock;
			for(int j = 0; j < ENTRIES_PER_TABLE_SECTOR; j++)
			{
				if(!(uiSectorBlock[j] & EOC))
				{
					unsigned uiSectorID = pSectorBlockEntryList[i].uiBlockID * ENTRIES_PER_TABLE_SECTOR + j;
          if(!freePoolQueue.push_back(uiSectorID))
					{
						bStop = true;
						break;
					}
				}
			}
		}
	}
	
	if(bStop)
		return DeviceDrive_SUCCESS;

	byte bBuffer[ 4096 ];
	byte bStatus;

	for(unsigned i = 0; i < fsBootBlock.BPB_FSTableSize; )
	{
		if(bStop)
			break;
		if(IsTableCacheEnabled())
		{
      int iPos;
			if(FSManager_BinarySearch(pSectorBlockEntryList, fsTableCache.iSize, i, &iPos))
			{
				i++;
				continue;
			}
		}

		unsigned uiBlockSize = (fsBootBlock.BPB_FSTableSize - i);
		if(uiBlockSize > 8) 
      uiBlockSize = 8;

		RETURN_IF_NOT(bStatus, Read(i + fsBootBlock.BPB_RsvdSecCnt + 1, uiBlockSize, (byte*)bBuffer), DeviceDrive_SUCCESS);

		unsigned* pTable = (unsigned*)bBuffer;

		for(unsigned j = 0; j < ENTRIES_PER_TABLE_SECTOR * uiBlockSize; j++)
		{
			if(!(pTable[j] & EOC))
			{
				unsigned uiSectorID = i * ENTRIES_PER_TABLE_SECTOR + j;
				if(!freePoolQueue.push_back(uiSectorID))
				{
					bStop = true;
					break;
				}
			}
		}

		i += uiBlockSize;
	}

	return DeviceDrive_SUCCESS;
}

byte DiskDrive::ReadRootDirectory()
{
	byte bStatus;
	byte bDataBuffer[512];
	FileSystem_PresentWorkingDirectory& FSpwd = FSMountInfo.FSpwd;

	RETURN_IF_NOT(bStatus, xRead(bDataBuffer, 0, 1), DeviceDrive_SUCCESS);
	
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)bDataBuffer, MemUtil_GetDS(), (unsigned)&FSpwd.DirEntry, sizeof(FileSystem_DIR_Entry));
	FSpwd.uiSectorNo = 0;
	FSpwd.bSectorEntryPosition = 0;
		
	return DeviceDrive_SUCCESS;
}

void DiskDrive::Mounted(bool mounted)
{
  _mounted = mounted;
  if(!mounted)
  {
    if(FSMountInfo.pFreePoolQueue)
      delete FSMountInfo.pFreePoolQueue;
    FSMountInfo.pFreePoolQueue = nullptr;
    FSMountInfo.FSTableCache.iSize = 0;
  }
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
		unsigned uiIndex = uiSectorIndex - uiStartSector;
		pCacheValue[ uiIndex ] = _mCache.Find(uiSectorIndex);
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
				if(_mCache.Full())
				{
					if(!_mCache.ReplaceCache(uiSectorIndex, bDataBuffer + (uiIndex * 512)))
					{
						printf("\n Cache Error: Disabling Disk Cache") ;
						_bEnableDiskCache = false;
						return DiskCache_SUCCESS ;
					}

					continue ;
				}

				if(!_mCache.Add(uiSectorIndex, bDataBuffer + (uiIndex * 512)))
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

byte DiskDrive::xRead(byte* bDataBuffer, unsigned uiSector, unsigned uiNoOfSectors)
{
  return Read(GetRealSectorNumber(uiSector), uiNoOfSectors, bDataBuffer);
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

    default:
      return DeviceDrive_ERR_UNKNOWN_DEVICE_TYPE ;
	}
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
		pCacheValue[ uiIndex ] = _mCache.Find(uiSectorIndex);

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
				if(_mCache.Full())
				{
					byte bStatus ;
					RETURN_IF_NOT(bStatus, RawWrite(uiSectorIndex, 1, (bDataBuffer + uiIndex * 512)), DiskCache_SUCCESS) ;

					if(!_mCache.ReplaceCache(uiSectorIndex, bDataBuffer + (uiIndex * 512)))
					{
						printf("\n Cache Error: Disabling Disk Cache") ;
						_bEnableDiskCache = false;
						return RawWrite(uiFirstBreak, uiLastBreak - uiFirstBreak + 1, (bDataBuffer + uiIndex * 512)) ;
					}
					continue ;
				}
			
        DiskCacheValue* pVal = _mCache.Add(uiSectorIndex, bDataBuffer + (uiIndex * 512));
        if(!pVal)
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

byte DiskDrive::xWrite(byte* bDataBuffer, unsigned uiSector, unsigned uiNoOfSectors)
{
  return Write(GetRealSectorNumber(uiSector), uiNoOfSectors, bDataBuffer);
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

  default:
    return DeviceDrive_ERR_UNKNOWN_DEVICE_TYPE ;
	}
}

byte DiskDrive::FlushDirtyCacheSectors(int iCount)
{
	if(!_bEnableDiskCache)
		return DiskCache_SUCCESS ;

  MutexGuard g(_driveMutex);

  while(iCount != 0)
  {
	  DiskCache::SecKeyCacheValue v;
    if(!_mCache.Get(v))
      break;
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

byte DiskDrive::FlushTableCache(int iFlushSize)
{
	if(!IsTableCacheEnabled())
		return DeviceDrive_SUCCESS;

	FileSystem_TableCache& fsTableCache = FSMountInfo.FSTableCache;
	FileSystem_BootBlock& fsBootBlock = FSMountInfo.FSBootBlock;
	SectorBlockEntry* pSectorBlockEntryList = fsTableCache.pSectorBlockEntryList;
	int iSize = fsTableCache.iSize;

	if(iFlushSize > iSize)
		iFlushSize = iSize;

	byte bStatus;
	
	for(int i = 0; i < iFlushSize; i++)
	{
		SectorBlockEntry* pBlock = &(pSectorBlockEntryList[i]);

		if(pBlock->uiWriteCount == 0)
			continue ;

		RETURN_IF_NOT(bStatus, Write(pBlock->uiBlockID + fsBootBlock.BPB_RsvdSecCnt + 1, 1, 
						(byte*)(pBlock->uiSectorBlock)), DeviceDrive_SUCCESS) ;
	}

	if(iFlushSize < iSize)
	{
		unsigned uiSrc = (unsigned)&pSectorBlockEntryList[iFlushSize] ;
		unsigned uiDest = (unsigned)&pSectorBlockEntryList[0] ;
		MemUtil_CopyMemory(MemUtil_GetDS(), uiSrc, MemUtil_GetDS(), uiDest, (iSize - iFlushSize) * sizeof(SectorBlockEntry)) ;
	}

	fsTableCache.iSize -= iFlushSize ;

	return DeviceDrive_SUCCESS;
}


void DiskDrive::StartReleaseCacheTask()
{
  StopReleaseCacheTask(false);

	int pid;
	char szDCFName[64] = "dcf-";
	strcat(szDCFName, DriveName().c_str());
	ProcessManager::Instance().CreateKernelImage((unsigned)&DiskCache_TaskFlushCache, ProcessManager::Instance().GetCurProcId(), false, (unsigned)this, 0, &pid, szDCFName);

	char szDCRName[64] = "dcr-";
	strcat(szDCRName, DriveName().c_str());
	ProcessManager::Instance().CreateKernelImage((unsigned)&DiskCache_TaskReleaseCache, ProcessManager::Instance().GetCurProcId(), false, (unsigned)this, 0, &pid, szDCRName);
}

void DiskDrive::ReleaseCache()
{
  if(Mounted())
    _mCache.LFUCacheCleanUp();
}

unsigned DiskDrive::GetRealSectorNumber(unsigned uiSectorID) const
{
	return uiSectorID + 1/*BPB*/ 
					+ FSMountInfo.FSBootBlock.BPB_RsvdSecCnt 
					+ FSMountInfo.FSBootBlock.BPB_FSTableSize;
}

unsigned DiskDrive::GetFreeSector()
{
	byte bBuffer[512];
	FileSystem_BootBlock& fsBootBlock = FSMountInfo.FSBootBlock ;
	
	for(unsigned i = 0; i < fsBootBlock.BPB_FSTableSize; ++i)
	{
		if(Read(i + fsBootBlock.BPB_RsvdSecCnt + 1, 1, (byte*)bBuffer) != DeviceDrive_SUCCESS)
      throw upan::exception(XLOC, "error reading from disk drive %s - while allocating free sector", DriveName().c_str());
	  unsigned* pTable = (unsigned*)bBuffer;
		for(unsigned j = 0; j < ENTRIES_PER_TABLE_SECTOR; ++j)
		{
			if(!(pTable[j] & EOC))
        return i * ENTRIES_PER_TABLE_SECTOR + j;
		}
	}
  throw upan::exception(XLOC, "%s disk drive is full - no more free space available!", DriveName().c_str());
}

RawDiskDrive::RawDiskDrive(const upan::string& name, RAW_DISK_TYPES type, void* device)
  : _name(name), _type(type), _device(device)
{
  switch(_type)
  {
    case ATA_HARD_DISK:
      _sectorSize = 512;
      _sizeInSectors = ATADeviceController_GetDeviceSectorLimit((ATAPort*)_device);
      break ;

    case USB_SCSI_DISK:
      _sectorSize = ((SCSIDevice*)_device)->uiSectorSize;
      _sizeInSectors = ((SCSIDevice*)_device)->uiSectors;
      break ;

    case FLOPPY_DISK:
      _sectorSize = 512;
      _sizeInSectors = 2880;
      break;

    default:
      throw upan::exception(XLOC, "\n Unsupported Raw Disk Type: %d", type);
  }
}

DiskDriveManager::DiskDriveManager() : _idSequence(0)
{
}

void DiskDriveManager::Create(const upan::string& driveName, 
  DEVICE_TYPE deviceType, DRIVE_NO driveNumber,
  unsigned uiLBAStartSector, unsigned uiSizeInSectors,
  unsigned uiSectorsPerTrack, unsigned uiTracksPerHead, unsigned uiNoOfHeads,
  void* device, RawDiskDrive* rawDisk,
  unsigned uiMaxSectorsInFreePoolCache, unsigned uiNoOfSectorsInTableCache,
  unsigned uiMountPointStart, unsigned uiMountPointEnd)
{
  MutexGuard g(_driveListMutex);
  DiskDrive* pDiskDrive = new DiskDrive(_idSequence++, driveName,
    deviceType, driveNumber,
    uiLBAStartSector, uiSizeInSectors,
    uiSectorsPerTrack, uiTracksPerHead, uiNoOfHeads,
    device, rawDisk,
    uiMaxSectorsInFreePoolCache, uiNoOfSectorsInTableCache,
    uiMountPointStart, uiMountPointEnd);
  _driveList.push_back(pDiskDrive);
}

RawDiskDrive* DiskDriveManager::CreateRawDisk(const upan::string& name, RAW_DISK_TYPES iType, void* pDevice)
{
  for(auto d : _rawDiskList)
  {
		if(d->Name() == name)
      throw upan::exception(XLOC, "\n Raw Drive '%s' already in the list", name.c_str());
	}
  RawDiskDrive* pDisk = new RawDiskDrive(name, iType, pDevice);
  _rawDiskList.push_back(pDisk);
	return pDisk;
}

byte DiskDriveManager::RemoveRawDiskEntry(const upan::string& name)
{
  for(auto it = _rawDiskList.begin(); it != _rawDiskList.end(); ++it)
  {
		if((*it)->Name() == name)
    {
      _rawDiskList.erase(it);
			return DeviceDrive_SUCCESS;
		}
  }
	return DeviceDrive_ERR_NOTFOUND;
}

RawDiskDrive* DiskDriveManager::GetRawDiskByName(const upan::string& name)
{
  auto it = upan::find_if(_rawDiskList.begin(), _rawDiskList.end(), [&name](const RawDiskDrive* d) { return d->Name() == name; });
  if(it == _rawDiskList.end())
    return nullptr;
  return *it;
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
  auto it = upan::find_if(_driveList.begin(), _driveList.end(), [&driveName, bCheckMount](const DiskDrive* d)
    {
      if(d->DriveName() == driveName)
        return d->Mounted() || !bCheckMount;
      return false;
    });
  if(it == _driveList.end())
    return nullptr;
  return *it;
}

DiskDrive* DiskDriveManager::GetByID(int iID, bool bCheckMount)
{	
  MutexGuard g(_driveListMutex);
	if(iID == ROOT_DRIVE)
		iID = ROOT_DRIVE_ID;
	else if(iID == CURRENT_DRIVE)
		iID = ProcessManager::Instance().GetCurrentPAS().iDriveID;
  auto it = upan::find_if(_driveList.begin(), _driveList.end(), [iID, bCheckMount](const DiskDrive* d)
    {
      if(d->Id() == iID)
        return d->Mounted() || !bCheckMount;
      return false;
    });
  if(it == _driveList.end())
    return nullptr;
  return *it;
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
			pDiskDrive->RawDisk()->UpdateSystemIndicator(pDiskDrive->LBAStartSector(), 0x93);
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

RESOURCE_KEYS DiskDriveManager::GetResourceType(DEVICE_TYPE deviceType)
{
	switch(deviceType)
	{
		case DEV_FLOPPY: return RESOURCE_FDD;
		case DEV_ATA_IDE: return RESOURCE_HDD;
		case DEV_SCSI_USB_DISK: return RESOURCE_USD;
    default: return RESOURCE_GENERIC_DISK;
	}
}

RESOURCE_KEYS DiskDriveManager::GetResourceType(RAW_DISK_TYPES diskType)
{
	switch(diskType)
	{
		case ATA_HARD_DISK: return RESOURCE_HDD;
		case USB_SCSI_DISK: return RESOURCE_USD;
	//TODO: FLOPPY falls under this category which should be changed to particular disk type
    default: return RESOURCE_GENERIC_DISK;
	}

}

void RawDiskDrive::Read(unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer)
{
  MutexGuard g(_diskMutex);
	switch(_type)
	{
		case ATA_HARD_DISK:
			if(ATADrive_Read((ATAPort*)Device(), uiStartSector, pDataBuffer, uiNoOfSectors) != ATADrive_SUCCESS)
        throw upan::exception(XLOC, "error reading from ata disk drive");
       break;
		case USB_SCSI_DISK:
			if(SCSIHandler_GenericRead((SCSIDevice*)Device(), uiStartSector, uiNoOfSectors, pDataBuffer) != SCSIHandler_SUCCESS)
        throw upan::exception(XLOC, "error reading from usb disk drive");
      break;
    default:
      throw upan::exception(XLOC, "error reading - unknown device type: %d", _type);
	}
}

void RawDiskDrive::Write(unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer)
{
	MutexGuard g(_diskMutex);
	switch(_type)
	{
		case ATA_HARD_DISK:
      if(ATADrive_Write((ATAPort*)Device(), uiStartSector, pDataBuffer, uiNoOfSectors) != ATADrive_SUCCESS)
        throw upan::exception(XLOC, "error writing to ata disk drive");
      break;
		case USB_SCSI_DISK:
			if(SCSIHandler_GenericWrite((SCSIDevice*)Device(), uiStartSector, uiNoOfSectors, pDataBuffer) != SCSIHandler_SUCCESS)
        throw upan::exception(XLOC, "error writing to usb disk drive");
      break;
    default:
      throw upan::exception(XLOC, "error reading - unknown device type: %d", _type);
	}
}

void RawDiskDrive::UpdateSystemIndicator(unsigned uiLBAStartSector, unsigned uiSystemIndicator)
{
	byte bBootSectorBuffer[512] ;

	Read(0, 1, bBootSectorBuffer);

	PartitionInfo* pPartitionTable = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;
	
  const unsigned NO_OF_PARTITIONS = 4;
	for(unsigned i = 0; i < NO_OF_PARTITIONS; i++)
	{
		if(pPartitionTable[i].LBAStartSector == uiLBAStartSector)
		{
			pPartitionTable[i].SystemIndicator = uiSystemIndicator ;
			break ;
		}
	}

	Write(0, 1, bBootSectorBuffer);
}
