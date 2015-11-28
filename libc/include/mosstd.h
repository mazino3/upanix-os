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
#ifndef _MOS_STD_H_
#define _MOS_STD_H_

#define pid_t int

typedef enum
{
	NEW,
	RUN,
	WAIT_SLEEP,
	WAIT_INT,
	WAIT_CHILD,
	WAIT_RESOURCE,
	TERMINATED,
	TOTAL_PROC_STATUS
} PROCESS_STATUS ;

typedef struct
{
	int pid ;
	char* pname ;
	PROCESS_STATUS status ;
	int iParentProcessID ;
	int iProcessGroupID ;
	int iUserID ;
} PS ;

extern int SysProcess_Exec(const char* szFileName, ...) ;
int SysProcess_ExecArgV(const char* szFileName, int iNoOfArgs, char** szArgList) ;
extern void SysProcess_WaitPID(int iProcessID) ;
extern void SysProcess_Exit(int iExitStatus) ;
extern int SysProcess_GetPID() ;
extern int SysProcess_GetProcList(PS** pProcList, unsigned* uiListSize) ;
extern void SysProcess_FreeProcListMem(PS* pProcList, unsigned uiListSize) ;

#define execl SysProcess_Exec
#define execv(file, argc, argv) SysProcess_ExecArgV(file, argc, argv)
#define waitpid(pid) SysProcess_WaitPID(pid)
#define _exit(exit_status) SysProcess_Exit(exit_status)
#define getpid() SysProcess_GetPID()
#define getpslist(pslist, size) SysProcess_GetProcList(pslist, size)
#define freepslist(pslist, size) SysProcess_FreeProcListMem(pslist, size)

#endif
