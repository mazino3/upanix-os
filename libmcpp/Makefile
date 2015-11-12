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
CXX = ${COMPILER}
INCLUDE = ${INCLUDES}
CPPFLAGS = ${CPP_FLAGS} ${INCLUDE}

MOS_LIBMCPP_OBJS := $(patsubst %.cpp,%.o,$(shell find . -name *.cpp))

build: subdir libmcpp.a

subdir:
	cd ds && make
	cd mem && make
	cd cal && make

libmcpp.a: ${MOS_LIBMCPP_OBJS}
	ar -r libmcpp.a ${MOS_LIBMCPP_OBJS}

clean:
	cd ds && make clean
	cd mem && make clean
	cd cal && make clean

distclean:
	cd ds && make distclean
	cd mem && make distclean
	cd cal && make distclean
	rm -f libmcpp.a
