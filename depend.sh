INCLUDE="-I./ \
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
\
-I${MOS_HOME}/libstdc++/include/ \
-I${MOS_HOME}/libstdc++/include/std"

# -D_GLIBCXX_PROFILE

export CXX=i686-elf-g++
export CXXFLAGS=" -c -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc \
-std=c++11 -nostdinc++ -fexceptions -frtti \
-fno-threadsafe-statics -fpermissive"

> depend.d
for i in `ls *.cpp 2> /dev/null`
do
	obj=${i%.*}.o
	$CXX ${INCLUDE} -M $i >> depend.d
	echo "\t@echo \"compiling $i...\"" >> depend.d
	echo "\t@$CXX ${CXXFLAGS} ${INCLUDE} $i -o ${obj}" >> depend.d
done

export CC=i686-elf-gcc
export CFLAGS=" -c -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc"

for i in `ls *.c 2> /dev/null`
do
	obj=${i%.*}.o
	$CC ${INCLUDE} -M $i >> depend.d
	echo "\t@echo \"compiling $i...\"" >> depend.d
	echo "\t@$CC ${CFLAGS} ${INCLUDE} $i -o ${obj}" >> depend.d
done
