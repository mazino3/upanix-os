/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define SUCCESS 1
#define FAILURE 0

# include <ctype.h>
# include <dtime.h>
# include <stdlib.h>
# include <KernelComponents.h>
# include <ReturnHandler.h>
# include <MemConstants.h>

typedef unsigned long long	DDWORD ;
typedef unsigned			DWORD ;

#define IS_KERNEL() (KERNEL_MODE == true || ProcessManager::GetCurrentProcessID() == NO_PROCESS_ID)

#define IS_KERNEL_PROCESS(pid) (SPECIAL_TASK || ProcessManager::Instance().IsKernelProcess(pid))

#define IS_FG_PROCESS_GROUP() (ProcessManager::Instance().GetCurrentPAS().isFGProcessGroup())

#define KERNEL_REAL_ADDRESS(ADDR) ((unsigned)ADDR + GLOBAL_DATA_SEGMENT_BASE)
#define KERNEL_VIRTUAL_ADDRESS(ADDR) ((unsigned)ADDR - GLOBAL_DATA_SEGMENT_BASE) 

#define RETURN_IF_NOT(RetVal, Func, CheckVal) \
RetVal = Func ;\
if(RetVal != CheckVal) \
return RetVal ;

#define RETURN_X_IF_NOT(Func, CheckVal, X) \
if(Func != CheckVal) \
return X ;

#define TRACE_LINE printf("\n TRACE: %d", __LINE__)

#define BCD_TO_DECIMAL(no)	((((no & 0xF0) >> 4) * 10) + (no & 0x0F))

#define ROOT_DRIVE_ID MountManager_GetRootDriveID()
#define ROOT_DRIVE_SYN "ROOT"

extern byte KERNEL_MODE ;
extern byte SPECIAL_TASK ;

#define LIB_PATH ":ROOT@/lib:ROOT@/usr/lib:"
#define BIN_PATH "ROOT@/bin/"
#define OSIN_PATH "ROOT@/osin/"

#define MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )

bool UpanixMain_IsKernelDebugOn() ;

#endif
