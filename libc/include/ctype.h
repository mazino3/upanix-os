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
#ifndef _TYPES_H_
#define _TYPES_H_

#define __isdigit_char(C)    (((unsigned char)((C) - '0')) <= 9)
#define __isdigit_int(C)     (((unsigned int)((C) - '0')) <= 9)

/* ASCII ords of \t, \f, \n, \r, and \v are 9, 12, 10, 13, 11 respectively. */
#define __C_isspace(c) \
	((sizeof(c) == sizeof(char)) \
	 ? ((((c) == ' ') || (((unsigned char)((c) - 9)) <= (13 - 9)))) \
	 : ((((c) == ' ') || (((unsigned int)((c) - 9)) <= (13 - 9)))))
#define __C_isblank(c) (((c) == ' ') || ((c) == '\t'))
#define __C_isdigit(c) \
	((sizeof(c) == sizeof(char)) \
	 ? (((unsigned char)((c) - '0')) < 10) \
	 : (((unsigned int)((c) - '0')) < 10))
#define __C_isxdigit(c) \
	(__C_isdigit(c) \
	 || ((sizeof(c) == sizeof(char)) \
		 ? (((unsigned char)((((c)) | 0x20) - 'a')) < 6) \
		 : (((unsigned int)((((c)) | 0x20) - 'a')) < 6)))
#define __C_iscntrl(c) \
	((sizeof(c) == sizeof(char)) \
	 ? ((((unsigned char)(c)) < 0x20) || ((c) == 0x7f)) \
	 : ((((unsigned int)(c)) < 0x20) || ((c) == 0x7f)))
#define __C_isalpha(c) \
	((sizeof(c) == sizeof(char)) \
	 ? (((unsigned char)(((c) | 0x20) - 'a')) < 26) \
	 : (((unsigned int)(((c) | 0x20) - 'a')) < 26))
#define __C_isalnum(c) (__C_isalpha(c) || __C_isdigit(c))
#define __C_isprint(c) \
	((sizeof(c) == sizeof(char)) \
	 ? (((unsigned char)((c) - 0x20)) <= (0x7e - 0x20)) \
	 : (((unsigned int)((c) - 0x20)) <= (0x7e - 0x20)))

#define __C_islower(c) \
    ((sizeof(c) == sizeof(char)) \
     ? (((unsigned char)((c) - 'a')) < 26) \
     : (((unsigned int)((c) - 'a')) < 26))
#define __C_isupper(c) \
    ((sizeof(c) == sizeof(char)) \
     ? (((unsigned char)((c) - 'A')) < 26) \
     : (((unsigned int)((c) - 'A')) < 26))
#define __C_isgraph(c) \
    ((sizeof(c) == sizeof(char)) \
     ? (((unsigned int)((c) - 0x21)) <= (0x7e - 0x21)) \
     : (((unsigned int)((c) - 0x21)) <= (0x7e - 0x21)))

#define __toascii(c) ((c) & 0x7f)
#define __C_tolower(c) (__C_isupper(c) ? ((c) | 0x20) : (c))
#define __C_toupper(c) (__C_islower(c) ? ((c) ^ 0x20) : (c))

#if defined (__cplusplus)
extern "C" {
#endif
bool isdigit(char ch) ;
#if defined(__cplusplus)
}
#endif

bool iswhitespace(char ch) ;
bool isalpha(char ch) ;
bool isalnum(char ch) ;
bool isprint(char ch) ;
bool isblank(char ch) ;
bool isgraph(char ch) ;
bool isspace(char ch) ;
bool iscntrl(char ch) ;
bool isxdigit(char ch) ;
int abs(int num) ;
int toascii(int c) ;
int tolower(int c) ;
int toupper(int c) ;

//void memset(char* dest, char ch, int len) ;
//void* memcpy(char* dest, const char* src, int len) ;

#endif
