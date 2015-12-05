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
# include <CommandLineParser.h>
# include <cstring.h>
# include <ctype.h>
# include <stdlib.h>

static char CommandLineParser_szCommandLineEntries[MAX_COMMAND_LINE_ENTRIES][COMMAND_LINE_SIZE] ;
static char CommandLineParser_szParameters[MAX_COMMAND_LINE_ENTRIES][COMMAND_LINE_SIZE] ;
static char CommandLineParser_szOptions[MAX_COMMAND_LINE_ENTRIES][COMMAND_LINE_SIZE] ;
static char* CommandLineParser_ArgV[MAX_COMMAND_LINE_ENTRIES] ;
static int CommandLineParser_iNoOfCommandLineEntries ;
static int CommandLineParser_iNoOfParamters ;
static int CommandLineParser_iNoOfOptions ;


bool CommandLineParser_TokenCompare(char ch)
{
	return iswhitespace(ch) ;
}

bool CommandLineParser_GroupToken(char ch)
{
	return (ch == '"') ;
}

void CommandLineParser_Copy(int index, const char* src, int len)
{
	memcpy(CommandLineParser_szCommandLineEntries[index], src, len) ;
	CommandLineParser_szCommandLineEntries[index][len] = '\0' ;
}

void CommandLineParser_ExpandCommandLine()
{
	int i ;
	for(i = 0; i < CommandLineParser_iNoOfCommandLineEntries; i++)
	{
		char* par = CommandLineParser_szCommandLineEntries[i] ;

		if(strlen(par) > 1)
		{
			if(par[0] == '$' && !iswhitespace(par[1]))
			{
				char temp[COMMAND_LINE_SIZE] ;
				
				int j ;
				for(j = 0; par[j] != '\0' && !iswhitespace(par[j]) && par[j] != '/'; j++) ;
				
				strcpy(temp, par + j) ;

				par[j] = '\0' ;
				char* val = getenv(par + 1) ;

				if(val == NULL)
					strcpy(par, "") ;	
				else
					strcpy(par, val) ;

				strcat(par, temp) ;
			}
		}
	}
}

void CommandLineParser_LoadParameters()
{
	CommandLineParser_iNoOfParamters = 0 ;

	int i ;
	for(i = 1; i < CommandLineParser_iNoOfCommandLineEntries; i++)
	{
		if(CommandLineParser_szCommandLineEntries[i][0] != '-')
			strcpy(CommandLineParser_szParameters[ CommandLineParser_iNoOfParamters++ ], CommandLineParser_szCommandLineEntries[i]) ;
	}
}

void CommandLineParser_LoadOptions()
{
	CommandLineParser_iNoOfOptions = 0 ;

	int i ;
	for(i = 1; i < CommandLineParser_iNoOfCommandLineEntries; i++)
	{
		if(CommandLineParser_szCommandLineEntries[i][0] == '-')
			strcpy(CommandLineParser_szOptions[ CommandLineParser_iNoOfOptions++ ], CommandLineParser_szCommandLineEntries[i]) ;
	}
}

void CommandLineParser_Parse(const char* szInputCommandLine)
{
	CommandLineParser_iNoOfCommandLineEntries =  0 ;
	CommandLineParser_iNoOfParamters =  0 ;
	CommandLineParser_iNoOfOptions =  0 ;

	strtok_c(szInputCommandLine, 
			&CommandLineParser_TokenCompare, 
			&CommandLineParser_GroupToken,
			&CommandLineParser_Copy, 
			&CommandLineParser_iNoOfCommandLineEntries) ;

	CommandLineParser_ExpandCommandLine() ;

	CommandLineParser_LoadParameters() ;
	CommandLineParser_LoadOptions() ;
}

char* CommandLineParser_GetCommand()
{
	if(CommandLineParser_iNoOfCommandLineEntries == 0)
		return NULL ;

	return CommandLineParser_szCommandLineEntries[0] ;
}

int CommandLineParser_GetNoOfCommandLineEntries()
{
	return CommandLineParser_iNoOfCommandLineEntries ;
}

int CommandLineParser_GetNoOfParameters()
{
	return CommandLineParser_iNoOfParamters ;
}

int CommandLineParser_GetNoOfOptions()
{
	return CommandLineParser_iNoOfOptions ;
}

char* CommandLineParser_GetParameterAt(int iPos)
{
	if(iPos < 0)
		return NULL ;

	if(iPos >= CommandLineParser_GetNoOfParameters())
		return NULL ;

	return CommandLineParser_szParameters[iPos] ;
}

char** CommandLineParser_GetArgV()
{
	int i ;
	for(i = 0; i < CommandLineParser_iNoOfCommandLineEntries; i++)
		CommandLineParser_ArgV[i] = CommandLineParser_szCommandLineEntries[i] ;

	return CommandLineParser_ArgV ;
}

bool CommandLineParser_IsOptPresent(char* opt)
{
	int i ;
	for(i = 0; i < CommandLineParser_iNoOfOptions; i++)
	{
		if(strcmp(CommandLineParser_szOptions[i], opt) == 0)
			return true ;
	}

	return false ;
}
