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
#include <BuiltInKeyboardDriver.h>
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
#include <syscalldefs.h>

#define IKP ( ProcessManager::Instance().IsKernelProcess(ProcessManager::GetCurrentProcessID()) )
#define KERNEL_ADDR(DO, TYPE, ADDR) (TYPE)( (DO) ? (IKP ? ADDR : (PROCESS_BASE + ADDR - GLOBAL_DATA_SEGMENT_BASE)) : ADDR )

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
