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
ASM = nasm

DLL_SRC = dll.asm 
DLL_OUTPUT = .dll

PROCINIT_SRC = proc_init.asm
PROCINIT_OUTPUT = .procinit

REALMODE_SRC = realmode.asm
REALMODE_OUTPUT = .realmode

osutils: ${DLL_OUTPUT} ${PROCINIT_OUTPUT} ${REALMODE_OUTPUT}

${DLL_OUTPUT}: ${DLL_SRC}
	${ASM} -fbin ${DLL_SRC} -o ${DLL_OUTPUT}

${PROCINIT_OUTPUT}: ${PROCINIT_SRC}
	${ASM} -fbin ${PROCINIT_SRC} -o ${PROCINIT_OUTPUT}

${REALMODE_OUTPUT}: ${REALMODE_SRC}
	${ASM} -fbin ${REALMODE_SRC} -o ${REALMODE_OUTPUT}

clean:
	rm -f *.o

distclean:
	rm -f *.o
	rm -f ${DLL_OUTPUT} ${PROCINIT_OUTPUT} ${REALMODE_OUTPUT}

