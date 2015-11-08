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
#ifndef _LIB_MDATE_H_
#define _LIB_MDATE_H_

#include <ctype.h>

/* Day of Month is the only one which is 1 based index */
typedef struct
{
	int dayOfMonth ;
	int month ;
	int year ;

	int dayOfWeek ;
} mdate ;

//Days
#define DAYS_PER_NLYEAR 365
#define DAYS_PER_LYEAR 366

//Time
#define SEC_PER_MIN 60
#define MIN_PER_HOUR 60
#define SEC_PER_HOUR 3600
#define SEC_PER_DAY 86400
#define HOURS_PER_DAY 24

typedef enum
{
	JANUARY = 1,
	FEBRUARY,
	MARCH,
	APRIL,
	MAY,
	JUNE,
	JULY,
	AUGUST,
	SEPTEMBER,
	OCTOBER,
	NOVEMBER,
	DECEMBER,
	MONTHS_PER_YEAR = DECEMBER,
	MONTHS_PER_YEAR_SIZE
} MONTHS ;

typedef enum
{
	MONDAY,
	TUESDAY,
	WEDNESDAY,
	THURSDAY,
	FRIDAY,
	SATURDAY,
	SUNDAY,
	DAYS_PER_WEEK
} DAYS ;

#define IS_LEAP_YEAR(year) ( (year % 4) == 0 )

bool mdate_Set(mdate* date, int dayOfMonth, int month, int year) ;
void mdate_Copy(mdate* d1, const mdate* d2) ;
void mdate_GetSeedDate(mdate* d1) ;
void mdate_Normalize(const mdate* src, mdate* dest) ;
int mdate_GetDaysInMonth(const mdate* date) ;
int mdate_GetDaysInMonth_1(int month, int year) ;
bool mdate_IsValidDayOfWeek(int dayOfWeek) ;
bool mdate_IsValidDayOfMonth(const mdate* date) ;
bool mdate_IsValidMonth(int month) ;
bool mdate_IsValidYear(int year) ;
bool mdate_IsValid(const mdate* date) ;

bool mdate_AddDays(mdate* date, int days) ;
bool mdate_AddWeeks(mdate* date, int days) ;
bool mdate_AddMonths(mdate* date, int months) ;
bool mdate_AddYears(mdate* date, int years) ;

bool mdate_SubtractDays(mdate* date, int days) ;
bool mdate_SubtractWeeks(mdate* date, int days) ;
bool mdate_SubtractMonths(mdate* date, int months) ;
bool mdate_SubtractYears(mdate* date, int years) ;

bool mdate_IsGreater(const mdate* a, const mdate* b) ;
bool mdate_IsLesser(const mdate* a, const mdate* b) ;
bool mdate_IsEqual(const mdate* a, const mdate* b) ;
bool mdate_IsNotEqual(const mdate* a, const mdate* b) ;

int mdate_SameYearDateDifference(const mdate* a, const mdate* b) ;
int mdate_DateDifference(const mdate* a, const mdate* b) ;
bool mdate_SeedDateDifference(const mdate* dt, int* days) ;

bool mdate_SetDayOfWeek(mdate* date) ;

const char* mdate_GetMonthName(const mdate* date) ;
const char* mdate_GetDayName(const mdate* date) ;

#endif
