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
# include <UserManager.h>
# include <FileOperations.h>
# include <MountManager.h>
# include <MemConstants.h>
# include <Display.h>
# include <ProcFileManager.h>
# include <Directory.h>
# include <StringUtil.h>
# include <DMM.h>
# include <GenericUtil.h>
# include <MemUtil.h>

#define __USER_LIST_FILE	".user.lst"
#define USER_TAB_SIZE	(MEM_USR_LIST_END - MEM_USR_LIST_START)

static UserTabHeader* UserManager_pUsrTabHeader ;
static UserTabEntry* UserManager_UseTabList ;
static int SYS_MAX_USERS ;
static char USER_LIST_FILE[30] ;

/***************** static functions ***********************************/
static byte UserManager_LoadUserTableList() ;
static byte UserManager_ValidateUserTableList() ;
static void UserManager_InitializeDefaultUserTable() ;
static byte UserManager_WriteUserTableList() ;
static byte UserManager_CreateNewUserList() ;
static byte UserManager_ValidateName(const char* szName) ;
static int UserManager_GetUserIDByName(const char* szUserName) ;

static byte UserManager_LoadUserTableList()
{
	int fd ;

	RETURN_X_IF_NOT(FileOperations_Open(&fd, USER_LIST_FILE, O_RDONLY), FileOperations_SUCCESS, UserManager_FAILURE) ;

	unsigned n ;
	const unsigned len = USER_TAB_SIZE ;

	byte bStatus = FileOperations_Read(fd, (char*)(MEM_USR_LIST_START), len, &n) ;

	RETURN_X_IF_NOT(FileOperations_Close(fd), FileOperations_SUCCESS, UserManager_FAILURE) ;

	if(bStatus != Directory_SUCCESS && bStatus != Directory_ERR_EOF)
		return UserManager_FAILURE ;

	RETURN_IF_NOT(bStatus, UserManager_ValidateUserTableList(), UserManager_SUCCESS) ;

	return UserManager_SUCCESS ;
}

static byte UserManager_ValidateUserTableList()
{
	if(UserManager_pUsrTabHeader->iNoOfUsers < 0 || UserManager_pUsrTabHeader->iNoOfUsers >= SYS_MAX_USERS || UserManager_pUsrTabHeader->iNextUser < -1 || UserManager_pUsrTabHeader->iNextUser >= SYS_MAX_USERS) 
		return UserManager_ERR_INVALID_USR_TAB_HEADER ;

	unsigned i, uiUserCount = 0 ;
		
	for(i = 0; i < SYS_MAX_USERS; i++)
	{
		if(((int*)&UserManager_UseTabList[i])[0] == -1 || UserManager_UseTabList[i].bType == NO_USER)
			continue ;

		uiUserCount++ ;
	}

	if(uiUserCount != UserManager_pUsrTabHeader->iNoOfUsers)
		return UserManager_ERR_INVALID_USR_FILE ;

	return UserManager_SUCCESS ;
}

static void UserManager_InitializeDefaultUserTable()
{
	UserManager_pUsrTabHeader->iNoOfUsers = 1 ;
	UserManager_pUsrTabHeader->iNextUser = -1 ;

	int i ;
	for(i = 0; i < SYS_MAX_USERS; i++)
		((int*)&UserManager_UseTabList[i])[0] = -1 ;

	String_Copy(UserManager_UseTabList[0].szUserName, "root") ;
	String_Copy(UserManager_UseTabList[0].szPassword, "root123") ;
	String_Copy(UserManager_UseTabList[0].szHomeDirPath, FS_ROOT_DIR) ;
	UserManager_UseTabList[0].bType = SUPER_USER ;
}

static byte UserManager_WriteUserTableList()
{
	int fd ;
	int n ;
	RETURN_X_IF_NOT(FileOperations_Open(&fd, USER_LIST_FILE, O_RDWR), FileOperations_SUCCESS, UserManager_FAILURE) ;

	RETURN_X_IF_NOT(FileOperations_Write(fd, (const char*)(MEM_USR_LIST_START), USER_TAB_SIZE, &n), FileOperations_SUCCESS, UserManager_FAILURE) ;

	RETURN_X_IF_NOT(FileOperations_Close(fd), FileOperations_SUCCESS, UserManager_FAILURE) ;

	return UserManager_SUCCESS ;	
}

static byte UserManager_CreateNewUserList()
{
	unsigned short usPerm = S_OWNER((ATTR_READ | ATTR_WRITE)) | S_GROUP(ATTR_READ) | S_OTHERS(ATTR_READ) ;

	RETURN_X_IF_NOT(FileOperations_Create(USER_LIST_FILE, ATTR_TYPE_FILE, usPerm), FileOperations_SUCCESS, UserManager_FAILURE) ;

	UserManager_InitializeDefaultUserTable() ;
	
	byte bStatus ;
	RETURN_IF_NOT(bStatus, UserManager_WriteUserTableList(), UserManager_SUCCESS) ;

	return UserManager_SUCCESS ;
}

static UserTabEntry* UserManager_GetFreeEntry()
{	
	if(!(UserManager_pUsrTabHeader->iNoOfUsers > 0) || UserManager_pUsrTabHeader->iNoOfUsers >= SYS_MAX_USERS)
		return NULL ;

	if(UserManager_pUsrTabHeader->iNextUser == -1)
		return &UserManager_UseTabList[ UserManager_pUsrTabHeader->iNoOfUsers ] ;

	int i = UserManager_pUsrTabHeader->iNextUser ;
	UserManager_pUsrTabHeader->iNextUser = ((int*)(&UserManager_UseTabList[i]))[0] ;

	return &UserManager_UseTabList[i] ;
}

static byte UserManager_ValidateName(const char* szName)
{
	if(UserManager_GetUserIDByName(szName) != -1)
		return UserManager_ERR_EXISTS ;

	int len = String_Length(szName) ;

	if(len > MAX_USER_LENGTH)
		return UserManager_ERR_TOO_LONG ;

	if(len < MIN_USER_LENGTH)
		return UserManager_ERR_TOO_SHORT ;

	int i ;
	for(i = 0; i < len; i++)
	{
		if(!(GenericUtil_IsDigit(szName[i]) || GenericUtil_IsAlpha(szName[i])))
			return UserManager_ERR_INVALID_NAME ;
	}

	return UserManager_SUCCESS ;
}

static int UserManager_GetUserIDByName(const char* szUserName)
{
	int i ;
	for(i = 0; i < UserManager_pUsrTabHeader->iNoOfUsers; i++)
	{
		if(String_Compare(UserManager_UseTabList[i].szUserName, szUserName) == 0)
			return i ;
	}

	return -1 ;
}

/**********************************************************************/

byte UserManager_Initialize()
{
	SYS_MAX_USERS = (USER_TAB_SIZE - sizeof(UserTabHeader))	/ (sizeof(UserTabEntry)) ;

	UserManager_pUsrTabHeader = (UserTabHeader*)(MEM_USR_LIST_START) ;
	UserManager_UseTabList = (UserTabEntry*)(MEM_USR_LIST_START + sizeof(UserTabHeader)) ;

	String_Copy(USER_LIST_FILE, OSIN_PATH) ;
	String_CanCat(USER_LIST_FILE, __USER_LIST_FILE) ;

	byte bStatus ;
	if(FileOperations_Exists(USER_LIST_FILE, ATTR_TYPE_FILE) != FileOperations_SUCCESS)
	{
		RETURN_IF_NOT(bStatus, UserManager_CreateNewUserList(), UserManager_SUCCESS) ;
		return UserManager_SUCCESS ;
	}

	if(UserManager_LoadUserTableList() != UserManager_SUCCESS)
	{	
		KC::MDisplay().Message("\n User List File Corrupted/Not Found. Using Default root User\n", Display::WHITE_ON_BLACK()) ;
		UserManager_InitializeDefaultUserTable() ;
	}
	
	return UserManager_SUCCESS ;	
}

byte UserManager_GetUserList(UserTabEntry** pUserTabList, int* iNoOfUsers)
{
	int pid = ProcessManager_iCurrentProcessID ;
	ProcessAddressSpace* pPAS = &ProcessManager_processAddressSpace[pid] ;
	UserTabEntry* pAddress = NULL ;

	*iNoOfUsers = UserManager_pUsrTabHeader->iNoOfUsers ;
	if(pPAS->bIsKernelProcess == true)
	{
		*pUserTabList = (UserTabEntry*)DMM_AllocateForKernel(sizeof(UserTabEntry) * (*iNoOfUsers)) ;
		pAddress = *pUserTabList ;
	}
	else
	{
		*pUserTabList = (UserTabEntry*)DMM_Allocate(pPAS, sizeof(UserTabEntry) * (*iNoOfUsers)) ;
		pAddress = (UserTabEntry*)(((unsigned)*pUserTabList + PROCESS_BASE) - GLOBAL_DATA_SEGMENT_BASE) ;
	}

	if(pAddress == NULL)
		return UserManager_FAILURE ;

	unsigned i, j ;
	for(i = 0, j = 0; j < *iNoOfUsers; i++)
	{
		if(((int*)&UserManager_UseTabList[i])[0] == -1 || UserManager_UseTabList[i].bType == NO_USER)
			continue ;

		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&UserManager_UseTabList[i], MemUtil_GetDS(), (unsigned)&pAddress[j++], sizeof(UserTabEntry)) ;
	}

	return UserManager_SUCCESS ;
}

byte UserManager_Create(const char* szUserName, const char* szPassword, const char* szHomeDirPath, byte bType)
{
	UserTabEntry* pUserTabEntry = UserManager_GetFreeEntry() ;

	if(pUserTabEntry == NULL)
		return UserManager_ERR_MAX_USR ;

	if(!(bType == NO_USER || bType == SUPER_USER || bType == NORMAL_USER))
		return UserManager_ERR_INVALID_USER_TYPE ;

	byte bStatus ;
	
	RETURN_IF_NOT(bStatus, UserManager_ValidateName(szUserName), UserManager_SUCCESS) ;

	RETURN_IF_NOT(bStatus, UserManager_ValidateName(szPassword), UserManager_SUCCESS) ;

	if(String_Length(szHomeDirPath) > MAX_HOME_DIR_LEN)
		return UserManager_ERR_TOO_LONG ;

	if(FileOperations_Exists(szHomeDirPath, ATTR_TYPE_DIRECTORY) != FileOperations_SUCCESS)
		return UserManager_ERR_INVALID_HOME_DIR ;

	String_Copy(pUserTabEntry->szUserName, szUserName) ;
	String_Copy(pUserTabEntry->szPassword, szPassword) ;
	String_Copy(pUserTabEntry->szHomeDirPath, szHomeDirPath) ;
	pUserTabEntry->bType = bType ;

	UserManager_pUsrTabHeader->iNoOfUsers++ ;

	RETURN_IF_NOT(bStatus, UserManager_WriteUserTableList(), UserManager_SUCCESS) ;

	return UserManager_SUCCESS ;
}

byte UserManager_Delete(const char* szUserName)
{
	int iIndex = UserManager_GetUserIDByName(szUserName) ;

	if(iIndex < 0)
		return UserManager_ERR_USER ;

	UserTabEntry* pUserTabEntry = &UserManager_UseTabList[iIndex] ;

	pUserTabEntry->bType = NO_USER ;
	((int*)pUserTabEntry)[0] = UserManager_pUsrTabHeader->iNextUser ;

	UserManager_pUsrTabHeader->iNextUser = iIndex ;
	UserManager_pUsrTabHeader->iNoOfUsers-- ;

	byte bStatus ;
	RETURN_IF_NOT(bStatus, UserManager_WriteUserTableList(), UserManager_SUCCESS) ;

	return UserManager_SUCCESS ;
}

int UserManager_GetUserEntryByName(const char* szUserName, UserTabEntry** pUserTabEntry)
{
	int i, j ;
	for(i = 0, j = 0; ; i++)
	{
		if(((int*)&UserManager_UseTabList[i])[0] == -1 || UserManager_UseTabList[i].bType == NO_USER)
			continue ;

		if(j >= UserManager_pUsrTabHeader->iNoOfUsers)
			break ;

		if(String_Compare(UserManager_UseTabList[i].szUserName, szUserName) == 0)
		{
			*pUserTabEntry = &UserManager_UseTabList[i] ;
			return i ;
		}
		j++ ;
	}

	*pUserTabEntry = NULL ;
	return -1 ;
}

