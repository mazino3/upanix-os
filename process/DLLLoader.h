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
#ifndef _DLL_LOADER_H_
#define _DLL_LOADER_H_

#define DLLLoader_SUCCESS							0
#define DLLLoader_ERR_SYSTEM_DLL_LIMIT_EXCEEDED		1
#define DLLLoader_ERR_HUGE_DLL_SIZE					2
#define DLLLoader_ERR_PROCESS_DLL_LIMIT_EXCEEDED	3
#define DLLLoader_ERR_NOT_PIC						4
#define DLLLoader_FAILURE							5

#define DLL_ELF_SEC_HEADER_PAGE 1

#include <Global.h>
#include <ProcessManager.h>
#include <BufferedReader.h>

# define REL_PLT_SUB_NAME	".plt" 
# define REL_DYN_SUB_NAME	".dyn" 
# define BSS_SEC_NAME		".bss"

typedef struct
{
  char szName[56] ;
  unsigned uiNoOfPages ;
  unsigned* uiAllocatedPageNumbers ;
} PACKED ProcessSharedObjectList ;

extern int DLLLoader_iNoOfProcessSharedObjectList ;

void DLLLoader_Initialize() ;
void DLLLoader_LoadELFDLL(const char* szDLLName, const char* szJustDLLName, Process* processAddressSpace) ;
void DLLLoader_DeAllocateProcessDLLPTEPages(Process* processAddressSpace) ;
int DLLLoader_GetProcessSharedObjectListIndexByName(const char* szDLLName) ;
ProcessSharedObjectList* DLLLoader_GetProcessSharedObjectListByName(const char* szDLLName) ;
ProcessSharedObjectList* DLLLoader_GetProcessSharedObjectListByIndex(int iIndex) ;
int DLLLoader_GetRelativeDLLStartAddress(const char* szDLLName) ;
unsigned DLLLoader_GetProcessDLLLoadAddress(Process* processAddressSpace, int iIndex) ;
byte DLLLoader_MapDLLPagesToProcess(Process* processAddressSpace, 
		ProcessSharedObjectList* pProcessSharedObjectList, unsigned uiAllocatedPagesCount) ;

#endif
