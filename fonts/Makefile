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
LD = ${HOME}/opt/cross/i686-elf/bin/ld

#fonts_FreeSans.sfn  u_vga16.o  u_vga16.sfn  unifont.o  unifont.sfn
FONT_FREE_SANS = fonts_FreeSans.sfn
FONT_U_VGA16 = u_vga16.sfn
FONT_UNIFONT = unifont.sfn

FONT_FREE_SANS_OBJ = fonts_FreeSans.o 
FONT_U_VGA16_OBJ = u_vga16.o
FONT_UNIFONT_OBJ = unifont.o

build: ${FONT_FREE_SANS_OBJ} ${FONT_U_VGA16_OBJ} ${FONT_UNIFONT_OBJ}

${FONT_FREE_SANS_OBJ}: ${FONT_FREE_SANS}
	${LD} -r -b binary -o ${FONT_FREE_SANS_OBJ} ${FONT_FREE_SANS}

${FONT_U_VGA16_OBJ}: ${FONT_U_VGA16}
	${LD} -r -b binary -o ${FONT_U_VGA16_OBJ} ${FONT_U_VGA16}

${FONT_UNIFONT_OBJ}: ${FONT_UNIFONT}
	${LD} -r -b binary -o ${FONT_UNIFONT_OBJ} ${FONT_UNIFONT}

#readelf -s ${FONT_FREE_SANS_OBJ}

clean:
	rm -f *.o

distclean:
	rm -f *.o
