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

#define PartitionManager_SUCCESS					0
#define PartitionManager_ERR_NOT_PARTITIONED		1
#define PartitionManager_ERR_INVALID				2
#define PartitionManager_ERR_PARTITION_TABLE_FULL	3
#define PartitionManager_ERR_PRIMARY_PARTITION_FULL	4
#define PartitionManager_ERR_INSUF_SPACE			5
#define PartitionManager_ERR_EXT_PARTITION_FULL		6
#define PartitionManager_ERR_NO_EXT_PARTITION		7
#define PartitionManager_ERR_EXT_PARTITION_BLOCKED	8
#define PartitionManager_ERR_PARTITION_TABLE_EMPTY	9
#define PartitionManager_ERR_ETX_PARTITION_TABLE_EMPTY	10
#define PartitionManager_ERR_NO_PRIM_PART			11
#define PartitionManager_FAILURE					12

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
    PartitionEntry(unsigned startSector, unsigned sizeInSectors) :
      _uiStartSector(startSector), _uiSize(sizeInSectors)
    {
    }

    unsigned StartSector() const { return _uiStartSector; }
    unsigned Size() const { return _uiSize; }

  private:
    const unsigned _uiStartSector;
    const unsigned _uiSize;
};

class ExtPartitionTable
{
  public:
    ExtPartitionTable(const PartitionInfo& cur, unsigned actualStartSec)
      : _currentPartition(cur), _uiActualStartSector(actualStartSec)
    {
    }
    //TODO: Remove default constructor - if possible make _extPartitions a list
    ExtPartitionTable() : _uiActualStartSector(0)
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
    PartitionTable();

    unsigned NoOfPrimaryPartitions() const { return _uiNoOfPrimaryPartitions; }
    unsigned NoOfExtPartitions() const { return _uiNoOfExtPartitions; }
    bool IsExtPartitionPresent() const { return _bIsExtPartitionPresent; }

    byte ReadPrimaryPartition(RawDiskDrive* pDisk);
    byte ReadExtPartition(RawDiskDrive* pDisk);
    byte CreatePrimaryPartitionEntry(RawDiskDrive* pDisk, unsigned uiSizeInSectors, PartitionInfo::PartitionTypes);
    byte DeletePrimaryPartition(RawDiskDrive* pDisk);
    byte CreateExtPartitionEntry(RawDiskDrive* pDisk, unsigned uiSizeInSectors);
    byte DeleteExtPartition(RawDiskDrive* pDisk);

    const PartitionInfo& ExtPartitionEntry() const { return _extPartitionEntry; }
    const PartitionInfo* GetPrimaryPartition(unsigned index)
    {
      if(index >= _uiNoOfPrimaryPartitions)
        return nullptr;
      return &_primaryParitions[index];
    }
    const PartitionInfo* GetExtPartition(unsigned index)
    {
      if(index >= _uiNoOfExtPartitions)
        return nullptr;
      return &_extPartitions[index].CurrentPartition();
    }
    const ExtPartitionTable* GetExtPartitionTable(unsigned index)
    {
      if(index >= _uiNoOfExtPartitions)
        return nullptr;
      return &_extPartitions[index];
    }

  private:
    byte VerifyMBR(RawDiskDrive* pDisk, const PartitionInfo* pPartitionInfo) const;

    unsigned _uiNoOfPrimaryPartitions;
    unsigned _uiNoOfExtPartitions;
    bool     _bIsExtPartitionPresent;

    PartitionInfo _primaryParitions[MAX_NO_OF_PRIMARY_PARTITIONS];
    PartitionInfo _extPartitionEntry;
    ExtPartitionTable _extPartitions[MAX_NO_OF_EXT_PARTITIONS];
};

#endif
