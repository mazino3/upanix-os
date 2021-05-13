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

typedef enum {
  PROC_FREE_FD = 1100,
  PROC_STDIN,
  PROC_STDOUT,
  PROC_STDERR,
} SPECIAL_FILES;

class FileDescriptor {
public:
  FileDescriptor(const upan::string& fileName, byte mode, int driveID, uint32_t fileSize, uint32_t startSectorID) :
      _fileName(fileName), _offset(mode & O_APPEND ? fileSize : 0), _mode(mode),
      _driveID(driveID), _fileSize(fileSize), _refCount(1), _lastReadSectorIndex(0),
      _lastReadSectorNo(startSectorID) {
  }

  const upan::string& getFileName() const {
    return _fileName;
  }

  uint32_t getOffset() const {
    return _offset;
  }

  void addOffset(uint32_t val) {
    _offset += val;
  }

  int getDriveId() const {
    return _driveID;
  }

  byte getMode() const {
    return _mode;
  }

  uint32_t getFileSize() const {
    return _fileSize;
  }

  int getRefCount() const {
    return _refCount;
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

  void setOffset(int offset) {
    _offset = offset;
  }

  void decrementRefCount() {
    --_refCount;
  }

  void incrementRefCount() {
    ++_refCount;
  }

  bool isStdOut() const {
    return _driveID == PROC_STDOUT || _driveID == PROC_STDERR;
  }

  bool isStdIn() const {
    return _driveID  == PROC_STDIN;
  }

private:
  upan::string _fileName;
  uint32_t _offset;
  byte _mode;
  int _driveID;
  uint32_t _fileSize;
  int _refCount;
  int _lastReadSectorIndex;
  uint32_t _lastReadSectorNo;
};
