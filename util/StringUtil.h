/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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

#include <Global.h>
#include <string.h>
#include <ustring.h>
#include <vector.h>

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
      memcpy(szToken[index], src, len);
			szToken[index][len] = '\0' ;
		}

		char szToken[10][33] ;
} ;

void String_Tokenize(const char* src, char chToken, int* iListSize, StringTokenizer& strTkCopy) ;
int String_Chr(const char* szStr, char ch) ;

#endif
