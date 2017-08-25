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

/**************************************************************************************/

void SectorBlockEntry::Load(DiskDrive& diskDrive, uint32_t sectortId)
{
  const FSBootBlock& fsBootBlock = diskDrive.FSMountInfo.GetBootBlock();
  diskDrive.Read(fsBootBlock.BPB_RsvdSecCnt + BLOCK_ID(sectortId) + 1, 1, (byte*)_sectorBlock);
  _blockId = BLOCK_ID(sectortId);
  _readCount = _writeCount = 0;
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

void FileSystemMountInfo::ReadFSBootBlock()
{
  byte bArrFSBootBlock[512];

  _diskDrive.Read(1, 1, bArrFSBootBlock);

  if(bArrFSBootBlock[510] != 0x55 || bArrFSBootBlock[511] != 0xAA)
    throw upan::exception(XLOC, "invalid BPB signature - %x, %x", bArrFSBootBlock[510], bArrFSBootBlock[511]);

  memcpy(&_fsBootBlock, bArrFSBootBlock, sizeof(FSBootBlock));

  if(_fsBootBlock.BPB_BootSig != 0x29)
    throw upan::exception(XLOC, "invalid BOOT signature: %x", _fsBootBlock.BPB_BootSig);

  // TODO: A write to HD image file from mos fs util is changing the CHS value !!
  // Needs to be fixed. So, this check is skipped for the time being

  /*
  if(fsBootBlock.BPB_SecPerTrk != pDiskDrive->uiSectorsPerTrack)
    return FileSystem_ERR_INVALID_SECTORS_PER_TRACK;

  if(fsBootBlock.BPB_NumHeads != pDiskDrive->uiNoOfHeads)
    return FileSystem_ERR_INVALID_NO_OF_HEADS;
  */

  if(_fsBootBlock.BPB_TotSec32 != _diskDrive.SizeInSectors())
    throw upan::exception(XLOC, "invalid BPB_TotSec32: %d", _fsBootBlock.BPB_TotSec32);

  if(_fsBootBlock.BPB_FSTableSize == 0)
    throw upan::exception(XLOC, "invalid BPB_FSTableSize: %d", _fsBootBlock.BPB_FSTableSize);

  if(_fsBootBlock.BPB_BytesPerSec != 0x200)
    throw upan::exception(XLOC, "invalid BPB_BytesPerSec: %d", _fsBootBlock.BPB_BytesPerSec);

  if(_fsBootBlock.BPB_jmpBoot[0] != 0xEB || _fsBootBlock.BPB_jmpBoot[1] != 0xFE || _fsBootBlock.BPB_jmpBoot[2] != 0x90)
    throw upan::exception(XLOC, "invalid BPB_jmpBoot");

  if(_diskDrive.DeviceType() == DEV_FLOPPY)
    if(_fsBootBlock.BPB_Media != 0xF0)
      throw upan::exception(XLOC, "invalid BPB_Media: %x", _fsBootBlock.BPB_Media);

  if(_fsBootBlock.BPB_ExtFlags != 0x0080)
    throw upan::exception(XLOC, "invalid BPB_ExtFlags: %x", _fsBootBlock.BPB_ExtFlags);

  if(_fsBootBlock.BPB_FSVer != 0x0100)
    throw upan::exception(XLOC, "invalid BPB_FSVer: %x", _fsBootBlock.BPB_FSVer);

  if(_fsBootBlock.BPB_FSInfo != 1)
    throw upan::exception(XLOC, "invalid BPB_FSInfo: %d", _fsBootBlock.BPB_FSInfo);

  if(_fsBootBlock.BPB_VolID != 0x01)
    throw upan::exception(XLOC, "invalid BPB_VolID: %x", _fsBootBlock.BPB_VolID);
}

void FileSystemMountInfo::WriteFSBootBlock()
{
  byte bSectorBuffer[512];

  bSectorBuffer[510] = 0x55; /* BootSector Signature */
  bSectorBuffer[511] = 0xAA;

  memcpy(bSectorBuffer, &_fsBootBlock, sizeof(FSBootBlock));

  _diskDrive.Write(1, 1, bSectorBuffer);
}

void FileSystemMountInfo::LoadFreeSectors()
{
  if(_freePoolQueue->full())
    return;

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
    return;

  byte bBuffer[ 4096 ];

  for(unsigned i = 0; i < _fsBootBlock.BPB_FSTableSize; )
  {
    if(bStop)
      break;

    int iPos;
    if(FSManager_BinarySearch(_fsTableCache, i, &iPos))
    {
      i++;
      continue;
    }

    unsigned uiBlockSize = (_fsBootBlock.BPB_FSTableSize - i);
    if(uiBlockSize > 8)
      uiBlockSize = 8;

    _diskDrive.Read(i + _fsBootBlock.BPB_RsvdSecCnt + 1, uiBlockSize, (byte*)bBuffer);

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
}

void FileSystemMountInfo::FlushTableCache(int iFlushSize)
{
  if(iFlushSize > _fsTableCache.size())
    iFlushSize = _fsTableCache.size();

  for(int i = 0; i < iFlushSize; i++)
  {
    auto& block = _fsTableCache[i];

    if(block.WriteCount() == 0)
      continue;

    _diskDrive.Write(block.BlockId() + _fsBootBlock.BPB_RsvdSecCnt + 1, 1, (byte*)(block.SectorBlock()));
  }

  if(iFlushSize < _fsTableCache.size())
    _fsTableCache.erase(0, iFlushSize);
}

void FileSystemMountInfo::AddToTableCache(unsigned uiSectorEntry)
{
  if((unsigned)_fsTableCache.size() == DiskDrive::MAX_SECTORS_IN_TABLE_CACHE)
    FlushTableCache(1);

  int iPos ;
  if(FSManager_BinarySearch(_fsTableCache, BLOCK_ID(uiSectorEntry), &iPos))
    return;

  _fsTableCache.insert(iPos, SectorBlockEntry());
  _fsTableCache[iPos].Load(_diskDrive, uiSectorEntry);

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

uint32_t FileSystemMountInfo::AllocateSector()
{
  if(_freePoolQueue->empty())
  {
    LoadFreeSectors();
    if(_freePoolQueue->empty())
      throw upan::exception(XLOC, "No free sectors available on disk: %s", _diskDrive.DriveName().c_str());
  }
  auto uiFreeSectorID = _freePoolQueue->front();
  _freePoolQueue->pop_front();
  return uiFreeSectorID;
}

void FileSystemMountInfo::DisplayCache()
{
  printf("\nSTART\n");
  for(const auto& block : _fsTableCache)
    printf(", %u", block.BlockId());
  printf(" :: SIZE = %d", _fsTableCache.size());
}

uint32_t FileSystemMountInfo::GetRealSectorNumber(uint32_t uiSectorID) const
{
  return uiSectorID + 1/*BPB*/
          + _fsBootBlock.BPB_RsvdSecCnt
          + _fsBootBlock.BPB_FSTableSize;
}

void FileSystemMountInfo::InitBootBlock(FSBootBlock* bootBlock)
{
  memcpy(&_fsBootBlock, bootBlock, sizeof(FSBootBlock));
}

void FileSystemMountInfo::UpdateUsedSectors(unsigned uiSectorEntryValue)
{
  if(uiSectorEntryValue == EOC)
    _fsBootBlock.uiUsedSectors++;
  else if(uiSectorEntryValue == 0)
    _fsBootBlock.uiUsedSectors--;
}

uint32_t FileSystemMountInfo::GetSectorEntryValue(const unsigned uiSectorID, byte bFromCahceOnly)
{
  SectorBlockEntry* pSectorBlockEntry = GetSectorEntryFromCache(uiSectorID) ;

  if(pSectorBlockEntry != NULL)
    return pSectorBlockEntry->Read(uiSectorID);

  if(bFromCahceOnly)
    throw upan::exception(XLOC, "sectory entry value not found in cache for sector:%u", uiSectorID);

  AddToTableCache(uiSectorID);

  return GetSectorEntryValue(uiSectorID, true);
}

void FileSystemMountInfo::SetSectorEntryValue(const unsigned uiSectorID, unsigned uiSectorEntryValue, byte bFromCahceOnly)
{
  if(!bFromCahceOnly)
    UpdateUsedSectors((uiSectorEntryValue));

  SectorBlockEntry* pSectorBlockEntry = GetSectorEntryFromCache(uiSectorID) ;

  if(pSectorBlockEntry != NULL)
  {
    pSectorBlockEntry->Write(uiSectorID, uiSectorEntryValue);
    return;
  }

  if(bFromCahceOnly)
    throw upan::exception(XLOC, "Sector block for sector id %d is not in cache", uiSectorID);

  AddToTableCache(uiSectorID);

  SetSectorEntryValue(uiSectorID, uiSectorEntryValue, true);
}

uint32_t FSManager_AllocateSector(DiskDrive* pDiskDrive)
{
  uint32_t uiFreeSectorID = pDiskDrive->FSMountInfo.AllocateSector();

  FSManager_SetSectorEntryValue(pDiskDrive, uiFreeSectorID, EOC, false);

  return uiFreeSectorID;
}

