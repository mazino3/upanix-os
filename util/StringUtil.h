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
#ifndef _STRING_UTIL_H_
#define _STRING_UTIL_H_

# include <Global.h>
# include <string.h>

void String_RawCopy(byte* dest, const byte* src, const unsigned uiLen) ;

class StringTokenizer
{
	public:
		virtual void operator()(int index, const char* src, int len) = 0 ;
} ;

//TODO: Has to be made Dynamic
class StringDefTokenizer : public StringTokenizer
{
	public:
		void operator()(int index, const char* src, int len)
		{
			String_RawCopy((byte*)(szToken[index]), (const byte*)src, len) ;
			szToken[index][len] = '\0' ;
		}

		char szToken[10][33] ;
} ;

unsigned String_Copy(char* dest, const char* src) ;
unsigned String_CanCat(char* dest, const char* src) ;
short String_Compare(const char* s1, const char* s2) ;
int String_Length(const char* str) ;
void String_Tokenize(const char* src, char chToken, int* iListSize, StringTokenizer& strTkCopy) ;
void String_ConvertNumberToString(char* strNumber, unsigned uiNumber) ;
byte String_ConvertStringToNumber(unsigned* uiNumber, char* strNumber) ;
short String_NCompare(const char* s1, const char* s2, unsigned len) ;
byte String_IsSubStr(const char* szMainString, const char* szSubString) ;
void String_Reverse(char* str) ;
int String_Chr(const char* szStr, char ch) ;

#endif
