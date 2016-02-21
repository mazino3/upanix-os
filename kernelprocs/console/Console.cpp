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
#include <Console.h>
#include <Keyboard.h>
#include <Display.h>
#include <StringUtil.h>
#include <Directory.h>
#include <CommandLineParser.h>
#include <DMM.h>
#include <ConsoleCommands.h>
#include <FileOperations.h>
#include <MemUtil.h>
#include <PIT.h>
#include <SessionManager.h>

#include <stdio.h>
#include <cstring.h>

char Console_PROMPT[10] ;
char Console_commandLine[COMMAND_LINE_SIZE] ;
unsigned int Console_currentCommandPos ;

void Console_Initialize()
{
	strcpy(Console_PROMPT, "\nupanix:") ;
	Console_currentCommandPos = 0 ;
	ConsoleCommands_Init() ;
	KC::MDisplay().LoadMessage("Console Initialization", Success);
}

void Console_ClearCommandLine()
{
	memset(Console_commandLine, 0, COMMAND_LINE_SIZE) ;
}

void Console_DisplayCommandLine()
{
	char* szPWD ;
	Directory_PresentWorkingDirectory( &ProcessManager::Instance().GetCurrentPAS(), &szPWD) ;

	KC::MDisplay().Message(Console_PROMPT, Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message(szPWD, Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message(" > ", Display::WHITE_ON_BLACK()) ;

	DMM_DeAllocateForKernel((unsigned)szPWD) ;
}

void Console_StartUpanixConsole()
{
  KC::MDisplay().RefreshScreen() ;
	
	debug_point = 0 ;

	int ch ;

//	byte bStatus ;
//	if((bStatus = FileOperations_ChangeDir(FS_ROOT_DIR)) != FileOperations_SUCCESS)
//	{
//		KC::MDisplay().Address("\n Directory Change Failed: ", bStatus) ;	
//	}

	Console_DisplayCommandLine() ;

	//Default init code
/*	Console_ProcessCommand("eusbprobe");
	Console_ProcessCommand("mount usda");
	Console_ProcessCommand("chd usda");
	Console_ProcessCommand("test");
*/
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
				SessionManager_SwitchToSession(SessionManager_KeyToSessionIDMap(ch)) ;
				break ;
			case Keyboard_F9:
			case Keyboard_F10:
				break ;
				
			case Keyboard_CAPS_LOCK:
				break ;
			case Keyboard_BACKSPACE:
				if(Console_currentCommandPos > 0)
				{
					Console_currentCommandPos-- ;
					int x = Console_commandLine[Console_currentCommandPos] == '\t' ? 4 : 1 ;
					KC::MDisplay().MoveCursor(-x) ;
					KC::MDisplay().ClearLine(Display::START_CURSOR_POS) ;
				}
				break ;
				
			case Keyboard_LEFT_SHIFT:
			case Keyboard_RIGHT_SHIFT:
				break ;
				
			case Keyboard_ESC:
				break ;

			case Keyboard_ENTER:
				Console_ProcessCommand() ;
				Console_DisplayCommandLine() ;
				break ;

			default:

				if(Console_currentCommandPos != COMMAND_LINE_SIZE)
				{
					KC::MDisplay().Character(ch, Display::WHITE_ON_BLACK()) ;
					Console_commandLine[Console_currentCommandPos++] = ch ;
				}
		}
	}
}

void Console_ProcessCommand(const char* cmd)
{
	if(!Console_ExecuteCommand(cmd))
		puts("\n No Such Command\n") ;
}

void Console_ProcessCommand()
{
	Console_commandLine[Console_currentCommandPos] = '\0' ;
	Console_currentCommandPos = 0 ;
	Console_ProcessCommand(Console_commandLine);
	Console_ClearCommandLine() ;
}

bool Console_ExecuteCommand(const char* szCommandLine)
{
	CommandLineParser_Parse(szCommandLine) ;

	if(CommandLineParser_GetNoOfCommandLineEntries() != 0)
	{
		return ConsoleCommands_ExecuteInternalCommand(CommandLineParser_GetCommand()) ;
	}

	return true ;
}

