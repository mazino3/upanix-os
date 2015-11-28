/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa_prajwal@yahoo.co.in'
 *                                                                          
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *                                                                          
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *                                                                          
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/
 */
#ifndef _MEM_CONSTANTS_H_
#define _MEM_CONSTANTS_H_

#define SYS_LINEAR_SELECTOR_DEFINED 0x8 
#define SYS_DATA_SELECTOR_DEFINED 0x18

extern unsigned int GLOBAL_DATA_SEGMENT_BASE ;

extern unsigned int SYS_CODE_SELECTOR ;
extern unsigned int SYS_LINEAR_SELECTOR ;
extern unsigned int SYS_DATA_SELECTOR ;
extern unsigned int SYS_TSS_SELECTOR ;
extern unsigned int USER_TSS_SELECTOR ;
extern unsigned int INT_TSS_SELECTOR_SV ;
extern unsigned int INT_TSS_SELECTOR_PF ;
extern unsigned int CALL_GATE_SELECTOR ;
extern unsigned int INT_GATE_SELECTOR ;

extern unsigned int SYS_TSS_LINEAR_ADDRESS ;
extern unsigned int USER_TSS_LINEAR_ADDRESS ;
extern unsigned int INT_TSS_LINEAR_ADDRESS_SV ;
extern unsigned int INT_TSS_LINEAR_ADDRESS_PF ;
extern unsigned int MULTIBOOT_INFO_ADDR ;
extern unsigned int CR0_CONTENT ;
extern byte CO_PROC_FPU_TYPE ;

extern unsigned RAM_SIZE ;

#define MEM_RESV_SIZE 0x1000000 // 16 MB
#define MEM_KERNEL_HEAP_SIZE 0x3000000 // 48 MB

#define PAGE_SIZE 4096 // 4 KB
#define PAGE_TABLE_ENTRIES 1024 // 1 KB
#define PAGE_TABLE_SIZE 4096 // 4 KB

#define MAX_PAGES_PER_PROCESS 0x400000 // 4 MB

#define MB * 1024 * 1024
#define KB * 1024

#define MEM_REAL_MODE_AREA_START	0x00000	
#define MEM_REAL_MODE_CODE			0x8000
#define MEM_REAL_MODE_AREA_END		0x20000

#define MEM_DMA_START			0x20000
#define MEM_DMA_FLOPPY_START	0x20000
#define MEM_DMA_FLOPPY_END		0x30000 // 65536 -> 64 KB
#define MEM_DMA_END				0x30000 

//#define MEM_DMA_ATA_PRD_START	0x20000 // 131072 -> 128 KB
//#define MEM_DMA_ATA_PRD_END		0x30000 // 196608 -> 192 KB

#define MEM_GDT_START		0x82000 // 532480 -> 520 KB
#define MEM_GDT_END			0x82800 // 534528 -> 522 KB

#define MEM_LDT_START		0x83000 // 534528 -> 524 KB
#define MEM_LDT_END			0x84800 // 542720 -> 530 KB

#define MEM_IDT_START		0x85000 // 542720 -> 532 KB
#define MEM_IDT_END			0x85800 // 544768 -> 534 KB

#define MEM_SYS_TSS_START	0x86000 // 545792 -> 536 KB
#define MEM_SYS_TSS_END		0x86800 // 546816 -> 538 KB

#define MEM_USER_TSS_START	0x87000 // 546816 -> 540 KB
#define MEM_USER_TSS_END	0x87400 // 547840 -> 541 KB

#define MEM_INT_TSS_START	0x88000 // 547840 -> 544 KB
#define MEM_INT_TSS_END		0x88400 // 548864 -> 545 KB

#define MEM_VIDEO_START		0xB8000 // 753664
#define MEM_VIDEO_END		0xB9400 // 0xB8000 + 0x1400 -> 5120 -> 5 KB

#define MEM_KERNEL_START	0x100000 // 1048576 -> 1 MB
#define MEM_KERNEL_END		0x2000000 // 33554432 -> 32 MB

/***** These addresses are Relative to Kernel Base ===> Their Phy Addr = Addr + Kernel Base ******/
#define MEM_PTE_START		0x300000 // 3 MB
#define MEM_PTE_END			0x700000 // 7 MB

#define MEM_PAGE_MAP_START	0x700000 // 7 MB
#define MEM_PAGE_MAP_END	0x720000 // 7 MB + 128 KB

#define MEM_PDE_START		0x720000 // 7 MB + 128 KB
#define MEM_PDE_END			0x721000 // 7 MB + 132 KB

#define MEM_PAS_START		0x721000 // 7 MB + 132 KB
#define MEM_PAS_END			0x821000 // 8 MB + 132 KB

#define MEM_PAGE_FAULT_HANDLER_STACK	0x828FFF // (8 MB + 132 KB + 8 * PAGE_SIZE - 1)
#define MEM_KERNEL_SERVICE_STACK		0x830FFF // (8 MB + 132 KB + 16 * PAGE_SIZE - 1)

/* File Descriptor Table -> Currently not used */
#define MEM_FDT_START		0x831000 // 8 MB + 196 KB
#define MEM_FDT_END			0x839000 // 8 MB + 228 KB

/* User Table */
#define MEM_USR_LIST_START	0x839000 // 8 MB + 228 KB
#define MEM_USR_LIST_END	0x83C000 // 8 MB + 240 KB

/* PGAS */
#define MEM_PGAS_START		0x83C000 // 8 MB + 240 KB
#define MEM_PGAS_END		0x83C400 // 8 MB + 241 KB

#define PROCESS_KERNEL_SHARE_SPACE		0x83C400 // 8 MB + 241 KB

// Mapped to Different Address Space
// Should be aligned to Page Boundary
#define PROCESS_DLL_PAGE_ADDR	0x83E000 // 8 MB + 248 KB 
#define PROCESS_SEC_HEADER_ADDR	0x83F000 // 8 MB + 252 KB
#define PROCESS_ENV_PAGE		0x840000 // 8 MB + 256 KB
#define PROCESS_VIDEO_BUFFER	0x841000 // 8 MB + 260 KB
#define PROCESS_FD_PAGE			0x842000 // 8 MB + 264 KB
#define EHCI_MMIO_BASE_ADDR		0x843000 // 8 MB + 268 KB
#define EHCI_MMIO_BASE_ADDR_END	0x863000 // 8 MB + 276 KB

/********** There is unused space between 8 MB + 241 KB + 6 * PAGE_SIZE to 9 MB. Can be used as required */

//#define MEM_HDD_DMA_START	0x900000 // 9 MB 
//#define MEM_HDD_DMA_END		0xA00000 // 10 MB

#define MEM_FD1_FS_START	0xA00000 // 10 MB
#define MEM_FD1_FS_END		0xA06400 // 10 MB + 25 KB

#define MEM_FD2_FS_START	0xA06400 // 10 MB + 25 KB
#define MEM_FD2_FS_END		0xA0C800 // 10 MB + 50 KB

#define MEM_CD_FS_START		0xA0C800 // 10 MB + 50 KB
#define MEM_CD_FS_END		0xA12C00 // 10 MB + 75 KB

#define MEM_HDD_FS_START	0xA19000 // 10 MB + 100 KB
#define MEM_HDD_FS_END		0xD00000 // 13 MB

#define MEM_USD_FS_START	0xD00000 // 13 MB
#define MEM_USD_FS_END		0x1000000 // 16 MB

#define PROCESS_HEAP_START_ADDRESS		0x80000000 // 2 GB
#define PROCESS_INIT_DOCK_ADDRESS		(PROCESS_BASE + 0x400000)

#endif
