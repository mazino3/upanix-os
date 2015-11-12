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
#ifndef _STDIO_H
#define _STDIO_H 1

# include <ctype.h>
# include <stdarg.h>

extern void SysDisplay_ClearScreen() ;
extern void SysDisplay_Character(const char ch, unsigned uiAttr) ;
extern void SysDisplay_Message(const char* szMessage, unsigned uiAttr) ;
extern void SysDisplay_Address(const char* szMessage, unsigned uiNumber) ;
extern void SysDisplay_RawCharacter(const char ch, unsigned uiAttr, bool bUpdateCursorOnScreen) ;

#define __UCLIBC_HAS_FLOATS__ 1

#define clrscr()			SysKC::MDisplay().ClearScreen()
#define putchar(ch)			SysKC::MDisplay().Character(ch, KC::MDisplay().WHITE_ON_BLACK())
//#define puts(msg)			SysKC::MDisplay().Message(msg, KC::MDisplay().WHITE_ON_BLACK())
#define putnum(msg, num) 	SysKC::MDisplay().Address(msg, num)
#define putrawc(ch, attr, upcur)	SysKC::MDisplay().RawCharacter(ch, attr, upcur)

#define OFFSET_TYPE		long int

# define P_tmpdir	"/tmp"

typedef struct __STDIO_FILE_STRUCT FILE;

typedef struct
{
	char* buf ;
	int pos ;
} STDBUF ;

typedef void cecho_func(char ch) ;

extern FILE *stdin ;
extern FILE *stdout ;
extern FILE *stderr ;

# include <bits/types.h>

# include <features.h>

// #if !defined __need_FILE && !defined __need___FILE

# define __need_size_t
# define __need_NULL
# include <stddef.h>

//# define __need_FILE
//# define __need___FILE

#ifndef L_vfprintf
#define L_vfprintf 1
#define VFPRINTF vfprintf
#define FMT_TYPE char
#endif 

#ifndef L_scanf
#define L_scanf
#endif

#ifndef L_vfscanf
#define L_vfscanf
#endif

//#ifndef NL_ARGMAX
//#define NL_ARGMAX 9
//#endif
//
//#ifndef L_vsscanf
//#define L_vsscanf
//#endif

#define EOF -1

#define STDIN_FILENO	1
#define STDOUT_FILENO	2
#define STDERR_FILENO	3

#define BUFSIZ 1024

#define IS_STD_STREAM(S) (S->__filedes == 0 || S->__filedes == 1 || S->__filedes == 2)

#if defined (__cplusplus)
extern "C" {
#endif
int printf(const char * __restrict format, ...) ;
int snprintf(char *__restrict buf, size_t size, const char * __restrict format, ...) ;
int asprintf(char **__restrict buf, const char * __restrict format, ...);
OFFSET_TYPE ftell(register FILE *stream) ;
int fseek(register FILE *stream, OFFSET_TYPE offset, int whence) ;
#if defined (__cplusplus)
}
#endif

int sprintf(char *__restrict buf, const char * __restrict format, ...) ;
int scanf(const char * __restrict format, ...) ;
int sscanf(const char * __restrict str, const char * __restrict format, ...) ;
int vsscanf(const char* __restrict buf, const char * __restrict format, va_list arg) ;
int fprintf(FILE * __restrict stream, const char * __restrict format, ...) ;
int fgetc(register FILE *stream) ;
int getc(register FILE *stream) ;
char* fgets(char *__restrict s, int n, register FILE * __restrict stream) ;

int scanf_cecho(cecho_func* cecho_func, const char * __restrict format, ...) ;
int vsscanf_cecho(cecho_func* cechoFunc, const char* __restrict buf, const char * __restrict format, va_list arg) ;

void putsl(const char* s, size_t len) ;

int fflush(FILE *stream);
FILE *fopen (__const char *__restrict __filename, __const char *__restrict __modes);
int fputs(register const char * __restrict s, FILE * __restrict stream) ;
int puts(register const char * __restrict s) ;
int fputc(int c, register FILE *stream) ;
int putc(int c, register FILE *stream) ;
size_t fwrite(const void * __restrict ptr, size_t size, size_t nmemb, register FILE * __restrict stream) ;
size_t fread(void * __restrict ptr, size_t size, size_t nmemb, FILE * __restrict stream) ;
int fclose(register FILE *stream) ;
int remove(register const char *filename) ;

extern int feof(register FILE *stream) ;
extern int vfprintf (FILE * __restrict stream, register const FMT_TYPE * __restrict format, va_list arg) ;
extern int vsprintf(char *__restrict buf, const char * __restrict format, va_list arg) ;
extern void perror(register const char *s) ;
extern int vsnprintf(char *__restrict buf, size_t size, const char * __restrict format, va_list arg) ;

# include <bits/uClibc_stdio.h>
# include <bits/stdio_lim.h>

typedef __STDIO_fpos_t fpos_t;

#define _IOFBF __STDIO_IOFBF 		/* Fully buffered.  */
#define _IOLBF __STDIO_IOLBF		/* Line buffered.  */
#define _IONBF __STDIO_IONBF		/* No buffering.  */

FILE* _stdio_get_stdin() ;
FILE* _stdio_get_stdout() ;
FILE* _stdio_get_stderr() ;

#define stdin	_stdio_get_stdin()
#define stdout	_stdio_get_stdout()
#define stderr	_stdio_get_stderr()

extern int vsscanf_cecho(cecho_func* cechoFunc, const char* __restrict buf, const char * __restrict format, va_list arg) ;
extern int scanf_cecho(cecho_func* cecho_func, const char * __restrict format, ...) ;

extern void _stdio_init(void) ;
extern void _stdio_term(void) ;

void rewind(register FILE *stream) ;
int ferror(register FILE *stream) ;

int close(int fd) ;
int getc_unlocked(FILE *stream) ; 
int ungetc(int c, register FILE *stream) ;
int setvbuf(register FILE * __restrict stream, register char * __restrict buf, int mode, size_t size) ;
FILE *fdopen(int filedes, const char *mode) ;

#endif
