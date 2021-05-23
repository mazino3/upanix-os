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
# include <SysCall.h>
# include <malloc.h>
# include <string.h>
# include <stdlib.h>
# include <stdio.h>

int exec(const char* szFileName, ...)
{
	__volatile__ int iProcessID ;
	__volatile__ int argc ;
	__volatile__ int* argv = NULL ;

	__volatile__ int i ;
	__volatile__ int* ref = (int*)&szFileName + 1 ;
	for(argc = 0; *(ref + argc); argc++) ;

	if(argc)
	{
		argv = (int*)malloc(sizeof(int) * argc) ;
		if(!argv)
			return -1 ;

		for(i = 0; i < argc; i++)
		{
			argv[i] = (volatile int)malloc(strlen((char*)(*(ref + i))) + 1) ;
			strcpy((char*)argv[i], (char*)(*(ref + i))) ;
		}
	}

	SysCallProc_Handle(&iProcessID, SYS_CALL_PROCESS_EXEC, false, (unsigned)szFileName, (unsigned)argc, (unsigned)argv, 4, 5, 6, 7, 8, 9);

	for(i = 0; i < argc; i++)
		free((void*)argv[i]) ;
	free((void*)argv) ;
	return iProcessID ;
}

int SysProcess_Exec(const char* szFileName, int iNoOfArgs, char *const szArgList[])
{
	__volatile__ int iProcessID ;
	SysCallProc_Handle(&iProcessID, SYS_CALL_PROCESS_EXEC, false, (unsigned)szFileName, (unsigned)iNoOfArgs, (unsigned)szArgList, 4, 5, 6, 7, 8, 9);
	return iProcessID ;
}

void SysProcess_WaitPID(int iProcessID)
{
	int iRetStatus ;
	SysCallProc_Handle(&iRetStatus, SYS_CALL_PROCESS_WAIT_PID, false, (unsigned)iProcessID, 2, 3, 4, 5, 6, 7, 8, 9);
}

void SysProcess_Exit(int iExitStatus)
{
	int iRetStatus ;
	SysCallProc_Handle(&iRetStatus, SYS_CALL_PROCESS_EXIT, false, (unsigned)iExitStatus, 2, 3, 4, 5, 6, 7, 8, 9);
}

void SysProcess_Sleep(unsigned milisec)
{
	int iRetStatus ;
	SysCallProc_Handle(&iRetStatus, SYS_CALL_PROCESS_SLEEP, false, milisec, 2, 3, 4, 5, 6, 7, 8, 9);
}

int SysProcess_GetPID()
{
	__volatile__ int iProcessID ;
	SysCallProc_Handle(&iProcessID, SYS_CALL_PROCESS_PID, false, 1, 2, 3, 4, 5, 6, 7, 8, 9);
	return iProcessID ;
}

const char* SysProcess_GetEnv(const char* szVar)
{
	__volatile__ int iRetStatus ;
	SysCallProc_Handle(&iRetStatus, SYS_CALL_PROCESS_GET_ENV, false, (unsigned)szVar, 2, 3, 4, 5, 6, 7, 8, 9);
	return (const char*)iRetStatus ;
}

int SysProcess_SetEnv(const char* szVar, const char* szVal)
{
	__volatile__ int iRetStatus ;
	SysCallProc_Handle(&iRetStatus, SYS_CALL_PROCESS_SET_ENV, false, (unsigned)szVar, (unsigned)szVal, 3, 4, 5, 6, 7, 8, 9);
	return iRetStatus ;
}

int SysProcess_GetProcList(PS** pProcList, unsigned* uiListSize)
{
	__volatile__ int iRetStatus ;
	SysCallProc_Handle(&iRetStatus, SYS_CALL_PROCESS_GET_PS_LIST, false, (unsigned)pProcList, (unsigned)uiListSize, 3, 4, 5, 6, 7, 8, 9);
	return iRetStatus ;
}

void SysProcess_FreeProcListMem(PS* pProcList, unsigned uiListSize)
{
	__volatile__ int iRetStatus ;
	SysCallProc_Handle(&iRetStatus, SYS_CALL_PROCESS_FREE_PS_LIST, false, (unsigned)pProcList, uiListSize, 3, 4, 5, 6, 7, 8, 9);
}

int SysProcess_IsProcessAlive(int pid) {
  __volatile__ int iRetStatus ;
  SysCallProc_Handle(&iRetStatus, SYS_CALL_PROCESS_ALIVE, false, (unsigned)pid, 2, 3, 4, 5, 6, 7, 8, 9);
  return iRetStatus ;
}

int SysProcess_ThreadExec(uint32_t threadCaller, uint32_t entryAddress, void* arg) {
  __volatile__ int iRetStatus ;
  SysCallProc_Handle(&iRetStatus, SYS_CALL_THREAD_EXEC, false, threadCaller, entryAddress, (uint32_t)arg, 4, 5, 6, 7, 8, 9);
  return iRetStatus ;
}

int SysProcess_IsChildAlive(int iProcessID) {
  __volatile__ int iRetStatus ;
  SysCallProc_Handle(&iRetStatus, SYS_CALL_PROCESS_CHILD_ALIVE, false, iProcessID, 2, 3, 4, 5, 6, 7, 8, 9);
  return iRetStatus ;
}