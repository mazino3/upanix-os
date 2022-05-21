/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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

extern uint32_t GLOBAL_DATA_SEGMENT_BASE ;

extern uint32_t SYS_CODE_SELECTOR ;
extern uint32_t SYS_LINEAR_SELECTOR ;
extern uint32_t SYS_DATA_SELECTOR ;
extern uint32_t SYS_TSS_SELECTOR ;
extern uint32_t USER_TSS_SELECTOR ;
extern uint32_t INT_TSS_SELECTOR_SV ;
extern uint32_t INT_TSS_SELECTOR_PF ;
extern uint32_t CALL_GATE_SELECTOR ;
extern uint32_t INT_GATE_SELECTOR ;

extern uint32_t MULTIBOOT_INFO_ADDR ;
extern uint32_t CR0_CONTENT ;
extern byte CO_PROC_FPU_TYPE ;

extern uint32_t GDT_BASE_ADDR;
extern uint32_t LDT_BASE_ADDR;
extern uint32_t IDT_BASE_ADDR;
extern uint32_t SYS_TSS_BASE_ADDR;
extern uint32_t USER_TSS_BASE_ADDR;

#define MB * 1024 * 1024
#define KB * 1024

#define MEM_GRAPHICS_VIDEO_MAP_SIZE 0x1000000 // 16 MB
#define MEM_KERNEL_HEAP_START 0x4000000 // 32 MB + MEM_GRAPHICS_VIDEO_MAP_SIZE + MEM_GRAPHICS_Z_BUFFER_SIZE = 64 MB
#define MEM_KERNEL_HEAP_SIZE 0x3000000 // 48 MB
#define MEM_KERNEL_PAGE_POOL_SIZE 0x1000000 // 16 MB
#define MEM_KERNEL_RESV_SIZE 0x8000000u // MEM_KERNEL_HEAP_START + MEM_KERNEL_HEAP_SIZE + MEM_KERNEL_PAGE_POOL_SIZE

#define PAGE_SIZE 4096u // 4 KB
#define PAGE_TABLE_ENTRIES 1024u // 1 KB
#define PAGE_TABLE_SIZE 4096 // 4 KB

#define MAX_PAGES_PER_PROCESS 0x400000 // 4 MB

#define MEM_REAL_MODE_AREA_START	0x00000	
#define MEM_REAL_MODE_CODE			0x8000
#define MEM_REAL_MODE_AREA_END		0x20000

#define MEM_DMA_START			0x20000
#define MEM_DMA_FLOPPY_START	0x20000
#define MEM_DMA_FLOPPY_END		0x30000 // 65536 -> 64 KB
#define MEM_DMA_END				0x30000 

#define MEM_VIDEO_START		0xB8000 // 753664
#define MEM_VIDEO_END		0xB9400 // 0xB8000 + 0x1400 -> 5120 -> 5 KB

#define MEM_KERNEL_START	0x100000 // 1048576 -> 1 MB
#define MEM_KERNEL_END		0x8000000 // 33554432 -> 32 MB

/***** These addresses are Relative to Kernel Base ===> Their Phy Addr = Addr + Kernel Base ******/
#define MEM_PTE_START		0x1000000 // 16 MB
#define MEM_PTE_END			0x1400000 // 20 MB

#define MEM_PAGE_MAP_START	0x1400000 // 20 MB
#define MEM_PAGE_MAP_END	0x1420000 // 20 MB + 128 KB

#define MEM_PDE_START		0x1420000 // 20 MB + 128 KB
#define MEM_PDE_END			0x1421000 // 20 MB + 132 KB

#define MEM_PAS_START		0x1421000 // 20 MB + 132 KB
#define MEM_PAS_END			0x1521000 // 21 MB + 132 KB

#define MEM_PAGE_FAULT_HANDLER_STACK	0x1528FFF // (21 MB + 132 KB + 8 * PAGE_SIZE - 1)
#define MEM_KERNEL_SERVICE_STACK		0x1530FFF // (21 MB + 132 KB + 16 * PAGE_SIZE - 1)

/* kernel page heap/pool */
#define MEM_KERNEL_PAGE_POOL_MAP_START 0x1531000 // 21 MB + 196 KB
#define MEM_KERNEL_PAGE_POOL_MAP_END   0x1533000 // 21 MB + 204 KB

#define PROCESS_KERNEL_SHARE_SPACE		0x153C400 // 21 MB + 241 KB

// Mapped to Different Address Space
// Should be aligned to Page Boundary
#define PROCESS_SEC_HEADER_ADDR	0x153F000 // 21 MB + 252 KB
#define PROCESS_ENV_PAGE		0x1540000 // 21 MB + 256 KB
#define PROCESS_VIDEO_BUFFER	0x1541000 // 21 MB + 260 KB

#define EHCI_MMIO_BASE_ADDR		0x1543000 // 21 MB + 268 KB
#define EHCI_MMIO_BASE_ADDR_END	0x1573000 // 48 pages
#define XHCI_MMIO_BASE_ADDR		0x1573000 // 21 MB + 268 KB + 48 pages
#define XHCI_MMIO_BASE_ADDR_END	0x15B3000 // 64 pages

#define MMAP_APIC_BASE 0x15BC000 // 12 MB + 187 pages + 1 page
#define MMAP_IOAPIC_BASE 0x15BD000 // 12 MB + 188 pages + 1 page

#define MEM_GRAPHICS_TEXT_BUFFER_START 0x1600000 // 22 MB
#define MEM_GRAPHICS_TEXT_BUFFER_END   0x160C800 // 22 MB + 50 KB

#define NET_ATH9K_MMIO_BASE_ADDR 0x1619000 // 22 MB + 100 KB
#define NET_ATH9K_MMIO_BASE_ADDR_END 0x1659000 // 22 MB + 100 KB + 64 pages
#define NET_E1000_MMIO_BASE_ADDR 0x1659000 // 22 MB + 100 KB  + 64 pages
#define NET_E1000_MMIO_BASE_ADDR_END 0x1699000 // + 64 pages

#define MEM_FD1_FS_START	0x1700000 // 23 MB
#define MEM_FD1_FS_END		0x1706400 // 23 MB + 25 KB

#define MEM_FD2_FS_START	0x1706400 // 23 MB + 25 KB
#define MEM_FD2_FS_END		0x170C800 // 23 MB + 50 KB

#define MEM_CD_FS_START		0x170C800 // 23 MB + 50 KB
#define MEM_CD_FS_END		0x1712C00 // 23 MB + 75 KB

#define MEM_HDD_FS_START	0x1719000 // 23 MB + 100 KB
#define MEM_HDD_FS_END		0x1A00000 // 26 MB

#define MEM_USD_FS_START	0x1A00000 // 26 MB
#define MEM_USD_FS_END		0x1D00000 // 29 MB

#define MEM_GRAPHICS_VIDEO_MAP_START 0x2000000 // 32 MB
#define MEM_GRAPHICS_Z_BUFFER_START	0x3000000 // 48 MB

#define PROCESS_HEAP_START_ADDRESS		0x80000000 // 2 GB
#define PROCESS_HEAP_SIZE		0x40000000 // 1 GB

#endif