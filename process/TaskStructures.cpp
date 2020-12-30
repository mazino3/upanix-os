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
#include <ustring.h>
#include <TaskStructures.h>
#include <ProcessConstants.h>
#include <MemManager.h>

void TaskState::BuildForUser(uint32_t stackStartAddress, unsigned uiPDEAddress, unsigned uiEntryAdddress, unsigned uiProcessEntryStackSize)
{
  int pr = 100 ;
  memset(this, 0, sizeof(TaskState)) ;

  CR3_PDBR = uiPDEAddress ;
  EIP = uiEntryAdddress ;
  ESP = stackStartAddress - PROCESS_CG_STACK_PAGES * PAGE_SIZE - uiProcessEntryStackSize ;

  if(pr == 1) //GDT - Low Priority
  {
    ES = 0x50 | 0x3 ;

    CS = 0x40 | 0x3 ;

    DS = 0x48 | 0x3 ;
    SS = 0x48 | 0x3 ;
    FS = 0x48 | 0x3 ;
    GS = 0x48 | 0x3 ;
    LDT = 0x0 ;
  }
  else if(pr == 2) //GDT - High Priority
  {
    ES = 0x8 ;

    CS = 0x10 ;

    DS = 0x18 ;
    SS = 0x18 ;
    FS = 0x18 ;
    GS = 0x18 ;
    LDT = 0x0 ;
  }
  else //LDT
  {
    ES = 0x8 | 0x7 ;

    CS = 0x10 | 0x7 ;

    DS = 0x18 | 0x7 ;
    FS = 0x18 | 0x7 ;
    GS = 0x18 | 0x7 ;

    SS = 0x20 | 0x7 ;

    SS0 = 0x28 | 0x4 ;
    ESP0 = PROCESS_BASE + stackStartAddress - GLOBAL_DATA_SEGMENT_BASE ;
    LDT = 0x50 ;
  }

  EFLAGS = 0x202 ;
  DEBUG_T_BIT = 0x00 ;
  IO_MAP_BASE = 103 ; // > TSS Limit => No I/O Permission Bit Map present
}

void TaskState::BuildForKernel(const unsigned uiTaskAddress, const unsigned uiStackTop, unsigned uiParam1, unsigned uiParam2) {
  memset(this, 0, sizeof(TaskState));

  EIP = uiTaskAddress ;

  ((unsigned*)(uiStackTop - 8))[0] = uiParam1 ;
  ((unsigned*)(uiStackTop - 8))[1] = uiParam2 ;
  ESP = uiStackTop - 12 ;

  ES = 0x8 | 0x4 ;

  CS = 0x10 | 0x4 ;

  DS = 0x18 | 0x4 ;
  FS = 0x18 | 0x4 ;
  GS = 0x18 | 0x4 ;

  SS = 0x20 | 0x4 ;

  LDT = 0x50 ;

  CR3_PDBR = MEM_PDBR ;
  EFLAGS = 0x202 ;
  DEBUG_T_BIT = 0x00 ;
  IO_MAP_BASE = 103 ; // > TSS Limit => No I/O Permission Bit Map present
}

void Descriptor::Build(unsigned uiLimit, unsigned uiBase, byte bType, byte bFlag)
{
  _usLimit15_0 = (0xFFFF & uiLimit) ;
  _usBase15_0 = (0xFFFF & uiBase) ;
  _bBase23_16 = (uiBase >> 16) & 0xFF ;
  _bType = bType ;
  _bLimit19_16_Flags = (bFlag << 4) | ((uiLimit >> 16) & 0xF) ;
  _bBase31_24 = (uiBase >> 24) & 0xFF ;
}

void ProcessLDT::BuildForUser()
{
  unsigned uiSegmentBase = PROCESS_BASE ;
  unsigned uiProcessLimit = 0xFFFFF ;

  NullDesc.Build(0, 0, 0, 0) ;
  LinearDesc.Build(uiProcessLimit, uiSegmentBase, 0xF2, 0xC) ;
  CodeDesc.Build(uiProcessLimit, uiSegmentBase, 0xFA, 0xC) ;
  DataDesc.Build(uiProcessLimit, uiSegmentBase, 0xF2, 0xC) ;
  StackDesc.Build(uiProcessLimit, uiSegmentBase, 0xF2, 0xC) ;

  CallGateStackDesc.Build(uiProcessLimit, GLOBAL_DATA_SEGMENT_BASE, 0x92, 0xC);
}

void ProcessLDT::BuildForKernel()
{
  NullDesc.Build(0, 0, 0, 0) ;
//BuildDescriptor(&processLDT->LinearDesc, 0xFFFFF, 0x00, 0x92, 0xC) ;
  LinearDesc.Build(0xFFFFF, GLOBAL_DATA_SEGMENT_BASE, 0x92, 0xC) ;
  CodeDesc.Build(0xFFFFF, GLOBAL_DATA_SEGMENT_BASE, 0x9A, 0xC) ;
  DataDesc.Build(0xFFFFF, GLOBAL_DATA_SEGMENT_BASE, 0x92, 0xC) ;
  StackDesc.Build(0xFFFFF, GLOBAL_DATA_SEGMENT_BASE, 0x92, 0xC) ;
}

