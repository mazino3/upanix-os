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
#ifndef _SYS_CALL_H_
#define _SYS_CALL_H_

#include <DMM.h>
# include <Display.h>
# include <ProcessManager.h>
#include <AsmUtil.h>
#include <DynamicLinkLoader.h>
#include <Directory.h>
#include <Keyboard.h>
#include <PIT.h>
#include <StringUtil.h>
#include <KernelService.h>
#include <FileOperations.h>
#include <ProcessEnv.h>

#include <SysCallDisplay.h>
#include <SysCallFile.h>
#include <SysCallKB.h>
#include <SysCallMem.h>
#include <SysCallProc.h>
#include <SysCallDrive.h>
#include <SysCallUtil.h>

#define IKP ( ProcessManager::Instance().IsKernelProcess(ProcessManager::GetCurrentProcessID()) )
#define KERNEL_ADDR(DO, TYPE, ADDR) (TYPE)( (DO) ? (IKP ? ADDR : (PROCESS_BASE + ADDR - GLOBAL_DATA_SEGMENT_BASE)) : ADDR )

#define NO_OF_SYSCALL_PARAMS 10

#define DO_SYS_CALL(SYS_CALL_ID) \
__asm__ __volatile__("pushl %0" : : "i"(SYS_CALL_ID)) ; \
__asm__ __volatile__("lcall $0x78, $0x00") ; 

/*********************************** SYS CALL NUMBERS *******************************/

typedef enum
{
	SYS_CALL_DISPLAY_START = 0,
		SYS_CALL_DISPLAY_MESSAGE,
		SYS_CALL_DISPLAY_CHARACTER,
		SYS_CALL_DISPLAY_CLR_SCR,
		SYS_CALL_DISPLAY_MOV_CURSOR,
		SYS_CALL_DISPLAY_CLR_LINE,
		SYS_CALL_DISPLAY_ADDRESS,
		SYS_CALL_DISPLAY_SET_CURSOR,
		SYS_CALL_DISPLAY_GET_CURSOR,
		SYS_CALL_DISPLAY_RAW_CHAR,
    SYS_CALL_DISPLAY_SIZE,
	SYS_CALL_DISPLAY_END,
	
	SYS_CALL_FILE_START = 200,
		SYS_CALL_CHANGE_DIR,
		SYS_CALL_PWD,
		SYS_CALL_CWD,
		SYS_CALL_MKDIR,
		SYS_CALL_RMDIR,
		SYS_CALL_GET_DIR_LIST,
		SYS_CALL_FILE_OPEN,
		SYS_CALL_FILE_CREATE,
		SYS_CALL_FILE_CLOSE,
		SYS_CALL_FILE_READ,
		SYS_CALL_FILE_WRITE,
		SYS_CALL_FILE_SEEK,
		SYS_CALL_FILE_TELL,
		SYS_CALL_FILE_MODE,
		SYS_CALL_FILE_STAT,
		SYS_CALL_FILE_STAT_FD,
		SYS_CALL_FILE_ACCESS,
		SYS_CALL_FILE_DUP2,
		SYS_CALL_FILE_RESET_STD,
	SYS_CALL_FILE_END,

	SYS_CALL_MEM_START = 400,
		SYS_CALL_ALLOC,
		SYS_CALL_FREE,
		SYS_CALL_GET_ALLOC_SIZE,
	SYS_CALL_MEM_END,

	SYS_CALL_PROC_START = 600,
		SYS_CALL_DLL_RELOCATE,
		SYS_CALL_PROCESS_EXEC,
		SYS_CALL_PROCESS_WAIT_PID,
		SYS_CALL_PROCESS_EXIT,
		SYS_CALL_PROCESS_SLEEP,
		SYS_CALL_PROCESS_PID,
		SYS_CALL_PROCESS_GET_ENV,
		SYS_CALL_PROCESS_SET_ENV,
		SYS_CALL_PROCESS_GET_PS_LIST,
		SYS_CALL_PROCESS_FREE_PS_LIST,
		SYS_CALL_DISABLE_TASK_SWITCH,
		SYS_CALL_ENABLE_TASK_SWITCH,
		SYS_CALL_PROCESS_CHILD_ALIVE,
	SYS_CALL_PROC_END,

	SYS_CALL_KB_START = 800,
		SYS_CALL_KB_READ,
	SYS_CALL_KB_END,

	SYS_CALL_DRIVE_START = 900,
		SYS_CALL_CHANGE_DRIVE,
		SYS_CALL_SHOW_DRIVES,
		SYS_CALL_MOUNT_DRIVE,
		SYS_CALL_UNMOUNT_DRIVE,
		SYS_CALL_FORMAT_DRIVE,
		SYS_CALL_CURRENT_DRIVE_STAT,
	SYS_CALL_DRIVE_END,

	SYS_CALL_UTIL_START = 1200,
		SYS_CALL_UTIL_DTIME,
		SYS_CALL_UTIL_REBOOT,
		SYS_CALL_UTIL_TOD,
	SYS_CALL_UTIL_END,

} SYS_CALL_NUMBERS ;

/************************************************************************************/
void SysCall_Initialize() ;
void SysCall_Entry(__volatile__ unsigned uiCSCorrection,
__volatile__ unsigned uiSysCallID, 
__volatile__ unsigned uiP1, 
__volatile__ unsigned uiP2, 
__volatile__ unsigned uiP3, 
__volatile__ unsigned uiP4, 
__volatile__ unsigned uiP5, 
__volatile__ unsigned uiP6, 
__volatile__ unsigned uiP7, 
__volatile__ unsigned uiP8, 
__volatile__ unsigned uiP9) ;

#endif
