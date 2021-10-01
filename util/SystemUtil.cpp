/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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
# include <SystemUtil.h>
# include <DMM.h>
# include <PS2KeyboardDriver.h>
# include <RTC.h>
# include <dtime.h>
# include <stdio.h>
# include <mdate.h>
# include <ProcessManager.h>
# include <DiskCache.h>
# include <try.h>
# include <DeviceDrive.h>
#include <PS2Controller.h>

void SystemUtil_Reboot()
{
  for(auto pDiskDrive : DiskDriveManager::Instance().DiskDriveList())
  {
		if(pDiskDrive->Mounted())
		{
			printf("\n UnMounting Drive: %-20s", pDiskDrive->DriveName().c_str());
      const auto& result = upan::trycall([&]() { pDiskDrive->UnMount(); });
			pDiskDrive->StopReleaseCacheTask(true);
      pDiskDrive->FlushDirtyCacheSectors();
      if(result.isBad())
				printf("\n Failed to UnMount Drive\n") ;
			else
				printf("[ Done ]") ;
		}
	}
	ProcessManager::Instance().Sleep(2000) ;
  PS2Controller::Instance().Reboot() ;
}

uint32_t SystemUtil_GetTimeOfDay()
{
	RTCDateTime rtcTime ;
	RTC::GetDateTime(rtcTime) ;

	mdate dt;
	mdate_Set(&dt, rtcTime.bDayOfMonth, rtcTime.bMonth, rtcTime.bCentury * 100 + rtcTime.bYear) ;
	
	int days ;	
	if(!mdate_SeedDateDifference(&dt, &days))
    throw upan::exception(XLOC, "invalid time");

  return days * 24 * 60 * 60 + rtcTime.bHour * 60 * 60 + rtcTime.bMinute * 60 + rtcTime.bSecond ;
	//tv->uimSec = tv->tSec * 1000 ;
}

void SystemUtil_GetRTCTimeFromTime(RTCTime* pRTCTime, const struct timeval* tv)
{
	mdate d1 ;
	mdate_GetSeedDate(&d1) ;
	
	int days = tv->tSec / (HRS_IN_DAY) ;
	mdate_AddDays(&d1, days) ;

	pRTCTime->bDayOfMonth = d1.dayOfMonth ;
	pRTCTime->bMonth = d1.month ;
	pRTCTime->bYear = d1.year % 100 ;
	pRTCTime->bCentury = d1.year / 100 ;

	int resi = tv->tSec % HRS_IN_DAY ;
	pRTCTime->bHour = resi / 3600 ;

	int resh = resi % 3600 ;
	pRTCTime->bMinute = resh / 60 ;
	pRTCTime->bSecond = resh % 60 ;
}

