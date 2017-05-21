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
#ifndef _UPANIXMAIN_H_
#define _UPANIXMAIN_H_

#include <Console.h>
#include <Display.h>
#include <Global.h>
#include <BuiltInKeyboardDriver.h>
#include <IDT.h>
#include <PIC.h>
#include <PIT.h>
#include <DMA.h>
#include <Floppy.h>
#include <MemConstants.h>

//Co processor types
typedef enum
{
	NO_CO_PROC = 1,
	CO_PROC_87_287 = 2,
	CO_PROC_387	=  3
} CO_PROC_TYPE ;

extern "C" void UpanixMain() ;
bool UpanixMain_isCoProcPresent() ;
int UpanixMain_KernelProcessID() ;
Mutex& UpanixMain_GetDMMMutex() ;
void DummyProcess() ;

#endif
