#ifndef _PROCESS_H
#define _PROCESS_H

#include <Atomic.h>
#include <TaskStructures.h>
#include <FSStructures.h>

class ProcessGroup;
class IRQ;

typedef enum
{
  NEW,
  RUN,
  WAIT_SLEEP,
  WAIT_INT,
  WAIT_CHILD,
  WAIT_RESOURCE,
  WAIT_KERNEL_SERVICE,
  WAIT_EVENT,
  TERMINATED,
} PROCESS_STATUS ;

class ProcessStateInfo
{
  public:
    ProcessStateInfo();
  unsigned uiSleepTime ;
  const IRQ* pIRQ ;
  int iWaitChildProcId ;
  RESOURCE_KEYS uiWaitResourceId;
  uint32_t _eventCompleted;
  bool bKernelServiceComplete ;
};

class Process
{
public:
  void Load(const char* szProcessName, unsigned *uiPDEAddress,
            unsigned* uiEntryAdddress, unsigned* uiProcessEntryStackSize, int iNumberOfParameters, char** szArgumentList);

private:
  void PushProgramInitStackData(unsigned uiPDEAddr, unsigned* uiProcessEntryStackSize, unsigned uiProgramStartAddress,
                                unsigned uiInitRelocAddress, unsigned uiTermRelocAddress, int iNumberOfParameters, char** szArgumentList);
  void CopyElfImage(unsigned uiPDEAddr, byte* bProcessImage, unsigned uiMemImageSize);

  public:
  TaskState taskState ;
  ProcessLDT processLDT ;
  bool bFree ;
  PROCESS_STATUS status ;
  ProcessStateInfo* pProcessStateInfo ;

  int iParentProcessID ;
  int iNextProcessID ;
  int iPrevProcessID ;

  unsigned _noOfPagesForPTE ;
  unsigned _noOfPagesForProcess ;

  byte bIsKernelProcess ;
  int iKernelStackBlockID ;

  unsigned uiAUTAddress ;
  unsigned _processBase ;

  int iDriveID ;
  FileSystem_PresentWorkingDirectory processPWD ;

  unsigned uiNoOfPagesForDLLPTE ;
  unsigned _startPDEForDLL ;

  ProcessGroup* _processGroup;

  int iUserID ;

  char* pname ;

  unsigned uiPageFaultAddress ;
  unsigned uiDMMFlag ;
} PACKED;

#endif
