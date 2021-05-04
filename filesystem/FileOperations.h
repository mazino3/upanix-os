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
#ifndef _FILE_OPERATIONS_H_
#define _FILE_OPERATIONS_H_

#define FileOperations_SUCCESS					0
#define FileOperations_ERR_NO_WRITE_PERM		1
#define FileOperations_ERR_NOT_EXISTS			2
#define FileOperations_ERR_NO_PERM				3
#define FileOperations_ERR_INVALID_FILE_ATTR	4
#define FileOperations_FAILURE					5

# include <Global.h>
# include <FileSystem.h>

#define ATTR_READ	0x4
#define ATTR_WRITE	0x2
#define ATTR_EXE	0x1

#define S_OWNER(perm)		((perm & 0x7) << 6)
#define S_GROUP(perm)		((perm & 0x7) << 3)
#define S_OTHERS(perm)		(perm & 0x7)

#define G_OWNER(perm)		((perm >> 6) & 0x7)
#define G_GROUP(perm)		((perm >> 3) & 0x7)
#define G_OTHERS(perm)		(perm & 0x7)

#define FILE_PERM_MASK	0x1FF
#define FILE_TYPE_MASK	0xF000

#define HAS_READ_PERM(perm)		((perm & 0x7) & ATTR_READ)
#define HAS_WRITE_PERM(perm)	((perm & 0x7) & ATTR_WRITE)
#define HAS_EXE_PERM(perm)		((perm & 0x7) & ATTR_EXE)

#define FILE_PERM(attr)	(attr & FILE_PERM_MASK)
#define FILE_TYPE(attr) (attr & FILE_TYPE_MASK)

#define S_ISDIR(attr) (FILE_TYPE(attr) == ATTR_TYPE_DIRECTORY)

#define FILE_STDOUT "STDOUT"
#define FILE_STDIN  "STDIN"
#define FILE_STDERR "STDERR"

typedef enum
{
	DIR_ACCESS_TIME = 0x01,
	DIR_MODIFIED_TIME = 0x02
} TIME_TYPE ;

typedef enum
{
	USER_OWNER,
	USER_GROUP,
	USER_OTHERS
} FILE_USER_TYPE ;

int FileOperations_Open(const char* szFileName, const byte mode) ;
byte FileOperations_Close(int fd) ;
bool FileOperations_ReadLine(int fd, upan::string& line);
int FileOperations_Read(int fd, char* buffer, int len) ;
void FileOperations_Write(int fd, const char* buffer, int len, int* pWriteLen) ;
void FileOperations_Create(const char* szFilePath, unsigned short usFileType, unsigned short usMode) ;
void FileOperations_Delete(const char* szFilePath) ;
bool FileOperations_Exists(const char* szFileName, unsigned short usFileType) ;
void FileOperations_Seek(int fd, int iOffset, int seekType) ;
void FileOperations_UpdateTime(const char* szFileName, int iDriveID, byte bTimeType) ;
uint32_t FileOperations_GetOffset(int fd) ;
const FileSystem::Node FileOperations_GetDirEntry(const char* szFileName);
const FileSystem_FileStat FileOperations_GetStat(const char* szFileName, int iDriveID) ;
byte FileOperations_GetFileOpenMode(int fd) ;
const FileSystem_FileStat FileOperations_GetStatFD(int iFD) ;
void FileOperations_SyncPWD() ;
void FileOperations_ChangeDir(const char* szFileName) ;
void FileOperations_GetDirectoryContent(const char* szPathAddress, FileSystem::Node** pDirList, int* iListSize) ;
bool FileOperations_FileAccess(const char* szFileName, int iDriveID, int mode) ;
void FileOperations_Dup2(int oldFD, int newFD) ;
void FileOperations_ReInitStdFd(int StdFD) ;
void FileOperations_GetCWD(char* szPathBuf, int iBufSize) ;

#endif
