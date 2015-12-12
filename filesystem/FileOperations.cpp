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
# include <FileOperations.h>
# include <ProcFileManager.h>
# include <Directory.h>
# include <FileSystem.h>
# include <ProcessManager.h>
# include <MemUtil.h>
# include <UserManager.h>
# include <MountManager.h>
# include <SystemUtil.h>
# include <DMM.h>
# include <StringUtil.h>
# include <Display.h>
# include <stdio.h>

/************************************************************************************************************/
static byte FileOperations_ValidateAndGetFileAttr(unsigned short usFileType, unsigned short usMode, 
	unsigned short* usFileAttr)
{
	usMode = FILE_PERM(usMode) ;
	usFileType = FILE_TYPE(usFileType) ;

	if(!(usFileType == ATTR_TYPE_FILE || usFileType == ATTR_TYPE_DIRECTORY))
		return FileOperations_ERR_INVALID_FILE_ATTR ;

	*usFileAttr = (usFileType | usMode) ;

	return FileOperations_SUCCESS ;
}

static byte FileOperations_HasPermission(int userType, unsigned short usFileAttr, byte mode)
{	
	unsigned short usMode = FILE_PERM(usFileAttr) ;
	
	byte bHasRead, bHasWrite ;

	switch(userType)
	{
		case USER_OWNER:
				bHasRead = HAS_READ_PERM(G_OWNER(usMode)) ;
				bHasWrite = HAS_WRITE_PERM(G_OWNER(usMode)) ;
				break ;

		case USER_OTHERS:
				bHasRead = HAS_READ_PERM(G_OTHERS(usMode)) ;
				bHasWrite = HAS_WRITE_PERM(G_OTHERS(usMode)) ;
				break ;

		default:
			return false ;
	}

	if(mode & O_RDONLY)
	{
			if(bHasRead || bHasWrite)
				return true ;
	}
	else if((mode & O_WRONLY) || (mode & O_RDWR) || (mode & O_APPEND))
	{
			if(bHasWrite)
				return true ;
	}

	return false ;
}

static int FileOperations_GetUserType(byte bIsKernelProcess, int iProcessUserID, int iFileUserID)
{
	if(bIsKernelProcess || iProcessUserID == ROOT_USER_ID || iFileUserID == iProcessUserID)
		return USER_OWNER ;

	return USER_OTHERS ;
}

static void FileOperations_ParseFilePathWithDrive(const char* szFileNameWithDrive, char* szFileName, unsigned* pDriveID)
{
	int i = String_Chr(szFileNameWithDrive, '@') ;
	
	if(i == -1)
	{
		*pDriveID = ProcessManager::Instance().GetCurrentPAS().iDriveID ;
		String_Copy(szFileName, szFileNameWithDrive) ;
		return ;
	}

	if(i > 32)
	{
		String_Copy(szFileName, szFileNameWithDrive) ;
		return ;
	}

	char szDriveName[33] ;

	String_RawCopy((byte*)szDriveName, (byte*)szFileNameWithDrive, i) ;
	szDriveName[i] = '\0' ;

	String_Copy(szFileName, szFileNameWithDrive + i + 1) ;

	if(String_Compare(szDriveName, ROOT_DRIVE_SYN) == 0)
	{
		*pDriveID = ROOT_DRIVE_ID ;
	}
	else
	{
		DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByDriveName(szDriveName, false) ;
		if(pDiskDrive == NULL)
			*pDriveID = ROOT_DRIVE_ID ;
		else
			*pDriveID = pDiskDrive->Id();
	}
}

/************************************************************************************************************/
byte FileOperations_Open(int* fd, const char* szFileName, const byte mode)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

	byte bStatus ;
	FileSystem_DIR_Entry dirEntry ;
	ProcessAddressSpace* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

	GET_DRIVE_FOR_FS_OPS(iDriveID, Directory_FAILURE) ;

	RETURN_IF_NOT(bStatus, Directory_GetDirEntry(szFile, pPAS, iDriveID, &dirEntry), Directory_SUCCESS)

	unsigned short usFileAttr = dirEntry.usAttribute ;
	if(FILE_TYPE(usFileAttr) != ATTR_TYPE_FILE)
		return FileOperations_ERR_NOT_EXISTS;

	int userType = FileOperations_GetUserType(pPAS->bIsKernelProcess, pPAS->iUserID, dirEntry.iUserID) ;

	if(!FileOperations_HasPermission(userType, usFileAttr, mode))
		return FileOperations_ERR_NO_PERM ;

	if( (mode & O_TRUNC) )
	{
		RETURN_IF_NOT(bStatus, FileOperations_Delete(szFileName), FileOperations_SUCCESS) ;
		RETURN_IF_NOT(bStatus, FileOperations_Create(szFileName, FILE_TYPE(dirEntry.usAttribute), FILE_PERM(dirEntry.usAttribute)), 
					FileOperations_SUCCESS) ;
		RETURN_IF_NOT(bStatus, Directory_GetDirEntry(szFile, pPAS, iDriveID, &dirEntry), Directory_SUCCESS)
	}

	char szFullFilePath[256] ;
	RETURN_IF_NOT(bStatus, Directory_FindFullDirPath(pDiskDrive, &dirEntry, szFullFilePath), Directory_SUCCESS) ;

	RETURN_X_IF_NOT(ProcFileManager_AllocateFD(fd, szFullFilePath, mode, iDriveID, dirEntry.uiSize, dirEntry.uiStartSectorID), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;

	return FileOperations_SUCCESS ;
}

byte FileOperations_Close(int fd)
{
	RETURN_X_IF_NOT(ProcFileManager_FreeFD(fd), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;
	return FileOperations_SUCCESS ;
}

byte FileOperations_Read(int fd, char* buffer, int len, unsigned* pReadLen)
{
	RETURN_X_IF_NOT(ProcFileManager_GetRealNonDuppedFD(fd, &fd), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;

	ProcFileDescriptor* pFDEntry ;
	ProcessAddressSpace* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

	RETURN_X_IF_NOT(ProcFileManager_GetFDEntry(fd, &pFDEntry), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;

	int iDriveID = pFDEntry->iDriveID ;

	GET_DRIVE_FOR_FS_OPS(iDriveID, Directory_FAILURE) ;
	FileSystem_CWD CWD ;
	if(pPAS->iDriveID == iDriveID)
	{
		CWD.pDirEntry = &(pPAS->processPWD.DirEntry) ;
		CWD.uiSectorNo = pPAS->processPWD.uiSectorNo ;
		CWD.bSectorEntryPosition = pPAS->processPWD.bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = &(pDiskDrive->FSMountInfo.FSpwd.DirEntry) ;
		CWD.uiSectorNo = pDiskDrive->FSMountInfo.FSpwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pDiskDrive->FSMountInfo.FSpwd.bSectorEntryPosition ;
	}

	byte bStatus ;
	RETURN_IF_NOT(bStatus, Directory_FileRead(pDiskDrive, &CWD, pFDEntry, (byte*)buffer, len, pReadLen), Directory_SUCCESS) ;

	RETURN_IF_NOT(bStatus, FileOperations_UpdateTime(pFDEntry->szFileName, iDriveID, DIR_ACCESS_TIME), FileOperations_SUCCESS) ;

	pFDEntry->uiOffset += (*pReadLen) ;

	return FileOperations_SUCCESS ;
}

byte FileOperations_Write(int fd, const char* buffer, int len, int* pWriteLen)
{
	RETURN_X_IF_NOT(ProcFileManager_GetRealNonDuppedFD(fd, &fd), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;

	*pWriteLen = -1 ;

	if(ProcFileManager_IsSTDOUT(fd) == ProcessManager_SUCCESS)
	{
		KC::MDisplay().nMessage(buffer, len, Display::WHITE_ON_BLACK()) ;
		*pWriteLen = len ;
		return FileOperations_SUCCESS ;
	}

	ProcFileDescriptor* pFDEntry ;
	RETURN_X_IF_NOT(ProcFileManager_GetFDEntry(fd, &pFDEntry), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;

	ProcessAddressSpace* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

	if( !(pFDEntry->mode & O_WRONLY || pFDEntry->mode & O_RDWR || pFDEntry->mode & O_APPEND) )
		return FileOperations_ERR_NO_WRITE_PERM ;

	int iDriveID = pFDEntry->iDriveID ;

	GET_DRIVE_FOR_FS_OPS(iDriveID, Directory_FAILURE) ;
	FileSystem_CWD CWD ;
	if(pPAS->iDriveID == iDriveID)
	{
		CWD.pDirEntry = &(pPAS->processPWD.DirEntry) ;
		CWD.uiSectorNo = pPAS->processPWD.uiSectorNo ;
		CWD.bSectorEntryPosition = pPAS->processPWD.bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = &(pDiskDrive->FSMountInfo.FSpwd.DirEntry) ;
		CWD.uiSectorNo = pDiskDrive->FSMountInfo.FSpwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pDiskDrive->FSMountInfo.FSpwd.bSectorEntryPosition ;
	}

	unsigned uiIncLen = len ;
	unsigned uiLimit = 1 MB ;
	unsigned n = 0;
	
	while(true)
	{
		n = (uiIncLen > uiLimit) ? uiLimit : uiIncLen ;
		
		RETURN_X_IF_NOT(Directory_FileWrite(pDiskDrive, &CWD, pFDEntry, (byte*)buffer, n), Directory_SUCCESS, FileOperations_FAILURE) ;

		pFDEntry->uiOffset += n ;

		uiIncLen -= n ;

		if(uiIncLen == 0)
			break ;
	}

	RETURN_X_IF_NOT(FileOperations_UpdateTime(pFDEntry->szFileName, iDriveID, DIR_ACCESS_TIME | DIR_MODIFIED_TIME), FileOperations_SUCCESS, FileOperations_FAILURE) ;
	
	*pWriteLen = len ;

	return FileOperations_SUCCESS ;
}

byte FileOperations_Create(const char* szFilePath, unsigned short usFileType, unsigned short usMode)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFilePath, szFile, (unsigned*)&iDriveID) ;

	ProcessAddressSpace* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

	unsigned short usFileAttr ;
	byte bStatus ;

	RETURN_IF_NOT(bStatus, FileOperations_ValidateAndGetFileAttr(usFileType, usMode, &usFileAttr), FileOperations_SUCCESS) ;

	byte bParentDirectoryBuffer[512] ;
	char szDirName[33] ;
	FileSystem_CWD CWD ;
	unsigned uiParentSectorNo ;
	byte bParentSectorPos ;

	RETURN_IF_NOT(bStatus, Directory_GetDirEntryForCreateDelete(pPAS, iDriveID, szFile,
							szDirName, &uiParentSectorNo, &bParentSectorPos, bParentDirectoryBuffer), Directory_SUCCESS) ;

	CWD.pDirEntry = ((FileSystem_DIR_Entry*)bParentDirectoryBuffer) + bParentSectorPos ;
	CWD.uiSectorNo = uiParentSectorNo ;
	CWD.bSectorEntryPosition = bParentSectorPos ;

	unsigned short usParentDirAttr = CWD.pDirEntry->usAttribute ;
	int userType = FileOperations_GetUserType(pPAS->bIsKernelProcess, pPAS->iUserID, CWD.pDirEntry->iUserID) ;

	if(!FileOperations_HasPermission(userType, usParentDirAttr, O_RDWR))
		return FileOperations_ERR_NO_PERM ;

	RETURN_X_IF_NOT(Directory_Create(pPAS, iDriveID, bParentDirectoryBuffer, &CWD, szDirName, usFileAttr), Directory_SUCCESS, FileOperations_FAILURE) ;

	return FileOperations_SUCCESS ;
}

byte FileOperations_Delete(const char* szFilePath)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFilePath, szFile, (unsigned*)&iDriveID) ;

	ProcessAddressSpace* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

	byte bStatus ;
	byte bParentDirectoryBuffer[512] ;
	char szDirName[33] ;
	FileSystem_CWD CWD ;
	unsigned uiParentSectorNo ;
	byte bParentSectorPos ;

	RETURN_IF_NOT(bStatus, Directory_GetDirEntryForCreateDelete(pPAS, iDriveID, szFile,
							szDirName, &uiParentSectorNo, &bParentSectorPos, bParentDirectoryBuffer), Directory_SUCCESS) ;

	CWD.pDirEntry = ((FileSystem_DIR_Entry*)bParentDirectoryBuffer) + bParentSectorPos ;
	CWD.uiSectorNo = uiParentSectorNo ;
	CWD.bSectorEntryPosition = bParentSectorPos ;

	unsigned short usParentDirAttr = CWD.pDirEntry->usAttribute ;
	int userType = FileOperations_GetUserType(pPAS->bIsKernelProcess, pPAS->iUserID, CWD.pDirEntry->iUserID) ;

	if(!FileOperations_HasPermission(userType, usParentDirAttr, O_RDWR))
		return FileOperations_ERR_NO_PERM ;

	FileSystem_DIR_Entry fileDirEntry ;

	RETURN_IF_NOT(bStatus, FileOperations_GetDirEntry(szFilePath, &fileDirEntry), FileOperations_SUCCESS) ;

	if(FileOperations_GetUserType(pPAS->bIsKernelProcess, pPAS->iUserID, fileDirEntry.iUserID) != USER_OWNER)
		return FileOperations_ERR_NO_PERM ;

	RETURN_X_IF_NOT(Directory_Delete(pPAS, iDriveID, bParentDirectoryBuffer, &CWD, szDirName), Directory_SUCCESS, 
		FileOperations_FAILURE) ;

	return FileOperations_SUCCESS ;
}

byte FileOperations_Exists(const char* szFileName, unsigned short usFileType)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

	FileSystem_DIR_Entry dirEntry ;
	ProcessAddressSpace* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

	byte bStatus ;
	RETURN_IF_NOT(bStatus, Directory_GetDirEntry(szFile, pPAS, iDriveID, &dirEntry), Directory_SUCCESS)

	if((dirEntry.usAttribute & usFileType) != usFileType)
		return FileOperations_ERR_NOT_EXISTS ;

	return FileOperations_SUCCESS ;
}

byte FileOperations_Seek(int fd, int iOffset, int seekType)
{
	byte bStatus ;
	RETURN_IF_NOT(bStatus, ProcFileManager_UpdateOffset(fd, seekType, iOffset), ProcFileManager_SUCCESS) ;
	return FileOperations_SUCCESS ;
}

byte FileOperations_GetOffset(int fd, unsigned* uiOffset)
{
	byte bStatus ;
	RETURN_IF_NOT(bStatus, ProcFileManager_GetOffset(fd, uiOffset), ProcFileManager_SUCCESS) ;
	return FileOperations_SUCCESS ;
}

byte FileOperations_GetCWD(char* szPathBuf, int iBufSize)
{
	byte bStatus ;
	FileSystem_PresentWorkingDirectory* pPWD = (FileSystem_PresentWorkingDirectory*)&(ProcessManager::Instance().GetCurrentPAS().processPWD) ;
	int iDriveID = ProcessManager::Instance().GetCurrentPAS().iDriveID ;

	GET_DRIVE_FOR_FS_OPS(iDriveID, Directory_FAILURE) ;

	char szFullFilePath[256] ;
	RETURN_IF_NOT(bStatus, Directory_FindFullDirPath(pDiskDrive, &pPWD->DirEntry, szFullFilePath), Directory_SUCCESS) ;

	if(String_Length(szFullFilePath) > iBufSize)
		return FileOperations_FAILURE ;

	String_Copy(szPathBuf, szFullFilePath);
	return FileOperations_SUCCESS ;
}

byte FileOperations_GetDirEntry(const char* szFileName, FileSystem_DIR_Entry* pDirEntry)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

	GET_DRIVE_FOR_FS_OPS(iDriveID, Directory_FAILURE) ;

	FileSystem_CWD CWD ;
	if(iDriveID == ProcessManager::Instance().GetCurrentPAS().iDriveID)
	{
		FileSystem_PresentWorkingDirectory* pPWD = (FileSystem_PresentWorkingDirectory*)&(ProcessManager::Instance().GetCurrentPAS().processPWD) ;
		CWD.pDirEntry = &pPWD->DirEntry ;
		CWD.uiSectorNo = pPWD->uiSectorNo ;
		CWD.bSectorEntryPosition = pPWD->bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = &(pDiskDrive->FSMountInfo.FSpwd.DirEntry) ;
		CWD.uiSectorNo = pDiskDrive->FSMountInfo.FSpwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pDiskDrive->FSMountInfo.FSpwd.bSectorEntryPosition ;
	}

	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
	RETURN_X_IF_NOT(Directory_GetDirEntryInfo(pDiskDrive, &CWD, szFile, &uiSectorNo, &bSectorPos, bDirectoryBuffer), Directory_SUCCESS, FileOperations_FAILURE) ;

	FileSystem_DIR_Entry* pSrcDirEntry = &((FileSystem_DIR_Entry*)bDirectoryBuffer)[bSectorPos] ;

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pSrcDirEntry,	MemUtil_GetDS(), (unsigned)pDirEntry, sizeof(FileSystem_DIR_Entry)) ;

	return FileOperations_SUCCESS ;
}

byte FileOperations_GetStat(const char* szFileName, int iDriveID, FileSystem_FileStat* pFileStat)
{
	char szFile[100] ;
	if(iDriveID == FROM_FILE)
	{
		FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;
	}
	else
	{
		String_Copy(szFile, szFileName) ;
	}

	GET_DRIVE_FOR_FS_OPS(iDriveID, Directory_FAILURE) ;

	FileSystem_CWD CWD ;

	if(iDriveID == ProcessManager::Instance().GetCurrentPAS().iDriveID)
	{
		FileSystem_PresentWorkingDirectory* pPWD = (FileSystem_PresentWorkingDirectory*)&(ProcessManager::Instance().GetCurrentPAS().processPWD) ;
		CWD.pDirEntry = &pPWD->DirEntry ;
		CWD.uiSectorNo = pPWD->uiSectorNo ;
		CWD.bSectorEntryPosition = pPWD->bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = &(pDiskDrive->FSMountInfo.FSpwd.DirEntry) ;
		CWD.uiSectorNo = pDiskDrive->FSMountInfo.FSpwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pDiskDrive->FSMountInfo.FSpwd.bSectorEntryPosition ;
	}

	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
	printf("\n Debug... Searching: %s", szFile) ;
	RETURN_X_IF_NOT(Directory_GetDirEntryInfo(pDiskDrive, &CWD, szFile, &uiSectorNo, &bSectorPos, bDirectoryBuffer), Directory_SUCCESS, FileOperations_FAILURE) ;

	FileSystem_DIR_Entry* pSrcDirEntry = &((FileSystem_DIR_Entry*)bDirectoryBuffer)[bSectorPos] ;

	pFileStat->st_dev = pDiskDrive->DriveNumber();
	pFileStat->st_mode = pSrcDirEntry->usAttribute ;
	pFileStat->st_uid = pSrcDirEntry->iUserID ;
	pFileStat->st_size = pSrcDirEntry->uiSize ;
	pFileStat->st_atime = pSrcDirEntry->AccessedTime ;
	pFileStat->st_mtime = pSrcDirEntry->ModifiedTime ;
	pFileStat->st_ctime = pSrcDirEntry->CreatedTime ;

	pFileStat->st_blksize = 512 ;
	pFileStat->st_blocks = (pSrcDirEntry->uiSize / 512) + ((pSrcDirEntry->uiSize % 512) ? 1 : 0 ) ;

	pFileStat->st_rdev = 0 ;
	pFileStat->st_gid = 1 ;
	pFileStat->st_nlink = 1 ;
	pFileStat->st_ino = 0 ;

	return FileOperations_SUCCESS ;
}

byte FileOperations_GetStatFD(int iFD, FileSystem_FileStat* pFileStat)
{
	ProcFileDescriptor* pFDEntry ;

	RETURN_X_IF_NOT(ProcFileManager_GetRealNonDuppedFD(iFD, &iFD), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;

	RETURN_X_IF_NOT(ProcFileManager_GetFDEntry(iFD, &pFDEntry), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;

	return FileOperations_GetStat(pFDEntry->szFileName, pFDEntry->iDriveID, pFileStat) ;
}

byte FileOperations_UpdateTime(const char* szFileName, int iDriveID, byte bTimeType)
{
	if(iDriveID == CURRENT_DRIVE)
		iDriveID = ProcessManager::Instance().GetCurrentPAS().iDriveID ;

	GET_DRIVE_FOR_FS_OPS(iDriveID, Directory_FAILURE) ;

	FileSystem_CWD CWD ;
	if(iDriveID == ProcessManager::Instance().GetCurrentPAS().iDriveID)
	{
		FileSystem_PresentWorkingDirectory* pPWD = (FileSystem_PresentWorkingDirectory*)&(ProcessManager::Instance().GetCurrentPAS().processPWD) ;
		CWD.pDirEntry = &pPWD->DirEntry ;
		CWD.uiSectorNo = pPWD->uiSectorNo ;
		CWD.bSectorEntryPosition = pPWD->bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = &(pDiskDrive->FSMountInfo.FSpwd.DirEntry) ;
		CWD.uiSectorNo = pDiskDrive->FSMountInfo.FSpwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pDiskDrive->FSMountInfo.FSpwd.bSectorEntryPosition ;
	}

	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
	RETURN_X_IF_NOT(Directory_GetDirEntryInfo(pDiskDrive, &CWD, szFileName, &uiSectorNo, &bSectorPos, bDirectoryBuffer), Directory_SUCCESS, FileOperations_FAILURE) ;

	FileSystem_DIR_Entry* pSrcDirEntry = &((FileSystem_DIR_Entry*)bDirectoryBuffer)[bSectorPos] ;
	if(bTimeType & DIR_ACCESS_TIME)
		SystemUtil_GetTimeOfDay(&(pSrcDirEntry->AccessedTime)) ;

	if(bTimeType & DIR_MODIFIED_TIME)
		SystemUtil_GetTimeOfDay(&(pSrcDirEntry->ModifiedTime)) ;

	RETURN_X_IF_NOT(Directory_RawWrite(pDiskDrive, uiSectorNo, uiSectorNo + 1, bDirectoryBuffer), Directory_SUCCESS, FileOperations_FAILURE) ;

	return FileOperations_SUCCESS ;
}

byte FileOperations_GetFileOpenMode(int fd, byte* mode)
{
	byte bStatus ;
	RETURN_X_IF_NOT(ProcFileManager_GetRealNonDuppedFD(fd, &fd), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;
	RETURN_IF_NOT(bStatus, ProcFileManager_GetMode(fd, mode), ProcFileManager_SUCCESS) ;
	return FileOperations_SUCCESS ;
}

byte FileOperations_SyncPWD()
{
	RETURN_X_IF_NOT(Directory_SyncPWD(&ProcessManager::Instance().GetCurrentPAS()), Directory_SUCCESS, FileOperations_FAILURE) ;
	return FileOperations_SUCCESS ;
}

byte FileOperations_ChangeDir(const char* szFileName)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

	ProcessAddressSpace* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;
	
	RETURN_X_IF_NOT(Directory_Change(szFile, iDriveID, pPAS), Directory_SUCCESS, FileOperations_FAILURE) ;

	return FileOperations_SUCCESS ;
}

byte FileOperations_GetDirectoryContent(const char* szPathAddress, FileSystem_DIR_Entry** pDirList, int* iListSize)
{
	int iDriveID ;
	char szPath[100] ;
	FileOperations_ParseFilePathWithDrive(szPathAddress, szPath, (unsigned*)&iDriveID) ;

	ProcessAddressSpace* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

	RETURN_X_IF_NOT(Directory_GetDirectoryContent(szPath, pPAS, iDriveID, pDirList, iListSize), Directory_SUCCESS, FileOperations_FAILURE) ;

	return FileOperations_SUCCESS ;
}

byte FileOperations_FileAccess(const char* szFileName, int iDriveID, int mode)
{
	char szFile[100] ;
	if(iDriveID == FROM_FILE)
	{
		FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;
	}
	else
	{
		String_Copy(szFile, szFileName) ;
	}

	byte bStatus ;
	FileSystem_DIR_Entry dirEntry ;
	ProcessAddressSpace* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

	GET_DRIVE_FOR_FS_OPS(iDriveID, Directory_FAILURE) ;

	RETURN_IF_NOT(bStatus, Directory_GetDirEntry(szFile, pPAS, iDriveID, &dirEntry), Directory_SUCCESS)

	unsigned short usFileAttr = dirEntry.usAttribute ;
	if(FILE_TYPE(usFileAttr) != ATTR_TYPE_FILE)
		return FileOperations_ERR_NOT_EXISTS ;
	int userType = FileOperations_GetUserType(pPAS->bIsKernelProcess, pPAS->iUserID, dirEntry.iUserID) ;

	if(!FileOperations_HasPermission(userType, usFileAttr, mode))
		return FileOperations_ERR_NO_PERM ;

	return FileOperations_SUCCESS;
}

byte FileOperations_Dup2(int oldFD, int newFD)
{
	RETURN_X_IF_NOT(ProcFileManager_Dup2(oldFD, newFD), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;
	return FileOperations_SUCCESS ;
}

byte FileOperations_ReInitStdFd(int StdFD)
{
	RETURN_X_IF_NOT(ProcFileManager_InitSTDFile(StdFD), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;
	return FileOperations_SUCCESS ;
}

