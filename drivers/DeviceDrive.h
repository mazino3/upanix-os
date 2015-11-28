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
#ifndef _DEVICE_DRIVE_H_
#define _DEVICE_DRIVE_H_

# include <Global.h>
# include <FSStructures.h>
# include <Atomic.h>
# include <DiskCache.h>

#define DeviceDrive_SUCCESS						0
#define DeviceDrive_ERR_UNKNOWN_DEVICE_TYPE		1
#define DeviceDrive_ERR_INVALID_DRIVE_NAME		2
#define	DeviceDrive_ERR_MOUNT					3
#define	DeviceDrive_ERR_UNMOUNT					4
#define	DeviceDrive_ERR_FORMAT					5
#define	DeviceDrive_ERR_PARTITION_UPDATE		6
#define	DeviceDrive_ERR_CURR_DRIVE_UMOUNT		7
#define	DeviceDrive_ERR_DUPLICATE				8
#define	DeviceDrive_ERR_NOTFOUND				9
#define DeviceDrive_FAILURE						10

#define GET_DRIVE_FOR_FS_OPS(DRIVE_ID, ERR) \
	DriveInfo* pDriveInfo = DeviceDrive_GetByID(DRIVE_ID, true) ; \
	if(pDriveInfo == NULL) \
		return ERR ;

typedef enum
{
	FS_UNKNOWN = 0x0,
	MOSFS = 0x83
} FS_TYPE ;

typedef enum
{
	DEV_FLOPPY = 0x0,
	DEV_CD,
	DEV_ATA_IDE = 0x80,
	DEV_ATAPI,
	DEV_SCSI_USB_DISK = 0x90
} DEVICE_TYPE ;

typedef enum
{
	FROM_FILE = -3,
	ROOT_DRIVE = -2,
	CURRENT_DRIVE = -1,
	FD_DRIVE0 = 0,
	FD_DRIVE1,
	FD_DRIVE2,
	FD_DRIVE3,
	CD_DRIVE0,
	CD_DRIVE1,
	HDD_DRIVE0,
	HDD_DRIVE1,
	USD_DRIVE0,
	USD_DRIVE1,
	USD_DRIVE2,
	USD_DRIVE3,
	USD_DRIVE4,
	USD_DRIVE5
} DRIVE_NO ;

typedef struct 
{
	char			driveName[33] ;
	int				iID ;
	byte			bMounted ;

	DEVICE_TYPE		deviceType ;
	FS_TYPE			fsType ;
	DRIVE_NO		driveNumber ;
	
	unsigned		uiLBAStartSector ;
	unsigned		uiSizeInSectors ;

	unsigned		uiStartCynlider ;
	unsigned		uiStartHead ;
	unsigned		uiStartSector ;

	unsigned		uiEndCynlider ;
	unsigned		uiEndHead ;
	unsigned		uiEndSector ;

	unsigned 		uiSectorsPerTrack ;
	unsigned 		uiTracksPerHead ;
	unsigned		uiNoOfHeads ;
} Drive ;

typedef struct DriveInfo DriveInfo ;
typedef struct RawDiskDrive RawDiskDrive ;

struct DriveInfo
{
	Drive			drive ;
	void*			pDevice ;
	RawDiskDrive*	pRawDisk ;

	bool			bEnableDiskCache ;
	DiskCache		mCache ;

	// FileSystem Mount Info
	unsigned				uiMountPointStart ;
	unsigned				uiMountPointEnd ;
	FileSystem_MountInfo	FSMountInfo ;

	unsigned				uiNoOfSectorsInFreePool ;
	unsigned				uiNoOfSectorsInTableCache ;
	byte					bFSCacheFlag ;

	Mutex			mDriveMutex ;

	DriveInfo* 		pNext ;
} ;

typedef struct
{
	unsigned long	ulTotalSize ;
	unsigned long	ulUsedSize ;
} DriveSpace ;

typedef struct
{
	Drive		drive ;
	DriveSpace	driveSpace ;
} DriveStat ;

typedef enum
{
	ATA_HARD_DISK = 100,
	USB_SCSI_DISK,
	FLOPPY_DISK,
} RAW_DISK_TYPES ;

struct RawDiskDrive
{
	char szNameID[32] ;
	RAW_DISK_TYPES iType ;
	unsigned uiSectorSize ;
	unsigned uiSizeInSectors ;
	void* pDevice ;

	Mutex mDiskMutex ;

	RawDiskDrive* pNext ;
} ;

extern DriveInfo* DeviceDrive_pCurrentDrive ;

class DriveRemoveClause
{
	public:
		virtual bool operator()(const DriveInfo* pDriveInfo) const = 0 ;
} ;

void DeviceDrive_Initialize() ;
void DeviceDrive_AddEntry(DriveInfo* pDriveInfo) ;
void DeviceDrive_RemoveEntryByCondition(const DriveRemoveClause& removeClause) ;
DriveInfo* DeviceDrive_CreateDriveInfo(bool bEnableDiskCache) ;
byte DeviceDrive_Read(DriveInfo* pDriveInfo, unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer) ;
byte DeviceDrive_Write(DriveInfo* pDriveInfo, unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer) ;
DriveInfo* DeviceDrive_GetByDriveName(const char* szDriveName, bool bCheckMount) ;
DriveInfo* DeviceDrive_GetByID(int iID, bool bCheckMount) ;
void DeviceDrive_DisplayList() ;
DriveInfo* DeviceDrive_GetNextDrive(const DriveInfo* pDriveInfo) ;

byte DeviceDrive_Change(const char* szDriveName) ;
byte DeviceDrive_GetList(DriveStat** pDriveList, int* iListSize) ;
byte DeviceDrive_MountDrive(const char* szDriveName) ;
byte DeviceDrive_UnMountDrive(const char* szDriveName) ;
byte DeviceDrive_FormatDrive(const char* szDriveName) ;
byte DeviceDrive_GetCurrentDrive(Drive* pDrive) ;

RawDiskDrive* DeviceDrive_GetFirstRawDiskEntry() ;
RawDiskDrive* DeviceDrive_CreateRawDisk(const char* szName, RAW_DISK_TYPES iType, void* pDevice) ;
byte DeviceDrive_AddRawDiskEntry(RawDiskDrive* pRawDisk) ;
byte DeviceDrive_RemoveRawDiskEntry(const char* szName) ;
RawDiskDrive* DeviceDrive_GetRawDiskByName(const char* szName) ;

byte DeviceDrive_RawDiskRead(RawDiskDrive* pDisk, unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer) ;
byte DeviceDrive_RawDiskWrite(RawDiskDrive* pDisk, unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer) ;

#endif
