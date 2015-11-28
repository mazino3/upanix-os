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

#ifndef _PROC_FILE_MANAGER_H_
#define _PROC_FILE_MANAGER_H_

# include <Global.h>
# include <ProcessManager.h>

#define ProcFileManager_SUCCESS					0
#define ProcFileManager_ERR_MAX_OPEN_FILES		1
#define ProcFileManager_ERR_INVALID_FD			2
#define ProcFileManager_ERR_INVALID_SEEK_TYPE	3
#define ProcFileManager_ERR_INVALID_OFFSET		4
#define ProcFileManager_ERR_REF_OPEN			5
#define ProcFileManager_ERR_INVALID_STDFD		6
#define ProcFileManager_FAILURE					7

#ifndef DEF_FILE_MODES
#define DEF_FILE_MODES
typedef enum
{	
	O_RDONLY = 1,
	O_WRONLY = 2,
	O_RDWR = 4,
	O_APPEND = 8,
	O_CREAT = 16,
	O_EXCL = 32,
	O_TRUNC = 64
} FileModes ;
#endif

typedef enum
{

	PROC_FREE_FD = 1100,
	PROC_STDIN,
	PROC_STDOUT,
	PROC_STDERR,

} SPECIAL_FILES;

#define	SEEK_SET 0
#define	SEEK_CUR 1
#define	SEEK_END 2

typedef struct
{
	char* szFileName ;
	unsigned uiOffset ;
	byte mode ;
	int iDriveID ;
	unsigned uiFileSize ;
	int RefCount ;
	int iLastReadSectorIndex ;
	unsigned uiLastReadSectorNumber ;
} PACKED ProcFileDescriptor ;

byte ProcFileManager_Initialize(__volatile__ unsigned uiPDEAddress, __volatile__ int iParentProcessID) ;
void ProcFileManager_InitForKernel() ;

void ProcFileManager_UnInitialize(ProcessAddressSpace* processAddressSpace) ;

byte ProcFileManager_GetFD(int* fd) ;
byte ProcFileManager_AllocateFD(int* fd, const char* szFileName, const byte mode, int iDriveID, const unsigned uiFileSize, unsigned uiStartSectorID) ;
byte ProcFileManager_FreeFD(int fd) ;
byte ProcFileManager_UpdateOffset(int fd, int seekType, int iOffset) ;
byte ProcFileManager_GetOffset(int fd, unsigned* uiOffset) ;
byte ProcFileManager_GetFDEntry(int fd, ProcFileDescriptor** pFDEntry) ;
byte ProcFileManager_GetMode(int fd, byte* mode) ;
byte ProcFileManager_IsSTDOUT(int fd) ;
byte ProcFileManager_IsSTDIN(int fd) ;
byte ProcFileManager_Dup2(int oldFD, int newFD) ;
byte ProcFileManager_GetRealNonDuppedFD(int dupedFD, int* realFD) ;
byte ProcFileManager_InitSTDFile(int StdFD) ;

#endif
