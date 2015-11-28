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
#include <Alloc.h>
#include <DMM.h>

int  iNewCount = 0 ;
void* operator new(size_t uiSize)
{
	iNewCount++ ;
	return (void*)DMM_AllocateForKernel(uiSize) ;
}

void* operator new[](size_t uiSize)
{
	iNewCount++ ;
	return (void*)DMM_AllocateForKernel(uiSize) ;
}

void* operator new(size_t uiSize, void* buffer)
{
	return buffer ;
}

void operator delete(void* p)
{
	iNewCount-- ;
	DMM_DeAllocateForKernel((unsigned)p) ;
}

void operator delete[](void* p)
{
	iNewCount-- ;
	DMM_DeAllocateForKernel((unsigned)p) ;
}

void operator delete(void*, void*)
{
}

