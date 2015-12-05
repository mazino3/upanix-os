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
#include <ctype.h>
#include <cstring.h>

bool iswhitespace(char ch)
{
	return __C_isspace(ch) ;
//	return (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') ;
}

bool isdigit(char ch)
{
	return __C_isdigit(ch) ;
//	return (ch >= '0' && ch <= '9') ;
}

bool isspace(char ch)
{
	return __C_isspace(ch) ;
//	return iswhitespace(ch) ;
}

bool isxdigit(char ch)
{
	return __C_isxdigit(ch) ;
//	return isdigit(ch) || (((((unsigned char)ch) | 0x20) - 'a') < 6) ;
}

bool isalpha(char ch)
{
	return __C_isalpha(ch) ;
//	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ;
}

bool isprint(char ch)
{
	return __C_isprint(ch) ;
}

bool isalnum(char ch)
{
	return __C_isalnum(ch) ;
}

bool isblank(char ch)
{
	return __C_isblank(ch) ;
}

bool isgraph(char ch)
{
	return __C_isgraph(ch) ;
}

bool iscntrl(char ch)
{
	return __C_iscntrl(ch) ;
}

//void memset(char* dest, char ch, int len)
//{
//	int i ;
//	for(i = 0; i < len; i++)
//		dest[i] = ch ;
//}
//
//void* memcpy (void *__restrict __dest, __const void *__restrict __src, size_t __n)
//{
//	int i ;
//	for(i = 0; i < __n; i++)
//		__dest[i] = __src[i] ;
//
//	return __dest ;
//}

int abs(int num)
{
	if(num < 0)
		return (num * -1) ;
	return num ;
}

int toascii(int c)
{
	return __toascii(c) ; 
}

int tolower(int c)
{
    return __C_tolower(c);
}

int toupper(int c)
{
    return __C_toupper(c);
}
