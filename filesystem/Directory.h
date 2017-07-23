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
#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_

#include <ProcessManager.h>
#include <FileSystem.h>
#include <ProcFileManager.h>
#include <DeviceDrive.h>

#define Directory_SUCCESS					0
#define Directory_ERR_UNKNOWN_DEVICE		1
#define Directory_ERR_FS_TABLE_CORRUPTED	2
#define Directory_ERR_EXISTS				3
#define Directory_ERR_NOT_EXISTS			4
#define Directory_ERR_ZERO_WRITE_SIZE		5
#define Directory_ERR_NOT_EMPTY				6
#define Directory_ERR_INVALID_OFFSET		7
#define Directory_ERR_IS_DIRECTORY			8
#define Directory_ERR_EOF					9
#define Directory_ERR_IS_NOT_DIRECTORY		10
#define Directory_ERR_NOT_DIR				11
#define Directory_ERR_SPECIAL_ENTRIES		12
#define Directory_ERR_INVALID_NAME			14
#define Directory_ERR_INVALID_DRIVE			15
#define Directory_FAILURE					16

#define DIR_SPECIAL_CURRENT		"."
#define DIR_SPECIAL_PARENT		".."

byte Directory_Create(Process* processAddressSpace, int iDriveID, byte* bParentDirectoryBuffer, const FileSystem_CWD* pCWD, 
		char* szDirName, unsigned short usDirAttribute) ;
byte Directory_Delete(Process* processAddressSpace, int iDriveID, byte* bParentDirectoryBuffer, const FileSystem_CWD* pCWD, 
				const char* szDirName) ;
byte Directory_GetDirEntryForCreateDelete(const Process* processAddressSpace, int iDriveID, const char* szDirPath, char* szDirName, 
			unsigned* uiSectorNo, byte* bSectorPos, byte* bDirectoryBuffer) ;
byte Directory_FindDirectory(DiskDrive* pDiskDrive, const FileSystem_CWD* pCWD, const char* szDirName, unsigned* uiSectorNo, byte* bSectorPos, byte* bIsPresent, byte* bDestSectorBuffer) ;
byte Directory_GetDirectoryContent(const char* szFileName, Process* processAddressSpace, int iDriveID, FileSystem_DIR_Entry** pDirList, int* iListSize) ;
byte Directory_FileWrite(DiskDrive* pDiskDrive, FileSystem_CWD* pCWD, ProcFileDescriptor* pFDEntry, byte* bDataBuffer, unsigned uiDataSize) ;
byte Directory_ActualFileWrite(DiskDrive* pDiskDrive, byte* bDataBuffer, ProcFileDescriptor* pFDEntry, 
								unsigned uiDataSize, FileSystem_DIR_Entry* dirFile) ;
byte Directory_FileRead(DiskDrive* pDiskDrive, FileSystem_CWD* pCWD, ProcFileDescriptor* pFDEntry, byte* bDataBuffer, 
						unsigned uiDataSize, unsigned* uiReadFileSize) ;
void Directory_PopulateDirEntry(FileSystem_DIR_Entry* dirEntry, char* szDirName, unsigned short usDirAttribute, int iUserID, unsigned uiParentSecNo, byte bParentSecPos) ;
byte Directory_GetDirEntryInfo(DiskDrive* pDiskDrive, FileSystem_CWD* pCWD, const char* szFileName, unsigned* uiSectorNo, byte* bSectorPos, byte* bDirectoryBuffer) ;
byte Directory_Change(const char* szFileName, int iDriveID, Process* processAddressSpace) ;
byte Directory_PresentWorkingDirectory(Process* processAddressSpace, char** uiReturnDirPathAddress) ;
byte Directory_GetDirEntry(const char* szFileName, Process* processAddressSpace, int iDriveID, FileSystem_DIR_Entry* pDirEntry) ;
byte Directory_FindFullDirPath(DiskDrive* pDiskDrive, const FileSystem_DIR_Entry* pDirEntry, char* szFullDirPath) ;
byte Directory_SyncPWD(Process* processAddressSpace) ;
byte Directory_RawDirEntryRead(DiskDrive* pDiskDrive, unsigned uiSectorID, byte bSecPos, FileSystem_DIR_Entry* pDestDirEntry) ;
void Directory_CopyDirEntry(FileSystem_DIR_Entry* pDest, FileSystem_DIR_Entry* pSrc) ;

#endif
