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
# include <malloc.h>
# include <stdlib.h>
# include <string.h>
# include <DMM.h>

void* calloc(size_t n, size_t s)
{
	void* a = malloc(n * s) ;
	memset(a, 0, n * s) ;
	return a ;
}

void* malloc(unsigned uiSizeInBytes)
{
	return DMM_AllocateForKernel(uiSizeInBytes);
}

void free(void* uiAddress)
{
	/* If Free fails!!! It should stop / crash the process */
	DMM_DeAllocateForKernel(uiAddress);
}

int get_alloc_size(void* uiAddress, int* size)
{
	return DMM_GetAllocSizeForKernel(uiAddress, size) ;
}

void* realloc(void* ptr, int s)
{
	void* new_ptr = NULL ;

	if(s)
		new_ptr = (void*)malloc(s) ;

	if(ptr)
	{
		int old_size ;

		if(get_alloc_size(ptr, &old_size) < 0)
			return (void*)NULL ;

		int copy_size = (old_size < s) ? old_size : s ;

		memcpy(new_ptr, ptr, copy_size) ;
	
		free(ptr) ;
	}

	return new_ptr ;
}
