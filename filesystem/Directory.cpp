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
#include <Directory.h>
#include <StringUtil.h>
#include <MemUtil.h>
#include <DeviceDrive.h>
#include <Display.h>
#include <DMM.h>
#include <ProcessEnv.h>
#include <SystemUtil.h>
#include <FileOperations.h>

#define MAX_SECTORS_PER_RW 8

/************************************* Static Functions ***********************************/
static void Directory_BufferedWrite(DiskDrive& diskDrive, unsigned uiSectorID, byte* bSectorBuffer, byte* bBuffer,
                                    unsigned& uiStartSectorID, unsigned& uiPrevSectorID, unsigned& iCount, bool bFlush)
{
  bool bNewBuffering = false ;

	if(bFlush)
	{
    if(iCount > 0)
      diskDrive.xWrite(bBuffer, uiStartSectorID, iCount);

    iCount = 0 ;
    return;
	}

  if(iCount == 0)
	{
    uiStartSectorID = uiPrevSectorID = uiSectorID ;
    memcpy(bBuffer, bSectorBuffer, 512);
    iCount = 1 ;
	}
  else if(uiPrevSectorID + 1 == uiSectorID)
	{
    uiPrevSectorID = uiSectorID ;
    memcpy(bBuffer + iCount * 512, bSectorBuffer, 512);
    ++iCount;
	}
	else
	{
		bNewBuffering = true ;
	}

  if(iCount == MAX_SECTORS_PER_RW || bNewBuffering)
	{
    diskDrive.xWrite(bBuffer, uiStartSectorID, iCount);

    iCount = 0 ;
		
		if(bNewBuffering)
		{	
      uiStartSectorID = uiPrevSectorID = uiSectorID ;
      memcpy(bBuffer, bSectorBuffer, 512);
      iCount = 1 ;
		}
	}
}

void Directory_GetLastReadSectorDetails(const ProcFileDescriptor* pFDEntry, int* iSectorIndex, unsigned* uiSectorID)
{
	*iSectorIndex = pFDEntry->iLastReadSectorIndex ;
	*uiSectorID = pFDEntry->uiLastReadSectorNumber ;
}

void Directory_SetLastReadSectorDetails(ProcFileDescriptor* pFDEntry, int iSectorIndex, unsigned uiSectorID)
{
	pFDEntry->iLastReadSectorIndex = iSectorIndex ;
	pFDEntry->uiLastReadSectorNumber = uiSectorID ;
}

/**********************************************************************************************/

void Directory_Create(Process* processAddressSpace, int iDriveID, byte* bParentDirectoryBuffer, const FileSystem_CWD* pCWD,
                      char* szDirName, unsigned short usDirAttribute)
{
	byte bSectorBuffer[512] ;
	unsigned uiSectorNo ;
	byte bSectorPos ;
	unsigned uiFreeSectorID ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

	FileSystem_PresentWorkingDirectory* pPWD = &(processAddressSpace->processPWD) ;

	if(pCWD->pDirEntry->uiStartSectorID == EOC)
	{
    uiFreeSectorID = pDiskDrive->_fileSystem.AllocateSector();
		uiSectorNo = uiFreeSectorID ;
		bSectorPos = 0 ;
		pCWD->pDirEntry->uiStartSectorID = uiFreeSectorID ;
	}
	else
	{
    if(Directory_FindDirectory(*pDiskDrive, *pCWD, szDirName, uiSectorNo, bSectorPos, bSectorBuffer))
      throw upan::exception(XLOC, "directory %s already exists", szDirName);

		if(bSectorPos == EOC_B)
		{
      uiFreeSectorID = pDiskDrive->_fileSystem.AllocateSector();
      pDiskDrive->_fileSystem.SetSectorEntryValue(uiSectorNo, uiFreeSectorID);
			uiSectorNo = uiFreeSectorID ;
			bSectorPos = 0 ;
		}
	}

  ((FileSystem_DIR_Entry*)bSectorBuffer)[bSectorPos].Init(szDirName, usDirAttribute, processAddressSpace->iUserID, pCWD->uiSectorNo, pCWD->bSectorEntryPosition);

  pDiskDrive->xWrite(bSectorBuffer, uiSectorNo, 1);

	pCWD->pDirEntry->uiSize++ ;

  pDiskDrive->xWrite(bParentDirectoryBuffer, pCWD->uiSectorNo, 1);

  if(pDiskDrive->Id() == processAddressSpace->iDriveID
     && pCWD->uiSectorNo == pPWD->uiSectorNo
     && pCWD->bSectorEntryPosition == pPWD->bSectorEntryPosition)
    pPWD->DirEntry = *pCWD->pDirEntry;

	//TODO: Required Only If "/" Dir Entry is Created
	if(strcmp((const char*)pCWD->pDirEntry->Name, FS_ROOT_DIR) == 0)
    pDiskDrive->_fileSystem.FSpwd.DirEntry = *pCWD->pDirEntry;
}

void Directory_Delete(Process* processAddressSpace, int iDriveID, byte* bParentDirectoryBuffer, const FileSystem_CWD* pCWD, const char* szDirName)
{
	byte bSectorBuffer[512] ;
	unsigned uiSectorNo ;
	byte bSectorPos ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

	FileSystem_PresentWorkingDirectory* pPWD = &(processAddressSpace->processPWD) ;

	if(pCWD->pDirEntry->uiStartSectorID == EOC)
	{
    throw upan::exception(XLOC, "directory %s doesn't exists to delete", szDirName);
	}
	else
	{
    if(!Directory_FindDirectory(*pDiskDrive, *pCWD, szDirName, uiSectorNo, bSectorPos, bSectorBuffer))
      throw upan::exception(XLOC, "directory %s doesn't exists to delete", szDirName);
	}

	FileSystem_DIR_Entry* delDir = ((FileSystem_DIR_Entry*)bSectorBuffer) + bSectorPos ;

	if((delDir->usAttribute & ATTR_TYPE_DIRECTORY) == ATTR_TYPE_DIRECTORY)
	{
		if(delDir->uiSize != 0)
      throw upan::exception(XLOC, "directory %s is not empty - can't delete", szDirName);
	}

	unsigned uiCurrentSectorID = delDir->uiStartSectorID ;
	unsigned uiNextSectorID ;

	while(uiCurrentSectorID != EOC)
	{
    uiNextSectorID = FileSystem_DeAllocateSector(pDiskDrive, uiCurrentSectorID);
		uiCurrentSectorID = uiNextSectorID ;
	}

	delDir->usAttribute |= ATTR_DELETED_DIR ;

  pDiskDrive->xWrite(bSectorBuffer, uiSectorNo, 1);

	pCWD->pDirEntry->uiSize-- ;

  pDiskDrive->xWrite(bParentDirectoryBuffer, pCWD->uiSectorNo, 1);
	
	if(pDiskDrive->Id() == processAddressSpace->iDriveID
			&& pCWD->uiSectorNo == pPWD->uiSectorNo
			&& pCWD->bSectorEntryPosition == pPWD->bSectorEntryPosition)
    pPWD->DirEntry = *pCWD->pDirEntry;

	//TODO: Required Only If "/" Dir Entry is Created
	if(strcmp((const char*)pCWD->pDirEntry->Name, FS_ROOT_DIR) == 0)
    pDiskDrive->_fileSystem.FSpwd.DirEntry = *pCWD->pDirEntry;
}

void Directory_GetDirEntryForCreateDelete(const Process* processAddressSpace, int iDriveID, const char* szDirPath, char* szDirName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDirectoryBuffer)
{
	FileSystem_CWD CWD ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  FileSystem* pFSMountInfo = &pDiskDrive->_fileSystem ;

	if(strlen(szDirPath) == 0 ||	strcmp(FS_ROOT_DIR, szDirPath) == 0)
    throw upan::exception(XLOC, "can't create/delete current/root directory");

	if(szDirPath[0] == '/' || processAddressSpace->iDriveID != iDriveID)
	{
		CWD.pDirEntry = &(pFSMountInfo->FSpwd.DirEntry) ;
    CWD.uiSectorNo = uiSectorNo = pFSMountInfo->FSpwd.uiSectorNo ;
    CWD.bSectorEntryPosition = bSectorPos = pFSMountInfo->FSpwd.bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = const_cast<FileSystem_DIR_Entry*>(&(processAddressSpace->processPWD.DirEntry)) ;
    CWD.uiSectorNo = uiSectorNo = processAddressSpace->processPWD.uiSectorNo ;
    CWD.bSectorEntryPosition = bSectorPos = processAddressSpace->processPWD.bSectorEntryPosition ;
	}

  pDiskDrive->xRead(bDirectoryBuffer, uiSectorNo, 1);

  int iListSize;

	StringDefTokenizer tokenizer ;

	String_Tokenize(szDirPath, '/', &iListSize, tokenizer) ;

  FileSystem_DIR_Entry tempDirEntry;
  for(int i = 0; i < iListSize - 1; i++)
	{
    if(!Directory_FindDirectory(*pDiskDrive, CWD, tokenizer.szToken[i], uiSectorNo, bSectorPos, bDirectoryBuffer))
      throw upan::exception(XLOC, "directory %s is not found", tokenizer.szToken[i]);

    tempDirEntry = *(((FileSystem_DIR_Entry*)bDirectoryBuffer) + bSectorPos);
    CWD.pDirEntry = &tempDirEntry;
    CWD.uiSectorNo = uiSectorNo ;
    CWD.bSectorEntryPosition = bSectorPos ;
	}

  strcpy(szDirName, tokenizer.szToken[iListSize - 1]) ;
	
  if(strcmp(DIR_SPECIAL_CURRENT, szDirName) == 0 || strcmp(DIR_SPECIAL_PARENT, szDirName) == 0)
    throw upan::exception(XLOC, "%s is a special directory", szDirName);
}

void Directory_GetDirectoryContent(const char* szFileName, Process* processAddressSpace, int iDriveID, FileSystem_DIR_Entry** pDirList, int* iListSize)
{
	byte bDirectoryBuffer[512] ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);
	FileSystem_CWD CWD ;
	if(processAddressSpace->iDriveID == iDriveID)
	{
		CWD.pDirEntry = &(processAddressSpace->processPWD.DirEntry) ;
		CWD.uiSectorNo = processAddressSpace->processPWD.uiSectorNo ;
		CWD.bSectorEntryPosition = processAddressSpace->processPWD.bSectorEntryPosition ;
	}
	else
	{
    CWD.pDirEntry = &(pDiskDrive->_fileSystem.FSpwd.DirEntry) ;
    CWD.uiSectorNo = pDiskDrive->_fileSystem.FSpwd.uiSectorNo ;
    CWD.bSectorEntryPosition = pDiskDrive->_fileSystem.FSpwd.bSectorEntryPosition ;
	}

	FileSystem_DIR_Entry* dirFile ;
	FileSystem_DIR_Entry* pAddress ;

	if(strlen(szFileName) == 0)
	{
		dirFile = CWD.pDirEntry ;
	}
	else
	{
		unsigned uiSectorNo ;
		byte bSectorPos ;

    Directory_ReadDirEntryInfo(*pDiskDrive, CWD, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

		dirFile = ((FileSystem_DIR_Entry*)bDirectoryBuffer) + bSectorPos ;

		if(dirFile->usAttribute & ATTR_DELETED_DIR)
      throw upan::exception(XLOC, "directory/file %s doesn't exists - it's deleted", szFileName);

		if((dirFile->usAttribute & ATTR_TYPE_DIRECTORY) != ATTR_TYPE_DIRECTORY)
		{
			*iListSize = 1 ;

			if(processAddressSpace->bIsKernelProcess == true)
			{
				*pDirList = (FileSystem_DIR_Entry*)DMM_AllocateForKernel(sizeof(FileSystem_DIR_Entry)) ;
				pAddress = *pDirList ;
			}
			else
			{
				*pDirList = (FileSystem_DIR_Entry*)DMM_Allocate(processAddressSpace, sizeof(FileSystem_DIR_Entry)) ;
				pAddress = (FileSystem_DIR_Entry*)(((unsigned)*pDirList + PROCESS_BASE) - GLOBAL_DATA_SEGMENT_BASE) ;
			}

			MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)dirFile, MemUtil_GetDS(), (unsigned)pAddress, sizeof(FileSystem_DIR_Entry)) ;

      return;
		}
	}

	unsigned uiCurrentSectorID ;
	int iScanDirCount = 0;
	byte bSectorPosIndex ;

	FileSystem_DIR_Entry* curDir ;

	iScanDirCount = 0 ;
	uiCurrentSectorID = dirFile->uiStartSectorID ;
	*iListSize = dirFile->uiSize ;

	if(processAddressSpace->bIsKernelProcess == true)
	{
		*pDirList = (FileSystem_DIR_Entry*)DMM_AllocateForKernel(sizeof(FileSystem_DIR_Entry) * (*iListSize)) ;
		pAddress = *pDirList ;
	}
	else
	{
		*pDirList = (FileSystem_DIR_Entry*)DMM_Allocate(processAddressSpace, sizeof(FileSystem_DIR_Entry) * (*iListSize)) ;
		pAddress = (FileSystem_DIR_Entry*)(((unsigned)*pDirList + PROCESS_BASE) - GLOBAL_DATA_SEGMENT_BASE) ;
	}

	while(uiCurrentSectorID != EOC)
	{
    pDiskDrive->xRead(bDirectoryBuffer, uiCurrentSectorID, 1);

		for(bSectorPosIndex = 0; bSectorPosIndex < DIR_ENTRIES_PER_SECTOR; bSectorPosIndex++)
		{
			curDir = ((FileSystem_DIR_Entry*)bDirectoryBuffer) + bSectorPosIndex ;

      if(!(curDir->usAttribute & ATTR_DELETED_DIR))
			{
        ++iScanDirCount;
        if(iScanDirCount > *iListSize)
          return;
        pAddress[iScanDirCount - 1] = *curDir;
			}
		}

    uiCurrentSectorID = pDiskDrive->_fileSystem.GetSectorEntryValue(uiCurrentSectorID);
	}
}

bool Directory_FindDirectory(DiskDrive& diskDrive, const FileSystem_CWD& cwd, const char* szDirName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDestSectorBuffer)
{
  FileSystem_DIR_Entry* pDirEntry = cwd.pDirEntry;
  if((pDirEntry->usAttribute & ATTR_TYPE_DIRECTORY) == 0)
    throw upan::exception(XLOC, "%s is not a directory", szDirName);

	byte bSectorBuffer[512] ;

	if(strcmp(szDirName, DIR_SPECIAL_CURRENT) == 0)
	{
    uiSectorNo = cwd.uiSectorNo ;
    bSectorPos = cwd.bSectorEntryPosition ;
    diskDrive.xRead(bDestSectorBuffer, uiSectorNo, 1);
    return true;
	}

	if(strcmp(szDirName, DIR_SPECIAL_PARENT) == 0)
	{
		if(strcmp((const char*)pDirEntry->Name, FS_ROOT_DIR) == 0)
		{
      uiSectorNo = cwd.uiSectorNo ;
      bSectorPos = cwd.bSectorEntryPosition ;
		}
		else
		{
      uiSectorNo = pDirEntry->uiParentSecID ;
      bSectorPos = pDirEntry->bParentSectorPos ;
		}
	
    diskDrive.xRead(bDestSectorBuffer, uiSectorNo, 1);
			
    return true;
	}

	byte bDeletedEntryFound ;
	unsigned uiCurrentSectorID ;
	unsigned uiNextSectorID ;
	unsigned uiScanDirCount ;
	byte bSectorPosIndex ;

	FileSystem_DIR_Entry* curDir ;

  uiSectorNo = EOC ;
  bSectorPos = EOC_B ;
	bDeletedEntryFound = false ;
	uiScanDirCount = 0 ;

	uiCurrentSectorID = pDirEntry->uiStartSectorID ;

	while(uiCurrentSectorID != EOC)
	{
    diskDrive.xRead(bSectorBuffer, uiCurrentSectorID, 1);

		for(bSectorPosIndex = 0; bSectorPosIndex < DIR_ENTRIES_PER_SECTOR; bSectorPosIndex++)
		{
			curDir = ((FileSystem_DIR_Entry*)bSectorBuffer) + bSectorPosIndex ;

			if(strcmp(szDirName, (const char*)curDir->Name) == 0 && !(curDir->usAttribute & ATTR_DELETED_DIR))
			{
        memcpy(bDestSectorBuffer, bSectorBuffer, 512);
        uiSectorNo = uiCurrentSectorID ;
        bSectorPos = bSectorPosIndex ;
        return true;
			}

			if((curDir->usAttribute & ATTR_DELETED_DIR)) 
			{
				if(bDeletedEntryFound == false)
				{
          memcpy(bDestSectorBuffer, bSectorBuffer, 512);
          uiSectorNo = uiCurrentSectorID ;
          bSectorPos = bSectorPosIndex ;
					bDeletedEntryFound = true ;
				}
			}
			else
			{
				uiScanDirCount++ ;
				if(uiScanDirCount >= pDirEntry->uiSize)
					break ;
			}
		}

    uiNextSectorID = diskDrive._fileSystem.GetSectorEntryValue(uiCurrentSectorID);

		if(uiScanDirCount >= pDirEntry->uiSize)
		{
			if(bDeletedEntryFound == true)
        return false;

			if(bSectorPosIndex < DIR_ENTRIES_PER_SECTOR - 1)
			{
        memcpy(bDestSectorBuffer, bSectorBuffer, 512);
        uiSectorNo = uiCurrentSectorID ;
        bSectorPos = bSectorPosIndex + 1 ;
        return false;
			}

			if(bSectorPosIndex == DIR_ENTRIES_PER_SECTOR - 1)
			{
				if(uiNextSectorID != EOC)
				{
          uiSectorNo = uiNextSectorID ;
          bSectorPos = 0 ;
          return false;
				}
				
        uiSectorNo = uiCurrentSectorID ;
        bSectorPos = EOC_B ;
        return false;
			}
		}
		uiCurrentSectorID = uiNextSectorID ;
	}

  return false;
}

void Directory_FileWrite(DiskDrive* pDiskDrive, FileSystem_CWD* pCWD, ProcFileDescriptor* pFDEntry, byte* bDataBuffer, unsigned uiDataSize)
{
	if(uiDataSize == 0)
    return throw upan::exception(XLOC, "zero byte file write");

	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	const char* szFileName = pFDEntry->szFileName ;
	unsigned uiOffset = pFDEntry->uiOffset ;

  Directory_ReadDirEntryInfo(*pDiskDrive, *pCWD, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

	FileSystem_DIR_Entry* dirFile = ((FileSystem_DIR_Entry*)bDirectoryBuffer) + bSectorPos ;

	if((dirFile->usAttribute & ATTR_TYPE_DIRECTORY) != 0)
    throw upan::exception(XLOC, "%s is a directory - can't do file-write", szFileName);

  Directory_ActualFileWrite(pDiskDrive, bDataBuffer, pFDEntry, uiDataSize, dirFile);

	if(dirFile->uiSize < (uiOffset + uiDataSize))
	{
		dirFile->uiSize = uiOffset + uiDataSize ;
    pDiskDrive->xWrite(bDirectoryBuffer, uiSectorNo, 1);
	}
}

void Directory_ActualFileWrite(DiskDrive* pDiskDrive, byte* bDataBuffer, ProcFileDescriptor* pFDEntry,
                               unsigned uiDataSize, FileSystem_DIR_Entry* dirFile)
{
	unsigned uiCurrentSectorID, uiNextSectorID, uiPrevSectorID = EOC ;
	int iStartWriteSectorNo, iStartWriteSectorPos ;
	int iSectorIndex ;
	unsigned uiWriteRemainingCount, uiWrittenCount ;
	unsigned uiCurrentFileSize ;
	unsigned uiOffset = pFDEntry->uiOffset ;

	byte bStartAllocation ;
	byte bSectorBuffer[512] ;

	iStartWriteSectorNo = uiOffset / 512 ;
	iStartWriteSectorPos = uiOffset % 512 ;

	uiCurrentFileSize = dirFile->uiSize ;

	Directory_GetLastReadSectorDetails(pFDEntry, &iSectorIndex, &uiCurrentSectorID) ;

	if(iSectorIndex < 0 || iSectorIndex > iStartWriteSectorNo)
	{
		iSectorIndex = 0 ;
		uiCurrentSectorID = dirFile->uiStartSectorID ;
	}
	
	while(iSectorIndex < iStartWriteSectorNo && uiCurrentSectorID != EOC)
	{
    uiNextSectorID = pDiskDrive->_fileSystem.GetSectorEntryValue(uiCurrentSectorID);

		iSectorIndex++ ;
		uiPrevSectorID = uiCurrentSectorID ;
		uiCurrentSectorID = uiNextSectorID ;
	}

	if(uiCurrentSectorID == EOC)
	{
		memset((char*)bSectorBuffer, 0, 512) ;

		do
		{
      uiCurrentSectorID = pDiskDrive->_fileSystem.AllocateSector();

			if(dirFile->uiStartSectorID == EOC)
			{
				dirFile->uiStartSectorID = uiCurrentSectorID ;
			}
			else
			{
        pDiskDrive->_fileSystem.SetSectorEntryValue(uiPrevSectorID, uiCurrentSectorID);
			}
			
			uiPrevSectorID = uiCurrentSectorID ;

      pDiskDrive->xWrite(bSectorBuffer, uiCurrentSectorID, 1);
			
			Directory_SetLastReadSectorDetails(pFDEntry, iSectorIndex, uiCurrentSectorID) ;

			iSectorIndex++ ;

		} while(iSectorIndex <= iStartWriteSectorNo) ;
	}
	else
	{
		Directory_SetLastReadSectorDetails(pFDEntry, iSectorIndex, uiCurrentSectorID) ;
	}

	uiWrittenCount = 0 ;
	uiWriteRemainingCount = uiDataSize ;

	if(iStartWriteSectorPos != 0)
	{
    pDiskDrive->xRead(bSectorBuffer, uiCurrentSectorID, 1);

		uiWrittenCount = 512 - iStartWriteSectorPos ;
		if(uiDataSize <= uiWrittenCount)
			uiWrittenCount = uiDataSize ;

    MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)bDataBuffer, MemUtil_GetDS(),
                       (unsigned)(bSectorBuffer + iStartWriteSectorPos), uiWrittenCount) ;

    pDiskDrive->xWrite(bSectorBuffer, uiCurrentSectorID, 1);
			
		if(uiWrittenCount == uiDataSize)
      return;

    uiNextSectorID = pDiskDrive->_fileSystem.GetSectorEntryValue(uiCurrentSectorID);

    uiPrevSectorID = uiCurrentSectorID ;
    uiCurrentSectorID = uiNextSectorID ;

    uiWriteRemainingCount -= uiWrittenCount ;
	}

	bStartAllocation = false ;
	
	unsigned uiBufStartSectorID, uiBufPrecSectorID ;
	byte bWriteBuffer[MAX_SECTORS_PER_RW * 512] ;
  uint32_t iBufCount = 0 ;

	while(true)
	{
		if(uiCurrentSectorID == EOC || bStartAllocation == true)
		{
			bStartAllocation = true ;
      uiCurrentSectorID = pDiskDrive->_fileSystem.AllocateSector();
      pDiskDrive->_fileSystem.SetSectorEntryValue(uiPrevSectorID, uiCurrentSectorID);
		}
		
		if(uiWriteRemainingCount < 512)
		{
			if(bStartAllocation == false && (uiOffset + uiDataSize) < uiCurrentFileSize)
			{
        pDiskDrive->xRead(bSectorBuffer, uiCurrentSectorID, 1);
			}

      memcpy(bSectorBuffer, (bDataBuffer + uiWrittenCount), uiWriteRemainingCount);

      Directory_BufferedWrite(*pDiskDrive, uiCurrentSectorID, bSectorBuffer, bWriteBuffer, uiBufStartSectorID,
                              uiBufPrecSectorID, iBufCount, false);

      Directory_BufferedWrite(*pDiskDrive, EOC, NULL, bWriteBuffer, uiBufStartSectorID, uiBufPrecSectorID, iBufCount, true);

      return;
		}

    Directory_BufferedWrite(*pDiskDrive, uiCurrentSectorID, bDataBuffer + uiWrittenCount, bWriteBuffer,
                            uiBufStartSectorID, uiBufPrecSectorID, iBufCount, false);
		
		uiWrittenCount += 512 ;
		uiWriteRemainingCount -= 512 ;

		if(uiWriteRemainingCount == 0)
		{
      Directory_BufferedWrite(*pDiskDrive, EOC, NULL, bWriteBuffer, uiBufStartSectorID, uiBufPrecSectorID, iBufCount, true);
      return;
		}

    uiNextSectorID = pDiskDrive->_fileSystem.GetSectorEntryValue(uiCurrentSectorID);
		uiPrevSectorID = uiCurrentSectorID ;
		uiCurrentSectorID = uiNextSectorID ;
	}

  throw upan::exception(XLOC, "fs table is corrupted for drive:%s", pDiskDrive->DriveName().c_str());
}

int Directory_FileRead(DiskDrive* pDiskDrive, FileSystem_CWD* pCWD, ProcFileDescriptor* pFDEntry, byte* bDataBuffer, unsigned uiDataSize)
{
	const char* szFileName = pFDEntry->szFileName ;
	unsigned uiOffset = pFDEntry->uiOffset ;

	FileSystem_DIR_Entry* pDirFile ;
	byte bDirectoryBuffer[512] ;
	unsigned uiSectorNo ;
	byte bSectorPos ;

  Directory_ReadDirEntryInfo(*pDiskDrive, *pCWD, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);
		
	pDirFile = ((FileSystem_DIR_Entry*)bDirectoryBuffer) + bSectorPos ;
	if((pDirFile->usAttribute & ATTR_TYPE_DIRECTORY) != 0)
    throw upan::exception(XLOC, "%s is a directory - can't file-read", szFileName);

	if(uiOffset >= pDirFile->uiSize)
    return 0;
		
	if(pDirFile->uiSize == 0)
	{
		bDataBuffer[0] = '\0' ;
    return 0;
	}

	unsigned uiCurrentSectorID, uiNextSectorID, uiStartSectorID ;
	int iStartReadSectorNo, iStartReadSectorPos ;
	int iSectorIndex ;
	unsigned uiCurrentFileSize ;
	int iReadRemainingCount, iReadCount, iCurrentReadSize, iSectorCount ;

	byte bSectorBuffer[MAX_SECTORS_PER_RW * 512] ;

	iStartReadSectorNo = uiOffset / 512 ;
	iStartReadSectorPos = uiOffset % 512 ;

	uiCurrentFileSize = pDirFile->uiSize ;

	Directory_GetLastReadSectorDetails(pFDEntry, &iSectorIndex, &uiCurrentSectorID) ;

	if(iSectorIndex < 0 || iSectorIndex > iStartReadSectorNo)
	{
		iSectorIndex = 0 ;
		uiCurrentSectorID = pDirFile->uiStartSectorID ;
	}
	
	while(iSectorIndex != iStartReadSectorNo)
	{
    uiNextSectorID = pDiskDrive->_fileSystem.GetSectorEntryValue(uiCurrentSectorID);
		iSectorIndex++ ;
		uiCurrentSectorID = uiNextSectorID ;
	}

	Directory_SetLastReadSectorDetails(pFDEntry, iSectorIndex, uiCurrentSectorID) ;
	unsigned uiLastReadSectorNumber = uiCurrentSectorID ;
	int iLastReadSectorIndex = iSectorIndex ;

	iReadCount = 0 ;
	iReadRemainingCount = (uiDataSize < (uiCurrentFileSize - uiOffset) && uiDataSize > 0) ? uiDataSize : (uiCurrentFileSize  - uiOffset) ;

	iSectorCount = 0 ;

	while(true)
	{
		if(uiCurrentSectorID == EOC)
		{
			Directory_SetLastReadSectorDetails(pFDEntry, iLastReadSectorIndex, uiLastReadSectorNumber) ;
      return iReadCount;
		}
		
		uiStartSectorID = uiCurrentSectorID ;

		iLastReadSectorIndex += iSectorCount ;
		uiLastReadSectorNumber = uiCurrentSectorID ;

		iSectorCount = 1 ;

		for(;;)
		{
      uiNextSectorID = pDiskDrive->_fileSystem.GetSectorEntryValue(uiCurrentSectorID);

			if(uiCurrentSectorID + 1 == uiNextSectorID)
			{
				uiCurrentSectorID = uiNextSectorID ;

				iSectorCount++ ;
				iCurrentReadSize = iSectorCount * 512 - iStartReadSectorPos ;

				if(iReadRemainingCount <= iCurrentReadSize)
				{
					if((iStartReadSectorPos + iReadRemainingCount) <= 512)
						iSectorCount-- ;

					iCurrentReadSize = iReadRemainingCount ;
					break ;
				}

				if(iSectorCount == MAX_SECTORS_PER_RW)
				{
          uiNextSectorID = pDiskDrive->_fileSystem.GetSectorEntryValue(uiCurrentSectorID);
					uiCurrentSectorID = uiNextSectorID ;
					break ;	
				}
			}
			else
			{
				iCurrentReadSize = iSectorCount * 512 - iStartReadSectorPos ;

				if(iReadRemainingCount <= iCurrentReadSize)
					iCurrentReadSize = iReadRemainingCount ;

				uiCurrentSectorID = uiNextSectorID ;
				break ;
			}
		}

    pDiskDrive->xRead(bSectorBuffer, uiStartSectorID, iSectorCount);

		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)bSectorBuffer + iStartReadSectorPos, MemUtil_GetDS(), (unsigned)bDataBuffer + iReadCount, iCurrentReadSize) ;

		iReadCount += iCurrentReadSize ;
		iReadRemainingCount -= iCurrentReadSize ;

		iStartReadSectorPos = 0 ;

		if(iReadRemainingCount <= 0)
		{
			Directory_SetLastReadSectorDetails(pFDEntry, iLastReadSectorIndex, uiLastReadSectorNumber) ;
      return iReadCount ;
		}
	}

  throw upan::exception(XLOC, "fs table is corrupted for drive:%s", pDiskDrive->DriveName().c_str());
}

void Directory_ReadDirEntryInfo(DiskDrive& diskDrive, const FileSystem_CWD& cwd, const char* szFileName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDirectoryBuffer)
{
	FileSystem_CWD CWD ;
  FileSystem_PresentWorkingDirectory& fsPwd = diskDrive._fileSystem.FSpwd;

	if(strlen(szFileName) == 0)
    throw upan::exception(XLOC, "file name can't be empty");

	if(szFileName[0] == '/')
	{
		if(strcmp(FS_ROOT_DIR, szFileName) == 0)
		{
      diskDrive.xRead(bDirectoryBuffer, fsPwd.uiSectorNo, 1);
      uiSectorNo = fsPwd.uiSectorNo ;
      bSectorPos = fsPwd.bSectorEntryPosition ;
      return;
		}

    CWD.pDirEntry = &fsPwd.DirEntry;
    CWD.uiSectorNo = fsPwd.uiSectorNo ;
    CWD.bSectorEntryPosition = fsPwd.bSectorEntryPosition ;
	}
	else
	{
    CWD.pDirEntry = cwd.pDirEntry ;
    CWD.uiSectorNo = cwd.uiSectorNo ;
    CWD.bSectorEntryPosition = cwd.bSectorEntryPosition ;
	}

  int iListSize = 0;

	StringDefTokenizer tokenizer ;

	String_Tokenize(szFileName, '/', &iListSize, tokenizer) ;

  FileSystem_DIR_Entry tempDirEntry;
  for(int i = 0; i < iListSize; i++)
	{
    if (!Directory_FindDirectory(diskDrive, CWD, tokenizer.szToken[i], uiSectorNo, bSectorPos, bDirectoryBuffer))
      throw upan::exception(XLOC, "file/directory %s doesn't exist", szFileName);

    tempDirEntry = *(((FileSystem_DIR_Entry*)bDirectoryBuffer) + bSectorPos);
    CWD.pDirEntry = &tempDirEntry;
    CWD.uiSectorNo = uiSectorNo ;
    CWD.bSectorEntryPosition = bSectorPos ;
	}
}

void Directory_Change(const char* szFileName, int iDriveID, Process* processAddressSpace)
{
	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

	FileSystem_CWD CWD ;
	FileSystem_PresentWorkingDirectory* pPWD = &(processAddressSpace->processPWD) ;
	if(iDriveID == processAddressSpace->iDriveID)
	{
		CWD.pDirEntry = &pPWD->DirEntry ;
		CWD.uiSectorNo = pPWD->uiSectorNo ;
		CWD.bSectorEntryPosition = pPWD->bSectorEntryPosition ;
	}
	else
	{
    CWD.pDirEntry = &(pDiskDrive->_fileSystem.FSpwd.DirEntry) ;
    CWD.uiSectorNo = pDiskDrive->_fileSystem.FSpwd.uiSectorNo ;
    CWD.bSectorEntryPosition = pDiskDrive->_fileSystem.FSpwd.bSectorEntryPosition ;
	}

  Directory_ReadDirEntryInfo(*pDiskDrive, CWD, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

	FileSystem_DIR_Entry* dirFile = ((FileSystem_DIR_Entry*)bDirectoryBuffer) + bSectorPos ;

	if((dirFile->usAttribute & ATTR_TYPE_DIRECTORY) == 0)
    throw upan::exception(XLOC, "%s is not a directory", szFileName);

	processAddressSpace->iDriveID = iDriveID ;

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)(((FileSystem_DIR_Entry*)bDirectoryBuffer) + bSectorPos), MemUtil_GetDS(), (unsigned)&pPWD->DirEntry, sizeof(FileSystem_DIR_Entry)) ;
	pPWD->uiSectorNo = uiSectorNo ;
	pPWD->bSectorEntryPosition = bSectorPos ;
	
	unsigned uiSecNo ;
	byte bSecPos ;
	char szPWD[256] ;
	char szTempPwd[256] = "" ;

	if(strcmp((const char*)dirFile->Name, FS_ROOT_DIR) == 0)
		strcpy(szPWD, FS_ROOT_DIR) ;
	else
	{
		while(true)
		{	
			strcpy(szPWD, FS_ROOT_DIR) ;

			strcat(szPWD, (const char*)dirFile->Name) ;
			strcat(szPWD, szTempPwd) ;

			uiSecNo = dirFile->uiParentSecID ;
			bSecPos = dirFile->bParentSectorPos ;

      pDiskDrive->xRead(bDirectoryBuffer, uiSecNo, 1);

			dirFile = ((FileSystem_DIR_Entry*)bDirectoryBuffer) + bSecPos ;

			if(strcmp((const char*)dirFile->Name, FS_ROOT_DIR) == 0)
				break ;

			strcpy(szTempPwd, szPWD) ;
		}
	}

	strcpy(szTempPwd, szPWD) ;
	strcpy(szPWD, pDiskDrive->DriveName().c_str());
	strcat(szPWD, "@") ;
	strcat(szPWD, szTempPwd) ;

	ProcessEnv_Set("PWD", szPWD) ;

}

void Directory_PresentWorkingDirectory(Process* processAddressSpace, char** uiReturnDirPathAddress)
{
	char* szPWD ;
	char* pAddress ;

	szPWD = ProcessEnv_Get("PWD") ;

	if(szPWD == NULL)
    throw upan::exception(XLOC, "PWD is not set");

	if(processAddressSpace->bIsKernelProcess == true)
	{
		*uiReturnDirPathAddress = (char*)DMM_AllocateForKernel(strlen(szPWD) + 1) ;
		pAddress = *uiReturnDirPathAddress ;
	}
	else
	{
		*uiReturnDirPathAddress = (char*)DMM_Allocate(processAddressSpace, strlen(szPWD) + 1) ;
		pAddress = (char*)(((unsigned)*uiReturnDirPathAddress + PROCESS_BASE) - GLOBAL_DATA_SEGMENT_BASE) ;
	}

	strcpy(pAddress, szPWD) ;
}

const FileSystem_DIR_Entry Directory_GetDirEntry(const char* szFileName, Process* processAddressSpace, int iDriveID)
{
	byte bDirectoryBuffer[512] ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

	FileSystem_CWD CWD ;
	if(processAddressSpace->iDriveID == iDriveID)
	{
		CWD.pDirEntry = &(processAddressSpace->processPWD.DirEntry) ;
		CWD.uiSectorNo = processAddressSpace->processPWD.uiSectorNo ;
		CWD.bSectorEntryPosition = processAddressSpace->processPWD.bSectorEntryPosition ;
	}
	else
	{
    CWD.pDirEntry = &(pDiskDrive->_fileSystem.FSpwd.DirEntry) ;
    CWD.uiSectorNo = pDiskDrive->_fileSystem.FSpwd.uiSectorNo ;
    CWD.bSectorEntryPosition = pDiskDrive->_fileSystem.FSpwd.bSectorEntryPosition ;
	}

	FileSystem_DIR_Entry* dirFile ;

	unsigned uiSectorNo ;
	byte bSectorPos ;

  Directory_ReadDirEntryInfo(*pDiskDrive, CWD, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

	dirFile = ((FileSystem_DIR_Entry*)bDirectoryBuffer) + bSectorPos ;

	if(dirFile->usAttribute & ATTR_DELETED_DIR)
    throw upan::exception(XLOC, "directory %s doesn't exists - it's deleted", szFileName);

  return *dirFile;
}

void Directory_SyncPWD(Process* processAddressSpace)
{
  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(processAddressSpace->iDriveID, true).goodValueOrThrow(XLOC);

	unsigned uiSectorNo = processAddressSpace->processPWD.uiSectorNo ;
	byte bSectorEntryPos = processAddressSpace->processPWD.bSectorEntryPosition ;

	byte bSectorBuffer[512] ;
  pDiskDrive->xRead(bSectorBuffer, uiSectorNo, 1);

  processAddressSpace->processPWD.DirEntry = (((FileSystem_DIR_Entry*)bSectorBuffer)[bSectorEntryPos]);
}

