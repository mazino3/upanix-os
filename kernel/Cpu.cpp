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

#include <Cpu.h>
#include <stdio.h>

Cpu::Cpu()
{
  uint32_t result = 0;
  // Test if the CPU supports the CPUID-Command
  __asm__ __volatile__("pushfl;"
                       "pushfl;"
                       "pop %%eax;"
                       "mov %%eax, %%ecx;"
                       "xorl $0x200000, %%eax;"
                       "push %%eax;"
                       "popfl;"
                       "pushfl;"
                       "pop %%eax;"
                       "xorl %%ecx, %%eax;"
                       "shrl $21, %%eax;"
                       "popfl;" : "=m"(result) : : "eax", "ecx", "memory");
  _cpuIdAvailable = (result == 0);
  if(_cpuIdAvailable)
    printf("\n CPUID is available");
}

bool Cpu::HasSupport(CPU_FEATURE feature)
{
  if(feature == CF_CPUID)
      return _cpuIdAvailable;

  if(!_cpuIdAvailable)
      return false;

  CPU_REGISTER r = (CPU_REGISTER)(feature & ~31);

  return GetIdRegister(0x1, r) & (1 << (feature - r));
}

uint32_t Cpu::GetIdRegister(uint32_t function, CPU_REGISTER reg)
{
  switch (reg)
  {
    case CR_EAX: {
      uint32_t temp;
      __asm__ __volatile__("cpuid" : "=a"(temp) : "a"(function) : "ebx", "ecx", "edx");
      return temp;
    }
    case CR_EBX: {
      uint32_t temp, temp2;
      __asm__ __volatile__("cpuid" : "=b"(temp), "=a"(temp2) : "a"(function) : "ecx", "edx");
      return temp;
    }
    case CR_ECX: {
      uint32_t temp, temp2;
      __asm__ __volatile__("cpuid" : "=c"(temp), "=a"(temp2) : "a"(function) : "ebx", "edx");
      return (temp);
    }
    case CR_EDX: {
      uint32_t temp, temp2;
      __asm__ __volatile__("cpuid" : "=d"(temp), "=a"(temp2) : "a"(function) : "ebx", "ecx");
      return (temp);
    }
  }
  return 0;
}

uint64_t Cpu::MSRread(uint32_t msr)
{
  uint32_t low, high;
  __asm__ __volatile__("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
  return ((uint64_t)high << 32) | low;
}

void Cpu::MSRwrite(uint32_t msr, uint64_t value)
{
  uint32_t low = value & 0xFFFFFFFF;
  uint32_t high = value >> 32;
  __asm__ __volatile__("wrmsr" :: "a"(low), "c"(msr), "d"(high));
}
