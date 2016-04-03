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
	switch(pDisk->Type())
	{
		case ATA_HARD_DISK:
				iSectorsPerTrack = ((ATAPort*)pDisk->Device())->id.usSectors ;
				iHeadsPerCynlider = ((ATAPort*)pDisk->Device())->id.usHead ;

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

	unsigned uiSectorLimit = pDisk->SizeInSectors();

	if(bPartitioned == false || uiActivePartitionCount != 1 || uiSectorCount > uiSectorLimit)
		return PartitionManager_FAILURE ;

	return PartitionManager_SUCCESS ;
}

byte PartitionTable::ReadPrimaryPartition(RawDiskDrive* pDisk)
{
	byte bBootSectorBuffer[512] ;

	RETURN_X_IF_NOT(pDisk->Read(0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	PartitionInfo* pPartitionInfo = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;
	
	RETURN_X_IF_NOT(PartitionManager_VerifyMBR(pDisk, pPartitionInfo), PartitionManager_SUCCESS, PartitionManager_ERR_NOT_PARTITIONED) ;

  _uiNoOfPrimaryPartitions = 0;
	for(unsigned i = 0; i < MAX_NO_OF_PRIMARY_PARTITIONS; i++)
	{
		if(PartitionManager_IsEmptyPartitionEntry(&pPartitionInfo[i]))
			break ;

		if(pPartitionInfo[i].SystemIndicator == SI_EXT)
		{
			_bIsExtPartitionPresent = true ;
      _extPartitionEntry = pPartitionInfo[i];
		}
    else
    {
      _primaryParitions[_uiNoOfPrimaryPartitions] = pPartitionInfo[i];
      ++_uiNoOfPrimaryPartitions;
    }
	}

	return PartitionManager_SUCCESS ;
}

byte PartitionTable::ReadExtPartition(RawDiskDrive* pDisk)
{
  _uiNoOfExtPartitions = 0;
	if(IsExtPartitionPresent() == false)
		return PartitionManager_SUCCESS ;

	unsigned uiExtSectorID, uiStartExtSectorID ;
	PartitionInfo* pPartitionInfo ;

	byte bBootSectorBuffer[512] ;

	uiExtSectorID = uiStartExtSectorID = _extPartitionEntry.LBAStartSector ;

	for(;;)
	{
		RETURN_X_IF_NOT(pDisk->Read(uiExtSectorID, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

		pPartitionInfo = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;
	
		if(PartitionManager_IsEmptyPartitionEntry(&pPartitionInfo[0]))
			break ;

		if(PartitionManager_IsEmptyPartitionEntry(&pPartitionInfo[1]))
		{
      _extPartitions[_uiNoOfExtPartitions] = ExtPartitionTable(pPartitionInfo[0], uiExtSectorID);
      ++_uiNoOfExtPartitions;
			break;
		}

    _extPartitions[_uiNoOfExtPartitions] = ExtPartitionTable(pPartitionInfo[0], pPartitionInfo[1], uiExtSectorID);
    ++_uiNoOfExtPartitions;

		uiExtSectorID = uiStartExtSectorID + pPartitionInfo[1].LBAStartSector ;
	}

	return PartitionManager_SUCCESS ;
}

/******************************************************************************************/

PartitionTable::PartitionTable() 
  : _uiNoOfPrimaryPartitions(0), 
    _uiNoOfExtPartitions(0),
    _bIsExtPartitionPresent(false)
{
}

byte PartitionTable::CreatePrimaryPartitionEntry(RawDiskDrive* pDisk, unsigned uiSizeInSectors, byte bIsActive, byte bIsExt)
{
	if(NoOfPrimaryPartitions() == MAX_NO_OF_PRIMARY_PARTITIONS)
		return PartitionManager_ERR_PRIMARY_PARTITION_FULL ;

	if(IsExtPartitionPresent() == true)
		return PartitionManager_ERR_EXT_PARTITION_BLOCKED ;

	if(NoOfPrimaryPartitions() == 0)
	{
		if(bIsExt)
			return PartitionManager_ERR_NO_PRIM_PART ;
	
		bIsActive = true ;
	}

	unsigned uiUsedSizeInSectors = DEF_START_SEC ;

	if(NoOfPrimaryPartitions() > 0)
		uiUsedSizeInSectors = _primaryParitions[NoOfPrimaryPartitions() - 1].LBAStartSector + _primaryParitions[NoOfPrimaryPartitions() - 1].LBANoOfSectors ;
		
	unsigned uiTotalSizeInSectors = pDisk->SizeInSectors();

	if(uiTotalSizeInSectors >= uiUsedSizeInSectors)
		if((uiTotalSizeInSectors - uiUsedSizeInSectors) < uiSizeInSectors)
			return PartitionManager_ERR_INSUF_SPACE ;

	unsigned uiLBAStartSector = uiUsedSizeInSectors ;
	unsigned uiLBAEndSector = uiUsedSizeInSectors + uiSizeInSectors - 1 ;

	byte bBootSectorBuffer[512] ;

	RETURN_X_IF_NOT(pDisk->Read(0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	bBootSectorBuffer[510] = 0x55 ;
	bBootSectorBuffer[511] = 0xAA ;

	PartitionInfo* pRealPartitionTableEntry = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

	unsigned i ;
	if(bIsActive)
	{
		for(i = 0; i < NoOfPrimaryPartitions(); i++)
		{
			_primaryParitions[i].BootIndicator = 0 ;
			pRealPartitionTableEntry[i].BootIndicator = 0 ;
		}
	}

	PartitionManager_PopulatePrimaryPartitionEntry(pDisk, &(_primaryParitions[NoOfPrimaryPartitions()]), uiLBAStartSector, uiLBAEndSector, bIsActive, bIsExt) ;

	PartitionManager_PopulatePrimaryPartitionEntry(pDisk, &(pRealPartitionTableEntry[NoOfPrimaryPartitions()]), uiLBAStartSector, uiLBAEndSector, bIsActive, bIsExt) ;

	RETURN_X_IF_NOT(pDisk->Write(0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

  ++_uiNoOfPrimaryPartitions;
	
	return PartitionManager_SUCCESS ;
}

byte PartitionTable::CreateExtPartitionEntry(RawDiskDrive* pDisk, unsigned uiSizeInSectors)
{
	if(NoOfExtPartitions() == MAX_NO_OF_EXT_PARTITIONS)
		return PartitionManager_ERR_EXT_PARTITION_FULL ;

	if(IsExtPartitionPresent() == false)
		return PartitionManager_ERR_NO_EXT_PARTITION ;

	unsigned uiUsedSizeInSectors = 0 ;
	if(NoOfExtPartitions() > 0)
	{
		uiUsedSizeInSectors = _extPartitions[NoOfExtPartitions() - 1].ActualStartSector()
      + _extPartitions[NoOfExtPartitions() - 1].CurrentPartition().LBAStartSector 
      + _extPartitions[NoOfExtPartitions() - 1].CurrentPartition().LBANoOfSectors 
      - _extPartitionEntry.LBAStartSector ;
	}

	unsigned uiTotalSizeInSectors = _extPartitionEntry.LBANoOfSectors ;

	if(uiTotalSizeInSectors >= uiUsedSizeInSectors)
		if((uiTotalSizeInSectors - uiUsedSizeInSectors) < uiSizeInSectors)
			return PartitionManager_ERR_INSUF_SPACE ;
	
	unsigned uiLBAStartSector = DEF_START_SEC ;
	unsigned uiLBANoOfSectors = uiSizeInSectors - DEF_START_SEC ;
	
	byte bBootSectorBuffer[512] ;

	unsigned uiNewPartitionSector = uiUsedSizeInSectors + _extPartitionEntry.LBAStartSector ;
  _extPartitions[NoOfExtPartitions()] = ExtPartitionTable(PartitionInfo(uiLBAStartSector, uiLBANoOfSectors), uiNewPartitionSector);

	RETURN_X_IF_NOT(pDisk->Read(uiNewPartitionSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	bBootSectorBuffer[510] = 0x55 ;
	bBootSectorBuffer[511] = 0xAA ;

	PartitionInfo* pRealPartitionTableEntry = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

	pRealPartitionTableEntry[0] = PartitionInfo(uiLBAStartSector, uiLBANoOfSectors);
	MemUtil_Set((byte*)&(pRealPartitionTableEntry[1]), 0, sizeof(PartitionInfo)) ;

	RETURN_X_IF_NOT(pDisk->Write(uiNewPartitionSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	if(NoOfExtPartitions() > 0)
	{
    _extPartitions[NoOfExtPartitions() - 1].NextPartition(PartitionInfo(uiLBAStartSector, uiLBANoOfSectors));

		unsigned uiPreviousExtPartitionStartSector ;

		uiPreviousExtPartitionStartSector = _extPartitions[NoOfExtPartitions() - 1].ActualStartSector();

		RETURN_X_IF_NOT(pDisk->Read(uiPreviousExtPartitionStartSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

		pRealPartitionTableEntry = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

		uiLBAStartSector = uiUsedSizeInSectors ;
		uiLBANoOfSectors = uiSizeInSectors ;

		pRealPartitionTableEntry[1] = PartitionInfo(uiLBAStartSector, uiLBANoOfSectors);
		_extPartitions[NoOfExtPartitions() - 1].NextPartition(PartitionInfo(uiLBAStartSector, uiLBANoOfSectors));

		RETURN_X_IF_NOT(pDisk->Write(uiPreviousExtPartitionStartSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
	}

  --_uiNoOfExtPartitions;
	
	return PartitionManager_SUCCESS ;
}

byte PartitionTable::DeletePrimaryPartition(RawDiskDrive* pDisk)
{
	if(NoOfPrimaryPartitions() == 0)
		return PartitionManager_ERR_PARTITION_TABLE_EMPTY ;
	
	byte bBootSectorBuffer[512] ;
	RETURN_X_IF_NOT(pDisk->Read(0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	PartitionInfo* pPartitionTableEntry ;
	PartitionInfo* pRealPartitionTableEntry ;

	if(IsExtPartitionPresent())
	{
		pPartitionTableEntry = &_extPartitionEntry ;

		pRealPartitionTableEntry = &(((PartitionInfo*)(bBootSectorBuffer + 0x1BE))[NoOfPrimaryPartitions()]) ;
		unsigned uiExtSector ;
		byte bExtBootSectorBuffer[512] ;
		PartitionInfo* pExtRealPartitionTable ;

		for(unsigned i = 0; i < NoOfExtPartitions(); i++)
		{
			uiExtSector = _extPartitions[i].ActualStartSector();
			RETURN_X_IF_NOT(pDisk->Read(uiExtSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
			pExtRealPartitionTable = ((PartitionInfo*)(bExtBootSectorBuffer + 0x1BE)) ;

			_extPartitions[i] = ExtPartitionTable();
			pExtRealPartitionTable[0] = PartitionInfo();
			pExtRealPartitionTable[1] = PartitionInfo();

			RETURN_X_IF_NOT(pDisk->Write(uiExtSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
		}

    _uiNoOfExtPartitions = 0;
    _bIsExtPartitionPresent = false;
	}
	else
	{
		pPartitionTableEntry = &_primaryParitions[NoOfPrimaryPartitions() - 1] ;
		pRealPartitionTableEntry = &(((PartitionInfo*)(bBootSectorBuffer + 0x1BE))[NoOfPrimaryPartitions() - 1]) ;
		--_uiNoOfPrimaryPartitions;
	}

	MemUtil_Set((byte*)pPartitionTableEntry, 0, sizeof(PartitionInfo)) ;
	MemUtil_Set((byte*)pRealPartitionTableEntry, 0, sizeof(PartitionInfo)) ;

	RETURN_X_IF_NOT(pDisk->Write(0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	return PartitionManager_SUCCESS ;
}

byte PartitionTable::DeleteExtPartition(RawDiskDrive* pDisk)
{
	if(NoOfExtPartitions() == 0 || IsExtPartitionPresent() == false)
		return PartitionManager_ERR_ETX_PARTITION_TABLE_EMPTY ;
	
	byte bExtBootSectorBuffer[512] ;

	unsigned uiCurrentExtPartitionStartSector ;
	uiCurrentExtPartitionStartSector = _extPartitions[NoOfExtPartitions() - 1].ActualStartSector();

	if(NoOfExtPartitions() > 1)
	{
		unsigned uiPreviousExtPartitionStartSector ;
		uiPreviousExtPartitionStartSector = _extPartitions[NoOfExtPartitions() - 2].ActualStartSector();

		RETURN_X_IF_NOT(pDisk->Read(uiPreviousExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

		PartitionInfo* pRealPartitionEntry = &((PartitionInfo*)(bExtBootSectorBuffer + 0x1BE))[1] ;
    *pRealPartitionEntry = PartitionInfo();

		_extPartitions[NoOfExtPartitions() - 2].NextPartition(PartitionInfo());

		RETURN_X_IF_NOT(pDisk->Write(uiPreviousExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
	}

	RETURN_X_IF_NOT(pDisk->Read(uiCurrentExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	MemUtil_Set((byte*)(bExtBootSectorBuffer + 0x1BE), 0, sizeof(PartitionInfo) * 2) ;

	RETURN_X_IF_NOT(pDisk->Write(uiCurrentExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

  --_uiNoOfExtPartitions;

	return PartitionManager_SUCCESS ;
}

