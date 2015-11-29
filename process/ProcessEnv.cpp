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
#include <ProcessEnv.h>
#include <MemManager.h>
#include <MemUtil.h>
#include <StringUtil.h>
#include <DMM.h>

#define PROCESS_ENV_LIST ((ProcessEnvEntry*)(PROCESS_ENV_PAGE - GLOBAL_DATA_SEGMENT_BASE))

static unsigned ProcessEnv_GetProcessEnvPageNumber(__volatile__ ProcessAddressSpace& processAddressSpace)
{
	unsigned uiPDEAddress = processAddressSpace.taskState.CR3_PDBR ;
	unsigned uiPDEIndex = ((PROCESS_ENV_PAGE >> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((PROCESS_ENV_PAGE >> 12) & 0x3FF) ;
	unsigned uiPTEAddress = ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] & 0xFFFFF000 ;
	return (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] & 0xFFFFF000) / PAGE_SIZE ;
}

byte ProcessEnv_Initialize(__volatile__ unsigned uiPDEAddress, __volatile__ int iParentProcessID)
{
	unsigned uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();

	char* pEnv =(char*)(uiFreePageNo * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE) ;
	for(unsigned i = 0; i < PAGE_SIZE; i += ENV_VAR_LENGTH)
		pEnv[i] = '\0' ;

	//TODO: To be set to Home Directory Env Var
	String_Copy(&pEnv[0], FS_ROOT_DIR) ;

	unsigned uiPDEIndex = ((PROCESS_ENV_PAGE>> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((PROCESS_ENV_PAGE>> 12) & 0x3FF) ;

	unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;

	// This page is a Read Only area for user process. 0x5 => 101 => User Domain, Read Only, Present Bit
	((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x5 ;
	Mem_FlushTLB();

	if(iParentProcessID != NO_PROCESS_ID)
	{
		__volatile__ ProcessAddressSpace& parentProcessAddrSpace = ProcessManager::Instance().GetAddressSpace(iParentProcessID);

		unsigned uiParentPageNo = ProcessEnv_GetProcessEnvPageNumber(parentProcessAddrSpace) ;

		char* pParentEnv = (char*)(uiParentPageNo * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE) ;

		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pParentEnv, MemUtil_GetDS(), (unsigned)pEnv, PAGE_SIZE) ;
	}

	return ProcessEnv_SUCCESS ;
}

void ProcessEnv_InitializeForKernelProcess()
{
	unsigned i ;
	for(i = 0; i < PAGE_SIZE; i += ENV_VAR_LENGTH)
		PROCESS_ENV_LIST[i].Val[0] = '\0' ;

	//TODO: To be set to Home Directory Env Var
	ProcessEnv_Set("PWD", FS_ROOT_DIR) ;
}

void ProcessEnv_UnInitialize(ProcessAddressSpace* processAddressSpace)
{
	MemManager::Instance().DeAllocatePhysicalPage(ProcessEnv_GetProcessEnvPageNumber(*processAddressSpace)) ;
}

char* ProcessEnv_Get(const char* szEnvVar)
{
	int i ;
	for(i = 0; i < NO_OF_ENVS; i++)
	{
		if(String_Compare(PROCESS_ENV_LIST[i].Var, szEnvVar) == 0)
		{
				return (char*)((unsigned)&(PROCESS_ENV_LIST[i].Val)) ;
		}
	}

	return NULL ;
}

byte ProcessEnv_Set(const char* szEnvVar, const char* szEnvValue)
{
	int i ;
	for(i = 0; i < NO_OF_ENVS; i++)
	{
		if(String_Compare(PROCESS_ENV_LIST[i].Var, szEnvVar) == 0)
		{
			String_Copy(PROCESS_ENV_LIST[i].Val, szEnvValue) ;
			return ProcessEnv_SUCCESS ;
		}
	}

	for(i = 0; i < NO_OF_ENVS; i++)
	{
		if(PROCESS_ENV_LIST[i].Val[0] == '\0')
		{
			String_Copy(PROCESS_ENV_LIST[i].Var, szEnvVar) ;
			String_Copy(PROCESS_ENV_LIST[i].Val, szEnvValue) ;
			return ProcessEnv_SUCCESS ;
		}
	}

	return ProcessEnv_ERR_FULL ;
}

