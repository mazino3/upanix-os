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
#include <KeyboardHandler.h>
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
#include <string.h>

void Console_StartUpanixConsole()
{
  Console::Instance().Start();
}

Console::Console() : _currentCommandPos(0)
{
  _commandLine = new char[COMMAND_LINE_SIZE];
	ConsoleCommands_Init() ;
	KC::MDisplay().LoadMessage("Console Initialization", Success);
}

void Console::ClearCommandLine()
{
  memset(_commandLine, 0, COMMAND_LINE_SIZE) ;
}

void Console::DisplayCommandLine()
{
	char* szPWD ;
	Directory_PresentWorkingDirectory( &ProcessManager::Instance().GetCurrentPAS(), &szPWD) ;
  printf("\nupanix:%s > ", szPWD);
	DMM_DeAllocateForKernel((unsigned)szPWD) ;
}

void Console::Start()
{
  KC::MDisplay().RefreshScreen() ;
	
	debug_point = 0 ;

//	byte bStatus ;
//	if((bStatus = FileOperations_ChangeDir(FS_ROOT_DIR)) != FileOperations_SUCCESS)
//	{
//		KC::MDisplay().Address("\n Directory Change Failed: ", bStatus) ;	
//	}

  DisplayCommandLine() ;

	//Default init code
/*	Console_ProcessCommand("eusbprobe");
	Console_ProcessCommand("mount usda");
	Console_ProcessCommand("chd usda");
	Console_ProcessCommand("test");
*/
	while(SUCCESS)
	{
    int ch = KeyboardHandler::Instance().GetCharInBlockMode();

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
        if(_currentCommandPos > 0)
				{
          _currentCommandPos-- ;
          int x = _commandLine[_currentCommandPos] == '\t' ? 4 : 1 ;
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
        ProcessCommand() ;
        DisplayCommandLine() ;
				break ;

			default:

        if(_currentCommandPos != COMMAND_LINE_SIZE)
				{
					KC::MDisplay().Character(ch, Display::WHITE_ON_BLACK()) ;
          _commandLine[_currentCommandPos++] = ch ;
				}
		}
	}
}

void Console::ProcessCommand()
{
  _commandLine[_currentCommandPos] = '\0' ;
  _currentCommandPos = 0 ;
  ExecuteCommand(_commandLine);
  ClearCommandLine() ;
}

void Console::ExecuteCommand(const char* szCommandLine)
{
  CommandLineParser::Instance().Parse(szCommandLine);
  auto command = CommandLineParser::Instance().GetCommand();
  if(command)
    ConsoleCommands_ExecuteInternalCommand(command);
}

