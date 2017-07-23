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
#ifndef _FS_STRUCTURES_H_
#define _FS_STRUCTURES_H_

#include <queue.h>

#define ENTRIES_PER_TABLE_SECTOR	(128)

typedef struct
{
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
} PACKED FileSystem_BootBlock ;

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

typedef struct
{
	byte			Name[33] ;
	struct timeval			CreatedTime ;
	struct timeval			AccessedTime ;
	struct timeval			ModifiedTime ;
	byte	 		bParentSectorPos ;
	unsigned short	usAttribute ;
	unsigned		uiSize ;
	unsigned		uiStartSectorID ;
	unsigned		uiParentSecID ;
	int				iUserID ;
} PACKED FileSystem_DIR_Entry ;

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

class SectorBlockEntry
{
public:
  uint32_t* SectorBlock() { return _sectorBlock; }
  const uint32_t BlockId() const { return _blockId; }
  const uint32_t ReadCount() const { return _readCount; }
  const uint32_t WriteCount() const { return _writeCount; }

  byte Load(DiskDrive& diskDrive, uint32_t sectortId);
  uint32_t Read(uint32_t sectorId);
  void Write(uint32_t sectorId, uint32_t value);

private:
  uint32_t _sectorBlock[ENTRIES_PER_TABLE_SECTOR];
  uint32_t _blockId;
  uint32_t _readCount;
  uint32_t _writeCount;
} PACKED;

typedef struct
{
	SectorBlockEntry* pSectorBlockEntryList ;
	int iSize ;
} PACKED FileSystem_TableCache ;

class FileSystemMountInfo
{
  public:
    FileSystemMountInfo() : pFreePoolQueue(nullptr)
    {
    }
    ~FileSystemMountInfo()
    {
      if(pFreePoolQueue)
        delete pFreePoolQueue;
    }
    upan::queue<unsigned>* pFreePoolQueue;
    FileSystem_TableCache FSTableCache;
    //Ouput
    FileSystem_BootBlock FSBootBlock; 
    FileSystem_PresentWorkingDirectory FSpwd;
};

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

#endif
