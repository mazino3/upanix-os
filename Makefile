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
CXX = i686-elf-g++

MOS_HOME = $(shell pwd)

#LIBMTERM = ${MOS_HOME}/libmterm
INCLUDE = -I./ \
-I${MOS_HOME}/bin -I${MOS_HOME}/kernel \
-I${MOS_HOME}/process \
-I${MOS_HOME}/display \
\
-I${MOS_HOME}/kernelprocs/console \
-I${MOS_HOME}/kernelprocs/session \
-I${MOS_HOME}/kernelprocs \
\
-I${MOS_HOME}/drivers/floppy \
-I${MOS_HOME}/drivers/keyboard \
-I${MOS_HOME}/drivers/mouse \
-I${MOS_HOME}/drivers/bus \
-I${MOS_HOME}/drivers/ide \
-I${MOS_HOME}/drivers/ide/vendorspec \
-I${MOS_HOME}/drivers/usb/ \
-I${MOS_HOME}/drivers/usb/disk \
-I${MOS_HOME}/drivers/net \
-I${MOS_HOME}/drivers/ \
\
-I${MOS_HOME}/util \
-I${MOS_HOME}/memory \
-I${MOS_HOME}/filesystem \
-I${MOS_HOME}/users \
-I${MOS_HOME}/exeparser \
-I${MOS_HOME}/syscall \
-I${MOS_HOME}/resource \
\
-I${MOS_HOME}/testsuite \
\
-I${MOS_HOME}/libc/include \
-I${MOS_HOME}/libc/sysdeps/mos/common/ \
-I${MOS_HOME}/libc/sysdeps/mos/common/bits \
\
-I${MOS_HOME}/libm/include \
-I${MOS_HOME}/libm/include/bits \
\
-I${MOS_HOME}/libmcpp/include \
-I${MOS_HOME}/libmcpp/ds \
-I${MOS_HOME}/libmcpp/mem

#CPP_FLAGS =  -c -O2 -Wall -Wextra -ffreestanding -nodefaultlibs -nostdlib -nostartfiles \
#						-fno-threadsafe-statics -fno-exceptions -fno-rtti -fpermissive ${INCLUDE}
CPP_FLAGS= -c -O0 -std=c++11 -Wall -Wextra -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc \
-nostdinc++ -fno-default-inline -fno-common -fno-non-call-exceptions -fno-exceptions -fno-rtti \
-fno-threadsafe-statics -fpermissive ${INCLUDE}
export CPP_FLAGS
export CXX
export MOS_HOME

BIN = ${MOS_HOME}/bin
BOOT = ${MOS_HOME}/boot
KERNEL = ${MOS_HOME}/kernel
PROCESS = ${MOS_HOME}/process
DISPLAY = ${MOS_HOME}/display
KERNEL_PROCS = ${MOS_HOME}/kernelprocs
DRIVERS = ${MOS_HOME}/drivers
UTIL = ${MOS_HOME}/util
MEMORY = ${MOS_HOME}/memory
FILE_SYSTEM = ${MOS_HOME}/filesystem
RESOURCE = ${MOS_HOME}/resource
USERS = ${MOS_HOME}/users
EXEPARSER = ${MOS_HOME}/exeparser
SYSCALL = ${MOS_HOME}/syscall
OSUTILS = ${MOS_HOME}/osutils
TESTSUITE = ${MOS_HOME}/testsuite
LIBC = ${MOS_HOME}/libc
LIBMCPP = ${MOS_HOME}/libmcpp
LIBM = ${MOS_HOME}/libm

FLOPPY_IMG = ${MOS_HOME}/floppy/1.44M-FLOPPY.img
FLOPPY_DEV = /dev/fd0

BOOT_OUTPUT = ${BIN}/Boot.bin
SETUP_OUTPUT = ${BIN}/Setup.bin
MOS_OUTPUT_FLAT = ${BIN}/MOS.bin
MOS_OUTPUT_AOUT = ${BIN}/MOS.aout

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
	cd ${RESOURCE} && make
	cd ${USERS} && make
	cd ${EXEPARSER} && make
	cd ${SYSCALL} && make
	cd ${OSUTILS} && make
	cd ${TESTSUITE} && make 
#	cd ${LIBMTERM} && make
	cd ${LIBC} && ./run_make.sh
	cd ${LIBMCPP} && ./run_make.sh
	#cd ${LIBM} && make
	
	@make -f Makefile.act flat

aout:
	cd ${BOOT} && make boot_aout
	cd ${KERNEL} && make
	cd ${PROCESS} && make
	cd ${DISPLAY} && make
	cd ${KERNEL_PROCS} && make
	cd ${DRIVERS} && make
	cd ${UTIL} && make
	cd $(MEMORY) && make
	cd ${FILE_SYSTEM} && make
	cd ${RESOURCE} && make
	cd ${USERS} && make
	cd ${EXEPARSER} && make
	cd ${SYSCALL} && make
	cd ${OSUTILS} && make
	cd ${TESTSUITE} && make 
#	cd ${LIBMTERM} && make
	cd ${LIBC} && ./run_make.sh
	cd ${LIBMCPP} && ./run_make.sh
	#cd ${LIBM} && make

	@make -f Makefile.act aout

image:
	creatboot ${BOOT_OUTPUT} ${FLOPPY_IMG}
	writesec ${SETUP_OUTPUT} ${FLOPPY_IMG} 3
	writesec ${MOS_OUTPUT_FLAT} ${FLOPPY_IMG} 5

floppy-img:
	creatboot ${BOOT_OUTPUT} ${FLOPPY_DEV}
	writesec ${SETUP_OUTPUT} ${FLOPPY_DEV} 3
	writesec ${MOS_OUTPUT_FLAT} ${FLOPPY_DEV} 5

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
	cd ${RESOURCE} && make clean
	cd ${USERS} && make clean
	cd ${EXEPARSER} && make clean
	cd ${SYSCALL} && make clean
	cd ${OSUTILS} && make clean
	cd ${TESTSUITE} && make clean
#	cd ${LIBMTERM} && make clean
	cd ${LIBC} && ./run_make.sh clean
	cd ${LIBMCPP} && ./run_make.sh clean
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
	cd ${RESOURCE} && make distclean
	cd ${USERS} && make distclean
	cd ${EXEPARSER} && make distclean
	cd ${SYSCALL} && make distclean
	cd ${OSUTILS} && make distclean
	cd ${TESTSUITE} && make distclean
#	cd ${LIBMTERM} && make distclean
	cd ${LIBC} && ./run_make.sh distclean
	cd ${LIBMCPP} && ./run_make.sh distclean
	#cd ${LIBM} && make distclean

	rm -f ${MOS_OUTPUT_FLAT}
	rm -f ${MOS_OUTPUT_AOUT}
