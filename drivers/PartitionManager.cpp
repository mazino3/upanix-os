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

#define SI_EXT	5
#define SI_EMTY	0x01
#define DEF_START_SEC 63

PartitionInfo::PartitionInfo(unsigned lbaStart, unsigned size, PartitionInfo::PartitionTypes type) :
  BootIndicator(type == ACTIVE ? 0x80 : 0x00),
  _StartHead(255),
  _StartSector(255),
  _StartCylinder(254),
  SystemIndicator(type == EXTENEDED ? SI_EXT : SI_EMTY),
  _EndHead(255),
  _EndSector(255),
  _EndCylinder(254),
	LBAStartSector(lbaStart),
	LBANoOfSectors(size)
{
	
/*
		case ATA_HARD_DISK:
				iSectorsPerTrack = ((ATAPort*)pDisk->Device())->id.usSectors ;
				iHeadsPerCynlider = ((ATAPort*)pDisk->Device())->id.usHead ;

				StartCylinder = (uiLBAStartSector / iSectorsPerTrack) / iHeadsPerCynlider ;
				StartHead = (uiLBAStartSector / iSectorsPerTrack) % iHeadsPerCynlider ;
				StartSector = (uiLBAStartSector % iSectorsPerTrack) + 1 ;
				
				EndCylinder = (uiLBAEndSector / iSectorsPerTrack) / iHeadsPerCynlider ;
				EndHead = (uiLBAEndSector / iSectorsPerTrack) % iHeadsPerCynlider ;
				EndSector = (uiLBAEndSector % iSectorsPerTrack) + 1 ;
				break ;
*/
}

byte PartitionTable::VerifyMBR(RawDiskDrive* pDisk, const PartitionInfo* pPartitionInfo) const
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

		if(!pPartitionInfo[i].IsEmpty())
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
	
	RETURN_X_IF_NOT(VerifyMBR(pDisk, pPartitionInfo), PartitionManager_SUCCESS, PartitionManager_ERR_NOT_PARTITIONED) ;

	for(unsigned i = 0; i < MAX_NO_OF_PRIMARY_PARTITIONS; i++)
	{
		if(pPartitionInfo[i].IsEmpty())
			break ;

		if(pPartitionInfo[i].SystemIndicator == SI_EXT)
		{
			_bIsExtPartitionPresent = true ;
      _extPartitionEntry = pPartitionInfo[i];
		}
    else
    {
      _primaryPartitions.push_back(new PartitionInfo(pPartitionInfo[i]));
    }
	}

	return PartitionManager_SUCCESS ;
}

byte PartitionTable::ReadExtPartition(RawDiskDrive* pDisk)
{
	if(!_bIsExtPartitionPresent)
		return PartitionManager_SUCCESS ;

	unsigned uiExtSectorID, uiStartExtSectorID ;
	PartitionInfo* pPartitionInfo ;

	byte bBootSectorBuffer[512] ;

	uiExtSectorID = uiStartExtSectorID = _extPartitionEntry.LBAStartSector ;

	for(;;)
	{
		RETURN_X_IF_NOT(pDisk->Read(uiExtSectorID, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

		pPartitionInfo = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;
	
		if(pPartitionInfo[0].IsEmpty())
			break ;

    _extPartitions.push_back(new ExtPartitionTable(pPartitionInfo[0], uiExtSectorID));

		if(pPartitionInfo[1].IsEmpty())
			break;

		uiExtSectorID = uiStartExtSectorID + pPartitionInfo[1].LBAStartSector ;
	}

	return PartitionManager_SUCCESS ;
}

/******************************************************************************************/

PartitionTable::PartitionTable() : _bIsExtPartitionPresent(false)
{
}

PartitionTable::~PartitionTable()
{
  for(auto pp : _primaryPartitions)
    delete pp;
  for(auto ep : _extPartitions)
    delete ep;
}

byte PartitionTable::CreatePrimaryPartitionEntry(RawDiskDrive* pDisk, unsigned uiSizeInSectors, PartitionInfo::PartitionTypes type)
{
	if(_primaryPartitions.size() == MAX_NO_OF_PRIMARY_PARTITIONS)
		return PartitionManager_ERR_PRIMARY_PARTITION_FULL ;

	if(_bIsExtPartitionPresent)
		return PartitionManager_ERR_EXT_PARTITION_BLOCKED ;

	if(_primaryPartitions.empty())
	{
		if(type == PartitionInfo::EXTENEDED)
			return PartitionManager_ERR_NO_PRIM_PART ;
	
    type = PartitionInfo::ACTIVE;
	}

	unsigned uiUsedSizeInSectors = DEF_START_SEC ;

	if(!_primaryPartitions.empty())
  {
    auto lastPP = _primaryPartitions.rbegin();
		uiUsedSizeInSectors = lastPP->LBAStartSector + lastPP->LBANoOfSectors;
  }
		
	unsigned uiTotalSizeInSectors = pDisk->SizeInSectors();

	if(uiTotalSizeInSectors >= uiUsedSizeInSectors)
		if((uiTotalSizeInSectors - uiUsedSizeInSectors) < uiSizeInSectors)
			return PartitionManager_ERR_INSUF_SPACE ;

	unsigned uiLBAStartSector = uiUsedSizeInSectors ;

	byte bBootSectorBuffer[512] ;

	RETURN_X_IF_NOT(pDisk->Read(0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	bBootSectorBuffer[510] = 0x55 ;
	bBootSectorBuffer[511] = 0xAA ;

	PartitionInfo* pRealPartitionTableEntry = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

	if(type == PartitionInfo::ACTIVE)
	{
    unsigned i = 0;
		for(auto pp : _primaryPartitions)
		{
      pp->BootIndicator = 0;
			pRealPartitionTableEntry[i++].BootIndicator = 0;
		}
	}

  PartitionInfo* pi = new PartitionInfo(uiLBAStartSector, uiSizeInSectors, type);
  pRealPartitionTableEntry[_primaryPartitions.size()] = *pi;
  _primaryPartitions.push_back(pi);

	RETURN_X_IF_NOT(pDisk->Write(0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	return PartitionManager_SUCCESS ;
}

byte PartitionTable::CreateExtPartitionEntry(RawDiskDrive* pDisk, unsigned uiSizeInSectors)
{
	if(!_bIsExtPartitionPresent)
		return PartitionManager_ERR_NO_EXT_PARTITION ;

  auto lastExtPartition = _extPartitions.rbegin();
	unsigned uiUsedSizeInSectors = 0 ;
	if(lastExtPartition != _extPartitions.rend())
	{
		uiUsedSizeInSectors = lastExtPartition->ActualStartSector()
      + lastExtPartition->CurrentPartition().LBAStartSector 
      + lastExtPartition->CurrentPartition().LBANoOfSectors 
      - _extPartitionEntry.LBAStartSector ;
	}

	const unsigned uiTotalSizeInSectors = _extPartitionEntry.LBANoOfSectors ;

	if(uiTotalSizeInSectors >= uiUsedSizeInSectors)
		if((uiTotalSizeInSectors - uiUsedSizeInSectors) < uiSizeInSectors)
			return PartitionManager_ERR_INSUF_SPACE ;
	
	const unsigned uiLBAStartSector = DEF_START_SEC ;
	const unsigned uiLBANoOfSectors = uiSizeInSectors - DEF_START_SEC ;
	const unsigned uiNewPartitionSector = uiUsedSizeInSectors + _extPartitionEntry.LBAStartSector ;

	byte bBootSectorBuffer[512] ;
  const PartitionInfo extPartitionInfo(uiLBAStartSector, uiLBANoOfSectors, PartitionInfo::NORMAL);

  _extPartitions.push_back(new ExtPartitionTable(extPartitionInfo, uiNewPartitionSector));

	RETURN_X_IF_NOT(pDisk->Read(uiNewPartitionSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	bBootSectorBuffer[510] = 0x55 ;
	bBootSectorBuffer[511] = 0xAA ;

	PartitionInfo* pRealPartitionTableEntry = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

	pRealPartitionTableEntry[0] = extPartitionInfo;
	MemUtil_Set((byte*)&(pRealPartitionTableEntry[1]), 0, sizeof(PartitionInfo)) ;

	RETURN_X_IF_NOT(pDisk->Write(uiNewPartitionSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	if(lastExtPartition != _extPartitions.rend())
	{
		const auto uiPreviousExtPartitionStartSector = lastExtPartition->ActualStartSector();

		RETURN_X_IF_NOT(pDisk->Read(uiPreviousExtPartitionStartSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

		pRealPartitionTableEntry = ((PartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

    const PartitionInfo nextPartitionInfo(uiUsedSizeInSectors, uiSizeInSectors, PartitionInfo::EXTENEDED);
		pRealPartitionTableEntry[1] = nextPartitionInfo;

		RETURN_X_IF_NOT(pDisk->Write(uiPreviousExtPartitionStartSector, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
	}

	return PartitionManager_SUCCESS ;
}

byte PartitionTable::DeletePrimaryPartition(RawDiskDrive* pDisk)
{
	if(_primaryPartitions.empty())
		return PartitionManager_ERR_PARTITION_TABLE_EMPTY ;
	
	byte bBootSectorBuffer[512] ;
	RETURN_X_IF_NOT(pDisk->Read(0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	PartitionInfo* pRealPartitionTableEntry ;

	if(_bIsExtPartitionPresent)
	{
		pRealPartitionTableEntry = &(((PartitionInfo*)(bBootSectorBuffer + 0x1BE))[_primaryPartitions.size()]);
		unsigned uiExtSector ;
		byte bExtBootSectorBuffer[512] ;
		PartitionInfo* pExtRealPartitionTable ;

    for(auto ep : _extPartitions)
		{
			uiExtSector = ep->ActualStartSector();
			RETURN_X_IF_NOT(pDisk->Read(uiExtSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
			pExtRealPartitionTable = ((PartitionInfo*)(bExtBootSectorBuffer + 0x1BE)) ;

			pExtRealPartitionTable[0] = PartitionInfo();
			pExtRealPartitionTable[1] = PartitionInfo();

			RETURN_X_IF_NOT(pDisk->Write(uiExtSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
      delete ep;
		}
    _extPartitions.clear();
    _bIsExtPartitionPresent = false;
    _extPartitionEntry = PartitionInfo();
	}
	else
	{
    _primaryPartitions.pop_back();
		pRealPartitionTableEntry = &(((PartitionInfo*)(bBootSectorBuffer + 0x1BE))[_primaryPartitions.size()]);
	}

  *pRealPartitionTableEntry = PartitionInfo();

	RETURN_X_IF_NOT(pDisk->Write(0, 1, bBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	return PartitionManager_SUCCESS ;
}

byte PartitionTable::DeleteExtPartition(RawDiskDrive* pDisk)
{
	if(_extPartitions.empty() || _bIsExtPartitionPresent == false)
		return PartitionManager_ERR_ETX_PARTITION_TABLE_EMPTY ;
	
	byte bExtBootSectorBuffer[512] ;

  auto lastExtPartition = _extPartitions.rbegin();
	const unsigned uiCurrentExtPartitionStartSector = lastExtPartition->ActualStartSector();
  ++lastExtPartition;
	if(lastExtPartition != _extPartitions.rend())
	{
		const unsigned uiPreviousExtPartitionStartSector = lastExtPartition->ActualStartSector();

		RETURN_X_IF_NOT(pDisk->Read(uiPreviousExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

		PartitionInfo* pRealPartitionEntry = &((PartitionInfo*)(bExtBootSectorBuffer + 0x1BE))[1] ;
    *pRealPartitionEntry = PartitionInfo();

		RETURN_X_IF_NOT(pDisk->Write(uiPreviousExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;
	}

	RETURN_X_IF_NOT(pDisk->Read(uiCurrentExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

	MemUtil_Set((byte*)(bExtBootSectorBuffer + 0x1BE), 0, sizeof(PartitionInfo) * 2) ;

	RETURN_X_IF_NOT(pDisk->Write(uiCurrentExtPartitionStartSector, 1, bExtBootSectorBuffer), DeviceDrive_SUCCESS, PartitionManager_FAILURE) ;

  _extPartitions.pop_back();

	return PartitionManager_SUCCESS ;
}

void PartitionTable::VerbosePrint() const
{
  printf("\n%-4s %-10s %-10s %-10s", "Boot", "Start", "End", "SysId");
  auto print = [](const PartitionInfo& pi) {
    const char* sysId = "EMPTY";
    if(pi.SystemIndicator == SI_EXT)
      sysId = "Extended";
    else if(pi.SystemIndicator == 0x93)
      sysId = "MOSFS";
    printf("\n%-4d %-10d %-10d %-10s", pi.BootIndicator, pi.LBAStartSector, pi.LBANoOfSectors, sysId);
  };

  for(const auto pp : _primaryPartitions)
    print(*pp);
  if(_bIsExtPartitionPresent)
  {
    print(_extPartitionEntry);
    for(const auto ep : _extPartitions)
      print(ep->CurrentPartition());
  }
}
