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
#ifndef _DYNAMIC_LINK_LOADER_H_
#define _DYNAMIC_LINK_LOADER_H_

#include <ProcessManager.h>
#include <KernelService.h>

#define DynamicLinkLoader_SUCCESS						0
#define DynamicLinkLoader_ERR_TABLE_SIZE_UNDER_FLOW		1
#define DynamicLinkLoader_ERR_SYM_NOT_FOUND				2
#define DynamicLinkLoader_ERR_NOT_PIC					3
#define DynamicLinkLoader_ERR_LIB_EFOUND				4
#define DynamicLinkLoader_FAILURE						5

uint32_t DynamicLinkLoader_Initialize(unsigned uiPDEAddress) ;
void DynamicLinkLoader_UnInitialize(ProcessAddressSpace* processAddressSpace) ;
bool DynamicLinkLoader_GetSymbolOffset(const char* szJustDLLName, const char* szSymName, unsigned* uiDynSymOffset, ProcessAddressSpace* processAddressSpace) ;
void DynamicLinkLoader_DoRelocation(ProcessAddressSpace* processAddressSpace, int iID, unsigned uiRelocationOffset, __volatile__ int* iDynamicSymAddress) ;

#endif
