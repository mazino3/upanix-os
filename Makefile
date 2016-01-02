#	 Upanix - An x86 based Operating System
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
UPANIX_HOME = $(shell pwd)

export UPANIX_HOME

BIN = ${UPANIX_HOME}/bin
BOOT = ${UPANIX_HOME}/boot
KERNEL = ${UPANIX_HOME}/kernel
PROCESS = ${UPANIX_HOME}/process
DISPLAY = ${UPANIX_HOME}/display
KERNEL_PROCS = ${UPANIX_HOME}/kernelprocs
DRIVERS = ${UPANIX_HOME}/drivers
UTIL = ${UPANIX_HOME}/util
MEMORY = ${UPANIX_HOME}/memory
FILE_SYSTEM = ${UPANIX_HOME}/filesystem
USERS = ${UPANIX_HOME}/users
EXEPARSER = ${UPANIX_HOME}/exeparser
SYSCALL = ${UPANIX_HOME}/syscall
OSUTILS = ${UPANIX_HOME}/osutils
TESTSUITE = ${UPANIX_HOME}/testsuite
LIBC = ${UPANIX_HOME}/libc
LIBMCPP = ${UPANIX_HOME}/libmcpp
LIBM = ${UPANIX_HOME}/libm
LIBCXXRT = ${UPANIX_HOME}/libcxxrt
LIBGCCEH = ${UPANIX_HOME}/libgcceh

FLOPPY_IMG = ${UPANIX_HOME}/floppy/1.44M-FLOPPY.img
FLOPPY_DEV = /dev/fd0

BOOT_OUTPUT = ${BIN}/Boot.bin
SETUP_OUTPUT = ${BIN}/Setup.bin
UPANIX_OUTPUT_FLAT = ${BIN}/upanix.bin
UPANIX_OUTPUT_AOUT = ${BIN}/upanix.elf

flat:
	cd ${BOOT} && make
	cd ${KERNEL} && make
	cd ${PROCESS} && make
	cd ${DISPLAY} && make
	cd ${KERNEL_PROCS} && make
	cd ${DRIVERS} && make
	cd ${UTIL} && make
	cd $(MEMORY) && make
	cd ${FILE_SYSTEM} && make
	cd ${USERS} && make
	cd ${EXEPARSER} && make
	cd ${SYSCALL} && make
	cd ${OSUTILS} && make
	cd ${TESTSUITE} && make 
#	cd ${LIBMTERM} && make
	cd ${LIBC} && make
	cd ${LIBMCPP} && make
	cd ${LIBCXXRT} && make
	cd ${LIBGCCEH} && make
	#cd ${LIBM} && make
	
	@make -f Makefile.act flat

elf:
	cd ${BOOT} && make boot_elf
	cd ${KERNEL} && make
	cd ${PROCESS} && make
	cd ${DISPLAY} && make
	cd ${KERNEL_PROCS} && make
	cd ${DRIVERS} && make
	cd ${UTIL} && make
	cd $(MEMORY) && make
	cd ${FILE_SYSTEM} && make
	cd ${USERS} && make
	cd ${EXEPARSER} && make
	cd ${SYSCALL} && make
	cd ${OSUTILS} && make
	cd ${TESTSUITE} && make 
#	cd ${LIBMTERM} && make
	cd ${LIBC} && make
	cd ${LIBMCPP} && make
	cd ${LIBCXXRT} && make
	cd ${LIBGCCEH} && make
	#cd ${LIBM} && make

	@make -f Makefile.act elf

grub-img:
	./build_load.sh
	
umount:
	./build_umount.sh

clean:
	cd ${BOOT} && make clean
	cd ${KERNEL} && make clean
	cd ${PROCESS} && make clean
	cd ${DISPLAY} && make clean	
	cd ${KERNEL_PROCS} && make clean	
	cd ${DRIVERS} && make clean	
	cd ${UTIL} && make clean
	cd ${MEMORY} && make clean
	cd ${FILE_SYSTEM} && make clean
	cd ${USERS} && make clean
	cd ${EXEPARSER} && make clean
	cd ${SYSCALL} && make clean
	cd ${OSUTILS} && make clean
	cd ${TESTSUITE} && make clean
#	cd ${LIBMTERM} && make clean
	cd ${LIBC} && make clean
	cd ${LIBMCPP} && make clean
	cd ${LIBCXXRT} && make clean
	cd ${LIBGCCEH} && make clean
	#cd ${LIBM} && make clean

distclean:
	cd ${BOOT} && make distclean
	cd ${KERNEL} && make distclean
	cd ${PROCESS} && make distclean
	cd ${DISPLAY} && make distclean	
	cd ${KERNEL_PROCS} && make distclean	
	cd ${DRIVERS} && make distclean	
	cd ${UTIL} && make distclean
	cd ${MEMORY} && make distclean
	cd ${FILE_SYSTEM} && make distclean
	cd ${USERS} && make distclean
	cd ${EXEPARSER} && make distclean
	cd ${SYSCALL} && make distclean
	cd ${OSUTILS} && make distclean
	cd ${TESTSUITE} && make distclean
#	cd ${LIBMTERM} && make distclean
	cd ${LIBC} && make distclean
	cd ${LIBMCPP} && make distclean
	cd ${LIBCXXRT} && make distclean
	cd ${LIBGCCEH} && make distclean
	#cd ${LIBM} && make distclean

	rm -f ${UPANIX_OUTPUT_FLAT}
	rm -f ${UPANIX_OUTPUT_AOUT}
