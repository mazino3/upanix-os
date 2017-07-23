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
#include <ProcessLoader.h>
#include <ProcessManager.h>
#include <ProcessAllocator.h>
#include <PIC.h>
#include <FileSystem.h>
#include <Directory.h>
#include <Display.h>
#include <MemManager.h>
#include <ElfParser.h>
#include <ElfProgHeader.h>
#include <DMM.h>
#include <MemUtil.h>
#include <DynamicLinkLoader.h>
#include <StringUtil.h>
#include <ProcFileManager.h>
#include <FileOperations.h>
#include <ElfRelocationSection.h>
#include <ElfSymbolTable.h>
#include <ElfDynamicSection.h>
#include <stdio.h>
#include <string.h>
#include <exception.h>
#include <uniq_ptr.h>
#include <try.h>

ProcessLoader::ProcessLoader() :
  PROCESS_DLL_FILE(upan::string(OSIN_PATH) + __PROCESS_DLL_FILE),
  PROCESS_START_UP_FILE(upan::string(OSIN_PATH) + __PROCESS_START_UP_FILE)
{
}

byte* ProcessLoader::LoadInitSection(unsigned& uiSectionSize, const upan::string& szSectionName)
{
	FileSystem_DIR_Entry DirEntry ;

  if(FileOperations_GetDirEntry(szSectionName.c_str(), &DirEntry) != FileOperations_SUCCESS)
    throw upan::exception(XLOC, "failed to get dir entry for: %s", szSectionName.c_str());
	
	if((DirEntry.usAttribute & ATTR_TYPE_DIRECTORY) == ATTR_TYPE_DIRECTORY)
    throw upan::exception(XLOC, "%s is a directory!", szSectionName.c_str());

	uiSectionSize = DirEntry.uiSize ;
  upan::uniq_ptr<byte[]> bSectionImage(new byte[sizeof(char) * uiSectionSize]);

  int fd;
  unsigned n;

  if(FileOperations_Open(&fd, szSectionName.c_str(), O_RDONLY) != FileOperations_SUCCESS)
    throw upan::exception(XLOC, "failed to open file: %s", szSectionName.c_str());

  if(FileOperations_Read(fd, (char*)bSectionImage.get(), 0, &n) != FileOperations_SUCCESS)
    throw upan::exception(XLOC, "error reading file: %s", szSectionName.c_str());

  if(FileOperations_Close(fd) != FileOperations_SUCCESS)
    throw upan::exception(XLOC, "error closing file: %s", szSectionName.c_str());

  if(n != uiSectionSize)
    throw upan::exception(XLOC, "could read only %u of %u bytes of %s section", n, uiSectionSize, szSectionName.c_str());

  return bSectionImage.release();
}

byte* ProcessLoader::LoadDLLInitSection(unsigned& uiSectionSize)
{
  return LoadInitSection(uiSectionSize, PROCESS_DLL_FILE);
}

byte* ProcessLoader::LoadStartUpInitSection(unsigned& uiSectionSize)
{
  return LoadInitSection(uiSectionSize, PROCESS_START_UP_FILE);
}

using namespace ELFSectionHeader ;

unsigned ProcessLoader_GetCeilAlignedAddress(unsigned uiAddress, unsigned uiAlign)
{
	while(true)
	{
		if((uiAddress % uiAlign) == 0)
			return uiAddress ;
	
		uiAddress++ ;
	}
	return 0 ;
}

unsigned ProcessLoader_GetFloorAlignedAddress(unsigned uiAddress, unsigned uiAlign)
{
	while(true)
	{
		if((uiAddress % uiAlign) == 0)
			return uiAddress ;
	
		uiAddress-- ;
	}
	return 0 ;
}
