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

  uint32_t SleepTime() const { return _sleepTime; }
  void SleepTime(const uint32_t s) { _sleepTime = s; }

  int WaitChildProcId() const { return _waitChildProcId; }
  void WaitChildProcId(const int id) { _waitChildProcId = id; }

  RESOURCE_KEYS WaitResourceId() const { return _waitResourceId; }
  void WaitResourceId(const RESOURCE_KEYS id) { _waitResourceId = id; }

  bool IsKernelServiceComplete() const { return _kernelServiceComplete; }
  void KernelServiceComplete(const bool v) { _kernelServiceComplete = v; }

  const IRQ* Irq() const { return _irq; }
  void Irq(const IRQ* irq) { _irq = irq; }

  bool IsEventCompleted();
  void EventCompleted();

private:
  unsigned      _sleepTime ;
  const IRQ*    _irq;
  int           _waitChildProcId ;
  RESOURCE_KEYS _waitResourceId;
  uint32_t      _eventCompleted;
  bool          _kernelServiceComplete ;
};

class Process
{
public:
  void Load(const char* szProcessName, unsigned *uiPDEAddress, unsigned* uiEntryAdddress,
            unsigned* uiProcessEntryStackSize, int iNumberOfParameters, char** szArgumentList);
  void DeAllocateAddressSpace();
  uint32_t GetDLLPageAddressForKernel();

private:
  void PushProgramInitStackData(unsigned uiPDEAddr, unsigned* uiProcessEntryStackSize, unsigned uiProgramStartAddress,
                                unsigned uiInitRelocAddress, unsigned uiTermRelocAddress, int iNumberOfParameters, char** szArgumentList);
  void CopyElfImage(unsigned uiPDEAddr, byte* bProcessImage, unsigned uiMemImageSize);

  uint32_t AllocateAddressSpace();
  uint32_t AllocatePDE();
  void AllocatePTE(const unsigned uiPDEAddress);
  void InitializeProcessSpaceForOS(const unsigned uiPDEAddress);
  void InitializeProcessSpaceForProcess(const unsigned uiPDEAddress);
  void DeAllocateProcessSpace();
  void DeAllocatePTE();

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
