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

# include <ProcFileManager.h>
# include <DMM.h>
# include <StringUtil.h>
# include <Display.h>
# include <Atomic.h>

typedef struct
{
	short TotalFDCount ;
	short LastFDAssigned ;
} PACKED ProcFileManager_FDHeader;

constexpr int PROC_SYS_MAX_OPEN_FILES((PAGE_SIZE - sizeof(ProcFileManager_FDHeader)) / sizeof(ProcFileDescriptor));

#define PROCESS_FD_TABLE_HEADER ((ProcFileManager_FDHeader*)(PROCESS_FD_PAGE - GLOBAL_DATA_SEGMENT_BASE))
#define PROCESS_FD_TABLE		((ProcFileDescriptor*)(PROCESS_FD_PAGE - GLOBAL_DATA_SEGMENT_BASE + sizeof(ProcFileManager_FDHeader)))

#define DUPPED (100)

#define NORMAL_FILES_START_FD 3
#define IS_VALID_PROC_FD(fd) (fd >= 0 && fd < PROC_SYS_MAX_OPEN_FILES)

class FDTableGuard
{
public:
  FDTableGuard()
  {
    if(ProcessManager::Instance().IsKernelProcess(ProcessManager::Instance().GetCurProcId()))
      _fdMutex.Lock();
  }

  ~FDTableGuard()
  {
    if(ProcessManager::Instance().IsKernelProcess(ProcessManager::Instance().GetCurProcId()))
      _fdMutex.UnLock();
  }

  FDTableGuard(const FDTableGuard&) = delete;
  FDTableGuard& operator=(const FDTableGuard&) = delete;

private:
  static Mutex _fdMutex;
};

Mutex FDTableGuard::_fdMutex;

static unsigned ProcFileManager_GetPage(Process* processAddressSpace)
{
	unsigned uiPDEAddress = processAddressSpace->taskState.CR3_PDBR ;
	unsigned uiPDEIndex = ((PROCESS_FD_PAGE >> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((PROCESS_FD_PAGE >> 12) & 0x3FF) ;
	unsigned uiPTEAddress = ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] & 0xFFFFF000 ;
	return (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] & 0xFFFFF000) / PAGE_SIZE ;
}

void ProcFileManager_Initialize(__volatile__ unsigned uiPDEAddress, __volatile__ int iParentProcessID)
{
	unsigned uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();

	unsigned uiPDEIndex = ((PROCESS_FD_PAGE >> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((PROCESS_FD_PAGE >> 12) & 0x3FF) ;

	unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;

	// This page is a Read Only area for user process. 0x5 => 101 => User Domain, Read Only, Present Bit
	((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x5 ;
	Mem_FlushTLB();

	/*
	if(iParentProcessID != NO_PROCESS_ID)
	{
		__volatile__ ProcessAddressSpace* parentProcessAddrSpace = &ProcessManager_processAddressSpace[iParentProcessID] ;

		unsigned uiParentPageNo = ProcessEnv_GetProcessEnvPageNumber(parentProcessAddrSpace) ;

		char* pParentEnv = (char*)(uiParentPageNo * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE) ;

		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pParentEnv, MemUtil_GetDS(), (unsigned)pEnv, PAGE_SIZE) ;
	}
	*/

	ProcFileManager_FDHeader* fdHeader = (ProcFileManager_FDHeader*)(uiFreePageNo * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE);
	ProcFileDescriptor* fdTable = (ProcFileDescriptor*)(uiFreePageNo * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE + sizeof(ProcFileManager_FDHeader));

	fdHeader->TotalFDCount = 3 ;
	fdHeader->LastFDAssigned = -1 ;

	for(unsigned i = 0; i < PROC_SYS_MAX_OPEN_FILES; i++)
	{
		fdTable[i].iDriveID = PROC_FREE_FD ;
		fdTable[i].szFileName = NULL ;
	}

	for(unsigned i = PROC_STDIN; i <= PROC_STDERR; i++)
	{
		fdTable[i - PROC_STDIN].iDriveID = i ;
		fdTable[i - PROC_STDIN].mode = O_RDWR ;
		fdTable[i - PROC_STDIN].uiOffset = 0 ;
		fdTable[i - PROC_STDIN].uiFileSize = 0 ;
		fdTable[i - PROC_STDIN].szFileName = NULL;
		fdTable[i - PROC_STDIN].RefCount = 1 ;
	}
}

void ProcFileManager_UnInitialize(Process* processAddressSpace)
{
	unsigned uiPageNumber = ProcFileManager_GetPage(processAddressSpace);

	ProcFileDescriptor* fdTable = (ProcFileDescriptor*)(uiPageNumber * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE + sizeof(ProcFileManager_FDHeader));

	for(unsigned i = 0; i < PROC_SYS_MAX_OPEN_FILES; i++)
	{
		if(fdTable[i].szFileName && fdTable[i].iDriveID != PROC_FREE_FD)
			DMM_DeAllocateForKernel((unsigned)fdTable[i].szFileName) ;

		fdTable[i].iDriveID = PROC_FREE_FD ;
	}

	MemManager::Instance().DeAllocatePhysicalPage(uiPageNumber) ;
}

void ProcFileManager_InitForKernel()
{
	PROCESS_FD_TABLE_HEADER->TotalFDCount = 3 ;
	PROCESS_FD_TABLE_HEADER->LastFDAssigned = -1 ;

	for(unsigned i = 0; i < PROC_SYS_MAX_OPEN_FILES; i++)
		PROCESS_FD_TABLE[i].iDriveID = PROC_FREE_FD ;

	for(unsigned i = PROC_STDIN; i <= PROC_STDERR; i++)
	{
		PROCESS_FD_TABLE[i - PROC_STDIN].iDriveID = i ;
		PROCESS_FD_TABLE[i - PROC_STDIN].mode = O_RDWR ;
		PROCESS_FD_TABLE[i - PROC_STDIN].uiOffset = 0 ;
		PROCESS_FD_TABLE[i - PROC_STDIN].uiFileSize = 0 ;
		PROCESS_FD_TABLE[i - PROC_STDIN].szFileName = NULL;
		PROCESS_FD_TABLE[i - PROC_STDIN].RefCount = 1 ;
	}
}

int ProcFileManager_GetFD()
{
  FDTableGuard g;

	if(PROCESS_FD_TABLE_HEADER->TotalFDCount == PROC_SYS_MAX_OPEN_FILES)
    throw upan::exception(XLOC, "can't open new file - max open files limit %d reached", PROC_SYS_MAX_OPEN_FILES);

	if(PROCESS_FD_TABLE_HEADER->LastFDAssigned == PROC_SYS_MAX_OPEN_FILES - 1 || PROCESS_FD_TABLE_HEADER->LastFDAssigned == -1)
		PROCESS_FD_TABLE_HEADER->LastFDAssigned = NORMAL_FILES_START_FD ;

  for(int i = PROCESS_FD_TABLE_HEADER->LastFDAssigned; i < PROC_SYS_MAX_OPEN_FILES; i++)
	{
		if(PROCESS_FD_TABLE[i].iDriveID == PROC_FREE_FD)
		{
      PROCESS_FD_TABLE_HEADER->LastFDAssigned = i;

      PROCESS_FD_TABLE_HEADER->TotalFDCount++ ;

      return i;
		}
	}

  throw upan::exception(XLOC, "can't open new file - max open files limit %d reached", PROC_SYS_MAX_OPEN_FILES);
}

int ProcFileManager_AllocateFD(const char* szFileName, const byte mode, int iDriveID, const unsigned uiFileSize, unsigned uiStartSectorID)
{
  const int fd = ProcFileManager_GetFD();

	if(mode & O_APPEND)
    PROCESS_FD_TABLE[fd].uiOffset = uiFileSize ;
	else
    PROCESS_FD_TABLE[fd].uiOffset = 0 ;
	
  PROCESS_FD_TABLE[fd].mode = mode ;
  PROCESS_FD_TABLE[fd].uiFileSize = uiFileSize ;
  PROCESS_FD_TABLE[fd].iDriveID = iDriveID ;

  PROCESS_FD_TABLE[fd].szFileName = (char*)DMM_AllocateForKernel(strlen(szFileName) + 1)  ;
  strcpy(PROCESS_FD_TABLE[fd].szFileName, szFileName) ;

  PROCESS_FD_TABLE[fd].RefCount = 1 ;

  PROCESS_FD_TABLE[fd].iLastReadSectorIndex = 0 ;
  PROCESS_FD_TABLE[fd].uiLastReadSectorNumber = uiStartSectorID ;

  return fd;
}

byte ProcFileManager_FreeFD(int fd)
{
  FDTableGuard g;

	if(!IS_VALID_PROC_FD(fd))
		return ProcFileManager_ERR_INVALID_FD ;

	if(PROCESS_FD_TABLE[fd].iDriveID == PROC_FREE_FD)
		return ProcFileManager_FAILURE;

	if(PROCESS_FD_TABLE[fd].RefCount > 1)
		return ProcFileManager_ERR_REF_OPEN ;

	PROCESS_FD_TABLE[fd].RefCount-- ;
		
	if(PROCESS_FD_TABLE[fd].szFileName)
		DMM_DeAllocateForKernel((unsigned)PROCESS_FD_TABLE[fd].szFileName) ;

	if(PROCESS_FD_TABLE[fd].mode == DUPPED)
		PROCESS_FD_TABLE[ PROCESS_FD_TABLE[fd].iDriveID ].RefCount--;

	PROCESS_FD_TABLE[fd].iDriveID = PROC_FREE_FD ;

	PROCESS_FD_TABLE_HEADER->TotalFDCount-- ;

	return ProcFileManager_SUCCESS ;
}

void ProcFileManager_UpdateOffset(int fd, int seekType, int iOffset)
{
  fd = ProcFileManager_GetRealNonDuppedFD(fd);

	if(!IS_VALID_PROC_FD(fd))
    throw upan::exception(XLOC, "invalid file descriptor: %d", fd);

	switch(seekType)
	{
		case SEEK_SET:
      break ;

		case SEEK_CUR:
			iOffset = PROCESS_FD_TABLE[fd].uiOffset + iOffset ;
		break ;

		case SEEK_END:
			iOffset = PROCESS_FD_TABLE[fd].uiFileSize + iOffset ;
		break ;

		default:
      throw upan::exception(XLOC, "invalid file seek type: %d", seekType);
	}

	if(iOffset < 0)
    throw upan::exception(XLOC, "invalid file offset %d", iOffset);

	PROCESS_FD_TABLE[fd].uiOffset = iOffset ;
}

uint32_t ProcFileManager_GetOffset(int fd)
{
	if(!IS_VALID_PROC_FD(fd))
    throw upan::exception(XLOC, "invalid file descriptor:%d", fd);

  return PROCESS_FD_TABLE[ProcFileManager_GetRealNonDuppedFD(fd)].uiOffset ;
}

byte ProcFileManager_GetMode(int fd)
{
	if(!IS_VALID_PROC_FD(fd))
    throw upan::exception(XLOC, "invalid file descriptor: %d", fd);

  return PROCESS_FD_TABLE[fd].mode ;
}

ProcFileDescriptor* ProcFileManager_GetFDEntry(int fd)
{
	if(!IS_VALID_PROC_FD(fd))
    throw upan::exception(XLOC, "invalid file descriptor:%d", fd);

  return &PROCESS_FD_TABLE[fd] ;
}

byte ProcFileManager_IsSTDOUT(int fd)
{
	if(!IS_VALID_PROC_FD(fd))
		return ProcFileManager_ERR_INVALID_FD ;

	if(PROCESS_FD_TABLE[fd].iDriveID == PROC_STDOUT || PROCESS_FD_TABLE[fd].iDriveID == PROC_STDERR)
		return ProcessManager_SUCCESS ;

	return ProcFileManager_FAILURE ;
}

byte ProcFileManager_IsSTDIN(int fd)
{
	if(!IS_VALID_PROC_FD(fd))
		return ProcFileManager_ERR_INVALID_FD ;

	if(PROCESS_FD_TABLE[fd].iDriveID == PROC_STDIN)
		return ProcessManager_SUCCESS ;

	return ProcFileManager_FAILURE ;
}

byte ProcFileManager_Dup2(int oldFD, int newFD)
{
	if(!IS_VALID_PROC_FD(oldFD))
		return ProcFileManager_ERR_INVALID_FD ;

	if(!IS_VALID_PROC_FD(newFD))
		return ProcFileManager_ERR_INVALID_FD ;
	
	if(PROCESS_FD_TABLE[newFD].iDriveID != PROC_FREE_FD)
		ProcFileManager_FreeFD(newFD);

	PROCESS_FD_TABLE[newFD].uiFileSize = 0 ;
	PROCESS_FD_TABLE[newFD].szFileName = NULL ;

	PROCESS_FD_TABLE[newFD].mode = DUPPED ;
	PROCESS_FD_TABLE[newFD].iDriveID = oldFD ;
	PROCESS_FD_TABLE[newFD].RefCount = 1 ;

	PROCESS_FD_TABLE[oldFD].RefCount++ ;

	PROCESS_FD_TABLE_HEADER->TotalFDCount++ ;

	return ProcFileManager_SUCCESS ;
}

int ProcFileManager_GetRealNonDuppedFD(int dupedFD)
{
	if(!IS_VALID_PROC_FD(dupedFD))
    throw upan::exception(XLOC, "invalid file descriptor:%d", dupedFD);

	if(PROCESS_FD_TABLE[dupedFD].iDriveID == PROC_FREE_FD)
    throw upan::exception(XLOC, "%d fd is a free fd - can't find real fd", dupedFD);
	
	if(PROCESS_FD_TABLE[dupedFD].mode != DUPPED)
    return dupedFD ;

  return ProcFileManager_GetRealNonDuppedFD(PROCESS_FD_TABLE[dupedFD].iDriveID) ;
}

byte ProcFileManager_InitSTDFile(int StdFD)
{
	switch(StdFD)
	{
		case 0:
		case 1:
		case 2:
				break;
		default:
				return ProcFileManager_ERR_INVALID_STDFD;
	}

	ProcFileManager_FreeFD(StdFD) ;

	PROCESS_FD_TABLE[StdFD].iDriveID = StdFD + PROC_STDIN ;
	PROCESS_FD_TABLE[StdFD].mode = O_RDWR ;
	PROCESS_FD_TABLE[StdFD].uiOffset = 0 ;
	PROCESS_FD_TABLE[StdFD].uiFileSize = 0 ;
	PROCESS_FD_TABLE[StdFD].szFileName = NULL;
	PROCESS_FD_TABLE[StdFD].RefCount = 1 ;

	return ProcFileManager_SUCCESS ;
}

