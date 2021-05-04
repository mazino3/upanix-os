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

static void FileOperations_ParseFilePathWithDrive(const char* szFileNameWithDrive, char* szFileName, unsigned* pDriveID)
{
	int i = String_Chr(szFileNameWithDrive, '@') ;
	
	if(i == -1)
	{
		*pDriveID = ProcessManager::Instance().GetCurrentPAS().driveID() ;
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

  FileSystem::Node dirEntry = Directory_GetDirEntry(szFile, pPAS, iDriveID);

  if(!dirEntry.IsFile())
    throw upan::exception(XLOC, "%s file doesn't exist", szFileName);

  if(!pPAS->hasFilePermission(dirEntry, mode))
    throw upan::exception(XLOC, "insufficient permission to open file %s", szFileName);

	if( (mode & O_TRUNC) )
	{
    FileOperations_Delete(szFileName);
    FileOperations_Create(szFileName, FILE_TYPE(dirEntry.Attribute()), FILE_PERM(dirEntry.Attribute()));
    dirEntry = Directory_GetDirEntry(szFile, pPAS, iDriveID);
	}

  return pPAS->fdTable().allocate(dirEntry.FullPath(*pDiskDrive), mode, iDriveID, dirEntry.Size(), dirEntry.StartSectorID());
}

byte FileOperations_Close(int fd) {
  try {
    ProcessManager::Instance().GetCurrentPAS().fdTable().free(fd);
  } catch(upan::exception& e) {
    e.Print();
    return FileOperations_FAILURE;
  }
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
	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;
  auto& fdEntry = pPAS->fdTable().getRealNonDupped(fd);

	int iDriveID = fdEntry.getDriveId();

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);
  FileSystem::CWD CWD ;
	if(pPAS->driveID() == iDriveID)
	{
		CWD.pDirEntry = &(pPAS->processPWD().DirEntry) ;
		CWD.uiSectorNo = pPAS->processPWD().uiSectorNo ;
		CWD.bSectorEntryPosition = pPAS->processPWD().bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = &(pDiskDrive->_fileSystem.FSpwd.DirEntry) ;
		CWD.uiSectorNo = pDiskDrive->_fileSystem.FSpwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pDiskDrive->_fileSystem.FSpwd.bSectorEntryPosition ;
	}

  int readLen = Directory_FileRead(pDiskDrive, &CWD, fdEntry, (byte*)buffer, len);

  FileOperations_UpdateTime(fdEntry.getFileName().c_str(), iDriveID, DIR_ACCESS_TIME);

  fdEntry.addOffset(readLen);

  return readLen;
}

void FileOperations_Write(int fd, const char* buffer, int len, int* pWriteLen)
{
  Process* pPAS = &ProcessManager::Instance().GetCurrentPAS();
  auto& f = pPAS->fdTable().getRealNonDupped(fd);

	*pWriteLen = -1 ;

	if(f.isStdOut()) {
		KC::MDisplay().nMessage(buffer, len, Display::WHITE_ON_BLACK()) ;
		*pWriteLen = len ;
    return;
	}

	if( !(f.getMode() & O_WRONLY || f.getMode() & O_RDWR || f.getMode() & O_APPEND) )
    throw upan::exception(XLOC, "insufficient permission to write file fd: %d", fd);

	int iDriveID = f.getDriveId();

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  FileSystem::CWD CWD ;
	if(pPAS->driveID() == iDriveID)
	{
		CWD.pDirEntry = &(pPAS->processPWD().DirEntry) ;
		CWD.uiSectorNo = pPAS->processPWD().uiSectorNo ;
		CWD.bSectorEntryPosition = pPAS->processPWD().bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = &(pDiskDrive->_fileSystem.FSpwd.DirEntry) ;
		CWD.uiSectorNo = pDiskDrive->_fileSystem.FSpwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pDiskDrive->_fileSystem.FSpwd.bSectorEntryPosition ;
	}

	unsigned uiIncLen = len ;
	unsigned uiLimit = 1 MB ;
	unsigned n = 0;
	
	while(true)
	{
		n = (uiIncLen > uiLimit) ? uiLimit : uiIncLen ;
		
    Directory_FileWrite(pDiskDrive, &CWD, f, (byte*)buffer, n);

    f.addOffset(n);

		uiIncLen -= n ;

		if(uiIncLen == 0)
			break ;
	}

  FileOperations_UpdateTime(f.getFileName().c_str(), iDriveID, DIR_ACCESS_TIME | DIR_MODIFIED_TIME);
	
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
  FileSystem::CWD CWD ;
	unsigned uiParentSectorNo ;
	byte bParentSectorPos ;

  Directory_GetDirEntryForCreateDelete(pPAS, iDriveID, szFile, szDirName, uiParentSectorNo, bParentSectorPos, bParentDirectoryBuffer);

  CWD.pDirEntry = ((FileSystem::Node*)bParentDirectoryBuffer) + bParentSectorPos ;
	CWD.uiSectorNo = uiParentSectorNo ;
	CWD.bSectorEntryPosition = bParentSectorPos ;

  if(!pPAS->hasFilePermission(*CWD.pDirEntry, O_RDWR))
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
  FileSystem::CWD CWD ;
	unsigned uiParentSectorNo ;
	byte bParentSectorPos ;

  Directory_GetDirEntryForCreateDelete(pPAS, iDriveID, szFile, szDirName, uiParentSectorNo, bParentSectorPos, bParentDirectoryBuffer);

  CWD.pDirEntry = ((FileSystem::Node*)bParentDirectoryBuffer) + bParentSectorPos ;
	CWD.uiSectorNo = uiParentSectorNo ;
	CWD.bSectorEntryPosition = bParentSectorPos ;

  if(!pPAS->hasFilePermission(*CWD.pDirEntry, O_RDWR))
    throw upan::exception(XLOC, "insufficient permission to delete file: %s", szFilePath);

  const FileSystem::Node& fileDirEntry = FileOperations_GetDirEntry(szFilePath);

  if(pPAS->fileUserType(fileDirEntry) != USER_OWNER)
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

    const FileSystem::Node dirEntry = Directory_GetDirEntry(szFile, pPAS, iDriveID);
    if((dirEntry.Attribute() & usFileType) != usFileType)
      return false;
  }
  catch(const upan::exception& ex)
  {
    ex.Print();
    return false;
  }
  return true;
}

void FileOperations_Seek(int fd, int iOffset, int seekType) {
  ProcessManager::Instance().GetCurrentPAS().fdTable().updateOffset(fd, seekType, iOffset);
}

uint32_t FileOperations_GetOffset(int fd) {
  return ProcessManager::Instance().GetCurrentPAS().fdTable().getOffset(fd);
}

void FileOperations_GetCWD(char* szPathBuf, int iBufSize) {
  FileSystem::PresentWorkingDirectory& pwd = ProcessManager::Instance().GetCurrentPAS().processPWD();
	int iDriveID = ProcessManager::Instance().GetCurrentPAS().driveID() ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  const upan::string& fullPath = pwd.DirEntry.FullPath(*pDiskDrive);

  if(fullPath.length() > iBufSize)
    throw upan::exception(XLOC, "%d buf-size is smaller than path size %d", iBufSize, fullPath.length());

  strcpy(szPathBuf, fullPath.c_str());
}

const FileSystem::Node FileOperations_GetDirEntry(const char* szFileName)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  FileSystem::CWD CWD ;
	if(iDriveID == ProcessManager::Instance().GetCurrentPAS().driveID())
	{
    FileSystem::PresentWorkingDirectory& pwd = ProcessManager::Instance().GetCurrentPAS().processPWD();
		CWD.pDirEntry = &pwd.DirEntry ;
		CWD.uiSectorNo = pwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pwd.bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = &(pDiskDrive->_fileSystem.FSpwd.DirEntry) ;
		CWD.uiSectorNo = pDiskDrive->_fileSystem.FSpwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pDiskDrive->_fileSystem.FSpwd.bSectorEntryPosition ;
	}

	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
  Directory_ReadDirEntryInfo(*pDiskDrive, CWD, szFile, uiSectorNo, bSectorPos, bDirectoryBuffer);

  return ((FileSystem::Node*)bDirectoryBuffer)[bSectorPos] ;
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

  FileSystem::CWD CWD ;

	if(iDriveID == ProcessManager::Instance().GetCurrentPAS().driveID())
	{
    FileSystem::PresentWorkingDirectory& pwd = ProcessManager::Instance().GetCurrentPAS().processPWD();
		CWD.pDirEntry = &pwd.DirEntry ;
		CWD.uiSectorNo = pwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pwd.bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = &(pDiskDrive->_fileSystem.FSpwd.DirEntry) ;
		CWD.uiSectorNo = pDiskDrive->_fileSystem.FSpwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pDiskDrive->_fileSystem.FSpwd.bSectorEntryPosition ;
	}

	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
	printf("\n Debug... Searching: %s", szFile) ;
  Directory_ReadDirEntryInfo(*pDiskDrive, CWD, szFile, uiSectorNo, bSectorPos, bDirectoryBuffer);

  FileSystem::Node* pSrcDirEntry = &((FileSystem::Node*)bDirectoryBuffer)[bSectorPos] ;

  FileSystem_FileStat fileStat;

  fileStat.st_dev = pDiskDrive->DriveNumber();
  fileStat.st_mode = pSrcDirEntry->Attribute() ;
  fileStat.st_uid = pSrcDirEntry->UserID() ;
  fileStat.st_size = pSrcDirEntry->Size() ;
  fileStat.st_atime = pSrcDirEntry->AccessedTime() ;
  fileStat.st_mtime = pSrcDirEntry->ModifiedTime() ;
  fileStat.st_ctime = pSrcDirEntry->CreatedTime() ;

  fileStat.st_blksize = 512 ;
  fileStat.st_blocks = (pSrcDirEntry->Size() / 512) + ((pSrcDirEntry->Size() % 512) ? 1 : 0 ) ;

  fileStat.st_rdev = 0 ;
  fileStat.st_gid = 1 ;
  fileStat.st_nlink = 1 ;
  fileStat.st_ino = 0 ;

  return fileStat;
}

const FileSystem_FileStat FileOperations_GetStatFD(int iFD) {
  const auto& fdEntry = ProcessManager::Instance().GetCurrentPAS().fdTable().getRealNonDupped(iFD);
  return FileOperations_GetStat(fdEntry.getFileName().c_str(), fdEntry.getDriveId());
}

void FileOperations_UpdateTime(const char* szFileName, int iDriveID, byte bTimeType)
{
	if(iDriveID == CURRENT_DRIVE)
		iDriveID = ProcessManager::Instance().GetCurrentPAS().driveID() ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  FileSystem::CWD CWD ;
	if(iDriveID == ProcessManager::Instance().GetCurrentPAS().driveID())
	{
    FileSystem::PresentWorkingDirectory& pwd = ProcessManager::Instance().GetCurrentPAS().processPWD();
		CWD.pDirEntry = &pwd.DirEntry ;
		CWD.uiSectorNo = pwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pwd.bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = &(pDiskDrive->_fileSystem.FSpwd.DirEntry) ;
		CWD.uiSectorNo = pDiskDrive->_fileSystem.FSpwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pDiskDrive->_fileSystem.FSpwd.bSectorEntryPosition ;
	}

	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
  Directory_ReadDirEntryInfo(*pDiskDrive, CWD, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

  FileSystem::Node* pSrcDirEntry = &((FileSystem::Node*)bDirectoryBuffer)[bSectorPos] ;
	if(bTimeType & DIR_ACCESS_TIME)
    pSrcDirEntry->AccessedTime(SystemUtil_GetTimeOfDay());

	if(bTimeType & DIR_MODIFIED_TIME)
    pSrcDirEntry->ModifiedTime(SystemUtil_GetTimeOfDay());

  pDiskDrive->xWrite(bDirectoryBuffer, uiSectorNo, 1);
}

byte FileOperations_GetFileOpenMode(int fd) {
  return ProcessManager::Instance().GetCurrentPAS().fdTable().getRealNonDupped(fd).getMode();
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

void FileOperations_GetDirectoryContent(const char* szPathAddress, FileSystem::Node** pDirList, int* iListSize)
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

    const FileSystem::Node& dirEntry = Directory_GetDirEntry(szFile, pPAS, iDriveID);

    if(!dirEntry.IsFile())
      return false;

    if(!pPAS->hasFilePermission(dirEntry, mode))
      return false;
  }
  catch(const upan::exception& ex)
  {
    ex.Print();
    return false;
  }

  return true;
}

void FileOperations_Dup2(int oldFD, int newFD) {
  ProcessManager::Instance().GetCurrentPAS().fdTable().dup2(oldFD, newFD);
}

void FileOperations_ReInitStdFd(int stdFD) {
  ProcessManager::Instance().GetCurrentPAS().fdTable().initStdFile(stdFD);
}

