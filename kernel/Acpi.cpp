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

#include <Acpi.h>
#include <MultiBoot.h>
#include <string.h>

Acpi::Acpi() : _isAvailable(false)
{
  auto rsdp = Rsdp::Search();
  if(rsdp)
    _isAvailable = true;
}

const Acpi::Rsdp* Acpi::Rsdp::Search()
{
  auto acpi_mmap = MultiBoot::Instance().GetMemMapArea(3);
  if(!acpi_mmap)
  {
    printf("\n GRUB multiboot info doesn't have ACPI memory mapped area");
    return nullptr;
  }

  for(unsigned i = 0; i < (acpi_mmap->length + RSDP_SIG_LEN - 1); i += 16)
  {
    uint8_t* a = reinterpret_cast<uint8_t*>(acpi_mmap->base_addr + i);
    Rsdp* rsdp = reinterpret_cast<Rsdp*>(a);
    if(rsdp->MatchSignature())
    {
      uint8_t checksum = 0;

      uint8_t* end = a + 20; // ACPI 1.0 spec size
      for(uint8_t* mem = a; mem != end; ++mem) 
        checksum += *mem; // First checksum

      if(!checksum && rsdp->Revision() == 2) // Test worked && newest ACPI spec
      {
          end = a + sizeof(Rsdp) - 3; // Current RDSP size - 3 reserved bytes
          for (uint8_t* mem = a; mem != end; ++mem) checksum += *mem; // Extended checksum
      }

      if(!checksum) 
        return rsdp;
    }
  }
  return nullptr;
}

bool Acpi::Rsdp::MatchSignature() const
{
  static const char* RSDP_SIGNATURE = "RSD PTR ";
  return memcmp(_signature, RSDP_SIGNATURE, RSDP_SIG_LEN) == 0;
}
