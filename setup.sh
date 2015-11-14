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

#set MOS_HOME to mos checkout directory path
export INCLUDE="-I./ \
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
-I${MOS_HOME}/libmcpp/mem \
-I${MOS_HOME}/libmcpp/cal"

export GLOBAL_HEADERS="$MOS_HOME/util/Global.h $MOS_HOME/util/AsmUtil.h"

export COMPILER=i686-elf-g++
export C_COMPILER=i686-elf-gcc

EXCEPTION_SUP="-fexceptions -frtti"
if [ "$#" -eq "1" ] && [ "$1" = "NE" ]
then	
	EXCEPTION_SUP="-fno-exceptions -fno-rtti"
fi

EH_FLAGS=""
if [ "$#" -eq "1" ] && [ "$1" = "EH" ]
then	
	EH_FLAGS=" -DIN_GCC -DIN_LIBGCC2 -Dinhibit_libc -fbuilding-libgcc -fno-stack-protector"
fi

export CPP_FLAGS=" -c -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc \
-std=c++11 -nostdinc++ -fno-threadsafe-statics -fpermissive ${EXCEPTION_SUP}"
export C_FLAGS=" -c -std=c11 -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc ${EH_FLAGS}"
