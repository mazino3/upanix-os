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
cmake_minimum_required(VERSION 3.10)
project(Upanix)

set(TARGET_BIN "upanix.elf")

set(CMAKE_CXX_COMPILER i686-elf-g++)
set(CMAKE_C_COMPILER i686-elf-gcc)

set(ENABLE_CXX_RT ON)
	
set(EXCEPTION_SUP " -fno-exceptions -fno-rtti")
if(ENABLE_CXX_RT)
	set(EXCEPTION_SUP " -fexceptions -frtti")
endif()

set(CMAKE_CXX_FLAGS " -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc -std=c++11 -nostdinc++ -fno-threadsafe-statics -fpermissive ${EXCEPTION_SUP}")
set(CMAKE_C_FLAGS " -std=c11 -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc")
set(CMAKE_EXE_LINKER_FLAGS " -Tkernel.ld")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories($ENV{UPANIX_HOME}/bin)
include_directories($ENV{UPANIX_HOME}/kernel)
include_directories($ENV{UPANIX_HOME}/process)
include_directories($ENV{UPANIX_HOME}/display)

include_directories($ENV{UPANIX_HOME}/kernelprocs/console)
include_directories($ENV{UPANIX_HOME}/kernelprocs/session)
include_directories($ENV{UPANIX_HOME}/kernelprocs)

include_directories($ENV{UPANIX_HOME}/drivers/floppy)
include_directories($ENV{UPANIX_HOME}/drivers/keyboard)
include_directories($ENV{UPANIX_HOME}/drivers/mouse)
include_directories($ENV{UPANIX_HOME}/drivers/bus)
include_directories($ENV{UPANIX_HOME}/drivers/ide)
include_directories($ENV{UPANIX_HOME}/drivers/ide/vendorspec)
include_directories($ENV{UPANIX_HOME}/drivers/usb/)
include_directories($ENV{UPANIX_HOME}/drivers/usb/uhci)
include_directories($ENV{UPANIX_HOME}/drivers/usb/ehci)
include_directories($ENV{UPANIX_HOME}/drivers/usb/xhci)
include_directories($ENV{UPANIX_HOME}/drivers/usb/devices)
include_directories($ENV{UPANIX_HOME}/drivers/video)
include_directories($ENV{UPANIX_HOME}/drivers/net)
include_directories($ENV{UPANIX_HOME}/drivers/net/mac80211)
include_directories($ENV{UPANIX_HOME}/drivers/net/ath9k)
include_directories($ENV{UPANIX_HOME}/drivers/)

include_directories($ENV{UPANIX_HOME}/util)
include_directories($ENV{UPANIX_HOME}/memory)
include_directories($ENV{UPANIX_HOME}/filesystem)
include_directories($ENV{UPANIX_HOME}/users)
include_directories($ENV{UPANIX_HOME}/exeparser)
include_directories($ENV{UPANIX_HOME}/syscall)
include_directories($ENV{UPANIX_HOME}/resource)

include_directories($ENV{UPANIX_HOME}/testsuite)

include_directories($ENV{UPANIX_HOME}/libc/include)
include_directories($ENV{UPANIX_HOME}/libc/stdio)
include_directories($ENV{UPANIX_HOME}/libc/sysdeps/mos/common/)
include_directories($ENV{UPANIX_HOME}/libc/sysdeps/mos/common/bits)

include_directories($ENV{UPANIX_HOME}/libmcpp/infra)
include_directories($ENV{UPANIX_HOME}/libmcpp/ds)
include_directories($ENV{UPANIX_HOME}/libmcpp/mem)
include_directories($ENV{UPANIX_HOME}/libmcpp/cal)

include_directories($ENV{UPANIX_HOME}/libcxxrt)

add_subdirectory(libgcceh)
add_subdirectory(libc)
add_subdirectory(libcxxrt)
add_subdirectory(libmcpp)

add_custom_target(osutils COMMAND make WORKING_DIRECTORY osutils)
add_custom_target(boot COMMAND make boot_elf WORKING_DIRECTORY boot)
add_custom_target(mem COMMAND make WORKING_DIRECTORY memory)

execute_process(COMMAND bash ./upanix_src_files.sh OUTPUT_VARIABLE UPANIX_SRCS)
string(REPLACE "\n" ";" UPANIX_SRCS ${UPANIX_SRCS})

execute_process(COMMAND bash -c "${CMAKE_CXX_COMPILER} -print-file-name=crtbegin.o" OUTPUT_VARIABLE CRTBEGIN_OBJ)
string(REPLACE "\n" "" CRTBEGIN_OBJ ${CRTBEGIN_OBJ})
execute_process(COMMAND bash -c "${CMAKE_CXX_COMPILER} -print-file-name=crtend.o" OUTPUT_VARIABLE CRTEND_OBJ)
string(REPLACE "\n" "" CRTEND_OBJ ${CRTEND_OBJ})

set(CRTINIT_OBJ "./osutils/crti.o ${CRTBEGIN_OBJ}")
set(CRTFINI_OBJ "${CRTEND_OBJ} ./osutils/crtn.o")

SET(OTHER_OBJS memory/Mem.o)

set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} ${CMAKE_EXE_LINKER_FLAGS} boot/fpu_init.obj boot/UpanixStart_Main.grub ${CRTINIT_OBJ} <OBJECTS> -o <TARGET> <LINK_LIBRARIES> ${CRTFINI_OBJ}")

add_executable(${TARGET_BIN} ${OTHER_OBJS} ${UPANIX_SRCS})

add_dependencies(${TARGET_BIN} osutils)
add_dependencies(${TARGET_BIN} boot)

target_link_libraries(${TARGET_BIN} cxxrt)
target_link_libraries(${TARGET_BIN} mcpp)
target_link_libraries(${TARGET_BIN} c)
target_link_libraries(${TARGET_BIN} gcceh)

set_source_files_properties(${OTHER_OBJS} PROPERTIES EXTERNAL_OBJECT true GENERATED true)
set_target_properties(${TARGET_BIN} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
