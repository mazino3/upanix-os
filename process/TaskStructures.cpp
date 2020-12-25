#include <ustring.h>
#include <TaskStructures.h>
#include <ProcessConstants.h>

void TaskState::Build(uint32_t stackStartAddress, unsigned uiPDEAddress, unsigned uiEntryAdddress, unsigned uiProcessEntryStackSize)
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

void Descriptor::Build(unsigned uiLimit, unsigned uiBase, byte bType, byte bFlag)
{
  _usLimit15_0 = (0xFFFF & uiLimit) ;
  _usBase15_0 = (0xFFFF & uiBase) ;
  _bBase23_16 = (uiBase >> 16) & 0xFF ;
  _bType = bType ;
  _bLimit19_16_Flags = (bFlag << 4) | ((uiLimit >> 16) & 0xF) ;
  _bBase31_24 = (uiBase >> 24) & 0xFF ;
}

void ProcessLDT::Build()
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

