/* Copyright (C) 2004       Manuel Novoa III    <mjn3@codepoet.org>
 *
 * GNU Library General Public License (LGPL) version 2 or later.
 *
 * Dedicated to Toni.  See uClibc/DEDICATION.mjn3 for details.
 */

#include "_stdio.h"
#include <fs.h>

FILE *fdopen(int filedes, const char *mode)
{
	intptr_t cur_mode;

	/**** MOSS ****/
	/* fnctl call to get file open mode changed to getomode */
	/**** MOSE ****/
	return (((cur_mode = getomode(filedes))) != -1)
		? _stdio_fopen(cur_mode, mode, NULL, filedes) 
		: NULL;
}
