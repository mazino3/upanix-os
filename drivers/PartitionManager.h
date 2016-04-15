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
#ifndef _PARTITION_MANAGER_H_
#define _PARTITION_MANAGER_H_

#include <Global.h>
#include <ATADeviceController.h>
#include <DeviceDrive.h>

#define MAX_NO_OF_PRIMARY_PARTITIONS	4
#define MAX_NO_OF_EXT_PARTITIONS		32

class PartitionInfo
{
  public:
    enum PartitionTypes { ACTIVE, EXTENEDED, NORMAL };

    PartitionInfo() :
      BootIndicator(0),
      _StartHead(0),
      _StartSector(0),
      _StartCylinder(0),
      SystemIndicator(0),
      _EndHead(0),
      _EndSector(0),
      _EndCylinder(0),
      LBAStartSector(0),
      LBANoOfSectors(0)
    {
    }

    PartitionInfo(unsigned lbaStart, unsigned size, PartitionTypes type);
	  bool IsEmpty() const 
    {
      return SystemIndicator == 0x0 || LBANoOfSectors == 0x0;
    }

	byte		BootIndicator;

	byte		_StartHead;
	byte		_StartSector;
	byte		_StartCylinder;

	byte		SystemIndicator;

	byte		_EndHead;
	byte		_EndSector;
	byte		_EndCylinder;

	unsigned 	LBAStartSector;
	unsigned 	LBANoOfSectors;

} PACKED;

class PartitionEntry
{
  public:
    PartitionEntry(unsigned startSector, unsigned sizeInSectors, byte fsId) :
      _uiLBAStartSector(startSector), _uiLBASize(sizeInSectors), _fsId(fsId)
    {
    }

    unsigned LBAStartSector() const { return _uiLBAStartSector; }
    unsigned LBASize() const { return _uiLBASize; }
    byte FileSystemID() const { return _fsId; }

  private:
    const unsigned _uiLBAStartSector;
    const unsigned _uiLBASize;
    const byte     _fsId;
};

class ExtPartitionTable
{
  public:
    ExtPartitionTable(const PartitionInfo& cur, unsigned actualStartSec)
      : _currentPartition(cur), _uiActualStartSector(actualStartSec)
    {
    }
    const PartitionInfo& CurrentPartition() const { return _currentPartition; }
    unsigned ActualStartSector() const { return _uiActualStartSector; }
  private:
    PartitionInfo _currentPartition;
    unsigned	_uiActualStartSector;
};

class PartitionTable
{
  public:
    PartitionTable(RawDiskDrive& disk);
    ~PartitionTable();

    void VerbosePrint() const;
    bool IsEmpty() const { return _partitions.empty(); }
    const upan::list<PartitionEntry> GetPartitions() const { return _partitions; }
    void ClearPartitionTable();
    void CreatePrimaryPartitionEntry(unsigned uiSizeInSectors, PartitionInfo::PartitionTypes);
    void DeletePrimaryPartition();
    void CreateExtPartitionEntry(unsigned uiSizeInSectors);
    void DeleteExtPartition();

  private:
    void ReadPrimaryPartition();
    void ReadExtPartition();
    bool VerifyMBR(const PartitionInfo* pPartitionInfo) const;

    bool _bIsExtPartitionPresent;
    RawDiskDrive& _disk;
    PartitionInfo _extPartitionEntry;
    upan::list<PartitionInfo*> _primaryPartitions;
    upan::list<ExtPartitionTable*> _extPartitions;
    upan::list<PartitionEntry> _partitions;
};

#endif
