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

void
Memory_Spike()
{
	byte bStatus ;
	unsigned uiPageNumber, i ;
	
	for(i = 0; i < 20; i++)
	{
		if((bStatus = KC::MMemManager().AllocatePhysicalPage(&uiPageNumber)) != MEM_SUCCESS)
		{
			KC::MDisplay().Address("\n Page Allocation Failed: ", bStatus) ;
			return ;
		}
	
		KC::MDisplay().Address("\nFree Page Number = ", uiPageNumber) ;
	}

	KC::MMemManager().DeAllocatePage(0x810) ;

	if((bStatus = KC::MMemManager().AllocatePhysicalPage(&uiPageNumber)) != MEM_SUCCESS)
	{
		KC::MDisplay().Address("\n Page Allocation Failed: ", bStatus) ;
		return ;
	}

	KC::MDisplay().Address("\nReFree Page Number = ", uiPageNumber) ;
}
