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

uint32_t FileSystem_DeAllocateSector(DiskDrive* pDiskDrive, unsigned uiCurrentSectorID)
{
  auto uiNextSectorID = pDiskDrive->_fileSystem.GetSectorEntryValue(uiCurrentSectorID);
  pDiskDrive->_fileSystem.SetSectorEntryValue(uiCurrentSectorID, 0);
  pDiskDrive->_fileSystem.AddToFreePoolCache(uiCurrentSectorID);
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

upan::string FileSystem_DIR_Entry::FullPath(DiskDrive& diskDrive)
{
  byte bSectorBuffer[512] ;

  const FileSystem_DIR_Entry* pParseDirEntry = this;

  upan::string fullPath = "";
  upan::string temp = "";

  bool bFirst = true ;

  while(true)
  {
    if(strcmp((const char*)pParseDirEntry->Name, FS_ROOT_DIR) == 0)
    {
      return upan::string(FS_ROOT_DIR) + fullPath;
    }
    else
    {
      upan::string curDir = pParseDirEntry->Name;
      if(!bFirst)
      {
        fullPath = curDir + FS_ROOT_DIR + fullPath;
      }
      else
      {
        fullPath = curDir;
        bFirst = false ;
      }
    }

    unsigned uiParSectorNo = pParseDirEntry->uiParentSecID ;
    byte bParSectorPos = pParseDirEntry->bParentSectorPos ;

    diskDrive.xRead(bSectorBuffer, uiParSectorNo, 1);

    pParseDirEntry = &((const FileSystem_DIR_Entry*)bSectorBuffer)[bParSectorPos] ;
  }

  throw upan::exception(XLOC, "failed to find full path for directory/file %s", Name);
}
