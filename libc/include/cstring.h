/* Copyright (C) 1991-1993, 1995-2003, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/*
 *	ISO C99 Standard: 7.21 String handling	<cstring.h>
 */

#ifndef	_STRING_H
#define	_STRING_H	1

#include <stdlib.h>
#include <features.h>

/* Get size_t and NULL from <stddef.h>.  */
#define	__need_size_t
#define	__need_NULL
#include <stddef.h>
#include <ctype.h>

typedef bool tok_compare_func(char ch) ;
typedef bool tok_group_func(char ch) ;
typedef void tok_copy_func(int index, const char* src, int len) ;

#if defined (__cplusplus)
extern "C" {
#endif
/* Compare N bytes of S1 and S2.  */
int memcmp(const __ptr_t s1, const __ptr_t s2, size_t len) ;
/* Compare S1 and S2.  */
int strcmp (__const char *__s1, __const char *__s2) ;
/* Return the length of S.  */
size_t strlen (__const char *__s) __attribute_pure__ __nonnull ((1));
char* __strtok_r (char *s, const char *delim, char **save_ptr) ;
/* Compare N characters of S1 and S2.  */
int strncmp (__const char *__s1, __const char *__s2, size_t __n) ;
/* Set N bytes of S to C.  */
void *memset (void *__s, int __c, size_t __n) ;
/* Copy N bytes of SRC to DEST.  */
void *memcpy (void *__restrict __dest, __const void *__restrict __src, size_t __n) ;
char* strdup(const char* s) ;
/* Find the first occurrence of NEEDLE in HAYSTACK.  */
extern char *strstr (__const char *__haystack, __const char *__needle) ;
#if defined (__cplusplus)
}
#endif

void* mempcpy(void * s1, const void * s2, size_t n) ;

/* Copy N bytes of SRC to DEST, guaranteeing
   correct behavior for overlapping strings.  */
extern void *memmove (void *__dest, __const void *__src, size_t __n) ;

/* Search N bytes of S for C.  */
extern void *memchr (__const void *__s, int __c, size_t __n) ;

/* Search no more than N bytes of S for C.  */
extern void *memrchr (const void * s, int c_in, size_t n) ;

/* Copy SRC to DEST.  */
extern char *strcpy (char *__restrict __dest, __const char *__restrict __src) ;
/* Copy no more than N characters of SRC to DEST.  */
extern char *strncpy (char *__restrict __dest,
		      __const char *__restrict __src, size_t __n) ;

/* Append SRC onto DEST.  */
extern char *strcat (char *__restrict __dest, __const char *__restrict __src) ;
/* Append no more than N characters from SRC onto DEST.  */
extern char *strncat (char *__restrict __dest, __const char *__restrict __src,
		      size_t __n) ;

/* Compare the collated forms of S1 and S2.  */
extern int strcoll (__const char *__s1, __const char *__s2) ;
/* Put a transformation of SRC into no more than N bytes of DEST.  */
extern size_t strxfrm (char *__restrict __dest,
		       __const char *__restrict __src, size_t __n) ;

/* Find the first occurrence of C in S.  */
extern char *strchr (__const char *__s, int __c) ;
/* Find the last occurrence of C in S.  */
extern char *strrchr (__const char *__s, int __c) ;

/* Return the length of the initial segment of S which
   consists entirely of characters not in REJECT.  */
extern size_t strcspn (__const char *__s, __const char *__reject) ;
/* Return the length of the initial segment of S which
   consists entirely of characters in ACCEPT.  */
extern size_t strspn (__const char *__s, __const char *__accept) ;
/* Return the length of the maximum initial segment of S
   which contains no characters from REJECT.  */
size_t strcspn (const char *s, const char *reject) ;
/* Find the first occurrence in S of any character in ACCEPT.  */
extern char *strpbrk (__const char *__s, __const char *__accept) ;

/* Divide S into tokens separated by characters in DELIM.  */
//extern char *strtok (char *__restrict __s, __const char *__restrict __delim)
//      __nonnull ((2));
extern char* strtok(char * __restrict s1, const char * __restrict s2) ;
extern void strtok_m(const char* src, char chToken, tok_copy_func* tc_copy, int* iListSize) ;
extern void strtok_c(const char* src, 
			tok_compare_func* tc_tok_func, 
			tok_group_func* tc_group_func, 
			tok_copy_func* tc_copy, 
			int* iListSize) ;

extern int strcnt(const char* str, const char ch) ;

/* Return a string describing the meaning of the `errno' code in ERRNUM.  */
extern const char* strerror (int __errnum) ;

extern size_t strnlen (__const char *__string, size_t __maxlen)
     __attribute_pure__ __nonnull ((1));

int strcasecmp(register const char *s1, register const char *s2) ;
int strncasecmp(register const char *s1, register const char *s2, size_t n) ;

int ffs(int i) ;

void *rawmemchr (const void * s, int c_in) ;

extern char * strtok_r (char *s, const char *delim, char **save_ptr);
//#define strtok_r(p, d, sp) __strtok_r(p, d, sp)

#endif /* cstring.h  */
