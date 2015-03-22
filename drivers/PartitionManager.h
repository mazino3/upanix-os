/*
 *	Mother Operating System - An x86 based Operating System
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

typedef struct
{
	byte		BootIndicator ;

	byte		StartHead ;
	byte		StartSector ;
	byte		StartCylinder ;

	byte		SystemIndicator ;

	byte		EndHead ;
	byte		EndSector ;
	byte		EndCylinder ;

	unsigned 	LBAStartSector ;
	unsigned 	LBANoOfSectors ;
} PACKED PartitionInfo ;

typedef struct
{
	PartitionInfo CurrentPartition ;
	PartitionInfo NextPartition ;
	unsigned	uiActualStartSector ;
} ExtPartitionTable ;

typedef struct
{
	unsigned uiNoOfPrimaryPartitions ;
	byte bIsExtPartitionPresent ;
	unsigned uiNoOfExtPartitions ;

	PartitionInfo PrimaryParitions [ MAX_NO_OF_PRIMARY_PARTITIONS ] ;
	PartitionInfo ExtPartitionEntry ;
	ExtPartitionTable ExtPartitions [ MAX_NO_OF_EXT_PARTITIONS ] ;
} PartitionTable ;

void PartitionManager_InitializePartitionTable(PartitionTable* pPartitionTable) ;
byte PartitionManager_ReadPartitionInfo(RawDiskDrive* pDisk, PartitionTable* pPartitionTable) ; 
byte PartitionManager_ClearPartitionTable(RawDiskDrive* pDisk) ;
byte PartitionManager_CreatePrimaryPartitionEntry(RawDiskDrive* pDisk, PartitionTable* pPartitionTable, unsigned uiSizeInSectors, byte bIsActive, byte bIsExt) ;
byte PartitionManager_CreateExtPartitionEntry(RawDiskDrive* pDisk, PartitionTable* pPartitionTable, unsigned uiSizeInSectors) ;
byte PartitionManager_DeletePrimaryPartition(RawDiskDrive* pDisk, PartitionTable* pPartitionTable) ;
byte PartitionManager_DeleteExtPartition(RawDiskDrive* pDisk, PartitionTable* pPartitionTable) ;
byte PartitionManager_UpdateSystemIndicator(RawDiskDrive* pDisk, unsigned uiLBAStartSector, unsigned uiSystemIndicator) ;

#endif
