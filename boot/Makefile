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
ASM = nasm

BIN = ${UPANIX_HOME}/bin
BOOT_SRC = Boot.asm
BOOT_OUTPUT = ${BIN}/Boot.bin
SETUP_SRC = Setup.asm
SETUP_OUTPUT = ${BIN}/Setup.bin
UPANIX_START_SRC = UpanixStart.asm
UPANIX_START_OBJ = UpanixStart.self
UPANIX_START_MAIN_SRC = UpanixStart_Main.asm
UPANIX_START_MAIN_OBJ = UpanixStart_Main.grub
FPU_INIT_SRC = fpu_init.asm
FPU_INIT_OBJ = fpu_init.obj

boot_flat: ${BOOT_OUTPUT} ${SETUP_OUTPUT} ${UPANIX_START_OBJ}

${BOOT_OUTPUT}: ${BOOT_SRC}
	${ASM} -fbin ${BOOT_SRC} -o ${BOOT_OUTPUT}

${SETUP_OUTPUT}: ${SETUP_SRC}
	${ASM} -fbin ${SETUP_SRC} -o ${SETUP_OUTPUT}
	
${UPANIX_START_OBJ}: ${UPANIX_START_SRC}
	${ASM} -faout ${UPANIX_START_SRC} -o ${UPANIX_START_OBJ}

boot_elf: ${UPANIX_START_MAIN_OBJ} ${FPU_INIT_OBJ}

${FPU_INIT_OBJ}: ${FPU_INIT_SRC}
	${ASM} -felf ${FPU_INIT_SRC} -o ${FPU_INIT_OBJ}

${UPANIX_START_MAIN_OBJ}: ${UPANIX_START_MAIN_SRC}
	${ASM} -felf ${UPANIX_START_MAIN_SRC} -o ${UPANIX_START_MAIN_OBJ}

clean:
	rm -f *.o
	rm -f ${FPU_INIT_OBJ}
	rm -f ${UPANIX_START_MAIN_OBJ}
	rm -f ${UPANIX_START_OBJ}

distclean:
	rm -f *.o
	rm -f ${FPU_INIT_OBJ}
	rm -f ${UPANIX_START_MAIN_OBJ}
	rm -f ${UPANIX_START_OBJ}
	rm -f ${BOOT_OUTPUT} ${SETUP_OUTPUT}

