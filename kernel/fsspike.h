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

void FSSpike() ;
void FileSystem_SpikeFSBootBlockRead() ;
void DirectorySpike() ;

static char* szDirNames[11] = { "T1", "T2", "T3", "T4", "T5", "T6", "T7", "T8", "T9", "T10", "T11" } ;

/* FileSystem SPIKE START*/
void
FSSpike()
{
	byte bStatus ;
	byte buf[1024] ;

	KC::MDisplay().Address("\n Floppy_FAT SIZE = ", sizeof(FileSystem_BootBlock)) ;
	bStatus = FileSystem_Format(&Floppy_Type144) ;

	if(bStatus != FileSystem_SUCCESS)
		KC::MDisplay().Address("\n Floppy FORMAT Failed : ", bStatus) ;
	else
		KC::MDisplay().Message("\n Floppy FORMAT Success\n", KC::MDisplay().WHITE_ON_BLACK()) ;

	bStatus = Floppy_Read(1, MemUtil_GetDS(), 0, 1, 512, buf) ;
	if(bStatus == Floppy_SUCCESS)
	{
		KC::MDisplay().Message("\nFLOPPY READ SUCCESS\n", KC::MDisplay().WHITE_ON_BLACK()) ;

		unsigned i ;
		for(i = 0; i < 512; i++)
			KC::MDisplay().Character(buf[i], ' ') ;
	}
	else
	{
		KC::MDisplay().Address("\nFLOPPY READ FAILURE :", bStatus) ;
	}

	FileSystem_SpikeFSBootBlockRead() ;

	DirectorySpike() ;
}

void
FileSystem_SpikeFSBootBlockRead()
{
	byte bStatus ;
	
	FSMountInfo.driveInfo = &Floppy_Type144 ;
	FSMountInfo.uiMountPoint = 0x400000 ;

	if((bStatus = FileSystem_Mount(&FSMountInfo)) != FileSystem_SUCCESS)
	{
		KC::MDisplay().Address("\nFailed to Mount the FileSystem : ", bStatus) ;
		return ;
	}

	FileSystem_BootBlock* FSBootBlock = &FSMountInfo.FSBootBlock ;

	if(FSBootBlock->BPB_jmpBoot[0] != 0xEB)
		KC::MDisplay().Message("\nS 1\n", KC::MDisplay().WHITE_ON_BLACK()) ;
		
	if(FSBootBlock->BPB_jmpBoot[1] != 0xFE)
		KC::MDisplay().Message("S 2\n", KC::MDisplay().WHITE_ON_BLACK()) ;

	if(FSBootBlock->BPB_jmpBoot[2] != 0x90)
		KC::MDisplay().Message("S 3\n", KC::MDisplay().WHITE_ON_BLACK()) ;

	if(FSBootBlock->BPB_BytesPerSec != 0x200)
		KC::MDisplay().Message("S 5\n", KC::MDisplay().WHITE_ON_BLACK()) ;
		
	if(FSBootBlock->BPB_RsvdSecCnt != 1 )
		KC::MDisplay().Message("S 7\n", KC::MDisplay().WHITE_ON_BLACK()) ;
		
	if(FSBootBlock->BPB_Media  != 0xF0 )
	   KC::MDisplay().Message("S 11\n", KC::MDisplay().WHITE_ON_BLACK()) ;
		
	if(FSBootBlock->BPB_FSTableSize != 23)
	   KC::MDisplay().Message("S 13\n", KC::MDisplay().WHITE_ON_BLACK()) ;
		
	if(FSBootBlock->BPB_ExtFlags  != 0x0080 )
	   KC::MDisplay().Message("S 14\n", KC::MDisplay().WHITE_ON_BLACK()) ;
		
	if(FSBootBlock->BPB_FSVer != 0x0100)
	   KC::MDisplay().Message("S 15\n", KC::MDisplay().WHITE_ON_BLACK()) ;
		
	if(FSBootBlock->BPB_FSInfo  != 1 )
	   KC::MDisplay().Message("S 17\n", KC::MDisplay().WHITE_ON_BLACK()) ;
		
	if(FSBootBlock->BPB_VolID != 0x01)
	   KC::MDisplay().Message("S 22\n", KC::MDisplay().WHITE_ON_BLACK()) ;
		
	if(String_Compare(FSBootBlock->BPB_VolLab, "No Name   "))
	   KC::MDisplay().Message("S 23\n", KC::MDisplay().WHITE_ON_BLACK()) ;
}

void
DirectorySpike()
{
	byte bStatus ;
	
	unsigned i ;	
	for(i = 0; i < DIR_ENTRIES_PER_SECTOR + 1; i++)
	{
		bStatus = Directory_Create(&FSMountInfo, 0, szDirNames[i], ATTR_DIR_DEFAULT | ATTR_TYPE_DIRECTORY) ;
		if(bStatus != Directory_SUCCESS)
		{
			KC::MDisplay().Address("\n DIR ERR at Level ", i) ;
			KC::MDisplay().Address(" As ", bStatus) ;
		}
	}

	bStatus = Directory_Delete(&FSMountInfo, 0, "T3") ;
	if(bStatus != Directory_SUCCESS)
		KC::MDisplay().Address("\n DIR Del Failed: ", bStatus) ;

	bStatus = Directory_Create(&FSMountInfo, 0, "DEL_DIR_REUSE", ATTR_DIR_DEFAULT | ATTR_TYPE_DIRECTORY) ;
	if(bStatus != Directory_SUCCESS)
		KC::MDisplay().Address("\n DIR Create On Del Failed: ", bStatus) ;

	bStatus = Directory_Create(&FSMountInfo, 0, "TEST_FILE", ATTR_FILE_DEFAULT | ATTR_TYPE_FILE) ;
	if(bStatus != Directory_SUCCESS)
		KC::MDisplay().Address("\n DIR Create On Del Failed: ", bStatus) ;

	byte bDataBuffer[520] ;
	
	String_Copy(bDataBuffer, "Test File Write. MOS FS Imp Test") ;

	bDataBuffer[512] = 'M' ;
	bDataBuffer[513] = 'O' ;
	bDataBuffer[514] = 'S' ;
	
	bStatus = Directory_FileWrite(&FSMountInfo, 0, "TEST_FILE", bDataBuffer, 0, 515) ;
	if(bStatus != Directory_SUCCESS)
		KC::MDisplay().Address("\n Failed to Write File: ", bStatus) ;
}
/* FileSystem SPIKE END */
