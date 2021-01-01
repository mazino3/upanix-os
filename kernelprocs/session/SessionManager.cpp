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
# include <SessionManager.h>
# include <Display.h>
# include <StringUtil.h>
# include <FileSystem.h>
# include <Directory.h>
# include <DMM.h>
# include <ATADrive.h>
# include <MemUtil.h>
# include <UserManager.h>
# include <GenericUtil.h>
# include <KeyboardHandler.h>
# include <ProcessGroup.h>
# include <KernelService.h>

#define MAX_NO_OF_SESSIONS 8

/************************** static functions *************************************/

static int SessionManager_List[MAX_NO_OF_SESSIONS] ;

static bool SessionManager_ValidateUser(const upan::string& name, const upan::string& password, const User& user)
{
  return name == user.Name() && password == user.Password();
}

static byte SessionManager_LoadShell(int* iShellProcessID)
{
	KC::MDisplay().Message("\n Loading Shell...", Display::WHITE_ON_BLACK()) ;

	char shell[33] ;
	strcpy(shell, BIN_PATH) ;
	strcat(shell, "msh") ;

	*iShellProcessID = KC::MKernelService().RequestProcessExec(shell, 0, NULL) ;
	if(*iShellProcessID < 0)
	{
		KC::MDisplay().Message(" Failed !!!", Display::WHITE_ON_BLACK()) ;
		return false ;
	}

	KC::MDisplay().Message(" Done.", Display::WHITE_ON_BLACK()) ;
	return true ;
}	

/***********************************************************************************/

void SessionManager_Initialize()
{
	unsigned i ;
	for(i = 0; i < MAX_NO_OF_SESSIONS; i++)
		SessionManager_List[i] = NO_PROCESS_ID ;
}

void SessionManager_StartSession()
{
	char szUserName[MAX_USER_LENGTH + 1] ;
	char szPassword[MAX_USER_LENGTH + 1] ;

	while(true)
	{
		KC::MDisplay().ClearScreen() ;

		KC::MDisplay().Message("\n  *********************  Welcome to Upanix V.3.0  *********************", Display::WHITE_ON_BLACK()) ;

		UserManager::Instance();

__OnlyLogin:
		KC::MDisplay().Message("\n\n Login: ", Display::WHITE_ON_BLACK()) ;
		GenericUtil_ReadInput(szUserName, MAX_USER_LENGTH, true) ;

		KC::MDisplay().Message("\n Password: ", Display::WHITE_ON_BLACK()) ;
		GenericUtil_ReadInput(szPassword, MAX_USER_LENGTH, false) ;

    const User* sessionUser = UserManager::Instance().GetUserEntryByName(szUserName);
    if(sessionUser == nullptr)
		{
      printf("\n\n Invalid User!");
			goto __OnlyLogin ;
		}

		if(!SessionManager_ValidateUser(szUserName, szPassword, *sessionUser))
		{
      printf("\n\n Login Incorrect");
			goto __OnlyLogin ;
		}
		
    printf("\n\n Login Successfull");
	
		int iShellProcessID ;
		if(!SessionManager_LoadShell(&iShellProcessID))
			goto __OnlyLogin ;

		ProcessManager::Instance().WaitOnChild(iShellProcessID) ;
	}	
}

int SessionManager_KeyToSessionIDMap(int key)
{
	return key - Keyboard_F1 ;
}

void SessionManager_SetSessionIDMap(int key, int pid)
{
	if(key >= MAX_NO_OF_SESSIONS)
		return ;

	SessionManager_List[key] = pid ;
}

void SessionManager_SwitchToSession(int key)
{
	if(SessionManager_List[key] == NO_PROCESS_ID)
	{
		int pid ;
    ProcessManager::Instance().CreateKernelProcess("session", (unsigned) &SessionManager_StartSession, NO_PROCESS_ID,
                                                   true, NULL, NULL, &pid) ;
		SessionManager_List[key] = pid ;
		return ;
	}

  ProcessManager::Instance().GetAddressSpace(SessionManager_List[key]).value().processGroup()->SwitchToFG();
	KC::MDisplay().RefreshScreen() ;
}

