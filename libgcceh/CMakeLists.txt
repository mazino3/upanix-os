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

set(GCCEH_DIR ${PROJECT_SOURCE_DIR}/libgcceh)

string(APPEND CMAKE_C_FLAGS " -DIN_GCC -DIN_LIBGCC2 -Dinhibit_libc -fbuilding-libgcc -fno-stack-protector")

execute_process(COMMAND bash -c "cd ${GCCEH_DIR}; find . -name '*.c' | grep -v CMakeFile" OUTPUT_VARIABLE GCCEH_SRC)
string(REPLACE "\n" ";" GCCEH_SRC ${GCCEH_SRC})

add_library(gcceh STATIC "${GCCEH_SRC}")
