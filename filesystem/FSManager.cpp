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
# include <FSManager.h>
# include <DMM.h>
# include <Global.h>
# include <FSStructures.h>
# include <MemUtil.h>
# include <Display.h>
# include <FileSystem.h>

#define BLOCK_ID(SectorID) (SectorID / ENTRIES_PER_TABLE_SECTOR)
#define BLOCK_OFFSET(SectorID) (SectorID % ENTRIES_PER_TABLE_SECTOR) 

/**************************** static functions **************************************/

static byte FSManager_GetSectorEntryValueDirect(DiskDrive* pDiskDrive, unsigned uiSectorID, unsigned* uiValue) ;
static byte FSManager_SetSectorEntryValueDirect(DiskDrive* pDiskDrive, unsigned uiSectorID, unsigned uiValue) ;
static void FSManager_UpdateUsedSectors(DiskDrive* pDiskDrive, unsigned uiSectorEntryValue) ;

byte FSManager_BinarySearch(const upan::vector<SectorBlockEntry>& blocks, unsigned uiBlockID, int* iPos)
{
  int low, high, mid ;

  *iPos = -1 ;

  for(low = 0, high = blocks.size() - 1; low <= high;)
	{
		mid = (low + high) / 2 ;

    if(uiBlockID == blocks[mid].BlockId())
    {
      *iPos = mid ;
      return true ;
    }

    if(uiBlockID < blocks[mid].BlockId())
		{
			high = mid - 1 ;
      *iPos = high + 1 ;
		}
		else
		{
			low = mid + 1 ;
      *iPos = low ;
		}
	}

	if(*iPos < 0)
		*iPos = 0 ;
  else if(*iPos >= blocks.size())
    *iPos = blocks.size() ;

	return false ;
}

static byte FSManager_GetSectorEntryValueDirect(DiskDrive* pDiskDrive, unsigned uiSectorID, unsigned* uiValue)
{
	unsigned pTable[ ENTRIES_PER_TABLE_SECTOR ] ;
	FileSystem_BootBlock* pFSBootBlock = &pDiskDrive->FSMountInfo.FSBootBlock ;

	RETURN_X_IF_NOT(pDiskDrive->Read(pFSBootBlock->BPB_RsvdSecCnt + BLOCK_ID(uiSectorID) + 1, 1, 
									(byte*)pTable),
					DeviceDrive_SUCCESS, FSManager_FAILURE) ;

	*uiValue = pTable[ BLOCK_OFFSET(uiSectorID) ] ;

	return FSManager_SUCCESS ;
}

static byte FSManager_SetSectorEntryValueDirect(DiskDrive* pDiskDrive, unsigned uiSectorID, unsigned uiValue)
{
	unsigned pTable[ ENTRIES_PER_TABLE_SECTOR ] ;

	FileSystem_BootBlock* pFSBootBlock = &pDiskDrive->FSMountInfo.FSBootBlock ;

	RETURN_X_IF_NOT(pDiskDrive->Read(pFSBootBlock->BPB_RsvdSecCnt + BLOCK_ID(uiSectorID) + 1, 1, 
									(byte*)pTable),
					DeviceDrive_SUCCESS, FSManager_FAILURE) ;

	pTable[ BLOCK_OFFSET(uiSectorID) ] = uiValue ;

	RETURN_X_IF_NOT(pDiskDrive->Write(pFSBootBlock->BPB_RsvdSecCnt + BLOCK_ID(uiSectorID) + 1, 1, 
									(byte*)pTable), 
					DeviceDrive_SUCCESS, FSManager_FAILURE) ;

	return FSManager_SUCCESS ;
}

static void FSManager_UpdateUsedSectors(DiskDrive* pDiskDrive, unsigned uiSectorEntryValue)
{
	if(uiSectorEntryValue == EOC)
		pDiskDrive->FSMountInfo.FSBootBlock.uiUsedSectors++ ;
	else if(uiSectorEntryValue == 0)
		pDiskDrive->FSMountInfo.FSBootBlock.uiUsedSectors-- ;
}

/**************************************************************************************/

byte FSManager_GetSectorEntryValue(DiskDrive* pDiskDrive, const unsigned uiSectorID, unsigned* uiSectorEntryValue, byte bFromCahceOnly)
{
  FileSystemMountInfo& fsMountInfo = pDiskDrive->FSMountInfo ;
  SectorBlockEntry* pSectorBlockEntry = fsMountInfo.GetSectorEntryFromCache(uiSectorID) ;

	if(pSectorBlockEntry != NULL)
	{
    *uiSectorEntryValue = pSectorBlockEntry->Read(uiSectorID);
		return FSManager_SUCCESS ;
	}
	
	if(bFromCahceOnly)
		return FSManager_FAILURE ;

	byte bStatus ;

  RETURN_IF_NOT(bStatus, fsMountInfo.AddToTableCache(uiSectorID), FSManager_SUCCESS) ;
	
	RETURN_IF_NOT(bStatus, FSManager_GetSectorEntryValue(pDiskDrive, uiSectorID, uiSectorEntryValue, true), FSManager_SUCCESS) ;
	
	return FSManager_SUCCESS ;
}

byte FSManager_SetSectorEntryValue(DiskDrive* pDiskDrive, const unsigned uiSectorID, unsigned uiSectorEntryValue, byte bFromCahceOnly)
{
	byte bStatus ;

	if(!bFromCahceOnly)
	{
		FSManager_UpdateUsedSectors(pDiskDrive, uiSectorEntryValue) ;
	}

  FileSystemMountInfo& fsMountInfo = pDiskDrive->FSMountInfo ;
  SectorBlockEntry* pSectorBlockEntry = fsMountInfo.GetSectorEntryFromCache(uiSectorID) ;

	if(pSectorBlockEntry != NULL)
  {
    pSectorBlockEntry->Write(uiSectorID, uiSectorEntryValue);
		return FSManager_SUCCESS ;
	}

	if(bFromCahceOnly)
		return FSManager_FAILURE ;
	
  RETURN_IF_NOT(bStatus, fsMountInfo.AddToTableCache(uiSectorID), FSManager_SUCCESS) ;

	RETURN_IF_NOT(bStatus, FSManager_SetSectorEntryValue(pDiskDrive, uiSectorID, uiSectorEntryValue, true), FSManager_SUCCESS) ;
		
	return FSManager_SUCCESS ;
}

byte FSManager_AllocateSector(DiskDrive* pDiskDrive, unsigned* uiFreeSectorID)
{
	byte bStatus ;

  pDiskDrive->FSMountInfo.AllocateSector(uiFreeSectorID);

	RETURN_IF_NOT(bStatus, FSManager_SetSectorEntryValue(pDiskDrive, *uiFreeSectorID, EOC, false), FSManager_SUCCESS) ;

	return FSManager_SUCCESS ;
}

byte SectorBlockEntry::Load(DiskDrive& diskDrive, uint32_t sectortId)
{
  FileSystemMountInfo* pFSMountInfo = (FileSystemMountInfo*)&diskDrive.FSMountInfo;
  FileSystem_BootBlock* pFSBootBlock = (FileSystem_BootBlock*)&pFSMountInfo->FSBootBlock;

  RETURN_X_IF_NOT(diskDrive.Read(pFSBootBlock->BPB_RsvdSecCnt + BLOCK_ID(sectortId) + 1, 1, (byte*)_sectorBlock), DeviceDrive_SUCCESS, FSManager_FAILURE) ;
  _blockId = BLOCK_ID(sectortId);
  _readCount = _writeCount = 0;

  return DeviceDrive_SUCCESS;
}

uint32_t SectorBlockEntry::Read(uint32_t sectorId)
{
  ++_readCount;
  return _sectorBlock[BLOCK_OFFSET(sectorId)] & EOC ;
}

void SectorBlockEntry::Write(uint32_t sectorId, uint32_t value)
{
  auto index = BLOCK_OFFSET(sectorId) ;
  _sectorBlock[index] = _sectorBlock[index] & 0xF0000000;
  _sectorBlock[index] = _sectorBlock[index] | (value & EOC);
  ++_writeCount;
}

void FileSystemMountInfo::UnallocateFreePoolQueue()
{
  if(_freePoolQueue)
  {
    delete _freePoolQueue;
    _freePoolQueue = nullptr;
  }
}

void FileSystemMountInfo::AllocateFreePoolQueue(uint32_t size)
{
  _freePoolQueue = new upan::queue<unsigned>(size);
}

byte FileSystemMountInfo::ReadFSBootBlock()
{
  byte bArrFSBootBlock[512];
  byte bStatus;

  RETURN_IF_NOT(bStatus, _diskDrive.Read(1, 1, bArrFSBootBlock), DeviceDrive_SUCCESS);

  if(bArrFSBootBlock[510] != 0x55 || bArrFSBootBlock[511] != 0xAA)
    return FileSystem_ERR_INVALID_BPB_SIGNATURE;

  memcpy(&FSBootBlock, bArrFSBootBlock, sizeof(FileSystem_BootBlock));

  if(FSBootBlock.BPB_BootSig != 0x29)
    return FileSystem_ERR_INVALID_BOOT_SIGNATURE;

  // TODO: A write to HD image file from mos fs util is changing the CHS value !!
  // Needs to be fixed. So, this check is skipped for the time being

  /*
  if(fsBootBlock.BPB_SecPerTrk != pDiskDrive->uiSectorsPerTrack)
    return FileSystem_ERR_INVALID_SECTORS_PER_TRACK;

  if(fsBootBlock.BPB_NumHeads != pDiskDrive->uiNoOfHeads)
    return FileSystem_ERR_INVALID_NO_OF_HEADS;
  */

  if(FSBootBlock.BPB_TotSec32 != _diskDrive.SizeInSectors())
    return FileSystem_ERR_INVALID_DRIVE_SIZE_IN_SECTORS;

  if(FSBootBlock.BPB_FSTableSize == 0)
    return FileSystem_ERR_ZERO_FATSZ32;

  if(FSBootBlock.BPB_BytesPerSec != 0x200)
    return FileSystem_ERR_UNSUPPORTED_SECTOR_SIZE;

  if(FSBootBlock.BPB_jmpBoot[0] != 0xEB || FSBootBlock.BPB_jmpBoot[1] != 0xFE || FSBootBlock.BPB_jmpBoot[2] != 0x90)
    return FileSystem_ERR_BPB_JMP;

  if(FSBootBlock.BPB_BytesPerSec != 0x200)
    return FileSystem_ERR_UNSUPPORTED_SECTOR_SIZE;

  if(_diskDrive.DeviceType() == DEV_FLOPPY)
    if(FSBootBlock.BPB_Media  != 0xF0)
      return FileSystem_ERR_UNSUPPORTED_MEDIA;

  if(FSBootBlock.BPB_ExtFlags  != 0x0080)
    return FileSystem_ERR_INVALID_EXTFLAG;

  if(FSBootBlock.BPB_FSVer != 0x0100)
    return FileSystem_ERR_FS_VERSION;

  if(FSBootBlock.BPB_FSInfo  != 1)
    return FileSystem_ERR_FSINFO_SECTOR;

  if(FSBootBlock.BPB_VolID != 0x01)
    return FileSystem_ERR_INVALID_VOL_ID;

  return DeviceDrive_SUCCESS;
}

byte FileSystemMountInfo::WriteFSBootBlock()
{
  byte bSectorBuffer[512];

  bSectorBuffer[510] = 0x55; /* BootSector Signature */
  bSectorBuffer[511] = 0xAA;

  memcpy(bSectorBuffer, &FSBootBlock, sizeof(FileSystem_BootBlock));

  byte bStatus;
  RETURN_IF_NOT(bStatus, _diskDrive.Write(1, 1, bSectorBuffer), DeviceDrive_SUCCESS) ;

  return DeviceDrive_SUCCESS;
}

byte FileSystemMountInfo::LoadFreeSectors()
{
  if(_freePoolQueue->full())
    return DeviceDrive_SUCCESS;

  bool bStop = false;

  // First do Cache Lookup
  for(const auto& sectorBlockEntry : _fsTableCache)
  {
    if(bStop)
      break;
    auto uiSectorBlock = sectorBlockEntry.SectorBlock();
    for(int j = 0; j < ENTRIES_PER_TABLE_SECTOR; j++)
    {
      if(!(uiSectorBlock[j] & EOC))
      {
        unsigned uiSectorID = sectorBlockEntry.BlockId() * ENTRIES_PER_TABLE_SECTOR + j;
        if(!_freePoolQueue->push_back(uiSectorID))
        {
          bStop = true;
          break;
        }
      }
    }
  }

  if(bStop)
    return DeviceDrive_SUCCESS;

  byte bBuffer[ 4096 ];
  byte bStatus;

  for(unsigned i = 0; i < FSBootBlock.BPB_FSTableSize; )
  {
    if(bStop)
      break;

    int iPos;
    if(FSManager_BinarySearch(_fsTableCache, i, &iPos))
    {
      i++;
      continue;
    }

    unsigned uiBlockSize = (FSBootBlock.BPB_FSTableSize - i);
    if(uiBlockSize > 8)
      uiBlockSize = 8;

    RETURN_IF_NOT(bStatus, _diskDrive.Read(i + FSBootBlock.BPB_RsvdSecCnt + 1, uiBlockSize, (byte*)bBuffer), DeviceDrive_SUCCESS);

    unsigned* pTable = (unsigned*)bBuffer;

    for(unsigned j = 0; j < ENTRIES_PER_TABLE_SECTOR * uiBlockSize; j++)
    {
      if(!(pTable[j] & EOC))
      {
        unsigned uiSectorID = i * ENTRIES_PER_TABLE_SECTOR + j;
        if(!_freePoolQueue->push_back(uiSectorID))
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

byte FileSystemMountInfo::FlushTableCache(int iFlushSize)
{
  if(iFlushSize > _fsTableCache.size())
    iFlushSize = _fsTableCache.size();

  byte bStatus;

  for(int i = 0; i < iFlushSize; i++)
  {
    auto& block = _fsTableCache[i];

    if(block.WriteCount() == 0)
      continue;

    RETURN_IF_NOT(bStatus, _diskDrive.Write(block.BlockId() + FSBootBlock.BPB_RsvdSecCnt + 1, 1, (byte*)(block.SectorBlock())), DeviceDrive_SUCCESS) ;
  }

  if(iFlushSize < _fsTableCache.size())
    _fsTableCache.erase(0, iFlushSize);

  return DeviceDrive_SUCCESS;
}

byte FileSystemMountInfo::AddToTableCache(unsigned uiSectorEntry)
{
  int iPos ;
  byte bStatus ;

  if((unsigned)_fsTableCache.size() == DiskDrive::MAX_SECTORS_IN_TABLE_CACHE)
    RETURN_IF_NOT(bStatus, FlushTableCache(1), DeviceDrive_SUCCESS) ;

  if(FSManager_BinarySearch(_fsTableCache, BLOCK_ID(uiSectorEntry), &iPos))
    return FSManager_SUCCESS ;

  _fsTableCache.insert(iPos, SectorBlockEntry());

  RETURN_X_IF_NOT(_fsTableCache[iPos].Load(_diskDrive, uiSectorEntry), DeviceDrive_SUCCESS, FSManager_FAILURE) ;

  return FSManager_SUCCESS ;
}

SectorBlockEntry* FileSystemMountInfo::GetSectorEntryFromCache(unsigned uiSectorEntry)
{
  if(_fsTableCache.empty())
    return NULL ;

  int iPos ;
  if(!FSManager_BinarySearch(_fsTableCache, BLOCK_ID(uiSectorEntry), &iPos))
    return NULL ;

  return &_fsTableCache[iPos] ;
}

byte FileSystemMountInfo::AllocateSector(uint32_t* uiFreeSectorID)
{
  byte bStatus;
  if(_freePoolQueue->empty())
  {
    RETURN_IF_NOT(bStatus, LoadFreeSectors(), DeviceDrive_SUCCESS) ;
    if(_freePoolQueue->empty())
      return FSManager_FAILURE;
  }
  *uiFreeSectorID = _freePoolQueue->front();
  _freePoolQueue->pop_front();
  return FSManager_SUCCESS;
}

void FileSystemMountInfo::DisplayCache()
{
  printf("\nSTART\n");
  for(const auto& block : _fsTableCache)
    printf(", %u", block.BlockId());
  printf(" :: SIZE = %d", _fsTableCache.size());
}
