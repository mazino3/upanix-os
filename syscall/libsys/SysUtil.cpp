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
# include <SysCall.h>
# include <dtime.h>

void SysUtil_GetDateTime(RTCTime* pRTCTime)
{
	int iRetStatus ;
	SysCallUtil_Handle(&iRetStatus, SYS_CALL_UTIL_DTIME, false, (unsigned)pRTCTime, 2, 3, 4, 5, 6, 7, 8, 9);
}

void SysUtil_Reboot()
{
	int iRetStatus ;
	SysCallUtil_Handle(&iRetStatus, SYS_CALL_UTIL_REBOOT, false, 1, 2, 3, 4, 5, 6, 7, 8, 9);
}

int SysUtil_GetTimeOfDay(struct timeval* pTV)
{
	int iRetStatus ;
	SysCallUtil_Handle(&iRetStatus, SYS_CALL_UTIL_TOD, false, (unsigned)pTV, 2, 3, 4, 5, 6, 7, 8, 9);
	return iRetStatus ;
}

uint32_t SysUtil_GetTimeSinceBoot() {
  return PIT_GetClockCount();
}