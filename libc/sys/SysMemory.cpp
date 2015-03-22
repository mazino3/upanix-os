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
# include <SysCall.h>

void* SysMemory_Alloc(unsigned uiSizeInBytes)
{
	__volatile__ int iAllocAddress ;
	SysCallDisplay_Handle(&iAllocAddress, SYS_CALL_ALLOC, false, uiSizeInBytes, 2, 3, 4, 5, 6, 7, 8, 9);
	return (void*)iAllocAddress ;
}

int SysMemory_Free(void* uiAddress)
{
	__volatile__ int iRetStatus ;
	SysCallDisplay_Handle(&iRetStatus, SYS_CALL_FREE, false, (unsigned)uiAddress, 2, 3, 4, 5, 6, 7, 8, 9);
	return iRetStatus ;
}

int SysMemory_GetAllocSize(void* uiAddress, int* size)
{
	__volatile__ int iRetStatus ;
	SysCallDisplay_Handle(&iRetStatus, SYS_CALL_GET_ALLOC_SIZE, false, (unsigned)uiAddress, (unsigned)size, 3, 4, 5, 6, 7, 8, 9);
	return iRetStatus ;
}
