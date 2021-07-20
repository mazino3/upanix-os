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

#pragma once

#include <IODescriptor.h>

class FileDescriptor : public IODescriptor {
public:
  FileDescriptor(int fd, byte mode, const upan::string& fileName, int driveID, uint32_t fileSize, uint32_t startSectorID) :
      IODescriptor(fd, mode),
      _fileName(fileName),
      _offset(mode & O_APPEND ? fileSize : 0),
      _driveID(driveID), _fileSize(fileSize),
      _lastReadSectorIndex(0),
      _lastReadSectorNo(startSectorID) {
  }

  int read(char* buffer, int len) override;
  int write(const char* buffer, int len) override;
  void seek(int seekType, int offset) override;

  uint32_t getSize() const override {
    return _fileSize;
  }

  const upan::string& getFileName() const {
    return _fileName;
  }

  uint32_t getOffset() const override {
    return _offset;
  }

  int getDriveId() const {
    return _driveID;
  }

  int getLastReadSectorIndex() const {
    return _lastReadSectorIndex;
  }

  void setLastReadSectorIndex(int v) {
    _lastReadSectorIndex = v;
  }

  uint32_t getLastReadSectorNo() const {
    return _lastReadSectorNo;
  }

  void setLastReadSectorNo(uint32_t v) {
    _lastReadSectorNo = v;
  }

private:
  upan::string _fileName;
  uint32_t _offset;
  int _driveID;
  uint32_t _fileSize;
  int _lastReadSectorIndex;
  uint32_t _lastReadSectorNo;
};
