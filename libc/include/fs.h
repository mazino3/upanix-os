/*
 *	Mother Operating System - An x86 based Operating System
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
#ifndef _FS_H_
#define _FS_H_

# include <FSStructures.h>
# include <dtime.h>
# include <stdlib.h>

#define ATTR_DIR_DEFAULT	0x01ED  //0000 0001 1110 1101 => 0000(Rsv) 000(Dir) 111(u:rwx) 101(g:r-x) 101(o:r-x)
#define ATTR_FILE_DEFAULT	0x03A4  //0000(Rsv) 001(File) 110(u:rw-) 100(g:r--) 100(o:r--)
#define ATTR_DELETED_DIR	0x1000
#define ATTR_TYPE_DIRECTORY	0x2000
#define ATTR_TYPE_FILE		0x4000

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

#define	SEEK_SET 0
#define	SEEK_CUR 1
#define	SEEK_END 2

#ifndef DEF_FILE_MODES
typedef enum
{	
	O_RDONLY = 1,
	O_WRONLY = 2,
	O_RDWR = 4,
	O_APPEND = 8,
	O_CREAT = 16,
	O_EXCL = 32,
	O_TRUNC = 64
} FileModes ;
#endif

/*
typedef struct
{
	byte bSecond ;
	byte bMinute ;
	byte bHour ;

	byte bDayOfWeek_Month ;
	byte bDayOfMonth ;
	byte bCentury ;
	byte bYear ;
} PACKED FileSystem_Time ;

typedef struct
{
	byte			Name[33] ;
	FileSystem_Time	CreatedTime ;
	FileSystem_Time	AccessedTime ;
	FileSystem_Time	ModifiedTime ;
	byte	 		bParentSectorPos ;
	unsigned short	usAttribute ;
	unsigned		uiSize ;
	unsigned		uiStartSectorID ;
	unsigned		uiParentSecID ;
	int				iUserID ;
} PACKED FileSystem_DIR_Entry ;
*/

/* This is same as FileSystem_FileStat in MOS Source and must be maintained in consistent with that */
struct stat
{
	int 	    st_dev;     /* ID of device containing file */
	int     	st_ino;     /* inode number */
	unsigned short    	st_mode;    /* protection */
	int   		st_nlink;   /* number of hard links */
	int     	st_uid;     /* user ID of owner */
	int     	st_gid;     /* group ID of owner */
	int     	st_rdev;    /* device ID (if special file) */
	unsigned    st_size;    /* total size, in bytes */
	unsigned	st_blksize; /* blocksize for filesystem I/O */
	unsigned  	st_blocks;  /* number of blocks allocated */

	struct timeval    	st_atime;   /* time of last access */
	struct timeval    	st_mtime;   /* time of last modification */
	struct timeval   	st_ctime;   /* time of last status change */
} ;

extern int SysFS_ChangeDirectory(const char* szDirPath) ;
extern void SysFS_PWD(char** uiReturnDirPathAddress) ;
extern int SysFS_CreateDirectory(const char* szDirPath, unsigned short usAttribute) ;
extern int SysFS_DeleteDirectory(const char* szDirPath) ;
extern int SysFS_GetDirContent(const char* szDirPath, FileSystem_DIR_Entry** pDirList, int* iListSize) ;
extern int SysFS_CreateFile(const char* szDirPath, unsigned short usAttribute) ;
extern int SysFS_FileOpen(const char* szFileName, byte bMode) ;
extern int SysFS_FileClose(int fd) ;
extern int SysFS_FileRead(int fd, char* buf, int len) ;
extern int SysFS_FileWrite(int fd, const char* buf, int len) ;

#define chdir(dir_path) SysFS_ChangeDirectory(dir_path)
#define getpwd(pwd_addr) SysFS_PWD(pwd_addr)
#define mkdir(dir_path, dir_attr) SysFS_CreateDirectory(dir_path, dir_attr)
#define unlink(dir_path) SysFS_DeleteDirectory(dir_path)
#define get_dir_content(dir_path, dirlist, listsize) SysFS_GetDirContent(dir_path, dirlist, listsize)

int create(const char* file_path, unsigned short file_attr) ;
int open(const char* file_name, byte mode) ;
int close(int fd) ;

int read(int fd, char* buf, int len) ;
int write(int fd, const char* buf, int len) ;
int lseek(int fd, int offset, int seekType) ;
unsigned tell(int fd) ;
int stat(const char* szFileName, struct stat* pFileStat) ;
int fstat(int iFD, struct stat* pFileStat) ;
int getomode(int fd) ;

#endif
