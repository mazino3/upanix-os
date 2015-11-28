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
#ifndef _USER_MANAGER_H_
#define _USER_MANAGER_H_

# include <Global.h>

#define UserManager_SUCCESS						0
#define UserManager_ERR_INVALID_USR_FILE		1
#define UserManager_ERR_INVALID_USR_TAB_HEADER	2
#define UserManager_ERR_INVALID_NAME			3
#define UserManager_ERR_TOO_LONG				4
#define UserManager_ERR_INVALID_USER_TYPE		5
#define UserManager_ERR_MAX_USR					6
#define UserManager_ERR_INVALID_HOME_DIR		7
#define UserManager_ERR_USER					8
#define UserManager_ERR_TOO_SHORT				9
#define UserManager_ERR_EXISTS					10
#define UserManager_FAILURE						11

#define MAX_USER_LENGTH				32
#define MIN_USER_LENGTH				6
#define MAX_HOME_DIR_LEN			64

#define ROOT_USER_ID			0
#define DERIVE_FROM_PARENT		(-1)

typedef enum
{	
	NO_USER,
	SUPER_USER,
	NORMAL_USER
} USER_TYPES;

typedef struct
{
	int		iNoOfUsers ;
	int		iNextUser ;
} PACKED UserTabHeader ;

typedef struct
{
	char	szUserName[MAX_USER_LENGTH + 1] ;
	char	szPassword[MAX_USER_LENGTH + 1] ;
	char	szHomeDirPath[MAX_HOME_DIR_LEN + 1] ;
	byte	bType ;
} PACKED UserTabEntry ;

byte UserManager_Initialize() ;
byte UserManager_GetUserList(UserTabEntry** pUserTabList, int* iNoOfUsers) ;
byte UserManager_Create(const char* szUserName, const char* szPassword, const char* szHomeDirPath, byte bType) ;
byte UserManager_Delete(const char* szUserName) ;
int UserManager_GetUserEntryByName(const char* szUserName, UserTabEntry** pUserTabEntry) ;

#endif
