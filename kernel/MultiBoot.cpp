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
#include <MultiBoot.h>
#include <MemConstants.h>
#include <PortCom.h>

MultiBoot::MultiBoot() : _realRamSize(0), _ramSize(0) {
  memcpy((void*)&_info, (void*)MULTIBOOT_INFO_ADDR, sizeof(multiboot_info_t));
  memset((void*)&_mmap, 0, sizeof(_mmap));

  if((_info.flags) & (1 << 6)) {
    int i = 0;
    const memory_map_t* mmap = (memory_map_t*)(_info.mmap_addr);
    while((unsigned)mmap < _info.mmap_addr + _info.mmap_length) {
      if (i >= MAX_MMAP_ENTRIES) {
        //PANIC
        while(1);
      }
      _mmap[i].base_addr = mmap->base_addr;
      _mmap[i].length = mmap->length;
      _mmap[i].type = mmap->type;
      //this is not required/used further
      _mmap[i].size = mmap->size;
      mmap = (memory_map_t*)((unsigned)mmap + mmap->size + sizeof(uint32_t));
      ++i;
    }
    for(i = 0; i < MAX_MMAP_ENTRIES && _mmap[i].size > 0; ++i) {
      _realRamSize += _mmap[i].length;
    }
    const uint32_t LITTLE_LESS_THAN_4GB = 4u * 1024 * 1024 * 1023;
    if(_realRamSize > LITTLE_LESS_THAN_4GB)
      _ramSize = LITTLE_LESS_THAN_4GB;
    else
      _ramSize = _realRamSize;
  }
  else //TODO: use some other approach to find RAM or KERNEL PANIC
    _ramSize = 128 * 1024 * 1024; //fake it
}

byte MultiBoot::GetBootDeviceID()
{
	return _info.boot_device[3] ;
}

byte MultiBoot::GetBootPartitionID()
{
	return _info.boot_device[2] ;
}

const framebuffer_info_t* MultiBoot::VideoFrameBufferInfo() const
{
  if((_info.flags) & (1 << 12))
    return &_info.framebuffer_info;
  return nullptr;
}

const memory_map_t* MultiBoot::GetACPIInfoMemMap() const {
  const int ACPI_INFO_MMAP_TYPE = 3;
  if((_info.flags) & (1 << 6)) {
    for (int i = 0; i < MAX_MMAP_ENTRIES && _mmap[i].size > 0; ++i) {
      if (_mmap[i].type == ACPI_INFO_MMAP_TYPE) {
        return &_mmap[i];
      }
    }
  }
  return nullptr;
}

void MultiBoot::Print()
{
  char buffer[256];
  upan::string msg;

  sprintf(buffer, "\n FLAG: 0x%x", _info.flags);
  msg += buffer;
  
  if((_info.flags) & 0x1)
  {
    sprintf(buffer, "\n LOWER_MEM: %u, UPPER_MEM: %u", _info.mem_lower, _info.mem_upper);
    msg += buffer;
  }

  if((_info.flags) & (1 << 6))
  {
    for (int i = 0; i < MAX_MMAP_ENTRIES && _mmap[i].size > 0; ++i) {
      sprintf(buffer, "\n%d) Size: %u, Address: %llu, Length: %llu, Type: %u", i+1, _mmap[i].size, _mmap[i].base_addr, _mmap[i].length, _mmap[i].type);
      msg += buffer;
    }
    sprintf(buffer, "\n Total RAM SIZE: %llu", _realRamSize);
    msg += buffer;
  }

  if((_info.flags) & (1 << 11))
  {
    sprintf(buffer, "\n VBE_CONTROL_INFO: 0x%x", _info.vbe_info.vbe_control_info);
    msg += buffer;
    sprintf(buffer, "\n VBE_MODE_INFO: 0x%x", _info.vbe_info.vbe_mode_info);
    msg += buffer;
    sprintf(buffer, "\n VBE_MODE: %u", _info.vbe_info.vbe_mode);
    msg += buffer;
    sprintf(buffer, "\n VBE_INTERFACE_SEG: 0x%x", _info.vbe_info.vbe_interface_seg);
    msg += buffer;
    sprintf(buffer, "\n VBE_INTERFACE_OFF: 0x%x", _info.vbe_info.vbe_interface_off);
    msg += buffer;
    sprintf(buffer, "\n VBE_INTERFACE_LEN: %u", _info.vbe_info.vbe_interface_len);
    msg += buffer;
    sprintf(buffer, "\n");
    msg += buffer;
  }
  else if((_info.flags) & (1 << 12))
  {
    sprintf(buffer, "\n FRAMEBUFFER_ADDR: 0x%x", _info.framebuffer_info.framebuffer_addr);
    msg += buffer;
    sprintf(buffer, "\n FRAMEBUFFER_PITCH: %u", _info.framebuffer_info.framebuffer_pitch);
    msg += buffer;
    sprintf(buffer, "\n FRAMEBUFFER_WIDTH: %u", _info.framebuffer_info.framebuffer_width);
    msg += buffer;
    sprintf(buffer, "\n FRAMEBUFFER_HEIGHT: %u", _info.framebuffer_info.framebuffer_height);
    msg += buffer;
    sprintf(buffer, "\n FRAMEBUFFER_BPP: %u", _info.framebuffer_info.framebuffer_bpp);
    msg += buffer;

    if(_info.framebuffer_info.framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED)
    {
      sprintf(buffer, "\n FRAMEBUFFER_PALETTE_ADDR: 0x%x", _info.framebuffer_info.framebuffer_palette_addr);
      msg += buffer;
      sprintf(buffer, "\n FRAMEBUFFER_PALETTE_NUM_COLORS: %u", _info.framebuffer_info.framebuffer_palette_num_colors);
      msg += buffer;
    }
    else if(_info.framebuffer_info.framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB)
    {
      sprintf(buffer, "\n FRAMEBUFFER_RED_POS: %u", _info.framebuffer_info.framebuffer_red_field_position);
      msg += buffer;
      sprintf(buffer, "\n FRAMEBUFFER_RED_SIZE: %u", _info.framebuffer_info.framebuffer_red_mask_size);
      msg += buffer;
      sprintf(buffer, "\n FRAMEBUFFER_GREEN_POS: %u", _info.framebuffer_info.framebuffer_green_field_position);
      msg += buffer;
      sprintf(buffer, "\n FRAMEBUFFER_GREEN_SIZE: %u", _info.framebuffer_info.framebuffer_green_mask_size);
      msg += buffer;
      sprintf(buffer, "\n FRAMEBUFFER_BLUE_POS: %u", _info.framebuffer_info.framebuffer_blue_field_position);
      msg += buffer;
      sprintf(buffer, "\n FRAMEBUFFER_BLUE_SIZE: %u", _info.framebuffer_info.framebuffer_blue_mask_size);
      msg += buffer;
    }
  }
  
  COM1::Instance().Write(msg);
  printf("%s", msg.c_str());
}
