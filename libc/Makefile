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
CXX = ${COMPILER}
CXXFLAGS = ${CPP_FLAGS} ${INCLUDE}

UPANIX_LIBC_OBJS := $(patsubst %.cpp,%.o,$(shell find . -name *.cpp))

build: subdir libc.a

subdir:
	cd ctype && make
	cd string && make
	cd sys && make
	cd stdio && make
	cd stdlib && make

libc.a: ${UPANIX_LIBC_OBJS}
	ar -r libc.a ${UPANIX_LIBC_OBJS}

clean:
	cd ctype && make clean
	cd string && make clean
	cd sys && make clean
	cd stdio && make clean
	cd stdlib && make clean

distclean:
	cd ctype && make distclean
	cd string && make distclean
	cd sys && make distclean
	cd stdio && make distclean
	cd stdlib && make distclean
	rm -f libc.a
