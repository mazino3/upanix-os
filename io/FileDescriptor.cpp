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

#include <fs.h>
#include <exception.h>
#include <FileDescriptor.h>
#include <ProcessManager.h>
#include <DeviceDrive.h>
#include <Directory.h>

int FileDescriptor::read(void* buffer, int len) {
  Process* pPAS = &ProcessManager::Instance().GetCurrentPAS();

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(_driveID, true).goodValueOrThrow(XLOC);
  FileSystem::CWD CWD ;
  if(pPAS->driveID() == _driveID)
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

  int readLen = Directory_FileRead(pDiskDrive, &CWD, *this, (byte*)buffer, len);

  FileOperations_UpdateTime(getFileName().c_str(), _driveID, DIR_ACCESS_TIME);

  _offset += readLen;

  return readLen;
}

int FileDescriptor::write(const void* buffer, int len) {
  Process* pPAS = &ProcessManager::Instance().GetCurrentPAS();

  if( !(getMode() & O_WRONLY || getMode() & O_RDWR || getMode() & O_APPEND) )
    throw upan::exception(XLOC, "insufficient permission to write file fd: %d", id());

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(_driveID, true).goodValueOrThrow(XLOC);

  FileSystem::CWD CWD ;
  if(pPAS->driveID() == _driveID)
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

    Directory_FileWrite(pDiskDrive, &CWD, *this, (byte*)buffer, n);

    _offset += n;

    uiIncLen -= n ;

    if(uiIncLen == 0)
      break ;
  }

  FileOperations_UpdateTime(getFileName().c_str(), _driveID, DIR_ACCESS_TIME | DIR_MODIFIED_TIME);

  return len;
}

void FileDescriptor::seek(int seekType, int offset) {
  switch(seekType)
  {
    case SEEK_SET:
      break ;

    case SEEK_CUR:
      offset += _offset;
      break ;

    case SEEK_END:
      offset += _fileSize;
      break ;

    default:
      throw upan::exception(XLOC, "invalid file seek type: %d", seekType);
  }

  if(offset < 0)
    throw upan::exception(XLOC, "invalid file offset %d", offset);

  _offset = offset;
}