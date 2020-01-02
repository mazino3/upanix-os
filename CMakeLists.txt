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

set(CROSS_TOOL_PATH $ENV{HOME}/opt/cross)

set(CMAKE_CXX_COMPILER ${CROSS_TOOL_PATH}/bin/i686-elf-g++)
set(CMAKE_C_COMPILER ${CROSS_TOOL_PATH}/bin/i686-elf-gcc)
set(CMAKE_LINKER ${CROSS_TOOL_PATH}/i686-elf/bin/ld)
set(CMAKE_AR ${CROSS_TOOL_PATH}/i686-elf/bin/ar)

set(ENABLE_CXX_RT ON)
	
set(EXCEPTION_SUP " -fno-exceptions -fno-rtti")
if(ENABLE_CXX_RT)
	set(EXCEPTION_SUP " -fexceptions -frtti")
endif()

set(CMAKE_CXX_FLAGS " -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc -std=c++11 -nostdinc++ -fno-threadsafe-statics -fpermissive ${EXCEPTION_SUP}")
set(CMAKE_C_FLAGS " -std=c11 -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc")
set(CMAKE_EXE_LINKER_FLAGS " -Tkernel.ld")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${PROJECT_SOURCE_DIR}/bin)
include_directories(${PROJECT_SOURCE_DIR}/kernel)
include_directories(${PROJECT_SOURCE_DIR}/process)
include_directories(${PROJECT_SOURCE_DIR}/display)

include_directories(${PROJECT_SOURCE_DIR}/kernelprocs/console)
include_directories(${PROJECT_SOURCE_DIR}/kernelprocs/session)
include_directories(${PROJECT_SOURCE_DIR}/kernelprocs)

include_directories(${PROJECT_SOURCE_DIR}/drivers/floppy)
include_directories(${PROJECT_SOURCE_DIR}/drivers/keyboard)
include_directories(${PROJECT_SOURCE_DIR}/drivers/mouse)
include_directories(${PROJECT_SOURCE_DIR}/drivers/bus)
include_directories(${PROJECT_SOURCE_DIR}/drivers/ide)
include_directories(${PROJECT_SOURCE_DIR}/drivers/ide/vendorspec)
include_directories(${PROJECT_SOURCE_DIR}/drivers/usb/)
include_directories(${PROJECT_SOURCE_DIR}/drivers/usb/uhci)
include_directories(${PROJECT_SOURCE_DIR}/drivers/usb/ehci)
include_directories(${PROJECT_SOURCE_DIR}/drivers/usb/xhci)
include_directories(${PROJECT_SOURCE_DIR}/drivers/usb/devices)
include_directories(${PROJECT_SOURCE_DIR}/drivers/video)
include_directories(${PROJECT_SOURCE_DIR}/drivers/net)
include_directories(${PROJECT_SOURCE_DIR}/drivers/net/mac80211)
include_directories(${PROJECT_SOURCE_DIR}/drivers/net/ath9k)
include_directories(${PROJECT_SOURCE_DIR}/drivers/)

include_directories(${PROJECT_SOURCE_DIR}/util)
include_directories(${PROJECT_SOURCE_DIR}/memory)
include_directories(${PROJECT_SOURCE_DIR}/filesystem)
include_directories(${PROJECT_SOURCE_DIR}/users)
include_directories(${PROJECT_SOURCE_DIR}/exeparser)
include_directories(${PROJECT_SOURCE_DIR}/syscall)
include_directories(${PROJECT_SOURCE_DIR}/resource)

include_directories(${PROJECT_SOURCE_DIR}/testsuite)

include_directories(${PROJECT_SOURCE_DIR}/libc/include)
include_directories(${PROJECT_SOURCE_DIR}/libc/stdio)
include_directories(${PROJECT_SOURCE_DIR}/libc/sysdeps/mos/common/)
include_directories(${PROJECT_SOURCE_DIR}/libc/sysdeps/mos/common/bits)

include_directories(${PROJECT_SOURCE_DIR}/libmcpp/infra)
include_directories(${PROJECT_SOURCE_DIR}/libmcpp/ds)
include_directories(${PROJECT_SOURCE_DIR}/libmcpp/mem)
include_directories(${PROJECT_SOURCE_DIR}/libmcpp/cal)

include_directories(${PROJECT_SOURCE_DIR}/libcxxrt)

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
