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
#include <PartitionManager.h>
#include <ATADrive.h>
#include <DMM.h>
#include <MemUtil.h>

#define SI_EXT	15
#define SI_EMTY	0x01
#define DEF_START_SEC 63

#define NO_OF_PARTITIONS 4
/****************************** Static Functions ******************************************/
static void PartitionManager_PopulatePrimaryPartitionEntry(RawDiskDrive* pDisk, PartitionInfo* pPartitionEntry, unsigned uiLBAStartSector, unsigned uiLBAEndSector, byte bIsActive, byte bIsExt)
{
	if(bIsActive)
		pPartitionEntry->BootIndicator = 0x80 ;
	else
		pPartitionEntry->BootIndicator = 0x00 ;

	if(bIsExt && !bIsActive)
		pPartitionEntry->SystemIndicator = SI_EXT ;
	else
		pPartitionEntry->SystemIndicator = SI_EMTY ;

	pPartitionEntry->LBAStartSector = uiLBAStartSector ;
	pPartitionEntry->LBANoOfSectors = uiLBAEndSector - uiLBAStartSector + 1 ;
	
	int iSectorsPerTrack, iHeadsPerCynlider ;
	switch(pDisk->iType)
	{
		case ATA_HARD_DISK:
				iSectorsPerTrack = ((ATAPort*)pDisk->pDevice)->id.usSectors ;
				iHeadsPerCynlider = ((ATAPort*)pDisk->pDevice)->id.usHead ;

				pPartitionEntry->StartCylinder = (uiLBAStartSector / iSectorsPerTrack) / iHeadsPerCynlider ;
				pPartitionEntry->StartHead = (uiLBAStartSector / iSectorsPerTrack) % iHeadsPerCynlider ;
				pPartitionEntry->StartSector = (uiLBAStartSector % iSectorsPerTrack) + 1 ;
				
				pPartitionEntry->EndCylinder = (uiLBAEndSector / iSectorsPerTrack) / iHeadsPerCynlider ;
				pPartitionEntry->EndHead = (uiLBAEndSector / iSectorsPerTrack) % iHeadsPerCynlider ;
				pPartitionEntry->EndSector = (uiLBAEndSector % iSectorsPerTrack) + 1 ;
				break ;

		default:
				pPartitionEntry->StartCylinder = 254 ;
				pPartitionEntry->StartHead = 255 ;
				pPartitionEntry->StartSector = 255 ;
				
				pPartitionEntry->EndCylinder = 254 ;
				pPartitionEntry->EndHead = 255 ;
				pPartitionEntry->EndSector = 255 ;
				break ;
	}
}

static void PartitionManager_PopulateExtPartitionEntry(PartitionInfo* pPartitionEntry, unsigned uiLBAStartSector, unsigned uiLBANoOfSectors)
{
	pPartitionEntry->BootIndicator = 0x00 ;
	pPartitionEntry->SystemIndicator = 0x05 ;

	pPartitionEntry->StartCylinder = 254 ;
	pPartitionEntry->StartHead = 255 ;
	pPartitionEntry->StartSector = 255 ;
    
	pPartitionEntry->EndCylinder = 254 ;
	pPartitionEntry->EndHead = 255 ;
	pPartitionEntry->EndSector = 255 ;

	pPartitionEntry->LBAStartSector = uiLBAStartSector ;
	pPartitionEntry->LBANoOfSectors = uiLBANoOfSectors ;
}

static byte PartitionManager_IsEmptyPartitionEntry(PartitionInfo* pPartitionEntry)
{
	return (pPartitionEntry->SystemIndicator == 0x0 || pPartitionEntry->LBANoOfSectors == 0x0) ;
}

static byte PartitionManager_VerifyMBR(RawDiskDrive* pDisk, const PartitionInfo* pPartitionInfo)
{
	unsigned i ;
	unsigned uiActivePartitionCount = 0 ;
	byte bPartitioned = false ;
	unsigned uiPrevEndLBASector = 0 ;
	unsigned uiSectorCount = 0 ;

	for(i = 0; i < MAX_NO_OF_PRIMARY_PARTITIONS; i++)
	{
		if(pPartitionInfo[i].BootIndicator == 0x80)
			uiActivePartitionCount++ ;

		if(!PartitionManager_IsEmptyPartitionEntry((PartitionInfo*)&pPartitionInfo[i]))
		{
			if(pPartitionInfo[i].LBAStartSector > uiPrevEndLBASector)
			{
				uiPrevEndLBASector = pPartitionInfo[i].LBAStartSector + pPartitionInfo[i].LBANoOfSectors - 1 ;
				uiSectorCount += pPartitionInfo[i].LBANoOfSectors ;
				bPartitioned = true ;
			}
			else
			{
				bPartitioned = false ;
				break ;
			}
		}
		else
			break ; 
	}

	unsigned uiSectorLimit = pDisk->uiSizeInSectors ;

	if(bPartitioned == false || uiActivePartitionCount != 1 || uiSectorCount > uiSectorLimit)
		return PartitionManager_FAILURE ;

	return PartitionManager_SUCCESS ;
}

static byte PartitionManager_ReadPrimaryPartition(RawDiskDrive* pDisk, PartitionTable* pPartitionTable)
{
	byte bBootSectorBuffer[512] ;

	RETURN_X_IF_NOT(DeviceDrive_RawDiskRead(pDisk, 0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	PartitionInfo* pPartitionInfo = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;
	
	RETURN_X_IF_NOT(PartitionManager_VerifyMBR(pDisk, pPartitionInfo), PartitionManager_SUCCESS, PartitionManager_ERR_NOT_PARTITIONED) ;

	unsigned i ;
	unsigned uiPartitionCount = 0 ;
	for(i = 0; i < MAX_NO_OF_PRIMARY_PARTITIONS; i++)
	{
		if(PartitionManager_IsEmptyPartitionEntry(&pPartitionInfo[i]))
			break ;

		if(pPartitionInfo[i].SystemIndicator == SI_EXT)
		{
			pPartitionTable->bIsExtPartitionPresent = true ;

			MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&pPartitionInfo[i], MemUtil_GetDS(), (unsigned)&(pPartitionTable->ExtPartitionEntry), sizeof(PartitionInfo)) ;
			continue ;
		}

		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&pPartitionInfo[i], MemUtil_GetDS(), (unsigned)&(pPartitionTable->PrimaryParitions[uiPartitionCount]), sizeof(PartitionInfo)) ;

		uiPartitionCount++ ;
	}

	pPartitionTable->uiNoOfPrimaryPartitions = uiPartitionCount ;

	return PartitionManager_SUCCESS ;
}

static byte PartitionManager_ReadExtPartition(RawDiskDrive* pDisk, PartitionTable* pPartitionTable)
{
	if(pPartitionTable->bIsExtPartitionPresent == false)
	{	
		pPartitionTable->uiNoOfExtPartitions = 0 ;
		return PartitionManager_SUCCESS ;
	}

	unsigned uiExtSectorID, uiStartExtSectorID ;
	PartitionInfo* pPartitionInfo ;
	unsigned uiPartitionCount = 0 ;

	byte bBootSectorBuffer[512] ;

	uiExtSectorID = uiStartExtSectorID = pPartitionTable->ExtPartitionEntry.LBAStartSector ;

	for(;;)
	{
		RETURN_X_IF_NOT(DeviceDrive_RawDiskRead(pDisk, uiExtSectorID, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

		pPartitionInfo = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;
	
		if(PartitionManager_IsEmptyPartitionEntry(&pPartitionInfo[0]))
			break ;

		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&pPartitionInfo[0], MemUtil_GetDS(), (unsigned)&(pPartitionTable->ExtPartitions[uiPartitionCount].CurrentPartition), sizeof(PartitionInfo)) ;

		pPartitionTable->ExtPartitions[uiPartitionCount].uiActualStartSector = uiExtSectorID ;

		if(PartitionManager_IsEmptyPartitionEntry(&pPartitionInfo[1]))
		{
			uiPartitionCount++ ;
			break ;
		}

		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&pPartitionInfo[1], MemUtil_GetDS(), (unsigned)&(pPartitionTable->ExtPartitions[uiPartitionCount].NextPartition), sizeof(PartitionInfo)) ;

		uiExtSectorID = uiStartExtSectorID + pPartitionInfo[1].LBAStartSector ;

		uiPartitionCount++ ;
	}

	pPartitionTable->uiNoOfExtPartitions = uiPartitionCount ;

	return PartitionManager_SUCCESS ;
}

/******************************************************************************************/

void PartitionManager_InitializePartitionTable(PartitionTable* pPartitionTable)
{
	pPartitionTable->uiNoOfPrimaryPartitions = 0 ;
	pPartitionTable->bIsExtPartitionPresent = false ;
	pPartitionTable->uiNoOfExtPartitions = 0 ;
}

byte PartitionManager_ReadPartitionInfo(RawDiskDrive* pDisk, PartitionTable* pPartitionTable)
{
	byte bStatus ;

	PartitionManager_InitializePartitionTable(pPartitionTable) ;

	RETURN_IF_NOT(bStatus, PartitionManager_ReadPrimaryPartition(pDisk, pPartitionTable), PartitionManager_SUCCESS) ;

	RETURN_IF_NOT(bStatus, PartitionManager_ReadExtPartition(pDisk, pPartitionTable), PartitionManager_SUCCESS) ;

	return PartitionManager_SUCCESS ;
}

byte PartitionManager_ClearPartitionTable(RawDiskDrive* pDisk)
{
	byte bBootSectorBuffer[512] ;

	RETURN_X_IF_NOT(DeviceDrive_RawDiskRead(pDisk, 0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	PartitionInfo* pPartitionTable = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;
	
	MemUtil_Set((byte*)pPartitionTable, 0, sizeof(PartitionInfo) * MAX_NO_OF_PRIMARY_PARTITIONS) ;

	bBootSectorBuffer[510] = 0x55 ;
	bBootSectorBuffer[511] = 0xAA ;

	RETURN_X_IF_NOT(DeviceDrive_RawDiskWrite(pDisk, 0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	return PartitionManager_SUCCESS ;
}

byte PartitionManager_CreatePrimaryPartitionEntry(RawDiskDrive* pDisk, PartitionTable* pPartitionTable, unsigned uiSizeInSectors, byte bIsActive, byte bIsExt)
{
	if(pPartitionTable->uiNoOfPrimaryPartitions == MAX_NO_OF_PRIMARY_PARTITIONS)
		return PartitionManager_ERR_PRIMARY_PARTITION_FULL ;

	if(pPartitionTable->bIsExtPartitionPresent == true)
		return PartitionManager_ERR_EXT_PARTITION_BLOCKED ;

	if(pPartitionTable->uiNoOfPrimaryPartitions == 0)
	{
		if(bIsExt)
			return PartitionManager_ERR_NO_PRIM_PART ;
	
		bIsActive = true ;
	}

	unsigned uiUsedSizeInSectors = DEF_START_SEC ;

	if(pPartitionTable->uiNoOfPrimaryPartitions > 0)
		uiUsedSizeInSectors = pPartitionTable->PrimaryParitions[ pPartitionTable->uiNoOfPrimaryPartitions - 1].LBAStartSector + pPartitionTable->PrimaryParitions[ pPartitionTable->uiNoOfPrimaryPartitions - 1].LBANoOfSectors ;
		
	unsigned uiTotalSizeInSectors = pDisk->uiSizeInSectors ;

	if(uiTotalSizeInSectors >= uiUsedSizeInSectors)
		if((uiTotalSizeInSectors - uiUsedSizeInSectors) < uiSizeInSectors)
			return PartitionManager_ERR_INSUF_SPACE ;

	unsigned uiLBAStartSector = uiUsedSizeInSectors ;
	unsigned uiLBAEndSector = uiUsedSizeInSectors + uiSizeInSectors - 1 ;

	byte bBootSectorBuffer[512] ;

	RETURN_X_IF_NOT(DeviceDrive_RawDiskRead(pDisk, 0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	bBootSectorBuffer[510] = 0x55 ;
	bBootSectorBuffer[511] = 0xAA ;

	PartitionInfo* pRealPartitionTableEntry = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

	unsigned i ;
	if(bIsActive)
	{
		for(i = 0; i < pPartitionTable->uiNoOfPrimaryPartitions; i++)
		{
			pPartitionTable->PrimaryParitions[i].BootIndicator = 0 ;
			pRealPartitionTableEntry[i].BootIndicator = 0 ;
		}
	}

	PartitionManager_PopulatePrimaryPartitionEntry(pDisk, &(pPartitionTable->PrimaryParitions[pPartitionTable->uiNoOfPrimaryPartitions]), uiLBAStartSector, uiLBAEndSector, bIsActive, bIsExt) ;

	PartitionManager_PopulatePrimaryPartitionEntry(pDisk, &(pRealPartitionTableEntry[pPartitionTable->uiNoOfPrimaryPartitions]), uiLBAStartSector, uiLBAEndSector, bIsActive, bIsExt) ;

	RETURN_X_IF_NOT(DeviceDrive_RawDiskWrite(pDisk, 0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	pPartitionTable->uiNoOfPrimaryPartitions++ ;
	
	return PartitionManager_SUCCESS ;
}

byte PartitionManager_CreateExtPartitionEntry(RawDiskDrive* pDisk, PartitionTable* pPartitionTable, unsigned uiSizeInSectors)
{
	if(pPartitionTable->uiNoOfExtPartitions == MAX_NO_OF_EXT_PARTITIONS)
		return PartitionManager_ERR_EXT_PARTITION_FULL ;

	if(pPartitionTable->bIsExtPartitionPresent == false)
		return PartitionManager_ERR_NO_EXT_PARTITION ;

	unsigned uiUsedSizeInSectors = 0 ;
	if(pPartitionTable->uiNoOfExtPartitions > 0)
	{
		uiUsedSizeInSectors = pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions - 1].uiActualStartSector + pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions - 1].CurrentPartition.LBAStartSector + pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions - 1].CurrentPartition.LBANoOfSectors - pPartitionTable->ExtPartitionEntry.LBAStartSector ;
	}

	unsigned uiTotalSizeInSectors = pPartitionTable->ExtPartitionEntry.LBANoOfSectors ;

	if(uiTotalSizeInSectors >= uiUsedSizeInSectors)
		if((uiTotalSizeInSectors - uiUsedSizeInSectors) < uiSizeInSectors)
			return PartitionManager_ERR_INSUF_SPACE ;
	
	unsigned uiLBAStartSector = DEF_START_SEC ;
	unsigned uiLBANoOfSectors = uiSizeInSectors - DEF_START_SEC ;
	
	PartitionManager_PopulateExtPartitionEntry(&(pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions].CurrentPartition), uiLBAStartSector, uiLBANoOfSectors) ;
	MemUtil_Set((byte*)&(pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions].NextPartition), 0, sizeof(PartitionInfo)) ;

	byte bBootSectorBuffer[512] ;

	unsigned uiNewPartitionSector = uiUsedSizeInSectors + pPartitionTable->ExtPartitionEntry.LBAStartSector ;
	pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions].uiActualStartSector = uiNewPartitionSector ;

	RETURN_X_IF_NOT(DeviceDrive_RawDiskRead(pDisk, uiNewPartitionSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	bBootSectorBuffer[510] = 0x55 ;
	bBootSectorBuffer[511] = 0xAA ;

	PartitionInfo* pRealPartitionTableEntry = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

	PartitionManager_PopulateExtPartitionEntry(&(pRealPartitionTableEntry[0]), uiLBAStartSector, uiLBANoOfSectors) ;
	MemUtil_Set((byte*)&(pRealPartitionTableEntry[1]), 0, sizeof(PartitionInfo)) ;

	RETURN_X_IF_NOT(DeviceDrive_RawDiskWrite(pDisk, uiNewPartitionSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	if(pPartitionTable->uiNoOfExtPartitions > 0)
	{
		PartitionManager_PopulateExtPartitionEntry(&(pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions - 1].NextPartition), uiLBAStartSector, uiLBANoOfSectors) ;

		unsigned uiPreviousExtPartitionStartSector ;

		uiPreviousExtPartitionStartSector = pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions - 1].uiActualStartSector ;

		RETURN_X_IF_NOT(DeviceDrive_RawDiskRead(pDisk, uiPreviousExtPartitionStartSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

		pRealPartitionTableEntry = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

		uiLBAStartSector = uiUsedSizeInSectors ;
		uiLBANoOfSectors = uiSizeInSectors ;

		PartitionManager_PopulateExtPartitionEntry(&(pRealPartitionTableEntry[1]), uiLBAStartSector, uiLBANoOfSectors) ;
		PartitionManager_PopulateExtPartitionEntry(&(pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions - 1].NextPartition), uiLBAStartSector, uiLBANoOfSectors) ;

		RETURN_X_IF_NOT(DeviceDrive_RawDiskWrite(pDisk, uiPreviousExtPartitionStartSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
	}

	pPartitionTable->uiNoOfExtPartitions++ ;
	
	return PartitionManager_SUCCESS ;
}

byte PartitionManager_DeletePrimaryPartition(RawDiskDrive* pDisk, PartitionTable* pPartitionTable)
{
	if(pPartitionTable->uiNoOfPrimaryPartitions == 0)
		return PartitionManager_ERR_PARTITION_TABLE_EMPTY ;
	
	byte bBootSectorBuffer[512] ;
	RETURN_X_IF_NOT(DeviceDrive_RawDiskRead(pDisk, 0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	PartitionInfo* pPartitionTableEntry ;
	PartitionInfo* pRealPartitionTableEntry ;

	if(pPartitionTable->bIsExtPartitionPresent)
	{
		pPartitionTableEntry = &pPartitionTable->ExtPartitionEntry ;

		pRealPartitionTableEntry = &(((PartitionInfo*)(bBootSectorBuffer + 0x1BE))[pPartitionTable->uiNoOfPrimaryPartitions]) ;
		unsigned i, uiExtSector ;
		byte bExtBootSectorBuffer[512] ;
		PartitionInfo* pExtRealPartitionTable ;

		for(i = 0; i < pPartitionTable->uiNoOfExtPartitions; i++)
		{
			uiExtSector = pPartitionTable->ExtPartitions[i].uiActualStartSector ;
			RETURN_X_IF_NOT(DeviceDrive_RawDiskRead(pDisk, uiExtSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
			pExtRealPartitionTable = ((PartitionInfo*)(bExtBootSectorBuffer + 0x1BE)) ;

			MemUtil_Set((byte*)&pPartitionTable->ExtPartitions[i].CurrentPartition, 0, sizeof(PartitionInfo)) ;
			MemUtil_Set((byte*)&pPartitionTable->ExtPartitions[i].NextPartition, 0, sizeof(PartitionInfo)) ;
			MemUtil_Set((byte*)&pExtRealPartitionTable[0], 0, sizeof(PartitionInfo)) ;
			MemUtil_Set((byte*)&pExtRealPartitionTable[1], 0, sizeof(PartitionInfo)) ;

			RETURN_X_IF_NOT(DeviceDrive_RawDiskWrite(pDisk, uiExtSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
		}

		pPartitionTable->uiNoOfExtPartitions = 0 ;
		pPartitionTable->bIsExtPartitionPresent = false ;
	}
	else
	{
		pPartitionTableEntry = &pPartitionTable->PrimaryParitions[pPartitionTable->uiNoOfPrimaryPartitions - 1] ;
		pRealPartitionTableEntry = &(((PartitionInfo*)(bBootSectorBuffer + 0x1BE))[pPartitionTable->uiNoOfPrimaryPartitions - 1]) ;
		
		pPartitionTable->uiNoOfPrimaryPartitions-- ;
	}

	MemUtil_Set((byte*)pPartitionTableEntry, 0, sizeof(PartitionInfo)) ;
	MemUtil_Set((byte*)pRealPartitionTableEntry, 0, sizeof(PartitionInfo)) ;

	RETURN_X_IF_NOT(DeviceDrive_RawDiskWrite(pDisk, 0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	return PartitionManager_SUCCESS ;
}

byte PartitionManager_DeleteExtPartition(RawDiskDrive* pDisk, PartitionTable* pPartitionTable)
{
	if(pPartitionTable->uiNoOfExtPartitions == 0 || pPartitionTable->bIsExtPartitionPresent == false)
		return PartitionManager_ERR_ETX_PARTITION_TABLE_EMPTY ;
	
	byte bExtBootSectorBuffer[512] ;

	unsigned uiCurrentExtPartitionStartSector ;
	uiCurrentExtPartitionStartSector = pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions - 1].uiActualStartSector ;

	if(pPartitionTable->uiNoOfExtPartitions > 1)
	{
		unsigned uiPreviousExtPartitionStartSector ;
		uiPreviousExtPartitionStartSector = pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions - 2].uiActualStartSector ;

		RETURN_X_IF_NOT(DeviceDrive_RawDiskRead(pDisk, uiPreviousExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

		PartitionInfo* pRealPartitionEntry = &((PartitionInfo*)(bExtBootSectorBuffer + 0x1BE))[1] ;
		MemUtil_Set((byte*)(pRealPartitionEntry), 0, sizeof(PartitionInfo)) ;

		MemUtil_Set((byte*)&pPartitionTable->ExtPartitions[pPartitionTable->uiNoOfExtPartitions - 2].NextPartition, 0, sizeof(PartitionInfo)) ;

		RETURN_X_IF_NOT(DeviceDrive_RawDiskWrite(pDisk, uiPreviousExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
	}

	RETURN_X_IF_NOT(DeviceDrive_RawDiskRead(pDisk, uiCurrentExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	MemUtil_Set((byte*)(bExtBootSectorBuffer + 0x1BE), 0, sizeof(PartitionInfo) * 2) ;

	RETURN_X_IF_NOT(DeviceDrive_RawDiskWrite(pDisk, uiCurrentExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	pPartitionTable->uiNoOfExtPartitions-- ;

	return PartitionManager_SUCCESS ;
}

byte PartitionManager_UpdateSystemIndicator(RawDiskDrive* pDisk, unsigned uiLBAStartSector, unsigned uiSystemIndicator)
{
	byte bBootSectorBuffer[512] ;

	RETURN_X_IF_NOT(DeviceDrive_RawDiskRead(pDisk, 0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	PartitionInfo* pPartitionTable = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;
	
	unsigned i ;
	for(i = 0; i < NO_OF_PARTITIONS; i++)
	{
		if(pPartitionTable[i].LBAStartSector == uiLBAStartSector)
		{
			pPartitionTable[i].SystemIndicator = uiSystemIndicator ;
			break ;
		}
	}

	RETURN_X_IF_NOT(DeviceDrive_RawDiskWrite(pDisk, 0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	return PartitionManager_SUCCESS ;
}

