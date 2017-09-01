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
# include <list.h>
# include <try.h>

/************************************************************************************************************/
static upan::result<unsigned short> FileOperations_ValidateAndGetFileAttr(unsigned short usFileType, unsigned short usMode)
{
	usMode = FILE_PERM(usMode) ;
	usFileType = FILE_TYPE(usFileType) ;

	if(!(usFileType == ATTR_TYPE_FILE || usFileType == ATTR_TYPE_DIRECTORY))
    return upan::result<unsigned short>::bad("invalid file attribute: %x", usFileType);

  return upan::good((unsigned short)(usFileType | usMode));
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
		strcpy(szFileName, szFileNameWithDrive) ;
		return ;
	}

	if(i > 32)
	{
		strcpy(szFileName, szFileNameWithDrive) ;
		return ;
	}

	char szDriveName[33] ;

  memcpy(szDriveName, szFileNameWithDrive, i);
	szDriveName[i] = '\0' ;

	strcpy(szFileName, szFileNameWithDrive + i + 1) ;

	if(strcmp(szDriveName, ROOT_DRIVE_SYN) == 0)
	{
		*pDriveID = ROOT_DRIVE_ID ;
	}
	else
	{
    DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByDriveName(szDriveName, false).goodValueOrElse(nullptr);
		if(pDiskDrive == NULL)
			*pDriveID = ROOT_DRIVE_ID ;
		else
			*pDriveID = pDiskDrive->Id();
	}
}

/************************************************************************************************************/
int FileOperations_Open(const char* szFileName, const byte mode)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  FileSystem_DIR_Entry dirEntry = Directory_GetDirEntry(szFile, pPAS, iDriveID);

	unsigned short usFileAttr = dirEntry.usAttribute ;
	if(FILE_TYPE(usFileAttr) != ATTR_TYPE_FILE)
    throw upan::exception(XLOC, "%s file doesn't exist", szFileName);

	int userType = FileOperations_GetUserType(pPAS->bIsKernelProcess, pPAS->iUserID, dirEntry.iUserID) ;

	if(!FileOperations_HasPermission(userType, usFileAttr, mode))
    throw upan::exception(XLOC, "insufficient permission to open file %s", szFileName);

	if( (mode & O_TRUNC) )
	{
    FileOperations_Delete(szFileName);
    FileOperations_Create(szFileName, FILE_TYPE(dirEntry.usAttribute), FILE_PERM(dirEntry.usAttribute));
    dirEntry = Directory_GetDirEntry(szFile, pPAS, iDriveID);
	}

  return ProcFileManager_AllocateFD(dirEntry.FullPath(*pDiskDrive).c_str(), mode, iDriveID, dirEntry.uiSize, dirEntry.uiStartSectorID);
}

byte FileOperations_Close(int fd)
{
	RETURN_X_IF_NOT(ProcFileManager_FreeFD(fd), ProcFileManager_SUCCESS, FileOperations_FAILURE) ;
	return FileOperations_SUCCESS ;
}

bool FileOperations_ReadLine(int fd, upan::string& line)
{
  line = "";
  const int CHUNK_SIZE = 64;
  char buffer[CHUNK_SIZE + 1];
  upan::list<upan::string> buffers;
  int line_size = 0;
  while(true)
  {
    int readLen = FileOperations_Read(fd, buffer, CHUNK_SIZE);
    
    if(readLen == 0)
      break;

    //find new line
    int i = 0;
    for(i = 0; i < readLen; ++i)
      if(buffer[i] == '\n')
        break;
    buffer[i] = '\0';

    line_size += i;
    buffers.push_back(buffer);

    int offset = i - readLen + 1;
    if(offset < 0)
    {
      FileOperations_Seek(fd, offset, SEEK_CUR);
      break;
    }
  }

  if(buffers.empty())
    return false;
  for(auto chunk : buffers)
    line += chunk;
  return true;
}

int FileOperations_Read(int fd, char* buffer, int len)
{
  fd = ProcFileManager_GetRealNonDuppedFD(fd);

	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

  ProcFileDescriptor* pFDEntry = ProcFileManager_GetFDEntry(fd);

	int iDriveID = pFDEntry->iDriveID ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);
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

  int readLen = Directory_FileRead(pDiskDrive, &CWD, pFDEntry, (byte*)buffer, len);

  FileOperations_UpdateTime(pFDEntry->szFileName, iDriveID, DIR_ACCESS_TIME);

  pFDEntry->uiOffset += readLen;

  return readLen;
}

void FileOperations_Write(int fd, const char* buffer, int len, int* pWriteLen)
{
  fd = ProcFileManager_GetRealNonDuppedFD(fd);

	*pWriteLen = -1 ;

	if(ProcFileManager_IsSTDOUT(fd) == ProcessManager_SUCCESS)
	{
		KC::MDisplay().nMessage(buffer, len, Display::WHITE_ON_BLACK()) ;
		*pWriteLen = len ;
    return;
	}

  ProcFileDescriptor* pFDEntry = ProcFileManager_GetFDEntry(fd);

	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

	if( !(pFDEntry->mode & O_WRONLY || pFDEntry->mode & O_RDWR || pFDEntry->mode & O_APPEND) )
    throw upan::exception(XLOC, "insufficient permission to write file fd: %d", fd);

	int iDriveID = pFDEntry->iDriveID ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

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
		
    Directory_FileWrite(pDiskDrive, &CWD, pFDEntry, (byte*)buffer, n);

		pFDEntry->uiOffset += n ;

		uiIncLen -= n ;

		if(uiIncLen == 0)
			break ;
	}

  FileOperations_UpdateTime(pFDEntry->szFileName, iDriveID, DIR_ACCESS_TIME | DIR_MODIFIED_TIME);
	
	*pWriteLen = len ;
}

void FileOperations_Create(const char* szFilePath, unsigned short usFileType, unsigned short usMode)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFilePath, szFile, (unsigned*)&iDriveID) ;

	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

  const unsigned short usFileAttr = FileOperations_ValidateAndGetFileAttr(usFileType, usMode).goodValueOrThrow(XLOC);

	byte bParentDirectoryBuffer[512] ;
	char szDirName[33] ;
	FileSystem_CWD CWD ;
	unsigned uiParentSectorNo ;
	byte bParentSectorPos ;

  Directory_GetDirEntryForCreateDelete(pPAS, iDriveID, szFile, szDirName, uiParentSectorNo, bParentSectorPos, bParentDirectoryBuffer);

	CWD.pDirEntry = ((FileSystem_DIR_Entry*)bParentDirectoryBuffer) + bParentSectorPos ;
	CWD.uiSectorNo = uiParentSectorNo ;
	CWD.bSectorEntryPosition = bParentSectorPos ;

	unsigned short usParentDirAttr = CWD.pDirEntry->usAttribute ;
	int userType = FileOperations_GetUserType(pPAS->bIsKernelProcess, pPAS->iUserID, CWD.pDirEntry->iUserID) ;

	if(!FileOperations_HasPermission(userType, usParentDirAttr, O_RDWR))
    throw upan::exception(XLOC, "insufficient permission to create file: %s", szFilePath);

  Directory_Create(pPAS, iDriveID, bParentDirectoryBuffer, &CWD, szDirName, usFileAttr);
}

void FileOperations_Delete(const char* szFilePath)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFilePath, szFile, (unsigned*)&iDriveID) ;

	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

	byte bParentDirectoryBuffer[512] ;
	char szDirName[33] ;
	FileSystem_CWD CWD ;
	unsigned uiParentSectorNo ;
	byte bParentSectorPos ;

  Directory_GetDirEntryForCreateDelete(pPAS, iDriveID, szFile, szDirName, uiParentSectorNo, bParentSectorPos, bParentDirectoryBuffer);

	CWD.pDirEntry = ((FileSystem_DIR_Entry*)bParentDirectoryBuffer) + bParentSectorPos ;
	CWD.uiSectorNo = uiParentSectorNo ;
	CWD.bSectorEntryPosition = bParentSectorPos ;

	unsigned short usParentDirAttr = CWD.pDirEntry->usAttribute ;
	int userType = FileOperations_GetUserType(pPAS->bIsKernelProcess, pPAS->iUserID, CWD.pDirEntry->iUserID) ;

	if(!FileOperations_HasPermission(userType, usParentDirAttr, O_RDWR))
    throw upan::exception(XLOC, "insufficient permission to delete file: %s", szFilePath);

  const FileSystem_DIR_Entry& fileDirEntry = FileOperations_GetDirEntry(szFilePath);

	if(FileOperations_GetUserType(pPAS->bIsKernelProcess, pPAS->iUserID, fileDirEntry.iUserID) != USER_OWNER)
    throw upan::exception(XLOC, "insufficient permission to delete file: %s", szFilePath);

  Directory_Delete(pPAS, iDriveID, bParentDirectoryBuffer, &CWD, szDirName);
}

bool FileOperations_Exists(const char* szFileName, unsigned short usFileType)
{
  try
  {
    int iDriveID ;
    char szFile[100] ;
    FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

    Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

    const FileSystem_DIR_Entry dirEntry = Directory_GetDirEntry(szFile, pPAS, iDriveID);
    if((dirEntry.usAttribute & usFileType) != usFileType)
      return false;
  }
  catch(const upan::exception& ex)
  {
    ex.Print();
    return false;
  }
  return true;
}

void FileOperations_Seek(int fd, int iOffset, int seekType)
{
  ProcFileManager_UpdateOffset(fd, seekType, iOffset);
}

uint32_t FileOperations_GetOffset(int fd)
{
  return ProcFileManager_GetOffset(fd);
}

void FileOperations_GetCWD(char* szPathBuf, int iBufSize)
{
	FileSystem_PresentWorkingDirectory* pPWD = (FileSystem_PresentWorkingDirectory*)&(ProcessManager::Instance().GetCurrentPAS().processPWD) ;
	int iDriveID = ProcessManager::Instance().GetCurrentPAS().iDriveID ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  const upan::string& fullPath = pPWD->DirEntry.FullPath(*pDiskDrive);

  if(fullPath.length() > iBufSize)
    throw upan::exception(XLOC, "%d buf-size is smaller than path size %d", iBufSize, fullPath.length());

  strcpy(szPathBuf, fullPath.c_str());
}

const FileSystem_DIR_Entry FileOperations_GetDirEntry(const char* szFileName)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

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
	
  Directory_ReadDirEntryInfo(*pDiskDrive, CWD, szFile, uiSectorNo, bSectorPos, bDirectoryBuffer);

  return ((FileSystem_DIR_Entry*)bDirectoryBuffer)[bSectorPos] ;
}

const FileSystem_FileStat FileOperations_GetStat(const char* szFileName, int iDriveID)
{
	char szFile[100] ;
	if(iDriveID == FROM_FILE)
	{
		FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;
	}
	else
	{
		strcpy(szFile, szFileName) ;
	}

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

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
  Directory_ReadDirEntryInfo(*pDiskDrive, CWD, szFile, uiSectorNo, bSectorPos, bDirectoryBuffer);

	FileSystem_DIR_Entry* pSrcDirEntry = &((FileSystem_DIR_Entry*)bDirectoryBuffer)[bSectorPos] ;

  FileSystem_FileStat fileStat;

  fileStat.st_dev = pDiskDrive->DriveNumber();
  fileStat.st_mode = pSrcDirEntry->usAttribute ;
  fileStat.st_uid = pSrcDirEntry->iUserID ;
  fileStat.st_size = pSrcDirEntry->uiSize ;
  fileStat.st_atime = pSrcDirEntry->AccessedTime ;
  fileStat.st_mtime = pSrcDirEntry->ModifiedTime ;
  fileStat.st_ctime = pSrcDirEntry->CreatedTime ;

  fileStat.st_blksize = 512 ;
  fileStat.st_blocks = (pSrcDirEntry->uiSize / 512) + ((pSrcDirEntry->uiSize % 512) ? 1 : 0 ) ;

  fileStat.st_rdev = 0 ;
  fileStat.st_gid = 1 ;
  fileStat.st_nlink = 1 ;
  fileStat.st_ino = 0 ;

  return fileStat;
}

const FileSystem_FileStat FileOperations_GetStatFD(int iFD)
{
  iFD = ProcFileManager_GetRealNonDuppedFD(iFD);
  const ProcFileDescriptor* pFDEntry = ProcFileManager_GetFDEntry(iFD);
  return FileOperations_GetStat(pFDEntry->szFileName, pFDEntry->iDriveID);
}

void FileOperations_UpdateTime(const char* szFileName, int iDriveID, byte bTimeType)
{
	if(iDriveID == CURRENT_DRIVE)
		iDriveID = ProcessManager::Instance().GetCurrentPAS().iDriveID ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

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
	
  Directory_ReadDirEntryInfo(*pDiskDrive, CWD, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

	FileSystem_DIR_Entry* pSrcDirEntry = &((FileSystem_DIR_Entry*)bDirectoryBuffer)[bSectorPos] ;
	if(bTimeType & DIR_ACCESS_TIME)
		SystemUtil_GetTimeOfDay(&(pSrcDirEntry->AccessedTime)) ;

	if(bTimeType & DIR_MODIFIED_TIME)
		SystemUtil_GetTimeOfDay(&(pSrcDirEntry->ModifiedTime)) ;

  pDiskDrive->xWrite(bDirectoryBuffer, uiSectorNo, 1);
}

byte FileOperations_GetFileOpenMode(int fd)
{
  return ProcFileManager_GetMode(ProcFileManager_GetRealNonDuppedFD(fd));
}

void FileOperations_SyncPWD()
{
  Directory_SyncPWD(&ProcessManager::Instance().GetCurrentPAS());
}

void FileOperations_ChangeDir(const char* szFileName)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;
	
  Directory_Change(szFile, iDriveID, pPAS);
}

void FileOperations_GetDirectoryContent(const char* szPathAddress, FileSystem_DIR_Entry** pDirList, int* iListSize)
{
	int iDriveID ;
	char szPath[100] ;
	FileOperations_ParseFilePathWithDrive(szPathAddress, szPath, (unsigned*)&iDriveID) ;

	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

  Directory_GetDirectoryContent(szPath, pPAS, iDriveID, pDirList, iListSize);
}

bool FileOperations_FileAccess(const char* szFileName, int iDriveID, int mode)
{
  try
  {
    char szFile[100] ;
    if(iDriveID == FROM_FILE)
    {
      FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;
    }
    else
    {
      strcpy(szFile, szFileName) ;
    }

    Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

    const FileSystem_DIR_Entry& dirEntry = Directory_GetDirEntry(szFile, pPAS, iDriveID);

    unsigned short usFileAttr = dirEntry.usAttribute ;
    if(FILE_TYPE(usFileAttr) != ATTR_TYPE_FILE)
      return false;
    int userType = FileOperations_GetUserType(pPAS->bIsKernelProcess, pPAS->iUserID, dirEntry.iUserID) ;

    if(!FileOperations_HasPermission(userType, usFileAttr, mode))
      return false;
  }
  catch(const upan::exception& ex)
  {
    ex.Print();
    return false;
  }

  return true;
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

