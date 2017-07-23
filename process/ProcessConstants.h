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
#ifndef _PROC_CONST_H_
#define _PROC_CONST_H_

#define MAX_NO_PROCESS 3500

#define PROCESS_STACK_PAGES 8
#define PROCESS_KERNEL_STACK_PAGES 8
#define PROCESS_CG_STACK_PAGES 8

#define PROCESS_SPACE_FOR_OS 28 // MEM_KERNEL_RESV_SIZE
#define PROCESS_BASE (PROCESS_SPACE_FOR_OS * PAGE_TABLE_ENTRIES * PAGE_SIZE)

#define NO_PROCESS_ID -1

#endif
