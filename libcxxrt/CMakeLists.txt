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

set(CXXRT_DIR ${PROJECT_SOURCE_DIR}/libcxxrt)

execute_process(COMMAND bash -c "cd ${CXXRT_DIR}; find . -name '*.c' | grep -v CMakeFile" OUTPUT_VARIABLE CXXRT_SRC1)
string(REPLACE "\n" ";" CXXRT_SRC1 ${CXXRT_SRC1})

execute_process(COMMAND bash -c "cd ${CXXRT_DIR}; find . -name '*.cc' | grep -v CMakeFile" OUTPUT_VARIABLE CXXRT_SRC2)
string(REPLACE "\n" ";" CXXRT_SRC2 ${CXXRT_SRC2})

add_library(cxxrt STATIC ${CXXRT_SRC1} ${CXXRT_SRC2})
