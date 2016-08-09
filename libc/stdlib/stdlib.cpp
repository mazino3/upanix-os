/*  Copyright (C) 2002     Manuel Novoa III
 *  From my (incomplete) stdlib library for linux and (soon) elks.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*  ATTENTION!   ATTENTION!   ATTENTION!   ATTENTION!   ATTENTION!
 *
 *  This code is currently under development.  Also, I plan to port
 *  it to elks which is a 16-bit environment with a fairly limited
 *  compiler.  Therefore, please refrain from modifying this code
 *  and, instead, pass any bug-fixes, etc. to me.  Thanks.  Manuel
 *
 *  ATTENTION!   ATTENTION!   ATTENTION!   ATTENTION!   ATTENTION! */

/* Oct 29, 2002
 * Fix a couple of 'restrict' bugs in mbstowcs and wcstombs.
 *
 * Nov 21, 2002
 * Add wscto{inttype} functions.
 */

#define _ISOC99_SOURCE			/* for ULLONG primarily... */
#include <limits.h>
#include <stdint.h>
//#include <inttypes.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <mosstd.h>
#include <fs.h>
#include <stdio.h>

/* Work around gcc's refusal to create aliases. 
 * TODO: Add in a define to disable the aliases? */

#include <stdlib.h>
#include <locale.h>
#include <cstring.h>

int errno;

/**********************************************************************/
extern unsigned long
_stdlib_strto_l_l(register const char * __restrict str,
				  char ** __restrict endptr, int base, int sflag) ;

extern unsigned long long _stdlib_strto_ll_l(register const char * __restrict str,
				   char ** __restrict endptr, int base, int sflag) ;

extern unsigned long
_stdlib_strto_l(register const char * __restrict str,
				char ** __restrict endptr, int base, int sflag);

extern unsigned long long _stdlib_strto_ll(register const char * __restrict str,
				 char ** __restrict endptr, int base, int sflag);

/**********************************************************************/
double atof(const char *nptr)
{
	return strtod(nptr, (char **) NULL);
}
/**********************************************************************/
#if INT_MAX < LONG_MAX

int abs(int j)
{
	return (j >= 0) ? j : -j;
}

#endif /* INT_MAX < LONG_MAX */

/**********************************************************************/
#if ULONG_MAX == UINTMAX_MAX
strong_alias(labs,imaxabs)
#endif

long int labs(long int j)
{
	return (j >= 0) ? j : -j;
}
/**********************************************************************/
#if defined(ULLONG_MAX) && (LLONG_MAX > LONG_MAX)

#if (ULLONG_MAX == UINTMAX_MAX)
strong_alias(llabs,imaxabs)
#endif

long long int llabs(long long int j)
{
	return (j >= 0) ? j : -j;
}

#endif /* defined(ULLONG_MAX) && (LLONG_MAX > LONG_MAX) */

/**********************************************************************/
int atoi(const char *nptr)
{
	return (int) strtol(nptr, (char **) NULL, 10);
}

/**********************************************************************/
#if defined(ULLONG_MAX) && (ULLONG_MAX == ULONG_MAX)
strong_alias(atol,atoll)
#endif

long atol(const char *nptr)
{
	return (int) strtol(nptr, (char **) NULL, 10);
}
/**********************************************************************/
#if defined(ULLONG_MAX) && (LLONG_MAX > LONG_MAX)

long long atoll(const char *nptr)
{
	return strtoll(nptr, (char **) NULL, 10);
}

#endif /* defined(ULLONG_MAX) && (LLONG_MAX > LONG_MAX) */
/**********************************************************************/
#if (ULONG_MAX == UINTMAX_MAX) && !defined(L_strtol_l)
strong_alias(strtol,strtoimax)
#endif

long strtol(const char * __restrict str, char ** __restrict endptr, int base)
{
    return _stdlib_strto_l(str, endptr, base, 1) ;
}
/**********************************************************************/
#if defined(ULLONG_MAX) && (LLONG_MAX > LONG_MAX)

long long strtoll(const char * __restrict str, char ** __restrict endptr, int base)
{
    return (long long)_stdlib_strto_ll(str, endptr, base, 1) ;
}

#endif
/**********************************************************************/
#if (ULONG_MAX == UINTMAX_MAX) && !defined(L_strtoul_l)
strong_alias(strtoul,strtoumax)
#endif

unsigned long strtoul(const char * __restrict str, char ** __restrict endptr, int base)
{
    return _stdlib_strto_l(str, endptr, base, 0) ;
}

/**********************************************************************/
#if defined(ULLONG_MAX) && (LLONG_MAX > LONG_MAX)

unsigned long long strtoull(const char * __restrict str, char ** __restrict endptr, int base)
{
    return _stdlib_strto_ll(str, endptr, base, 0) ;
}

#endif
/**********************************************************************/
/* Support routines follow */
/**********************************************************************/
/* Set if we want errno set appropriately. */
/* NOTE: Implies _STRTO_ENDPTR below */
#define _STRTO_ERRNO            1

/* Set if we want support for the endptr arg. */
/* Implied by _STRTO_ERRNO. */
#define _STRTO_ENDPTR           1

#if _STRTO_ERRNO
#undef _STRTO_ENDPTR
#define _STRTO_ENDPTR           1
#define SET_ERRNO(X)            __set_errno(X)
#else
#define SET_ERRNO(X)            ((void)(X))	/* keep side effects */
#endif

/**********************************************************************/
#define Wchar char
#define Wuchar unsigned char
#define ISSPACE(C) isspace((C))

unsigned long _stdlib_strto_l(register const Wchar * __restrict str,
							  Wchar ** __restrict endptr, int base,
							  int sflag)
{
	return _stdlib_strto_l_l(str, endptr, base, sflag) ;
}

/* This is the main work fuction which handles both strtol (sflag = 1) and
 * strtoul (sflag = 0). */

unsigned long _stdlib_strto_l_l(register const Wchar * __restrict str,
										Wchar ** __restrict endptr, int base,
										int sflag)
{
    unsigned long number, cutoff;
#if _STRTO_ENDPTR
    const Wchar *fail_char;
#define SET_FAIL(X)       fail_char = (X)
#else
#define SET_FAIL(X)       ((void)(X)) /* Keep side effects. */
#endif
    unsigned char negative, digit, cutoff_digit;

	assert(((unsigned int)sflag) <= 1);

	SET_FAIL(str);

    while (ISSPACE(*str)) { /* Skip leading whitespace. */
		++str;
    }

    /* Handle optional sign. */
    negative = 0;
    switch(*str) {
		case '-': negative = 1;	/* Fall through to increment str. */
		case '+': ++str;
    }

    if (!(base & ~0x10)) {		/* Either dynamic (base = 0) or base 16. */
		base += 10;				/* Default is 10 (26). */
		if (*str == '0') {
			SET_FAIL(++str);
			base -= 2;			/* Now base is 8 or 16 (24). */
			if ((0x20|(*str)) == 'x') { /* WARNING: assumes ascii. */
				++str;
				base += base;	/* Base is 16 (16 or 48). */
			}
		}

		if (base > 16) {		/* Adjust in case base wasn't dynamic. */
			base = 16;
		}
    }

	number = 0;

    if (((unsigned)(base - 2)) < 35) { /* Legal base. */
		cutoff_digit = ULONG_MAX % base;
		cutoff = ULONG_MAX / base;
		do {
			digit = (((Wuchar)(*str - '0')) <= 9)
				? (*str - '0')
				: ((*str >= 'A')
				   ? (((0x20|(*str)) - 'a' + 10)) /* WARNING: assumes ascii. */
					  : 40);

			if (digit >= base) {
				break;
			}

			SET_FAIL(++str);

			if ((number > cutoff)
				|| ((number == cutoff) && (digit > cutoff_digit))) {
				number = ULONG_MAX;
				negative &= sflag;
				SET_ERRNO(ERANGE);
			} else {
				number = number * base + digit;
			}
		} while (1);
	}

#if _STRTO_ENDPTR
    if (endptr) {
		*endptr = (Wchar *) fail_char;
    }
#endif

	{
		unsigned long tmp = ((negative)
							 ? ((unsigned long)(-(1+LONG_MIN)))+1
							 : LONG_MAX);
		if (sflag && (number > tmp)) {
			number = tmp;
			SET_ERRNO(ERANGE);
		}
	}

	return negative ? (unsigned long)(-((long)number)) : number;
}
/**********************************************************************/
unsigned long long _stdlib_strto_ll(register const Wchar * __restrict str,
									Wchar ** __restrict endptr, int base,
									int sflag)
{
	return _stdlib_strto_ll_l(str, endptr, base, sflag) ;
}

/* This is the main work fuction which handles both strtoll (sflag = 1) and
 * strtoull (sflag = 0). */

unsigned long long _stdlib_strto_ll_l(register const Wchar * __restrict str,
											  Wchar ** __restrict endptr, int base,
											  int sflag)
{
    unsigned long long number;
#if _STRTO_ENDPTR
    const Wchar *fail_char;
#define SET_FAIL(X)       fail_char = (X)
#else
#define SET_FAIL(X)       ((void)(X)) /* Keep side effects. */
#endif
	unsigned int n1;
    unsigned char negative, digit;

	assert(((unsigned int)sflag) <= 1);

	SET_FAIL(str);

    while (ISSPACE(*str)) {		/* Skip leading whitespace. */
		++str;
    }

    /* Handle optional sign. */
    negative = 0;
    switch(*str) {
		case '-': negative = 1;	/* Fall through to increment str. */
		case '+': ++str;
    }

    if (!(base & ~0x10)) {		/* Either dynamic (base = 0) or base 16. */
		base += 10;				/* Default is 10 (26). */
		if (*str == '0') {
			SET_FAIL(++str);
			base -= 2;			/* Now base is 8 or 16 (24). */
			if ((0x20|(*str)) == 'x') { /* WARNING: assumes ascii. */
				++str;
				base += base;	/* Base is 16 (16 or 48). */
			}
		}

		if (base > 16) {		/* Adjust in case base wasn't dynamic. */
			base = 16;
		}
    }

	number = 0;

    if (((unsigned)(base - 2)) < 35) { /* Legal base. */
		do {
			digit = (((Wuchar)(*str - '0')) <= 9)
				? (*str - '0')
				: ((*str >= 'A')
				   ? (((0x20|(*str)) - 'a' + 10)) /* WARNING: assumes ascii. */
					  : 40);

			if (digit >= base) {
				break;
			}

			SET_FAIL(++str);

#if 1
			/* Optional, but speeds things up in the usual case. */
			if (number <= (ULLONG_MAX >> 6)) {
				number = number * base + digit;
			} else
#endif
			{
				n1 = ((unsigned char) number) * base + digit;
				number = (number >> CHAR_BIT) * base;

				if (number + (n1 >> CHAR_BIT) <= (ULLONG_MAX >> CHAR_BIT)) {
					number = (number << CHAR_BIT) + n1;
				} else {		/* Overflow. */
					number = ULLONG_MAX;
					negative &= sflag;
					SET_ERRNO(ERANGE);
				}
			}

		} while (1);
	}

#if _STRTO_ENDPTR
    if (endptr) {
		*endptr = (Wchar *) fail_char;
    }
#endif

	{
		unsigned long long tmp = ((negative)
								  ? ((unsigned long long)(-(1+LLONG_MIN)))+1
								  : LLONG_MAX);
		if (sflag && (number > tmp)) {
			number = tmp;
			SET_ERRNO(ERANGE);
		}
	}

	return negative ? (unsigned long long)(-((long long)number)) : number;
}
/**********************************************************************/
/* Made _Exit() an alias for _exit(), as per C99. */
/*  #ifdef L__Exit */

/*  void _Exit(int status) */
/*  { */
/*  	_exit(status); */
/*  } */

/*  #endif */
/**********************************************************************/
void *bsearch(const void *key, const void *base, size_t /* nmemb */ high,
			  size_t size, int (*compar)(const void *, const void *))
{
	register char *p;
	size_t low;
	size_t mid;
	int r;

	if (size > 0) {				/* TODO: change this to an assert?? */
		low = 0;
		while (low < high) {
			mid = low + ((high - low) >> 1); /* Avoid possible overflow here. */
			p = ((char *)base) + mid * size; /* Could overflow here... */
			r = (*compar)(key, p); /* but that's an application problem! */
			if (r > 0) {
				low = mid + 1;
			} else if (r < 0) {
				high = mid;
			} else {
				return p;
			}
		}
	}
	return NULL;
}
/**********************************************************************/
/* This code is derived from a public domain shell sort routine by
 * Ray Gardner and found in Bob Stout's snippets collection.  The
 * original code is included below in an #if 0/#endif block.
 *
 * I modified it to avoid the possibility of overflow in the wgap
 * calculation, as well as to reduce the generated code size with
 * bcc and gcc. */

void qsort(void  *base, size_t nel, size_t width, int (*comp)(const void *, const void *))
{
	size_t wgap, i, j, k;
	char tmp;

	if((nel > 1) && (width > 0)) 
	{
		printf("\n nel = %d", nel) ;
		printf("\n width = %d", width) ;

		assert( nel <= ((size_t)(-1)) / width ); /* check for overflow */

		wgap = 0;
		
		do 
		{
			wgap = 3 * wgap + 1;
		} while (wgap < (nel - 1) / 3);

		/* From the above, we know that either wgap == 1 < nel or */
		/* ((wgap-1)/3 < (int) ((nel-1)/3) <= (nel-1)/3 ==> wgap <  nel. */
		wgap *= width;			/* So this can not overflow if wnel doesn't. */
		nel *= width;			/* Convert nel to 'wnel' */

		do 
		{
			i = wgap;
			do
		   	{
				j = i;
				do
			   	{
					register char *a;
					register char *b;

					j -= wgap;
					a = j + ((char *)base);
					b = a + wgap;
					if ( (*comp)(a, b) <= 0 ) {
						break;
					}
					k = width;
					do
				   	{
						tmp = *a;
						*a++ = *b;
						*b++ = tmp;
					} while ( --k );
				} while (j >= wgap);
				i += width;
			} while (i < nel);
			wgap = (wgap - width)/3;
		} while (wgap);
	}
}

/* ---------- original snippets version below ---------- */

#if 0
/*
**  ssort()  --  Fast, small, qsort()-compatible Shell sort
**
**  by Ray Gardner,  public domain   5/90
*/

#include <stddef.h>

void ssort (void  *base,
            size_t nel,
            size_t width,
            int (*comp)(const void *, const void *))
{
      size_t wnel, gap, wgap, i, j, k;
      char *a, *b, tmp;

      wnel = width * nel;
      for (gap = 0; ++gap < nel;)
            gap *= 3;
      while ( gap /= 3 )
      {
            wgap = width * gap;
            for (i = wgap; i < wnel; i += width)
            {
                  for (j = i - wgap; ;j -= wgap)
                  {
                        a = j + (char *)base;
                        b = a + wgap;
                        if ( (*comp)(a, b) <= 0 )
                              break;
                        k = width;
                        do
                        {
                              tmp = *a;
                              *a++ = *b;
                              *b++ = tmp;
                        } while ( --k );
                        if (j < wgap)
                              break;
                  }
            }
      }
}
#endif

/**********************************************************************/
#ifdef L__stdlib_mb_cur_max

size_t _stdlib_mb_cur_max(void)
{
#ifdef __CTYPE_HAS_UTF_8_LOCALES
	return __UCLIBC_CURLOCALE_DATA.mb_cur_max;
#else
#ifdef __CTYPE_HAS_8_BIT_LOCALES
#ifdef __UCLIBC_MJN3_ONLY__
#warning need to change this when/if transliteration is implemented
#endif
#endif
	return 1;
#endif
}

#endif
/**********************************************************************/
#ifdef L_mblen

int mblen(register const char *s, size_t n)
{
	static mbstate_t state;
	size_t r;

	if (!s) {
		state.__mask = 0;
#ifdef __CTYPE_HAS_UTF_8_LOCALES
		return ENCODING == __ctype_encoding_utf8;
#else
		return 0;
#endif
	}

	if ((r = mbrlen(s, n, &state)) == (size_t) -2) {
		/* TODO: Should we set an error state? */
		state.__wc = 0xffffU;	/* Make sure we're in an error state. */
		return (size_t) -1;		/* TODO: Change error code above? */
	}
	return r;
}

#endif
/**********************************************************************/
#ifdef L_mbtowc

int mbtowc(wchar_t *__restrict pwc, register const char *__restrict s, size_t n)
{
	static mbstate_t state;
	size_t r;

	if (!s) {
		state.__mask = 0;
#ifdef __CTYPE_HAS_UTF_8_LOCALES
		return ENCODING == __ctype_encoding_utf8;
#else
		return 0;
#endif
	}

	if ((r = mbrtowc(pwc, s, n, &state)) == (size_t) -2) {
		/* TODO: Should we set an error state? */
		state.__wc = 0xffffU;	/* Make sure we're in an error state. */
		return (size_t) -1;		/* TODO: Change error code above? */
	}
	return r;
}

#endif
/**********************************************************************/
#ifdef L_wctomb

/* Note: We completely ignore state in all currently supported conversions. */

int wctomb(register char *__restrict s, wchar_t swc)
{
	return (!s)
		?
#ifdef __CTYPE_HAS_UTF_8_LOCALES
		(ENCODING == __ctype_encoding_utf8)
#else
		0						/* Encoding is stateless. */
#endif
		: ((ssize_t) wcrtomb(s, swc, NULL));
}

#endif
/**********************************************************************/
#ifdef L_mbstowcs

size_t mbstowcs(wchar_t * __restrict pwcs, const char * __restrict s, size_t n)
{
	mbstate_t state;
	const char *e = s;			/* Needed because of restrict. */

	state.__mask = 0;			/* Always start in initial shift state. */
	return mbsrtowcs(pwcs, &e, n, &state);
}

#endif
/**********************************************************************/
#ifdef L_wcstombs

/* Note: We completely ignore state in all currently supported conversions. */

size_t wcstombs(char * __restrict s, const wchar_t * __restrict pwcs, size_t n)
{
	const wchar_t *e = pwcs;	/* Needed because of restrict. */

	return wcsrtombs(s, &e, n, NULL);
}

#endif
/**********************************************************************/
#if defined(L_wcstol) || defined(L_wcstol_l)

#if (ULONG_MAX == UINTMAX_MAX) && !defined(L_wcstol_l)
strong_alias(wcstol,wcstoimax)
#endif

#if defined(ULLONG_MAX) && (ULLONG_MAX == ULONG_MAX)
strong_alias(__XL(wcstol),__XL(wcstoll))
#endif

long __XL(wcstol)(const wchar_t * __restrict str,
				  wchar_t ** __restrict endptr, int base   __LOCALE_PARAM )
{
    return __XL_NPP(_stdlib_wcsto_l)(str, endptr, base, 1   __LOCALE_ARG );
}

__XL_ALIAS(wcstol)

#endif
/**********************************************************************/
#if defined(L_wcstoll) || defined(L_wcstoll_l)

#if defined(ULLONG_MAX) && (LLONG_MAX > LONG_MAX)

#if !defined(L_wcstoll_l)
#if (ULLONG_MAX == UINTMAX_MAX)
strong_alias(wcstoll,wcstoimax)
#endif
strong_alias(wcstoll,wcstoq)
#endif

long long __XL(wcstoll)(const wchar_t * __restrict str,
						wchar_t ** __restrict endptr, int base
						__LOCALE_PARAM )
{
    return (long long) __XL_NPP(_stdlib_wcsto_ll)(str, endptr, base, 1
												  __LOCALE_ARG );
}

__XL_ALIAS(wcstoll)

#endif /* defined(ULLONG_MAX) && (LLONG_MAX > LONG_MAX) */

#endif
/**********************************************************************/
#if defined(L_wcstoul) || defined(L_wcstoul_l)

#if (ULONG_MAX == UINTMAX_MAX) && !defined(L_wcstoul_l)
strong_alias(wcstoul,wcstoumax)
#endif

#if defined(ULLONG_MAX) && (ULLONG_MAX == ULONG_MAX)
strong_alias(__XL(wcstoul),__XL(wcstoull))
#endif

unsigned long __XL(wcstoul)(const wchar_t * __restrict str,
							wchar_t ** __restrict endptr, int base
							__LOCALE_PARAM )
{
    return __XL_NPP(_stdlib_wcsto_l)(str, endptr, base, 0   __LOCALE_ARG );
}

__XL_ALIAS(wcstoul)

#endif
/**********************************************************************/
#if defined(L_wcstoull) || defined(L_wcstoull_l)

#if defined(ULLONG_MAX) && (LLONG_MAX > LONG_MAX)

#if !defined(L_wcstoull_l)
#if (ULLONG_MAX == UINTMAX_MAX)
strong_alias(wcstoull,wcstoumax)
#endif
strong_alias(wcstoull,wcstouq)
#endif

unsigned long long __XL(wcstoull)(const wchar_t * __restrict str,
								  wchar_t ** __restrict endptr, int base
								  __LOCALE_PARAM )
{
    return __XL_NPP(_stdlib_wcsto_ll)(str, endptr, base, 0   __LOCALE_ARG );
}

__XL_ALIAS(wcstoull)

#endif /* defined(ULLONG_MAX) && (LLONG_MAX > LONG_MAX) */

#endif
/**********************************************************************/

/* MOSS */
void exit(int rv)
{
    /* If we are using stdio, try to shut it down.  At the very least,
	 * this will attempt to commit all buffered writes.  It may also
	 * unbuffer all writable files, or close them outright.
	 * Check the stdio routines for details. */
//	if (_stdio_term) 
	    _stdio_term();

	_exit(rv);
}

extern "C" char* SysProcess_GetEnv(const char* szVar) ;
extern "C" int SysProcess_SetEnv(const char* szVar, const char* szVal) ;

char* getenv(const char* var)
{
	return SysProcess_GetEnv(var) ;
}

int setenv(const char* var, const char* val)
{
	return SysProcess_SetEnv(var, val) ;
}

/* MOSE */

unsigned clock()
{
	//stub
	return 0 ;
}

void set_rwx(char* rwx, unsigned short perm)
{
	if(HAS_READ_PERM(perm))
		rwx[0] = 'r' ;

	if(HAS_WRITE_PERM(perm))
		rwx[1] = 'w' ;

	if(HAS_EXE_PERM(perm))
		rwx[2] = 'x' ;
}

void format_dir_attr(unsigned short attr, char* fmAttr)
{
	static const char* DIR_ATTR_FM_BASE = "----------" ;

	typedef enum
	{
		DIR_ATTR_FM_FTYPE = 0,
		DIR_ATTR_FM_OR,
		DIR_ATTR_FM_OW,
		DIR_ATTR_FM_OX,
		DIR_ATTR_FM_GR,
		DIR_ATTR_FM_GW,
		DIR_ATTR_FM_GX,
		DIR_ATTR_FM_TR,
		DIR_ATTR_FM_TW,
		DIR_ATTR_FM_TX
	} DIR_ATTR_FM ;
	
	strcpy(fmAttr, DIR_ATTR_FM_BASE) ;

	if(S_ISDIR(attr))
		fmAttr[DIR_ATTR_FM_FTYPE] = 'd' ;

	set_rwx(fmAttr + DIR_ATTR_FM_OR, G_OWNER(attr)) ;
	set_rwx(fmAttr + DIR_ATTR_FM_GR, G_GROUP(attr)) ;
	set_rwx(fmAttr + DIR_ATTR_FM_TR, G_OTHERS(attr)) ;
}

static const char PROC_STATUS_DESC[TOTAL_PROC_STATUS + 1][30] = {
	"New", "Run", "Sleep", "Wait(int)", "Wait(child)", "Wait(resource)", "Wait(service)", "Wait(event)", "unknown" } ;

const char* getProcStatusDesc(int iStatus)
{
	if(iStatus < 0 || iStatus >= TOTAL_PROC_STATUS)
		return  PROC_STATUS_DESC[TOTAL_PROC_STATUS] ;

	return PROC_STATUS_DESC[iStatus] ;
}
