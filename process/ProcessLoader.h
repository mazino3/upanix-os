/*
 *	Mother Operating System - An x86 based Operating System
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
#ifndef _PROCESS_LOADER_H_
#define _PROCESS_LOADER_H_

#include <Global.h>
#include <ElfSectionHeader.h>
#include <DLLLoader.h>
#include <BufferedReader.h>

#define ProcessLoader_SUCCESS							0
#define ProcessLoader_ERR_ZERO_PROCESS_SIZE				1
#define ProcessLoader_ERR_COPY_IMAGE					2
#define ProcessLoader_ERR_HUGE_PROCESS_SIZE				3
#define ProcessLoader_ERR_TEXT_ADDR_LESS_THAN_LIMIT		4
#define ProcessLoader_ERR_NOT_ALIGNED_TO_PAGE			5
#define ProcessLoader_ERR_NO_PROC_INIT					6
#define ProcessLoader_FAILURE							7

#define __PROCESS_START_UP_FILE	".procinit"
#define __PROCESS_DLL_FILE		".dll"

extern char PROCESS_START_UP_FILE[30] ;
extern char PROCESS_DLL_FILE[30] ;

byte ProcessLoader_Load(const char* szProcessName, ProcessAddressSpace* pProcessAddressSpace, unsigned *uiPDEAddress,
		unsigned* uiEntryAdddress, unsigned* uiProcessEntryStackSize, int iNumberOfParameters,
		char** szArgumentList) ;
byte ProcessLoader_LoadELFExe(const char* szProcessName, ProcessAddressSpace* pProcessAddressSpace, unsigned *uiPDEAddress,
		unsigned *uiEntryAdddress, unsigned* uiProcessEntryStackSize, 
		int iNumberOfParameters, char** szArgumentList) ;
byte ProcessLoader_CopyImage(const char* szProcessName, unsigned uiPDEAddr, unsigned uiNoOfPagesForProcess) ;
byte ProcessLoader_CopyProcessPage(const char* szProcessName, unsigned uiProcessAddress, unsigned uiOffset) ;
void ProcessLoader_CopyElfImage(unsigned uiPDEAddr, byte* bProcessImage, unsigned uiMemImageSize, unsigned uiMinMemAddr) ;

void ProcessLoader_PushProgramInitStackData(unsigned uiPDEAddr, unsigned uiNoOfPagesForProcess, 
				unsigned uiProcessBase, unsigned* uiProcessEntryStackSize,
				unsigned uiProgramStartAddress, unsigned uiInitRelocAddress, unsigned uiTermRelocAddress, int iNumberOfParameters, 
				char** szArgumentList) ;

unsigned ProcessLoader_GetCeilAlignedAddress(unsigned uiAddress, unsigned uiAlign) ;
unsigned ProcessLoader_GetFloorAlignedAddress(unsigned uiAddress, unsigned uiAlign) ;
byte ProcessLoader_LoadInitSections(ProcessAddressSpace* pProcessAddressSpace, unsigned* uiSectionSize, byte** bSectionImage, const char* szSectionName) ;

#endif
