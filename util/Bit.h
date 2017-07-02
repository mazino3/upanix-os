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
#ifndef _BIT_H_
#define _BIT_H_

# include <Global.h>

class Bit
{
  public:
    static inline bool IsSet(const unsigned dword, const unsigned bitPos)
    {
      return dword & bitPos ? true : false;
    }

    static inline unsigned Set(const unsigned dword, const unsigned bitPos, const bool enable)
    {
      return enable ? dword | bitPos : dword & ~(bitPos);
    }

    static inline uint8_t Byte1(const uint32_t val) { return Byte(val, 0); }
    static inline uint8_t Byte2(const uint32_t val) { return Byte(val, 1); }
    static inline uint8_t Byte3(const uint32_t val) { return Byte(val, 2); }
    static inline uint8_t Byte4(const uint32_t val) { return Byte(val, 3); }

    static inline bool IsPowerOfTwo(const uint32_t n) { return (n & (n - 1)) == 0; }

  private:
    static inline uint8_t Byte(const uint32_t val, const uint32_t id)
    {
      return (val >> (id * 8)) & 0xFF;
    }
};

#endif
