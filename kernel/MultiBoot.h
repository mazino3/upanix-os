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
#ifndef _MULTI_BOOT_H_
#define _MULTI_BOOT_H_

#include <Global.h>
#include <stdio.h>

/**** These Multi boot structures are taken from GRUB multiboot.h ****/

/* The Multiboot header.  */
typedef struct
{
  unsigned long magic;
  unsigned long flags;
  unsigned long checksum;
  unsigned long header_addr;
  unsigned long load_addr;
  unsigned long load_end_addr;
  unsigned long bss_end_addr;
  unsigned long entry_addr;
} PACKED multiboot_header_t;

/* The symbol table for a.out.  */
typedef struct
{
  unsigned tabsize;
  unsigned strsize;
  unsigned addr;
  unsigned reserved;
} PACKED aout_symbol_table_t;

/* The section header table for ELF.  */
typedef struct
{
  unsigned num;
  unsigned size;
  unsigned addr;
  unsigned shndx;
} PACKED elf_section_header_table_t;

typedef struct 
{
  unsigned vbe_control_info;
  unsigned vbe_mode_info;
  unsigned short vbe_mode;
  unsigned short vbe_interface_seg;
  unsigned short vbe_interface_off;
  unsigned short vbe_interface_len;
} PACKED vbe_info_t;

typedef struct 
{
  unsigned long long framebuffer_addr;
  unsigned framebuffer_pitch;
  unsigned framebuffer_width;
  unsigned framebuffer_height;
  byte framebuffer_bpp;
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED 0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB     1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT 2
  byte framebuffer_type;
  union
  {
    struct
    {
      unsigned framebuffer_palette_addr;
      unsigned short framebuffer_palette_num_colors;
    } PACKED;
    struct
    {
      byte framebuffer_red_field_position;
      byte framebuffer_red_mask_size;
      byte framebuffer_green_field_position;
      byte framebuffer_green_mask_size;
      byte framebuffer_blue_field_position;
      byte framebuffer_blue_mask_size;
    } PACKED;
  } PACKED;
} PACKED framebuffer_info_t;

/* The Multiboot information.  */
typedef struct
{
  unsigned flags;
  unsigned mem_lower;
  unsigned mem_upper;
  char boot_device[4];
  unsigned cmdline;
  unsigned mods_count;
  unsigned mods_addr;
  union
  {
    aout_symbol_table_t aout_sym;
    elf_section_header_table_t elf_sec;
  } PACKED u;
  unsigned mmap_length;
  unsigned mmap_addr;
  
  unsigned drives_length;
  unsigned drives_addr;

  unsigned  config_table;
  unsigned  boot_loader_name;

  unsigned apm_table;

  vbe_info_t vbe_info;

  framebuffer_info_t framebuffer_info;
} PACKED multiboot_info_t;

/* The module structure.  */
typedef struct module
{
  unsigned long mod_start;
  unsigned long mod_end;
  unsigned long string;
  unsigned long reserved;
} PACKED module_t;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size.  */
typedef struct memory_map
{
  uint32_t size;
  uint64_t base_addr;
  uint64_t length;
  uint32_t type;
} PACKED memory_map_t;

/********************** These are UPANIX Specific declarations ***********************/

class MultiBoot
{
	private:
		MultiBoot();
	public:
		static MultiBoot& Instance()
		{
			static MultiBoot instance;
			return instance;
		}
    //GetRamSize - for 32bit OS - with a cap of 4 GB
		unsigned GetRamSize() { return _ramSize; }
    uint64_t GetRealRamSize() { return _realRamSize; }
		byte GetBootDeviceID();
		byte GetBootPartitionID();
    const framebuffer_info_t* VideoFrameBufferInfo() const;
    const memory_map_t* GetACPIInfoMemMap() const;
    void Print();
	private:
    static const int MAX_MMAP_ENTRIES = 20;

		multiboot_info_t _info;
    memory_map_t     _mmap[MAX_MMAP_ENTRIES];
    uint64_t         _realRamSize;
    unsigned         _ramSize;
};

#endif 
