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
# include <Atomic.h>
# include <ProcessManager.h>

#define DUPPED (100)

constexpr int PROC_SYS_MAX_OPEN_FILES = 4096;

FileDescriptorTable::FileDescriptorTable() : _fdIdCounter(0) {
  allocate("", O_RDWR, PROC_STDIN, 0, 0);
  allocate("", O_RDWR, PROC_STDOUT, 0, 0);
  allocate("", O_RDWR, PROC_STDERR, 0, 0);
}

FileDescriptorTable::~FileDescriptorTable() noexcept {
  for(auto& x : _fdTable) {
    delete x.second;
  }
}

int FileDescriptorTable::allocate(const upan::string &fileName, byte mode, int driveID, uint32_t fileSize, uint32_t startSectorID) {
  MutexGuard g(_fdMutex);

  if(_fdTable.size() >= PROC_SYS_MAX_OPEN_FILES) {
    throw upan::exception(XLOC, "can't open new file - max open files limit %d reached", PROC_SYS_MAX_OPEN_FILES);
  }

  auto i = _fdTable.insert(FDTable::value_type(
      _fdIdCounter++, new FileDescriptor(fileName, mode, driveID, fileSize, startSectorID)));

  if (!i.second) {
    throw upan::exception(XLOC, "failed to create an entry in File Descriptor table");
  }

  return i.first->first;
}

FileDescriptorTable::FDTable::iterator FileDescriptorTable::getItr(int fd) {
  auto i = _fdTable.find(fd);
  if (i == _fdTable.end()) {
    throw upan::exception(XLOC, "invalid file descriptor: %d", fd);
  }
  return i;
}

FileDescriptor& FileDescriptorTable::get(int fd) {
  return *(getItr(fd)->second);
}

FileDescriptor& FileDescriptorTable::getRealNonDupped(int fd) {
  auto& f = get(fd);

  if(f.getMode() != DUPPED)
    return f;

  return getRealNonDupped(f.getDriveId());
}

void FileDescriptorTable::free(int fd) {
  MutexGuard g(_fdMutex);
  auto e = getItr(fd);

  if (e->second->getRefCount() > 1) {
    throw upan::exception(XLOC, "file descriptor is open - refcount: %d", e->second->getRefCount());
  }

  e->second->decrementRefCount();

  if (e->second->getMode() == DUPPED) {
    get(e->second->getDriveId()).decrementRefCount();
  }

  delete e->second;
  _fdTable.erase(e);
}

void FileDescriptorTable::updateOffset(int fd, int seekType, int offset) {
  auto& f = getRealNonDupped(fd);

	switch(seekType)
	{
		case SEEK_SET:
      break ;

		case SEEK_CUR:
			offset += f.getOffset();
		break ;

		case SEEK_END:
		  offset += f.getFileSize();
		break ;

		default:
      throw upan::exception(XLOC, "invalid file seek type: %d", seekType);
	}

	if(offset < 0)
    throw upan::exception(XLOC, "invalid file offset %d", offset);

	f.setOffset(offset);
}

uint32_t FileDescriptorTable::getOffset(int fd) {
  return getRealNonDupped(fd).getOffset();
}

void FileDescriptorTable::dup2(int oldFD, int newFD) {
  auto& oldF = get(oldFD);
  auto& newF = get(newFD);

  if (newF.getDriveId() != PROC_FREE_FD) {
    free(newFD);
  }

  oldF.incrementRefCount();
  _fdTable.insert(FDTable::value_type(newFD, new FileDescriptor("", DUPPED, oldFD, 0, 0)));
}

void FileDescriptorTable::initStdFile(int stdFD)
{
	switch(stdFD)
	{
		case 0:
		case 1:
		case 2:
				break;
		default:
				throw upan::exception(XLOC, "can't initialize invalid std fd: %d", stdFD);
	}

  free(stdFD);
  _fdTable.insert(FDTable::value_type(stdFD, new FileDescriptor("", O_RDWR, stdFD + PROC_STDIN, 0, 0)));
}

