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
# include <GenericUtil.h>
# include <Display.h>
# include <Keyboard.h>
# include <Display.h>
# include <MemConstants.h>
# include <ProcessEnv.h>
# include <StringUtil.h>
# include <ProcFileManager.h>
# include <FileOperations.h>
# include <DMM.h>


class StrPathTokenizer : public StringTokenizer
{
	public:
		StrPathTokenizer(const char* szFileName, char* szFullFilePath) : 
			m_szFileName(szFileName), 
			m_szFullFilePath(szFullFilePath), 
			m_bFound(false) 
	{}
	
	void operator()(int index, const char* src, int len)
	{
		if(!m_bFound)
		{
			byte szTemp[128] ;

      memcpy(szTemp, src, len);
			szTemp[len] = '\0' ;
			
			if(szTemp[len - 1] != '/')
				strcat((char*)szTemp, "/") ;
			
			strcat((char*)szTemp, m_szFileName) ;

			if(FileOperations_Exists((const char*)szTemp, ATTR_TYPE_FILE) == FileOperations_SUCCESS)
			{
				m_bFound = true ;

        memcpy(m_szFullFilePath, src, len);
				m_szFullFilePath[len] = '\0' ;
				
				if(m_szFullFilePath[len - 1] != '/')
					strcat(m_szFullFilePath, "/") ;
			}
		}
	}

	bool IsFound() { return m_bFound ; }

	private:
	const char* m_szFileName ;
	char* m_szFullFilePath ;
	bool m_bFound ;
} ;

void GenericUtil_ReadInput(char* szInputBuffer, const int iMaxReadLength, byte bEcho)
{
	int iCurrentReadPos = 0 ;
	byte ch ;

	while(SUCCESS)
	{
		ch = Keyboard_GetKeyInBlockMode() ;

		switch(ch)
		{
			case Keyboard_LEFT_ALT:
			case Keyboard_LEFT_CTRL:
				break ;
			case Keyboard_F1:
			case Keyboard_F2:
			case Keyboard_F3:
			case Keyboard_F4:
			case Keyboard_F5:
			case Keyboard_F6:
			case Keyboard_F7:
			case Keyboard_F8:
//				SessionManager_SwitchToSession(SessionManager_KeyToSessionIDMap(ch)) ;
				break ;
			case Keyboard_F9:
			case Keyboard_F10:
				break ;
				
			case Keyboard_CAPS_LOCK:
				break ;
			case Keyboard_BACKSPACE:
				if(iCurrentReadPos > 0)
				{
					if(bEcho)
					{
						KC::MDisplay().MoveCursor(-1) ;
						KC::MDisplay().ClearLine(Display::START_CURSOR_POS) ;
					}
					iCurrentReadPos-- ;
				}
				break ;
				
			case Keyboard_LEFT_SHIFT:
			case Keyboard_RIGHT_SHIFT:
				break ;
				
			case Keyboard_ESC:
				break ;

			case Keyboard_ENTER:
				szInputBuffer[iCurrentReadPos] = '\0' ;
				return ;

			default:

				if(iCurrentReadPos != iMaxReadLength)
				{
					szInputBuffer[iCurrentReadPos++] = ch ;
					if(bEcho)
						KC::MDisplay().Character(ch, Display::WHITE_ON_BLACK()) ;
				}
		}
	}
}

unsigned GenericUtil_Power(unsigned uiNum, unsigned uiPow)
{
	unsigned uiRes = 1 ;
	unsigned i ;

	for(i = 0; i < uiPow; i++)
		uiRes *= uiNum ;

	return uiRes ;
}

byte GenericUtil_GetFullFilePathFromEnv(const char* szPathEnvVar, const char* szPathEnvDefVal, const char* szFileName, char* szFullFilePath)
{
	char* s = ProcessEnv_Get(szPathEnvVar) ;
	char szEnvValue[256] ;
	
	if(s == NULL)
	{
		//Deafult Env Path
		strcpy(szEnvValue, szPathEnvDefVal) ;
	}
	else
	{
		//Deafult Env Path
		strcpy(szEnvValue, s) ;
		strcat(szEnvValue, szPathEnvDefVal) ;
	}

	// If no path env, then its from root "/"
	if(String_Length(szEnvValue) == 0)
	{
		strcpy(szFullFilePath, "/") ;
		return true ;
	}

	int iListSize ;
	StrPathTokenizer tokenizer(szFileName, szFullFilePath) ;
	String_Tokenize(szEnvValue, ':', &iListSize, tokenizer) ;

	return (byte)tokenizer.IsFound() ;
}

