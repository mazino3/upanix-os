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

/* MOS File System Support */

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

/********************************** Static Functions *****************************************/

static void FileSystem_CalculateFileSystemSize(FileSystem_BootBlock* pFSBootBlock)
{
	/*
	unsigned uiTempVal1 = pFSBootBlock->BPB_TotSec32 - pFSBootBlock->BPB_RsvdSecCnt ; 
	unsigned uiTempVal2 = ((256 * 1) + 1) / 2 ;
	// where 256 * 1 = 256 * pFSBootBlock->BPB_SecPerClus

	pFSBootBlock->BPB_FSTableSize = (uiTempVal1 + (uiTempVal2 - 1)) / uiTempVal2 ;
	*/
	
	pFSBootBlock->BPB_FSTableSize = ( pFSBootBlock->BPB_TotSec32 - pFSBootBlock->BPB_RsvdSecCnt - 1 ) / ( ENTRIES_PER_TABLE_SECTOR + 1 ) ;
}

static void FileSystem_PopulateRootDirEntry(FileSystem_DIR_Entry* rootDir, unsigned uiSectorNo)
{
	String_Copy((char*)rootDir->Name, FS_ROOT_DIR) ;
	
	rootDir->usAttribute = ATTR_DIR_DEFAULT | ATTR_TYPE_DIRECTORY ;
	
	SystemUtil_GetTimeOfDay(&rootDir->CreatedTime) ;
	SystemUtil_GetTimeOfDay(&rootDir->AccessedTime) ;
	SystemUtil_GetTimeOfDay(&rootDir->ModifiedTime) ;

	rootDir->uiStartSectorID = EOC ;
	rootDir->uiSize = 0 ;

	rootDir->uiParentSecID = uiSectorNo ;
	rootDir->bParentSectorPos = 0 ;

	rootDir->iUserID = ROOT_USER_ID ;
}

static void FileSystem_InitFSBootBlock(FileSystem_BootBlock* pFSBootBlock, DiskDrive* pDiskDrive)
{
	pFSBootBlock->BPB_jmpBoot[0] = 0xEB ; /****************/
	pFSBootBlock->BPB_jmpBoot[1] = 0xFE ; /* JMP $ -- ARR */
	pFSBootBlock->BPB_jmpBoot[2] = 0x90 ; /****************/

	pFSBootBlock->BPB_BytesPerSec = 0x200; // 512 ;
	pFSBootBlock->BPB_RsvdSecCnt = 2 ;

	if(pDiskDrive->DeviceType() == DEV_FLOPPY)
		pFSBootBlock->BPB_Media  = MEDIA_REMOVABLE ;
	else
		pFSBootBlock->BPB_Media  = MEDIA_FIXED ;

	pFSBootBlock->BPB_SecPerTrk = pDiskDrive->SectorsPerTrack();
	pFSBootBlock->BPB_NumHeads = pDiskDrive->NoOfHeads();
	pFSBootBlock->BPB_HiddSec  = 0 ;
	pFSBootBlock->BPB_TotSec32 = pDiskDrive->SizeInSectors();
	
/*	pFSBootBlock->BPB_FSTableSize ; ---> Calculated */
	pFSBootBlock->BPB_ExtFlags  = 0x0080 ; 
	pFSBootBlock->BPB_FSVer = 0x0100 ;  //version 1.0
	pFSBootBlock->BPB_FSInfo  = 1 ;  //Typical Value for FSInfo Sector

	pFSBootBlock->BPB_BootSig = 0x29 ;
	pFSBootBlock->BPB_VolID = 0x01 ;  //TODO: Required to be set to current Date/Time of system ---- Not Mandatory
	String_Copy((char*)pFSBootBlock->BPB_VolLab, "No Name   ") ;  //10 + 1(\0) characters only -- ARR

	pFSBootBlock->uiUsedSectors = 1 ;

	FileSystem_CalculateFileSystemSize(pFSBootBlock) ;
}

static byte FileSystem_GetFSBootBlock(DiskDrive* pDiskDrive)
{
	byte bArrFSBootBlock[512] ;
	byte bStatus ;
	
	RETURN_IF_NOT(bStatus, pDiskDrive->Read(1, 1, bArrFSBootBlock), DeviceDrive_SUCCESS) ;

	if(bArrFSBootBlock[510] != 0x55 || bArrFSBootBlock[511] != 0xAA)
		return FileSystem_ERR_INVALID_BPB_SIGNATURE ;

	FileSystem_BootBlock* pFSBootBlock = &(pDiskDrive->FSMountInfo.FSBootBlock) ;

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&bArrFSBootBlock, MemUtil_GetDS(), 
						(unsigned)pFSBootBlock, sizeof(FileSystem_BootBlock)) ;
	
	if(pFSBootBlock->BPB_BootSig != 0x29)
		return FileSystem_ERR_INVALID_BOOT_SIGNATURE ;

	// TODO: A write to HD image file from mos fs util is changing the CHS value !!
	// Needs to be fixed. So, this check is skipped for the time being

	/*
	if(pFSBootBlock->BPB_SecPerTrk != pDiskDrive->uiSectorsPerTrack)
		return FileSystem_ERR_INVALID_SECTORS_PER_TRACK ;

	if(pFSBootBlock->BPB_NumHeads != pDiskDrive->uiNoOfHeads)
		return FileSystem_ERR_INVALID_NO_OF_HEADS ;
	*/
	
	if(pFSBootBlock->BPB_TotSec32 != pDiskDrive->SizeInSectors())
		return FileSystem_ERR_INVALID_DRIVE_SIZE_IN_SECTORS ;
	
	if(pFSBootBlock->BPB_FSTableSize == 0)
		return FileSystem_ERR_ZERO_FATSZ32 ;

	if(pFSBootBlock->BPB_BytesPerSec != 0x200)
		return FileSystem_ERR_UNSUPPORTED_SECTOR_SIZE ;
		
	return FileSystem_SUCCESS ;
}

static byte FileSystem_ReadRootDirectory(DiskDrive* pDiskDrive)
{
	byte bStatus ;
	unsigned uiSec ;
	FileSystem_PresentWorkingDirectory* pFSpwd = &pDiskDrive->FSMountInfo.FSpwd ;
	byte bDataBuffer[512] ;

	uiSec = FileSystem_GetRealSectorNumber(0, pDiskDrive) ;
		
	RETURN_IF_NOT(bStatus, pDiskDrive->Read(uiSec, 1, bDataBuffer), DeviceDrive_SUCCESS) ;
	
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)bDataBuffer, MemUtil_GetDS(), (unsigned)&pFSpwd->DirEntry, sizeof(FileSystem_DIR_Entry)) ;

	pFSpwd->uiSectorNo = 0 ;
	pFSpwd->bSectorEntryPosition = 0 ;
		
	return FileSystem_SUCCESS ;
}

static byte FileSystem_InitializeMountInfo(DiskDrive* pDiskDrive)
{
	FileSystemMountInfo *pFSMountInfo = &(pDiskDrive->FSMountInfo) ;
	
	unsigned uiMountPointStart = pDiskDrive->MountPointStart();
	unsigned uiMountPointEnd = pDiskDrive->MountPointEnd();

	unsigned uiMountSpaceLimit = uiMountPointEnd - uiMountPointStart ;

	unsigned uiSizeOfTableCahce = FileSystem_GetSizeForTableCache(pDiskDrive->NoOfSectorsInTableCache());

	pDiskDrive->bFSCacheFlag = ENABLE_TABLE_CAHCE | ENABLE_FREE_POOL_CACHE ;

	if(uiSizeOfTableCahce > uiMountSpaceLimit)
	{
		KC::MDisplay().Message("\n Critical Error: FS Mount Cache size insufficient. ", ' ') ;
		pDiskDrive->bFSCacheFlag &= ~((byte)ENABLE_TABLE_CAHCE) ;
		KernelUtil::Wait(3000) ;
	}

  pFSMountInfo->pFreePoolQueue = new upan::queue<unsigned>(pDiskDrive->MaxSectorsInFreePoolCache());

	pFSMountInfo->FSTableCache.pSectorBlockEntryList = NULL ;
	pFSMountInfo->FSTableCache.iSize = 0 ;

	if(FileSystem_IsTableCacheEnabled(pDiskDrive))
	{
		pFSMountInfo->FSTableCache.pSectorBlockEntryList = (SectorBlockEntry*)(uiMountPointStart) ;
	}

	return FileSystem_SUCCESS ;
}


/**********************************************************************************************/

byte FileSystem_VerifyBootBlock(DiskDrive* pDiskDrive)
{
	FileSystem_BootBlock* pFSBootBlock = &pDiskDrive->FSMountInfo.FSBootBlock ;

	if(pFSBootBlock->BPB_jmpBoot[0] != 0xEB || pFSBootBlock->BPB_jmpBoot[1] != 0xFE || pFSBootBlock->BPB_jmpBoot[2] != 0x90)
		return FileSystem_ERR_BPB_JMP ;

	if(pFSBootBlock->BPB_BytesPerSec != 0x200)
		return FileSystem_ERR_UNSUPPORTED_SECTOR_SIZE ;
	
	if(pDiskDrive->DeviceType() == DEV_FLOPPY)
		if(pFSBootBlock->BPB_Media  != 0xF0)
			return FileSystem_ERR_UNSUPPORTED_MEDIA ;
		
	if(pFSBootBlock->BPB_ExtFlags  != 0x0080)
		return FileSystem_ERR_INVALID_EXTFLAG ;
		
	if(pFSBootBlock->BPB_FSVer != 0x0100)
		return FileSystem_ERR_FS_VERSION ;
		
	if(pFSBootBlock->BPB_FSInfo  != 1)
		return FileSystem_ERR_FSINFO_SECTOR ;
		
	if(pFSBootBlock->BPB_VolID != 0x01)
		return FileSystem_ERR_INVALID_VOL_ID ;

	return FileSystem_SUCCESS ;
}

unsigned 
FileSystem_GetRealSectorNumber(const unsigned uiSectorID, const DiskDrive* pDiskDrive)
{
	return uiSectorID + 1/*BPB*/ 
					+ pDiskDrive->FSMountInfo.FSBootBlock.BPB_RsvdSecCnt 
					+ pDiskDrive->FSMountInfo.FSBootBlock.BPB_FSTableSize ;
}

byte
FileSystem_GetSectorEntryValue(DiskDrive* pDiskDrive, const unsigned uiSectorID, unsigned* uiSectorEntryValue)
{
	FileSystem_BootBlock* pFSBootBlock = (FileSystem_BootBlock*)&(pDiskDrive->FSMountInfo.FSBootBlock) ;
	
	if(uiSectorID > (pFSBootBlock->BPB_FSTableSize * pFSBootBlock->BPB_BytesPerSec / 4))
		return FileSystem_ERR_INVALID_CLUSTER_ID ;

	RETURN_X_IF_NOT(FSManager_GetSectorEntryValue(pDiskDrive, uiSectorID, uiSectorEntryValue, false), FSManager_SUCCESS, FileSystem_FAILURE) ;

	return FileSystem_SUCCESS ;
}

byte
FileSystem_SetSectorEntryValue(DiskDrive* pDiskDrive, const unsigned uiSectorID, unsigned uiSectorEntryValue)
{
	FileSystem_BootBlock* pFSBootBlock = (FileSystem_BootBlock*)&(pDiskDrive->FSMountInfo.FSBootBlock) ;
	
	if(uiSectorID > (pFSBootBlock->BPB_FSTableSize * pFSBootBlock->BPB_BytesPerSec / 4))
		return FileSystem_ERR_INVALID_CLUSTER_ID ;
	
	RETURN_X_IF_NOT(FSManager_SetSectorEntryValue(pDiskDrive, uiSectorID, uiSectorEntryValue, false), FSManager_SUCCESS, FileSystem_FAILURE) ;

	return FileSystem_SUCCESS ;
}

byte FileSystem_Format(DiskDrive* pDiskDrive)
{
	byte bStatus ;

	if(pDiskDrive->DeviceType() == DEV_FLOPPY)
	{
;//		RETURN_IF_NOT(bStatus, Floppy_Format(pDiskDrive->driveNo), Floppy_SUCCESS) ;
	}

	byte bFSBootBlockBuffer[512] ;
	byte bSectorBuffer[512] ;
	FileSystem_BootBlock* pFSBootBlock = (FileSystem_BootBlock*)(bFSBootBlockBuffer) ;

	/************************ FAT Boot Block [START] *******************************/
	FileSystem_InitFSBootBlock(((FileSystem_BootBlock*)bFSBootBlockBuffer), pDiskDrive) ;
	
	bFSBootBlockBuffer[510] = 0x55 ; /* BootSector Signature */
	bFSBootBlockBuffer[511] = 0xAA ;

	RETURN_IF_NOT(bStatus, pDiskDrive->Write(1, 1, bFSBootBlockBuffer), DeviceDrive_SUCCESS) ;
	/************************* FAT Boot Block [END] **************************/
	
	/*********************** FAT Table [START] *************************************/

	unsigned i ;
	for(i = 0; i < 512; i++)
		bSectorBuffer[i] = 0 ;

	for(i = 0; i < pFSBootBlock->BPB_FSTableSize; i++)
	{
		if(i == 0)
			((unsigned*)&bSectorBuffer)[0] = EOC ;
		
		RETURN_IF_NOT(bStatus, pDiskDrive->Write(i + pFSBootBlock->BPB_RsvdSecCnt + 1, 1, bSectorBuffer), DeviceDrive_SUCCESS) ;

		if(i == 0)
			((unsigned*)&bSectorBuffer)[0] = 0 ;
	}
	/*************************** FAT Table [END] **************************************/

	/*************************** Root Directory [START] *******************************/
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pFSBootBlock, MemUtil_GetDS(), 
						(unsigned)&(pDiskDrive->FSMountInfo.FSBootBlock), 
						sizeof(FileSystem_BootBlock)) ;

	unsigned uiSec = FileSystem_GetRealSectorNumber(0, pDiskDrive) ;

	FileSystem_PopulateRootDirEntry(((FileSystem_DIR_Entry*)&bSectorBuffer), uiSec) ;
	
	RETURN_IF_NOT(bStatus, pDiskDrive->Write(uiSec, 1, bSectorBuffer), DeviceDrive_SUCCESS) ;
	/*************************** Root Directory [END] ********************************/

	pDiskDrive->Mounted(false);

	return FileSystem_SUCCESS ;
}

byte FileSystem_Mount(DiskDrive* pDiskDrive)
{
	byte bStatus ;
	
	if(pDiskDrive->Mounted())
		return FileSystem_ERR_ALREADY_MOUNTED ;

	RETURN_IF_NOT(bStatus, FileSystem_InitializeMountInfo(pDiskDrive), FileSystem_SUCCESS) ;

	RETURN_IF_NOT(bStatus, FileSystem_GetFSBootBlock(pDiskDrive), FileSystem_SUCCESS) ;

	RETURN_IF_NOT(bStatus, FSManager_Mount(pDiskDrive), FileSystem_SUCCESS) ;

	RETURN_IF_NOT(bStatus, FileSystem_ReadRootDirectory(pDiskDrive), FileSystem_SUCCESS) ;

	pDiskDrive->Mounted(true);

	return FileSystem_SUCCESS ;
}

byte FileSystem_UnMount(DiskDrive* pDiskDrive)
{
	byte bStatus ;

	if(!pDiskDrive->Mounted())
		return FileSystem_ERR_NOT_MOUNTED ;

	byte bSectorBuffer[512] ;

	bSectorBuffer[510] = 0x55 ; /* BootSector Signature */
	bSectorBuffer[511] = 0xAA ;

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&pDiskDrive->FSMountInfo.FSBootBlock,
	MemUtil_GetDS(), (unsigned)bSectorBuffer, sizeof(FileSystem_BootBlock)) ;

	RETURN_IF_NOT(bStatus, pDiskDrive->Write(1, 1, bSectorBuffer), DeviceDrive_SUCCESS) ;

	RETURN_IF_NOT(bStatus, FSManager_UnMount(pDiskDrive), FileSystem_SUCCESS) ;

	pDiskDrive->FlushDirtyCacheSectors();
	pDiskDrive->Mounted(false);

	return FileSystem_SUCCESS ;
}

byte FileSystem_AllocateSector(DiskDrive* pDiskDrive, unsigned* uiFreeSectorID)
{
	RETURN_X_IF_NOT(FSManager_AllocateSector(pDiskDrive, uiFreeSectorID), FSManager_SUCCESS, FileSystem_FAILURE) ;

	return FileSystem_SUCCESS ;
}

byte FileSystem_DeAllocateSector(DiskDrive* pDiskDrive, unsigned uiCurrentSectorID, unsigned* uiNextSectorID)
{
	byte bStatus ;

	RETURN_IF_NOT(bStatus, FileSystem_GetSectorEntryValue(pDiskDrive, uiCurrentSectorID, uiNextSectorID), FileSystem_SUCCESS) ;

	RETURN_IF_NOT(bStatus, FileSystem_SetSectorEntryValue(pDiskDrive, uiCurrentSectorID, 0), FileSystem_SUCCESS) ;

	if(FileSystem_IsFreePoolCacheEnabled(pDiskDrive))
	{
    pDiskDrive->FSMountInfo.pFreePoolQueue->push_back(uiCurrentSectorID);
	}

	return FileSystem_SUCCESS ;
}

byte FileSystem_IsFreePoolCacheEnabled(const DiskDrive* pDiskDrive)
{
	return pDiskDrive->bFSCacheFlag & ENABLE_FREE_POOL_CACHE ;
}

byte FileSystem_IsTableCacheEnabled(const DiskDrive* pDiskDrive)
{
	return pDiskDrive->bFSCacheFlag & ENABLE_TABLE_CAHCE ;
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
