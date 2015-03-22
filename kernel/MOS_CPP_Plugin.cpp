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
#include <MOSMain.h>
#include <stdio.h>
#include <DMM.h>

void Global_Object_Init()
{
	//flushSector.FlushSector() ;
}

extern "C"
{
	void *__dso_handle = 0; //Attention! Optimally, you should remove the '= 0' part and define this in your asm script.

	int __cxa_atexit(void (*f)(void *), void *objptr, void *dso)
	{
		return 0; 
	};

	void __cxa_pure_virtual()
	{
		printf("\n C++ Runtime Error: Pure virtual function called!\n") ;
		while(1) ;
	}
}
