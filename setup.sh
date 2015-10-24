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

MOS_HOME=`pwd`
export MOS_HOME

INCLUDES="-I./ \
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
-I${MOS_HOME}/libmcpp/mem"

export INCLUDES

GLOBAL_HEADERS="$MOS_HOME/util/Global.h $MOS_HOME/util/AsmUtil.h"
export GLOBAL_HEADERS

COMPILER=g++
export COMPILER

CPP_FLAGS=" -c -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc \
-nostdinc++ -fno-default-inline -fno-common -fno-non-call-exceptions -fno-exceptions -fno-rtti \
-fno-threadsafe-statics -m32 "
export CPP_FLAGS

#C_FLAGS=" -c -O2 -Wall -ffreestanding -pedantic "
#C_FLAGS=" -c -O1 -mtune=i386 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc "  # For compiling with gcc 4. & above

#dd if=/dev/zero of=DRV1.FDD.img bs=512 count=2880
#dd if=/dev/zero of=30MHD.img bs=512 count=62730

#dd if=/usr/lib/grub/stage1 of=floppy/1.44M-FLOPPY.img bs=512 count=1
#dd if=/usr/lib/grub/stage2 of=floppy/1.44M-FLOPPY.img bs=512 seek=1

#. use_local_gcc_3.3.4.sh
#for i in `find . -name "*.c"`; do echo $i; sed 's/boolean/bool/g' $i > 1; mv 1 $i; done
#find . -name "Makefile" -exec perl -p -i -e 's/C_FLAGS/CPP_FLAGS/g' {} \;
