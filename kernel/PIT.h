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
#ifndef _TIMER_H_
#define _TIMER_H_

#include <Global.h>
class IRQ ;

void PIT_Initialize() ;
void PIT_Handler() ;

__volatile__ unsigned PIT_GetClockCount() ;

__volatile__ unsigned char PIT_IsContextSwitch() ;
__volatile__ void PIT_SetContextSwitch(bool flag) ;

__volatile__ unsigned char PIT_IsTaskSwitch() ;
__volatile__ void PIT_SetTaskSwitch(bool flag) ;

__volatile__ unsigned PIT_RoundSleepTime(__volatile__ unsigned uiSleepTime) ;

#endif
