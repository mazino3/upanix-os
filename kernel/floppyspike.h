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

void FloppyRelatedSpike() ;

void
FloppyRelatedSpike()
{
	byte bStatus ;

/*	if((bStatus = Floppy_Format(0)) != Floppy_SUCCESS)
	{
		KC::MDisplay().Address("\nFLOPPY FORMAT FAILED : ", bStatus) ;
		return ;
	}*/

	byte buf[1024] ;
	bStatus = Floppy_Read(0, MemUtil_GetDS(), 0, 1, 512, buf) ;
	if(bStatus == Floppy_SUCCESS)
	{
		KC::MDisplay().Message("\nFLOPPY READ SUCCESS\n", KC::MDisplay().WHITE_ON_BLACK()) ;

		buf[512] = '\0' ;
		KC::MDisplay().Message(buf, ' ') ;
	}
	else
	{
		KC::MDisplay().Address("\nFLOPPY READ FAILURE :", bStatus) ;
	}

	buf[0] = 'M' ;
	buf[1] = 'O' ;
	buf[2] = 'S' ;
	bStatus = Floppy_Write(0, MemUtil_GetDS(), 0, 1, 512, buf) ;
	if(bStatus == Floppy_SUCCESS)
	{
		KC::MDisplay().Message("\nFLOPPY WRITE SUCCESS\n", KC::MDisplay().WHITE_ON_BLACK()) ;
	}
	else
	{
		KC::MDisplay().Address("\nFLOPPY WRITE FAILURE :", bStatus) ;
	}

	bStatus = Floppy_Read(0, MemUtil_GetDS(), 0, 1, 512, buf) ;
	if(bStatus == Floppy_SUCCESS)
	{
		KC::MDisplay().Message("\nFLOPPY READ SUCCESS\n", KC::MDisplay().WHITE_ON_BLACK()) ;

		buf[512] = '\0' ;
		KC::MDisplay().Message(buf, ' ') ;
	}
	else
	{
		KC::MDisplay().Address("\nFLOPPY READ FAILURE :", bStatus) ;
	}
}
