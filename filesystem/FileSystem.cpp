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

/* Upanix File System Support */

#include <Global.h>
#include <StringUtil.h>
#include <FileSystem.h>
#include <MemConstants.h>
#include <MemUtil.h>
#include <Display.h>
#include <DeviceDrive.h>
#include <UserManager.h>
#include <FSManager.h>
#include <SystemUtil.h>
#include <FileOperations.h>
#include <DMM.h>
#include <RTC.h>
#include <KernelUtil.h>
#include <DiskCache.h>

uint32_t FileSystem_GetSectorEntryValue(DiskDrive* pDiskDrive, const unsigned uiSectorID)
{
  const FSBootBlock& fsBootBlock = pDiskDrive->FSMountInfo.GetBootBlock();
	
  if(uiSectorID > (fsBootBlock.BPB_FSTableSize * fsBootBlock.BPB_BytesPerSec / 4))
    throw upan::exception(XLOC, "invalid cluster id: %u", uiSectorID);

  return FSManager_GetSectorEntryValue(pDiskDrive, uiSectorID, false);
}

void FileSystem_SetSectorEntryValue(DiskDrive* pDiskDrive, const unsigned uiSectorID, unsigned uiSectorEntryValue)
{
  const FSBootBlock& fsBootBlock = pDiskDrive->FSMountInfo.GetBootBlock();
	
  if(uiSectorID > (fsBootBlock.BPB_FSTableSize * fsBootBlock.BPB_BytesPerSec / 4))
    throw upan::exception(XLOC, "invalid cluster id: %u", uiSectorID);
	
  FSManager_SetSectorEntryValue(pDiskDrive, uiSectorID, uiSectorEntryValue, false);
}

uint32_t FileSystem_AllocateSector(DiskDrive* pDiskDrive)
{
  return FSManager_AllocateSector(pDiskDrive);
}

uint32_t FileSystem_DeAllocateSector(DiskDrive* pDiskDrive, unsigned uiCurrentSectorID)
{
  auto uiNextSectorID = FileSystem_GetSectorEntryValue(pDiskDrive, uiCurrentSectorID);
  FileSystem_SetSectorEntryValue(pDiskDrive, uiCurrentSectorID, 0);
  pDiskDrive->FSMountInfo.AddToFreePoolCache(uiCurrentSectorID);
  return uiNextSectorID;
}

unsigned FileSystem_GetSizeForTableCache(unsigned uiNoOfSectorsInTableCache)
{
	return ( sizeof(SectorBlockEntry) * uiNoOfSectorsInTableCache ) ;
}

/*
void FileSystem_UpdateTime(time_t* pTime)
{
	RTCTime rtcTime ;
	RTC_GetTime(&rtcTime) ;

	pTime->bHour = rtcTime.bHour ;
	pTime->bMinute = rtcTime.bMinute ;
	pTime->bSecond = rtcTime.bSecond ;
	
	pTime->bDayOfWeek_Month = (rtcTime.bDayOfWeek & 0x0F) | ((rtcTime.bMonth & 0x0F) << 4) ;
	pTime->bDayOfMonth = rtcTime.bMonth ;
	pTime->bCentury = rtcTime.bCentury ;
	pTime->bYear = rtcTime.bYear ;
}
*/

void FSBootBlock::Init(const DiskDrive& diskDrive)
{
  BPB_jmpBoot[0] = 0xEB ; /****************/
  BPB_jmpBoot[1] = 0xFE ; /* JMP $ -- ARR */
  BPB_jmpBoot[2] = 0x90 ; /****************/

  BPB_BytesPerSec = 0x200; // 512 ;
  BPB_RsvdSecCnt = 2 ;

  if(diskDrive.DeviceType() == DEV_FLOPPY)
    BPB_Media  = MEDIA_REMOVABLE ;
  else
    BPB_Media  = MEDIA_FIXED ;

  BPB_SecPerTrk = diskDrive.SectorsPerTrack();
  BPB_NumHeads = diskDrive.NoOfHeads();
  BPB_HiddSec  = 0 ;
  BPB_TotSec32 = diskDrive.SizeInSectors();

/*	pFSBootBlock->BPB_FSTableSize ; ---> Calculated */
  BPB_ExtFlags  = 0x0080 ;
  BPB_FSVer = 0x0100 ;  //version 1.0
  BPB_FSInfo  = 1 ;  //Typical Value for FSInfo Sector

  BPB_BootSig = 0x29 ;
  BPB_VolID = 0x01 ;  //TODO: Required to be set to current Date/Time of system ---- Not Mandatory
  strcpy((char*)BPB_VolLab, "No Name   ") ;  //10 + 1(\0) characters only -- ARR

  uiUsedSectors = 1 ;

  BPB_FSTableSize = (BPB_TotSec32 - BPB_RsvdSecCnt - 1) / (ENTRIES_PER_TABLE_SECTOR + 1) ;
}

void FileSystem_DIR_Entry::Init(char* szDirName, unsigned short usDirAttribute, int iUserID, unsigned uiParentSecNo, byte bParentSecPos)
{
  strcpy((char*)Name, szDirName) ;

  usAttribute = usDirAttribute ;

  SystemUtil_GetTimeOfDay(&CreatedTime) ;
  SystemUtil_GetTimeOfDay(&AccessedTime) ;
  SystemUtil_GetTimeOfDay(&ModifiedTime) ;

  uiStartSectorID = EOC ;
  uiSize = 0 ;

  uiParentSecID = uiParentSecNo ;
  bParentSectorPos = bParentSecPos ;

  iUserID = iUserID ;
}

void FileSystem_DIR_Entry::InitAsRoot(uint32_t parentSectorId)
{
  Init(FS_ROOT_DIR, ATTR_DIR_DEFAULT | ATTR_TYPE_DIRECTORY, ROOT_USER_ID, parentSectorId, 0);
}
