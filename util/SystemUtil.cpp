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
# include <SystemUtil.h>
# include <FSCommand.h>
# include <DMM.h>
# include <KBDriver.h>
# include <RTC.h>
# include <dtime.h>
# include <stdio.h>
# include <mdate.h>
# include <DiskCache.h>

void SystemUtil_Reboot()
{
	DriveInfo* pDriveInfo = NULL ;

	while((pDriveInfo = DeviceDrive_GetNextDrive(pDriveInfo)) != NULL)
	{
		if(pDriveInfo->drive.bMounted)
		{
			printf("\n UnMounting Drive: %-20s", pDriveInfo->drive.driveName) ;
			
			byte bStatus = FSCommand_Mounter(pDriveInfo, FS_UNMOUNT) ;
			
			DiskCache_StopReleaseCacheTask(pDriveInfo) ;
			DiskCache_FlushDirtyCacheSectors(pDriveInfo) ;

			if(bStatus != FSCommand_SUCCESS)
				printf("\n Failed to UnMount Drive\n") ;
			else
				printf("[ Done ]") ;
		}
	}

	ProcessManager_Sleep(2000) ;
	KBDriver_Reboot() ;
}

byte SystemUtil_GetTimeOfDay(struct timeval* tv)
{
	RTCDateTime rtcTime ;
	RTC::GetDateTime(rtcTime) ;

	mdate dt;
	mdate_Set(&dt, rtcTime.bDayOfMonth, rtcTime.bMonth, rtcTime.bCentury * 100 + rtcTime.bYear) ;
	
	int days ;	
	if(!mdate_SeedDateDifference(&dt, &days))
		return FAILURE ;

	tv->tSec = days * 24 * 60 * 60 + rtcTime.bHour * 60 * 60 + rtcTime.bMinute * 60 + rtcTime.bSecond ;
	//tv->uimSec = tv->tSec * 1000 ;
	return SUCCESS ;
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

