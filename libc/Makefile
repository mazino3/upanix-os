#	 Mother Operating System - An x86 based Operating System
#	 Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa_prajwal@yahoo.co.in'
#																			 
#	 This program is free software: you can redistribute it and/or modify
#	 it under the terms of the GNU General Public License as published by
#	 the Free Software Foundation, either version 3 of the License, or
#	 (at your option) any later version.
#																			 
#	 This program is distributed in the hope that it will be useful,
#	 but WITHOUT ANY WARRANTY; without even the implied warranty of
#	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	 GNU General Public License for more details.
#																			 
#	 You should have received a copy of the GNU General Public License
#	 along with this program.  If not, see <http://www.gnu.org/licenses/
CC = ${C_COMPILER}
INCLUDE = ${INCLUDES}
CFLAGS = ${C_FLAGS} ${INCLUDE}

MOS_LIBC_OBJS = \
	./stdlib/dtime.o \
	./stdlib/assert.o \
	./stdlib/alloc.o \
	./stdlib/abort.o \
	./stdlib/div.o \
	./stdlib/ldiv.o \
	./stdlib/tempname.o \
	./stdlib/error.o \
	./stdlib/stdlib.o \
	./stdlib/strtod.o \
	./stdio/ftello.o \
	./stdio/vprintf.o \
	./stdio/setbuf.o \
	./stdio/tmpnam_r.o \
	./stdio/clearerr.o \
	./stdio/vfprintf.o \
	./stdio/__fsetlocking.o \
	./stdio/dprintf.o \
	./stdio/printf.o \
	./stdio/_stdio.o \
	./stdio/tempnam.o \
	./stdio/__freadable.o \
	./stdio/_fopen.o \
	./stdio/fileno.o \
	./stdio/fgetpos.o \
	./stdio/_fwrite.o \
	./stdio/getline.o \
	./stdio/_WRITE.o \
	./stdio/vsnprintf.o \
	./stdio/setvbuf.o \
	./stdio/setbuffer.o \
	./stdio/freopen.o \
	./stdio/snprintf.o \
	./stdio/fread.o \
	./stdio/putchar.o \
	./stdio/_trans2w.o \
	./stdio/tmpfile.o \
	./stdio/fflush.o \
	./stdio/_wcommit.o \
	./stdio/_cs_funcs.o \
	./stdio/scanf.o \
	./stdio/fputc.o \
	./stdio/asprintf.o \
	./stdio/getdelim.o \
	./stdio/remove.o \
	./stdio/ferror.o \
	./stdio/fputs.o \
	./stdio/vsprintf.o \
	./stdio/flockfile.o \
	./stdio/tmpnam.o \
	./stdio/fseeko.o \
	./stdio/fcloseall.o \
	./stdio/_uintmaxtostr.o \
	./stdio/_store_inttype.o \
	./stdio/vdprintf.o \
	./stdio/fopen.o \
	./stdio/rewind.o \
	./stdio/sprintf.o \
	./stdio/__fwriting.o \
	./stdio/_adjust_pos.o \
	./stdio/ftrylockfile.o \
	./stdio/__fwritable.o \
	./stdio/gets.o \
	./stdio/fsetpos.o \
	./stdio/__fpurge.o \
	./stdio/funlockfile.o \
	./stdio/__freading.o \
	./stdio/fprintf.o \
	./stdio/_rfill.o \
	./stdio/_trans2r.o \
	./stdio/fwrite.o \
	./stdio/_fpmaxtostr.o \
	./stdio/__fbufsize.o \
	./stdio/perror.o \
	./stdio/setlinebuf.o \
	./stdio/fgets.o \
	./stdio/vasprintf.o \
	./stdio/fgetc.o \
	./stdio/feof.o \
	./stdio/fclose.o \
	./stdio/_load_inttype.o \
	./stdio/getw.o \
	./stdio/__fpending.o \
	./stdio/puts.o \
	./stdio/ungetc.o \
	./stdio/getchar.o \
	./stdio/fwide.o \
	./stdio/fdopen.o \
	./stdio/_READ.o \
	./sys/SysDisplay.o \
	./sys/SysKB.o \
	./sys/SysFS.o \
	./sys/SysDrive.o \
	./sys/SysMemory.o \
	./sys/SysProcess.o \
	./sys/SysUtil.o \
	./stdcal/mcal.o \
	./stdcal/mdate.o \
	./string/rawmemchr.o \
	./string/strcspn.o \
	./string/memcmp.o \
	./string/memrchr.o \
	./string/strstr.o \
	./string/strspn.o \
	./string/string.o \
	./string/strtok_r.o \
	./ctype/kb.o \
	./ctype/ctype.o

build: subdir libc.a

subdir:
	cd ctype && make
	cd string && make
	cd sys && make
	cd stdio && make
	cd stdlib && make
	cd stdcal && make

libc.a: ${MOS_LIBC_OBJS}
	ar -r libc.a ${MOS_LIBC_OBJS}

clean:
	cd ctype && make clean
	cd string && make clean
	cd sys && make clean
	cd stdio && make clean
	cd stdlib && make clean
	cd stdcal && make clean

distclean:
	cd ctype && make distclean
	cd string && make distclean
	cd sys && make distclean
	cd stdio && make distclean
	cd stdlib && make distclean
	cd stdcal && make distclean
	rm -f libc.a
