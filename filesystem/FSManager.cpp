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

static SectorBlockEntry* FSManager_GetSectorEntryFromCache(const upan::vector<SectorBlockEntry>& fsTableCache, unsigned uiSectorEntry) ;
static byte FSManager_AddSectorEntryIntoCache(DiskDrive* pDiskDrive, unsigned uiSectorEntry) ;
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

static SectorBlockEntry* FSManager_GetSectorEntryFromCache(const upan::vector<SectorBlockEntry>& fsTableCache, unsigned uiSectorEntry)
{
  if(fsTableCache.empty())
		return NULL ;

	int iPos ;
  if(!FSManager_BinarySearch(fsTableCache, BLOCK_ID(uiSectorEntry), &iPos))
		return NULL ;

  return &fsTableCache[iPos] ;
}

static byte FSManager_AddSectorEntryIntoCache(DiskDrive* pDiskDrive, unsigned uiSectorEntry)
{	
	FileSystemMountInfo* pFSMountInfo = (FileSystemMountInfo*)&pDiskDrive->FSMountInfo ;
  auto& fsTableCache = pFSMountInfo->FSTableCache;
	int iPos ;
	byte bStatus ;

  if((unsigned)fsTableCache.size() == DiskDrive::MAX_SECTORS_IN_TABLE_CACHE)
		RETURN_IF_NOT(bStatus, pDiskDrive->FlushTableCache(1), DeviceDrive_SUCCESS) ;

  if(FSManager_BinarySearch(fsTableCache, BLOCK_ID(uiSectorEntry), &iPos))
		return FSManager_SUCCESS ;

  fsTableCache.insert(iPos, SectorBlockEntry());

  RETURN_X_IF_NOT(fsTableCache[iPos].Load(*pDiskDrive, uiSectorEntry), DeviceDrive_SUCCESS, FSManager_FAILURE) ;

	return FSManager_SUCCESS ;
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
	if(!pDiskDrive->IsTableCacheEnabled())
	{
		return FSManager_GetSectorEntryValueDirect(const_cast<DiskDrive*>(pDiskDrive), uiSectorID, uiSectorEntryValue) ;
	}
	
	FileSystemMountInfo* pFSMountInfo = (FileSystemMountInfo*)&pDiskDrive->FSMountInfo ;
  SectorBlockEntry* pSectorBlockEntry = FSManager_GetSectorEntryFromCache(pFSMountInfo->FSTableCache, uiSectorID) ;

	if(pSectorBlockEntry != NULL)
	{
    *uiSectorEntryValue = pSectorBlockEntry->Read(uiSectorID);
		return FSManager_SUCCESS ;
	}
	
	if(bFromCahceOnly)
		return FSManager_FAILURE ;

	byte bStatus ;

	RETURN_IF_NOT(bStatus, FSManager_AddSectorEntryIntoCache(pDiskDrive, uiSectorID), FSManager_SUCCESS) ;
	
	RETURN_IF_NOT(bStatus, FSManager_GetSectorEntryValue(pDiskDrive, uiSectorID, uiSectorEntryValue, true), FSManager_SUCCESS) ;
	
	return FSManager_SUCCESS ;
}

byte FSManager_SetSectorEntryValue(DiskDrive* pDiskDrive, const unsigned uiSectorID, unsigned uiSectorEntryValue, byte bFromCahceOnly)
{
	byte bStatus ;

	if(!pDiskDrive->IsTableCacheEnabled())
	{
		RETURN_IF_NOT(bStatus, FSManager_SetSectorEntryValueDirect(pDiskDrive, uiSectorID, uiSectorEntryValue),
						FSManager_SUCCESS) ;

		FSManager_UpdateUsedSectors(pDiskDrive, uiSectorEntryValue) ;

		return FSManager_SUCCESS ;
	}
	
	if(!bFromCahceOnly)
	{
		FSManager_UpdateUsedSectors(pDiskDrive, uiSectorEntryValue) ;
	}

	FileSystemMountInfo* pFSMountInfo = (FileSystemMountInfo*)&pDiskDrive->FSMountInfo ;
  SectorBlockEntry* pSectorBlockEntry = FSManager_GetSectorEntryFromCache(pFSMountInfo->FSTableCache, uiSectorID) ;

	if(pSectorBlockEntry != NULL)
  {
    pSectorBlockEntry->Write(uiSectorID, uiSectorEntryValue);
		return FSManager_SUCCESS ;
	}

	if(bFromCahceOnly)
		return FSManager_FAILURE ;
	
	RETURN_IF_NOT(bStatus, FSManager_AddSectorEntryIntoCache(pDiskDrive, uiSectorID), FSManager_SUCCESS) ;

	RETURN_IF_NOT(bStatus, FSManager_SetSectorEntryValue(pDiskDrive, uiSectorID, uiSectorEntryValue, true), FSManager_SUCCESS) ;
		
	return FSManager_SUCCESS ;
}

byte FSManager_AllocateSector(DiskDrive* pDiskDrive, unsigned* uiFreeSectorID)
{
	byte bStatus ;

	if(!pDiskDrive->IsFreePoolCacheEnabled())
	{
    *uiFreeSectorID = pDiskDrive->GetFreeSector();
	}
	else
	{
		if(pDiskDrive->FSMountInfo.pFreePoolQueue->empty())
    {
			RETURN_IF_NOT(bStatus, pDiskDrive->LoadFreeSectors(), DeviceDrive_SUCCESS) ;
      if(pDiskDrive->FSMountInfo.pFreePoolQueue->empty())
        return FSManager_FAILURE;
		}
    *uiFreeSectorID = pDiskDrive->FSMountInfo.pFreePoolQueue->front();
    pDiskDrive->FSMountInfo.pFreePoolQueue->pop_front();
	}

	RETURN_IF_NOT(bStatus, FSManager_SetSectorEntryValue(pDiskDrive, *uiFreeSectorID, EOC, false), FSManager_SUCCESS) ;

	return FSManager_SUCCESS ;
}

void display_cahce(DiskDrive* pDiskDrive)
{
  printf("\nSTART\n");
  for(const auto& block : pDiskDrive->FSMountInfo.FSTableCache)
    printf(", %u", block.BlockId());
  printf(" :: SIZE = %d", pDiskDrive->FSMountInfo.FSTableCache.size());
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
