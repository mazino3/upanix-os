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
#ifndef _FileSystem_H_
#define _FileSystem_H_

#include <Global.h>
#include <queue.h>
#include <vector.h>

#define MEDIA_REMOVABLE	0xF0
#define MEDIA_FIXED		0xF8

#define EOC		0x0FFFFFFF
#define EOC_B	0xFF
#define DIR_ENTRIES_PER_SECTOR 7

#define FS_ROOT_DIR "/"

#define ENTRIES_PER_TABLE_SECTOR	(128)

class DiskDrive;

class FSBootBlock
{
public:
  byte			BPB_jmpBoot[3] ;

  byte			BPB_Media ;
  unsigned short	BPB_SecPerTrk ;
  unsigned short	BPB_NumHeads ;

  unsigned short	BPB_BytesPerSec ;
  unsigned		BPB_TotSec32 ;
  unsigned		BPB_HiddSec ;

  unsigned short	BPB_RsvdSecCnt ;
  unsigned		BPB_FSTableSize ;

  unsigned short	BPB_ExtFlags ;
  unsigned short	BPB_FSVer ;
  unsigned short	BPB_FSInfo ;

  byte			BPB_BootSig ;
  unsigned		BPB_VolID ;
  byte			BPB_VolLab[11 + 1] ;

  unsigned		uiUsedSectors ;

  void Init(const DiskDrive& diskDrive);

} PACKED;

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
*/

class FileSystem_DIR_Entry
{
public:
  void Init(char* szDirName, unsigned short usDirAttribute, int iUserID, unsigned uiParentSecNo, byte bParentSecPos);
  void InitAsRoot(uint32_t parentSectorId);
  upan::string FullPath(DiskDrive& diskDrive);

  byte            Name[33] ;
  struct timeval  CreatedTime ;
  struct timeval  AccessedTime ;
  struct timeval  ModifiedTime ;
  byte            bParentSectorPos ;
  unsigned short  usAttribute ;
  unsigned        uiSize ;
  unsigned        uiStartSectorID ;
  unsigned        uiParentSecID ;
  int             iUserID ;
} PACKED;

typedef struct
{
  FileSystem_DIR_Entry DirEntry ;
  unsigned uiSectorNo ;
  byte bSectorEntryPosition ;
} PACKED FileSystem_PresentWorkingDirectory ;

typedef struct
{
  FileSystem_DIR_Entry* pDirEntry ;
  unsigned uiSectorNo ;
  byte bSectorEntryPosition ;
} PACKED FileSystem_CWD ;

class DiskDrive;

typedef struct
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
} FileSystem_FileStat ;

uint32_t FileSystem_DeAllocateSector(DiskDrive* pDiskDrive, unsigned uiCurrentSectorID) ;
unsigned FileSystem_GetSizeForTableCache(unsigned uiNoOfSectorsInTableCache) ;

#endif

