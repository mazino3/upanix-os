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
#include <VM86.h>
#include <DMM.h>
#include <Directory.h>
#include <FileOperations.h>
#include <MemConstants.h>
#include <video.h>
#include <SystemUtil.h>

extern byte* GetArea() ;

bool CheckBDA()
{
	byte* bios = (byte*)(0 - GLOBAL_DATA_SEGMENT_BASE) ;

	for(int i = 0; i < 0x500; i++)
	{
		if(bios[i] != GetArea()[i])
			return false ;
	}

	return true ;
}

bool DumpBDA()
{
	RETURN_X_IF_NOT(FileOperations_Create("mem_dump", ATTR_TYPE_FILE, ATTR_FILE_DEFAULT), FileOperations_SUCCESS, false) ;

	int fd ;
	RETURN_X_IF_NOT(FileOperations_Open(&fd, "mem_dump", O_RDWR), FileOperations_SUCCESS, false) ;

	int len ;
	char* bios = (char*)(0 - GLOBAL_DATA_SEGMENT_BASE) ;
	RETURN_X_IF_NOT(FileOperations_Write(fd, bios, 0x500, &len), FileOperations_SUCCESS, false) ;

	RETURN_X_IF_NOT(FileOperations_Close(fd), FileOperations_SUCCESS, false) ;

	return true ;
}

bool LoadBDA()
{
	int fd ;
	RETURN_X_IF_NOT(FileOperations_Open(&fd, "mem_dump", O_RDONLY), FileOperations_SUCCESS, false) ;

	unsigned len ;
	char* bios = (char*)(0 - GLOBAL_DATA_SEGMENT_BASE) ;
	RETURN_X_IF_NOT(FileOperations_Read(fd, bios, 0x500, &len), FileOperations_SUCCESS, false) ;

	RETURN_X_IF_NOT(FileOperations_Close(fd), FileOperations_SUCCESS, false) ;

	return true ;
}

void VM86_Test()
{
	static const char* szFileName = "ROOT@/osin/.realmode" ;
	FileSystem_DIR_Entry DirEntry ;
	if(FileOperations_GetDirEntry(szFileName, &DirEntry) != FileOperations_SUCCESS)
	{
		printf("\n Failed to file details for %s", szFileName) ;
		return ;
	}
	
	unsigned uiSize = DirEntry.uiSize ;
	byte* bImage = (byte*)DMM_AllocateForKernel(sizeof(char) * uiSize) ;

	int fd ;
	if(FileOperations_Open(&fd, szFileName, O_RDONLY) != FileOperations_SUCCESS)
	{
		printf("\n Failed to open File: %s", szFileName) ;
		return ;
	}

	unsigned n ;
	if(FileOperations_Read(fd, (char*)bImage, uiSize, &n) != FileOperations_SUCCESS) 
	{
		FileOperations_Close(fd) ;
		printf("\n Failed to Read from file: %s", szFileName) ;
		DMM_DeAllocateForKernel((unsigned)bImage) ;
		return ;
	}

	if(uiSize != n)
	{
		FileOperations_Close(fd) ;
		printf("\n No. of bytes Read: %u, but Requested bytes: %u", n, uiSize) ;
		DMM_DeAllocateForKernel((unsigned)bImage) ;
		return ;
	}

	FileOperations_Close(fd) ;

	byte* pDest = (byte*)(MEM_REAL_MODE_CODE - GLOBAL_DATA_SEGMENT_BASE) ;
	memcpy(pDest, bImage, uiSize) ;

	unsigned* val = (unsigned*)(0x100 - GLOBAL_DATA_SEGMENT_BASE) ;
	//*val = 200 ;
	printf("\n Before value: %u\n", *val) ;

	typedef void (*_fptr)() ;
	_fptr fptr = (_fptr)(pDest) ;
	if(!CheckBDA())
	{
		printf("\n BDA corrupted after kernel load") ;
		return ;
	}

	__asm__ __volatile__("pusha") ;
	fptr() ;
	__asm__ __volatile__("popa") ;

	printf("\n After value: %u", *val) ;
	Video_AllTries() ;
	
	DMM_DeAllocateForKernel((unsigned)bImage) ;
}

