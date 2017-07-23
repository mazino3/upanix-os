#ifndef _PROCESS_H
#define _PROCESS_H

#include <TaskStructures.h>

class ProcessGroup;

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
  TaskState taskState ;
  ProcessLDT processLDT ;
  byte bFree ;
  PROCESS_STATUS status ;
  ProcessStateInfo* pProcessStateInfo ;

  int iParentProcessID ;
  int iNextProcessID ;
  int iPrevProcessID ;

  unsigned uiNoOfPagesForPTE ;
  unsigned uiNoOfPagesForProcess ;

  byte bIsKernelProcess ;
  int iKernelStackBlockID ;

  unsigned uiAUTAddress ;
  unsigned uiProcessBase ;

  int iDriveID ;
  FileSystem_PresentWorkingDirectory processPWD ;

  unsigned uiNoOfPagesForDLLPTE ;
  unsigned uiStartPDEForDLL ;

  ProcessGroup* _processGroup;

  int iUserID ;

  char* pname ;

  unsigned uiPageFaultAddress ;
  unsigned uiDMMFlag ;
} PACKED;

#endif
