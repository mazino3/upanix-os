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

#define BLOCK_ID(SectorID) (SectorID / ENTRIES_PER_TABLE_SECTOR)
#define BLOCK_OFFSET(SectorID) (SectorID % ENTRIES_PER_TABLE_SECTOR) 

/**************************** static functions **************************************/

static SectorBlockEntry* FSManager_GetSectorEntryFromCache(const FileSystem_TableCache* pFSTableCache, unsigned uiSectorEntry) ;
static byte FSManager_AddSectorEntryIntoCache(DiskDrive* pDiskDrive, unsigned uiSectorEntry) ;
static void FSManager_UpdateReadCount(SectorBlockEntry* pSectorBlockEntry) ;
static void FSManager_UpdateWriteCount(SectorBlockEntry* pSectorBlockEntry) ;
static byte FSManager_GetSectorEntryValueDirect(DiskDrive* pDiskDrive, unsigned uiSectorID, unsigned* uiValue) ;
static byte FSManager_SetSectorEntryValueDirect(DiskDrive* pDiskDrive, unsigned uiSectorID, unsigned uiValue) ;
static void FSManager_UpdateUsedSectors(DiskDrive* pDiskDrive, unsigned uiSectorEntryValue) ;

byte FSManager_BinarySearch(SectorBlockEntry* pList, int iSize, unsigned uiBlockID, int* iPos)
{
	int low, high, mid ;

	*iPos = -1 ;

	for(low = 0, high = iSize - 1; low <= high;)
	{
		mid = (low + high) / 2 ;

		if(uiBlockID == pList[mid].uiBlockID)
		{
			*iPos = mid ;
			return true ;
		}

		if(uiBlockID < pList[mid].uiBlockID)
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
	else if(*iPos >= iSize)
		*iPos = iSize ;

	return false ;
}

static SectorBlockEntry* FSManager_GetSectorEntryFromCache(const FileSystem_TableCache* pFSTableCache, unsigned uiSectorEntry)
{
	SectorBlockEntry* pSectorBlockEntryList = pFSTableCache->pSectorBlockEntryList ;
	int iSize = pFSTableCache->iSize ;

	if(!iSize)
		return NULL ;

	int iPos ;
	if(!FSManager_BinarySearch(pSectorBlockEntryList, iSize, BLOCK_ID(uiSectorEntry), &iPos))
		return NULL ;

	return &pSectorBlockEntryList[iPos] ;
}

static byte FSManager_AddSectorEntryIntoCache(DiskDrive* pDiskDrive, unsigned uiSectorEntry)
{	
	FileSystemMountInfo* pFSMountInfo = (FileSystemMountInfo*)&pDiskDrive->FSMountInfo ;
	FileSystem_BootBlock* pFSBootBlock = (FileSystem_BootBlock*)&pFSMountInfo->FSBootBlock ;
	FileSystem_TableCache* pFSTableCache = (FileSystem_TableCache*)&pFSMountInfo->FSTableCache ;
	SectorBlockEntry* pSectorBlockEntryList = pFSTableCache->pSectorBlockEntryList ;
	int iSize = pFSTableCache->iSize ;
	int iPos ;
	byte bStatus ;

	if((unsigned)iSize == pDiskDrive->NoOfSectorsInTableCache())
	{
		RETURN_IF_NOT(bStatus, pDiskDrive->FlushTableCache(1), DeviceDrive_SUCCESS) ;
		iSize = pFSTableCache->iSize ;
	}

	if(FSManager_BinarySearch(pSectorBlockEntryList, iSize, BLOCK_ID(uiSectorEntry), &iPos))
		return FSManager_SUCCESS ;

	unsigned uiSrc = (unsigned)&pSectorBlockEntryList[iPos] ;
	unsigned uiDest = (unsigned)&pSectorBlockEntryList[iPos + 1] ;

	MemUtil_CopyMemoryBack(MemUtil_GetDS(), uiSrc, MemUtil_GetDS(), uiDest, (iSize - iPos) * sizeof(SectorBlockEntry)) ;
//	memmove((void*)uiDest, (void*)uiSrc, (iSize - iPos) * sizeof(SectorBlockEntry)) ;

	RETURN_X_IF_NOT(pDiskDrive->Read(pFSBootBlock->BPB_RsvdSecCnt + BLOCK_ID(uiSectorEntry) + 1, 1, 
					(byte*)&(pSectorBlockEntryList[iPos].uiSectorBlock)),
					DeviceDrive_SUCCESS, FSManager_FAILURE) ;

	pSectorBlockEntryList[iPos].uiBlockID = BLOCK_ID(uiSectorEntry) ;
	pSectorBlockEntryList[iPos].uiReadCount = 0 ;
	pSectorBlockEntryList[iPos].uiWriteCount = 0 ;

	pFSTableCache->iSize++ ;

	return FSManager_SUCCESS ;
}

static void FSManager_UpdateReadCount(SectorBlockEntry* pSectorBlockEntry)
{
	pSectorBlockEntry->uiReadCount++ ;
}

static void FSManager_UpdateWriteCount(SectorBlockEntry* pSectorBlockEntry)
{
	pSectorBlockEntry->uiWriteCount++ ;
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
	SectorBlockEntry* pSectorBlockEntry = FSManager_GetSectorEntryFromCache(&pFSMountInfo->FSTableCache, uiSectorID) ;

	if(pSectorBlockEntry != NULL)
	{
		unsigned* pTable = (unsigned*)&(pSectorBlockEntry->uiSectorBlock) ;
		unsigned uiIndex = BLOCK_OFFSET(uiSectorID) ;
		*uiSectorEntryValue = pTable[uiIndex] & EOC ;
		FSManager_UpdateReadCount(pSectorBlockEntry) ;
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
	SectorBlockEntry* pSectorBlockEntry = FSManager_GetSectorEntryFromCache(&pFSMountInfo->FSTableCache, uiSectorID) ;

	if(pSectorBlockEntry != NULL)
	{
		unsigned* pTable = (unsigned*)&(pSectorBlockEntry->uiSectorBlock) ;
		unsigned uiIndex = BLOCK_OFFSET(uiSectorID) ;

		uiSectorEntryValue &= EOC ;
		pTable[uiIndex] = pTable[uiIndex] & 0xF0000000 ;
		pTable[uiIndex] = pTable[uiIndex] | uiSectorEntryValue ;

		FSManager_UpdateWriteCount(pSectorBlockEntry) ;

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
	int i ;
	KC::MDisplay().Message("\nSTART\n", ' ') ;
	for(i = 0; i < pDiskDrive->FSMountInfo.FSTableCache.iSize; i++)
	{
		KC::MDisplay().Address(", ", pDiskDrive->FSMountInfo.FSTableCache.pSectorBlockEntryList[i].uiBlockID) ;
	}
	KC::MDisplay().Address(" :: SIZE = ", pDiskDrive->FSMountInfo.FSTableCache.iSize) ;
}
