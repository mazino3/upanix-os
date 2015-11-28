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
#ifndef _PROCESS_GROUP_MANAGER_H_
#define _PROCESS_GROUP_MANAGER_H_

#define ProcessGroupManager_SUCCESS				0
#define ProcessGroupManager_FAILURE				1
#define ProcessGroupManager_ERR_NO_FREE_PGAS	2

# include <Global.h>
# include <DisplayManager.h>

#define MAX_NO_PROCESS_GROUP 20

typedef struct FGProcessList FGProcessList ;

struct FGProcessList 
{
	int iProcessID ;
	FGProcessList* pNext ;
} ;

typedef struct
{
	int				iProcessCount ;
	byte			bFree ;
	FGProcessList	fgProcessListHead ;
	DisplayBuffer		videoBuffer ;
} ProcessGroup ;

#define NO_PROCESS_GROUP_ID -1

extern int ProcessGroupManager_iFGProcessGroup ;
extern ProcessGroup* ProcessGroupManager_AddressSpace ;

byte ProcessGroupManager_Initialize() ;
byte ProcessGroupManager_CreateProcessGroup(byte bIsFGProcessGroup, int* iProcessGroupID) ;
byte ProcessGroupManager_DestroyProcessGroup(int iProcessGroupID) ;
void ProcessGroupManager_PutOnFGProcessList(int iProcessID) ;
void ProcessGroupManager_RemoveFromFGProcessList(int iDeleteProcessID) ;
int ProcessGroupManager_GetProcessCount(int iProcessGroupID) ;
void ProcessGroupManager_AddProcessToGroup(int iProcessID) ;
void ProcessGroupManager_RemoveProcessFromGroup(int iProcessID) ;
void ProcessGroupManager_SwitchFGProcessGroup(int iProcessID) ;
bool ProcessGroupManager_IsFGProcessGroup(int iProcessGroupID) ;
bool ProcessGroupManager_IsFGProcess(int iProcessGroupID, int iProcessID) ;
const ProcessGroup* ProcessGroupManager_GetFGProcessGroup() ;

#endif
