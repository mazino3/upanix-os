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
#ifndef _STD_LIB_H_
#define _STD_LIB_H_

# include <features.h>
# include <abort.h>
# include <stddef.h>
# include <bits/uClibc_uintmaxtostr.h>

typedef unsigned char byte ;

#ifndef NULL
#define NULL 0x0
#endif

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#define UNUSED __attribute__((unused))
#define ALIGNED(x) __attribute__((aligned(x)))
#else
#define PACKED
#warning __GNUC__ Not Defined ---> PACKED is Masked
#define UNUSED
#warning __GNUC__ Not Defined ---> UNUSED is Masked
#define ALIGNED(x)
#warning __GNUC__ Not Defined ---> ALIGNED is Masked
#endif

#ifndef L_strtoll_l
#define L_strtoll_l
#endif

#ifndef L__stdlib_strto_ll_l
#define L__stdlib_strto_ll_l
#endif

#ifndef L_strtoll
#define L_strtoll
#endif

#ifndef L__stdlib_strto_ll
#define L__stdlib_strto_ll
#endif

#ifndef _LIBC
#define _LIBC
#endif

int abs(int j) ;
long int labs(long int j) ;

#if defined (__cplusplus)
extern "C" {
#endif
long long int llabs(long long int j) ;
long strtol(const char * __restrict str, char ** __restrict endptr, int base) ;
#if defined (__cplusplus)
}
#endif
int atoi(const char *nptr) ;
long atol(const char *nptr) ;
long long atoll(const char *nptr) ;
double atof(const char *nptr) ;
long long strtoll(const char * __restrict str, char ** __restrict endptr, int base) ;
double strtod(const char *__restrict str, char **__restrict endptr) ;
long double strtold(const char *str, char **endptr) ;
float strtof(const char *str, char **endptr) ;
unsigned long long strtoull(const char * __restrict str, char ** __restrict endptr, int base) ;

/******************/
extern void SysUtil_Reboot() ;
extern void SysProcess_Sleep(unsigned milisec) ;

#define reboot() SysUtil_Reboot()
#define sleep(s) SysProcess_Sleep((s) * 1000) ;

void exit(int rv) ;
int gettimeofday(struct timeval* pTV) ;
/******************/

/* Returned by `div'.  */
typedef struct
  {
    int quot;			/* Quotient.  */
    int rem;			/* Remainder.  */
  } div_t;

/* Returned by `ldiv'.  */
#ifndef __ldiv_t_defined
typedef struct
  {
    long int quot;		/* Quotient.  */
    long int rem;		/* Remainder.  */
  } ldiv_t;
# define __ldiv_t_defined	1
#endif

extern div_t div (int __numer, int __denom) __THROW __attribute__ ((__const__));
extern ldiv_t ldiv (long int __numer, long int __denom) __THROW __attribute__ ((__const__));

unsigned long strtoul(const char * __restrict str, char ** __restrict endptr, int base);

/* Algos Start */
void qsort (void  *base, size_t nel, size_t width, int (*comp)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t /* nmemb */ high,
			  size_t size, int (*compar)(const void *, const void *)) ;
/* Algos End */

char* getenv(const char* var) ;
int setenv(const char* var, const char* val) ;

unsigned clock() ;
void format_dir_attr(unsigned short attr, char* fmAttr) ;

const char* getProcStatusDesc(int iStatus) ;

#endif
