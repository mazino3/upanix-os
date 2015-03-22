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
# include <FSManager.h>
# include <DMM.h>
# include <Global.h>
# include <DSUtil.h>
# include <FSStructures.h>
# include <MemUtil.h>
# include <Display.h>

#define BLOCK_ID(SectorID) (SectorID / ENTRIES_PER_TABLE_SECTOR)
#define BLOCK_OFFSET(SectorID) (SectorID % ENTRIES_PER_TABLE_SECTOR) 

/**************************** static functions **************************************/

static byte FSManager_BinarySearch(SectorBlockEntry* pList, int iSize, unsigned uiBlockID, int* iPos) ;
static SectorBlockEntry* FSManager_GetSectorEntryFromCache(const FileSystem_TableCache* pFSTableCache, unsigned uiSectorEntry) ;
static byte FSManager_FlushEntries(DriveInfo* pDriveInfo, int iFlushSize) ;
static byte FSManager_AddSectorEntryIntoCache(DriveInfo* pDriveInfo, unsigned uiSectorEntry) ;
static void FSManager_UpdateReadCount(SectorBlockEntry* pSectorBlockEntry) ;
static void FSManager_UpdateWriteCount(SectorBlockEntry* pSectorBlockEntry) ;
static byte FSManager_LoadFreeSectors(DriveInfo* pDriveInfo) ;
static byte FSManager_GetFreeSector(DriveInfo* pDriveInfo, unsigned* uiSectorID) ;
static byte FSManager_GetSectorEntryValueDirect(DriveInfo* pDriveInfo, unsigned uiSectorID, unsigned* uiValue) ;
static byte FSManager_SetSectorEntryValueDirect(DriveInfo* pDriveInfo, unsigned uiSectorID, unsigned uiValue) ;
static void FSManager_UpdateUsedSectors(DriveInfo* pDriveInfo, unsigned uiSectorEntryValue) ;

static byte FSManager_BinarySearch(SectorBlockEntry* pList, int iSize, unsigned uiBlockID, int* iPos)
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

static byte FSManager_FlushEntries(DriveInfo* pDriveInfo, int iFlushSize)
{
	if(!FileSystem_IsTableCacheEnabled(pDriveInfo))
		return FSManager_SUCCESS ;

	FileSystem_TableCache* pFSTableCache = (FileSystem_TableCache*)&pDriveInfo->FSMountInfo.FSTableCache ;
	FileSystem_BootBlock* pFSBootBlock = (FileSystem_BootBlock*)&pDriveInfo->FSMountInfo.FSBootBlock ;
	SectorBlockEntry* pSectorBlockEntryList = pFSTableCache->pSectorBlockEntryList ;
	int iSize = pFSTableCache->iSize ;

	if(iFlushSize > iSize)
		iFlushSize = iSize ;

	byte bStatus ;
	
	int i ;

	for(i = 0; i < iFlushSize; i++)
	{
		SectorBlockEntry* pBlock = &(pSectorBlockEntryList[i]);

		if(pBlock->uiWriteCount == 0)
			continue ;

		RETURN_IF_NOT(bStatus, DeviceDrive_Write(pDriveInfo, 
						pBlock->uiBlockID + pFSBootBlock->BPB_RsvdSecCnt + 1, 1, 
						(byte*)(pBlock->uiSectorBlock)), DeviceDrive_SUCCESS) ;
	}

	if(iFlushSize < iSize)
	{
		unsigned uiSrc = (unsigned)&pSectorBlockEntryList[iFlushSize] ;
		unsigned uiDest = (unsigned)&pSectorBlockEntryList[0] ;

		MemUtil_CopyMemory(MemUtil_GetDS(), uiSrc, MemUtil_GetDS(), uiDest, (iSize - iFlushSize) * sizeof(SectorBlockEntry)) ;
	}

	pFSTableCache->iSize -= iFlushSize ;

	return FSManager_SUCCESS ;
}

static byte FSManager_AddSectorEntryIntoCache(DriveInfo* pDriveInfo, unsigned uiSectorEntry)
{	
	FileSystem_MountInfo* pFSMountInfo = (FileSystem_MountInfo*)&pDriveInfo->FSMountInfo ;
	FileSystem_BootBlock* pFSBootBlock = (FileSystem_BootBlock*)&pFSMountInfo->FSBootBlock ;
	FileSystem_TableCache* pFSTableCache = (FileSystem_TableCache*)&pFSMountInfo->FSTableCache ;
	SectorBlockEntry* pSectorBlockEntryList = pFSTableCache->pSectorBlockEntryList ;
	int iSize = pFSTableCache->iSize ;
	int iPos ;
	byte bStatus ;

	if(iSize == pDriveInfo->uiNoOfSectorsInTableCache)
	{
		RETURN_IF_NOT(bStatus, FSManager_FlushEntries(pDriveInfo, 1), FSManager_SUCCESS) ;
		iSize = pFSTableCache->iSize ;
	}

	if(FSManager_BinarySearch(pSectorBlockEntryList, iSize, BLOCK_ID(uiSectorEntry), &iPos))
		return FSManager_SUCCESS ;

	unsigned uiSrc = (unsigned)&pSectorBlockEntryList[iPos] ;
	unsigned uiDest = (unsigned)&pSectorBlockEntryList[iPos + 1] ;

	MemUtil_CopyMemoryBack(MemUtil_GetDS(), uiSrc, MemUtil_GetDS(), uiDest, (iSize - iPos) * sizeof(SectorBlockEntry)) ;
//	memmove((void*)uiDest, (void*)uiSrc, (iSize - iPos) * sizeof(SectorBlockEntry)) ;

	RETURN_X_IF_NOT(DeviceDrive_Read(pDriveInfo, 
					pFSBootBlock->BPB_RsvdSecCnt + BLOCK_ID(uiSectorEntry) + 1, 1, 
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

static byte FSManager_LoadFreeSectors(DriveInfo* pDriveInfo)
{
	if(!FileSystem_IsFreePoolCacheEnabled(pDriveInfo))
		return FSManager_SUCCESS ;

	byte bStatus ;
	int i, j ;

	DSUtil_Queue* pFreePoolQueue = &pDriveInfo->FSMountInfo.FreePoolQueue ;
	FileSystem_BootBlock* pFSBootBlock = &pDriveInfo->FSMountInfo.FSBootBlock ;
	byte bBuffer[ 4096 ] ;

	unsigned* pTable ;
	unsigned uiSectorID ;
	byte bStop = false ;

	SectorBlockEntry* pSectorBlockEntryList = NULL ;
	int iSize = 0 ;

	if(FileSystem_IsTableCacheEnabled(pDriveInfo))
	{
		FileSystem_TableCache* pFSTableCache = &(pDriveInfo->FSMountInfo.FSTableCache) ;
		iSize = pFSTableCache->iSize ;
		unsigned* uiSectorBlock ;

		pSectorBlockEntryList = pFSTableCache->pSectorBlockEntryList ;

		// First do Cache Lookup
		for(i = 0; i < iSize; i++)
		{
			if(bStop)
				break ;

			uiSectorBlock = pSectorBlockEntryList[i].uiSectorBlock ;

			for(j = 0; j < ENTRIES_PER_TABLE_SECTOR; j++)
			{
				if(!(uiSectorBlock[j] & EOC))
				{
					uiSectorID = pSectorBlockEntryList[i].uiBlockID * ENTRIES_PER_TABLE_SECTOR + j;
					if(DSUtil_WriteToQueue(pFreePoolQueue, uiSectorID) == DSUtil_ERR_QUEUE_FULL)
					{
						bStop = true ;
						break ;
					}
				}
			}
		}
	}
	
	if(bStop)
		return FSManager_SUCCESS ;

	int iPos ;
	unsigned uiBlockSize = 1 ;
	
	for(i = 0; i < pFSBootBlock->BPB_FSTableSize; )
	{
		if(bStop)
			break ;

		if(FileSystem_IsTableCacheEnabled(pDriveInfo))
		{
			if(FSManager_BinarySearch(pSectorBlockEntryList, iSize, i, &iPos))
			{
				i++ ;
				continue ;
			}
		}

		uiBlockSize = (pFSBootBlock->BPB_FSTableSize - i) ;
		if(uiBlockSize > 8) uiBlockSize = 8 ;

		RETURN_IF_NOT(bStatus, DeviceDrive_Read(pDriveInfo, i + pFSBootBlock->BPB_RsvdSecCnt + 1, uiBlockSize, (byte*)bBuffer), DeviceDrive_SUCCESS) ;

		pTable = (unsigned*)bBuffer ;

		for(j = 0; j < ENTRIES_PER_TABLE_SECTOR * uiBlockSize ; j++)
		{
			if(!(pTable[j] & EOC))
			{
				uiSectorID = i * ENTRIES_PER_TABLE_SECTOR + j;
				if(DSUtil_WriteToQueue(pFreePoolQueue, uiSectorID) == DSUtil_ERR_QUEUE_FULL)
				{
					bStop = true ;
					break ;
				}
			}
		}

		i += uiBlockSize ;
	}

	return FSManager_SUCCESS ;
}

static byte FSManager_GetFreeSector(DriveInfo* pDriveInfo, unsigned* uiSectorID)
{
	byte bStatus ;
	unsigned i, j ;
	byte bBuffer[512] ;
	unsigned* pTable ;

	FileSystem_BootBlock* pFSBootBlock = &pDriveInfo->FSMountInfo.FSBootBlock ;
	
	for(i = 0; i < pFSBootBlock->BPB_FSTableSize; i++)
	{
		RETURN_IF_NOT(bStatus, DeviceDrive_Read(pDriveInfo, i + pFSBootBlock->BPB_RsvdSecCnt + 1, 1, (byte*)bBuffer), DeviceDrive_SUCCESS) ;

		pTable = (unsigned*)bBuffer ;

		for(j = 0; j < ENTRIES_PER_TABLE_SECTOR; j++)
		{
			if(!(pTable[j] & EOC))
			{
				*uiSectorID = i * ENTRIES_PER_TABLE_SECTOR + j;
				return FSManager_SUCCESS ;
			}
		}
	}

	return FSManager_FAILURE ;
}

static byte FSManager_GetSectorEntryValueDirect(DriveInfo* pDriveInfo, unsigned uiSectorID, unsigned* uiValue)
{
	unsigned pTable[ ENTRIES_PER_TABLE_SECTOR ] ;
	FileSystem_BootBlock* pFSBootBlock = &pDriveInfo->FSMountInfo.FSBootBlock ;

	RETURN_X_IF_NOT(DeviceDrive_Read(pDriveInfo, 
									pFSBootBlock->BPB_RsvdSecCnt + BLOCK_ID(uiSectorID) + 1, 1, 
									(byte*)pTable),
					DeviceDrive_SUCCESS, FSManager_FAILURE) ;

	*uiValue = pTable[ BLOCK_OFFSET(uiSectorID) ] ;

	return FSManager_SUCCESS ;
}

static byte FSManager_SetSectorEntryValueDirect(DriveInfo* pDriveInfo, unsigned uiSectorID, unsigned uiValue)
{
	unsigned pTable[ ENTRIES_PER_TABLE_SECTOR ] ;

	FileSystem_BootBlock* pFSBootBlock = &pDriveInfo->FSMountInfo.FSBootBlock ;

	RETURN_X_IF_NOT(DeviceDrive_Read(pDriveInfo, 
									pFSBootBlock->BPB_RsvdSecCnt + BLOCK_ID(uiSectorID) + 1, 1, 
									(byte*)pTable),
					DeviceDrive_SUCCESS, FSManager_FAILURE) ;

	pTable[ BLOCK_OFFSET(uiSectorID) ] = uiValue ;

	RETURN_X_IF_NOT(DeviceDrive_Write(pDriveInfo, 
									pFSBootBlock->BPB_RsvdSecCnt + BLOCK_ID(uiSectorID) + 1, 1, 
									(byte*)pTable), 
					DeviceDrive_SUCCESS, FSManager_FAILURE) ;

	return FSManager_SUCCESS ;
}

static void FSManager_UpdateUsedSectors(DriveInfo* pDriveInfo, unsigned uiSectorEntryValue)
{
	if(uiSectorEntryValue == EOC)
		pDriveInfo->FSMountInfo.FSBootBlock.uiUsedSectors++ ;
	else if(uiSectorEntryValue == 0)
		pDriveInfo->FSMountInfo.FSBootBlock.uiUsedSectors-- ;
}

/**************************************************************************************/

byte FSManager_Mount(DriveInfo* pDriveInfo)
{
	return FSManager_LoadFreeSectors(pDriveInfo) ;
}

byte FSManager_UnMount(DriveInfo* pDriveInfo)
{
	return FSManager_FlushEntries(pDriveInfo, pDriveInfo->uiNoOfSectorsInTableCache) ;
}

byte FSManager_GetSectorEntryValue(DriveInfo* pDriveInfo, const unsigned uiSectorID, unsigned* uiSectorEntryValue, byte bFromCahceOnly)
{
	if(!FileSystem_IsTableCacheEnabled(pDriveInfo))
	{
		return FSManager_GetSectorEntryValueDirect(const_cast<DriveInfo*>(pDriveInfo), uiSectorID, uiSectorEntryValue) ;
	}
	
	FileSystem_MountInfo* pFSMountInfo = (FileSystem_MountInfo*)&pDriveInfo->FSMountInfo ;
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

	RETURN_IF_NOT(bStatus, FSManager_AddSectorEntryIntoCache(pDriveInfo, uiSectorID), FSManager_SUCCESS) ;
	
	RETURN_IF_NOT(bStatus, FSManager_GetSectorEntryValue(pDriveInfo, uiSectorID, uiSectorEntryValue, true), FSManager_SUCCESS) ;
	
	return FSManager_SUCCESS ;
}

byte FSManager_SetSectorEntryValue(DriveInfo* pDriveInfo, const unsigned uiSectorID, unsigned uiSectorEntryValue, byte bFromCahceOnly)
{
	byte bStatus ;

	if(!FileSystem_IsTableCacheEnabled(pDriveInfo))
	{
		RETURN_IF_NOT(bStatus, FSManager_SetSectorEntryValueDirect(pDriveInfo, uiSectorID, uiSectorEntryValue),
						FSManager_SUCCESS) ;

		FSManager_UpdateUsedSectors(pDriveInfo, uiSectorEntryValue) ;

		return FSManager_SUCCESS ;
	}
	
	if(!bFromCahceOnly)
	{
		FSManager_UpdateUsedSectors(pDriveInfo, uiSectorEntryValue) ;
	}

	FileSystem_MountInfo* pFSMountInfo = (FileSystem_MountInfo*)&pDriveInfo->FSMountInfo ;
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
	
	RETURN_IF_NOT(bStatus, FSManager_AddSectorEntryIntoCache(pDriveInfo, uiSectorID), FSManager_SUCCESS) ;

	RETURN_IF_NOT(bStatus, FSManager_SetSectorEntryValue(pDriveInfo, uiSectorID, uiSectorEntryValue, true), FSManager_SUCCESS) ;
		
	return FSManager_SUCCESS ;
}

byte FSManager_AllocateSector(DriveInfo* pDriveInfo, unsigned* uiFreeSectorID)
{
	byte bStatus ;

	if(!FileSystem_IsFreePoolCacheEnabled(pDriveInfo))
	{
		RETURN_IF_NOT(bStatus, FSManager_GetFreeSector(pDriveInfo, uiFreeSectorID), FSManager_SUCCESS) ;
	}
	else
	{
		DSUtil_Queue* pFreePoolQueue = &pDriveInfo->FSMountInfo.FreePoolQueue ;

		if(DSUtil_ReadFromQueue(pFreePoolQueue, uiFreeSectorID) == DSUtil_ERR_QUEUE_EMPTY)
		{
			RETURN_IF_NOT(bStatus, FSManager_LoadFreeSectors(pDriveInfo), FSManager_SUCCESS) ;
			if(DSUtil_ReadFromQueue(pFreePoolQueue, uiFreeSectorID) != DSUtil_SUCCESS)
				return FSManager_FAILURE ;
		}
	}

	RETURN_IF_NOT(bStatus, FSManager_SetSectorEntryValue(pDriveInfo, *uiFreeSectorID, EOC, false), FSManager_SUCCESS) ;

	return FSManager_SUCCESS ;
}

void display_cahce(DriveInfo* pDriveInfo)
{
	int i ;
	KC::MDisplay().Message("\nSTART\n", ' ') ;
	for(i = 0; i < pDriveInfo->FSMountInfo.FSTableCache.iSize; i++)
	{
		KC::MDisplay().Address(", ", pDriveInfo->FSMountInfo.FSTableCache.pSectorBlockEntryList[i].uiBlockID) ;
	}
	KC::MDisplay().Address(" :: SIZE = ", pDriveInfo->FSMountInfo.FSTableCache.iSize) ;
}
