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

#include <stdlib.h>

class Acpi
{
  private:
    Acpi();
    Acpi(const Acpi&);
  public:
    static Acpi& Instance()
    {
      static Acpi acpi;
      return acpi;
    }
    bool IsAvailable() const { return _isAvailable; }

  private:
    class Rsdp
    {
      private:
        Rsdp();
        Rsdp(const Rsdp&);
      public:
        static const Rsdp* Search();
        bool MatchSignature() const;
        uint8_t Revision() const { return _rev; }
      private:
        static const int RSDP_SIG_LEN = 8;
        static const int OEMID_LEN = 6;

        char      _signature[RSDP_SIG_LEN]; //"RSD PTR "
        uint8_t   _checksum; // Checksum of the bytes 0-19 (defined in ACPI rev 1); bytes must add to 0
        char      _oemID[OEMID_LEN];
        uint8_t   _rev;
        void*     _rootSystemTable;
        // All the fields above are used to calc the checksum
        uint32_t  _rootSystemTableLen;
        uint64_t  _extendedRootSystemTableAddr; // 64 bit phys addr
        uint8_t   _checksumEx; // Checksum of the whole structure including the other checksum field
        uint8_t   _reserved[3];
    } PACKED;
  private:
    bool _isAvailable;
};
