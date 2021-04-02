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
# include <SysCallUtil.h>
# include <SystemUtil.h>
# include <RTC.h>

byte SysCallUtil_IsPresent(unsigned uiSysCallID)
{
	return (uiSysCallID > SYS_CALL_UTIL_START && uiSysCallID < SYS_CALL_UTIL_END) ;
}

void SysCallUtil_Handle(
__volatile__ int* piRetVal,
__volatile__ unsigned uiSysCallID, 
__volatile__ bool bDoAddrTranslation,
__volatile__ unsigned uiP1, 
__volatile__ unsigned uiP2, 
__volatile__ unsigned uiP3, 
__volatile__ unsigned uiP4, 
__volatile__ unsigned uiP5, 
__volatile__ unsigned uiP6, 
__volatile__ unsigned uiP7, 
__volatile__ unsigned uiP8, 
__volatile__ unsigned uiP9)
{
	switch(uiSysCallID)
	{
		case SYS_CALL_UTIL_DTIME : 
			//P1 => Ret RTC Pointer
			{
				RTCDateTime* pRTCTime = KERNEL_ADDR(bDoAddrTranslation, RTCDateTime*, uiP1) ;

				*piRetVal = 0 ;
				RTC::GetDateTime((*pRTCTime)) ;
			}
			break ;

    case SYS_CALL_UTIL_BTIME:
      {
        *piRetVal = SysUtil_GetTimeSinceBoot();
      }
      break;

    case SYS_CALL_UTIL_TOD :
      // P1 => Ret timeval Pointer
      {
        struct timeval* tv = KERNEL_ADDR(bDoAddrTranslation, struct timeval*, uiP1) ;

        *piRetVal = 0 ;
        try
        {
          tv->tSec = SystemUtil_GetTimeOfDay();
        }
        catch(...)
        {
          *piRetVal = -1 ;
        }
      }
      break ;

    case SYS_CALL_UTIL_REBOOT :
			{
				*piRetVal = 0 ;
				SystemUtil_Reboot() ;
			}
			break ;
	}
}
