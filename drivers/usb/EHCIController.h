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
#ifndef _EHCI_CONTROLLER_H_
#define _EHCI_CONTROLLER_H_

# include <Global.h>
# include <list.h>
# include <USBController.h>
# include <EHCIStructures.h>

#define EHCIController_SUCCESS		0
#define EHCIController_FAILURE		2

#define MAX_EHCI_ENTRIES 32
#define MAX_EHCI_TD_PER_BULK_RW 64
#define EHCI_MAX_BYTES_PER_TD 4096

#define QH_TYPE_CONTROL 1

class EHCIController
{
  public:
    EHCIController(PCIEntry*, int iMemMapIndex);
    byte Probe();
    void DisplayStats();
    byte AsyncDoorBell();
    EHCIQueueHead* CreateDeviceQueueHead(int iMaxPacketSize, int iEndPtAddr, int iDevAddr);

  private:
    byte PerformBiosToOSHandoff();
    void SetupInterrupts();
    byte SetupPeriodicFrameList();
    byte SetupAsyncList();
    void SetSchedEnable(unsigned uiScheduleType, bool bEnable);
    void SetFrameListSize();
    void Start();
    void Stop();
    byte SetConfigFlag(bool bSet);
    bool CheckHCActive();
    void SetupPorts();
    unsigned GetNoOfPortsActual();
    unsigned GetNoOfPorts();
    bool StartAsyncSchedule();
    bool StopAsyncSchedule();
    bool WaitCheckAsyncScheduleStatus(bool bValue);
    bool PollWait(unsigned* pValue, int iBitPos, unsigned value);
    void AddAsyncQueueHead(EHCIQueueHead* pQH);

    PCIEntry* _pPCIEntry;
    EHCICapRegisters* _pCapRegs;
    EHCIOpRegisters* _pOpRegs;
    EHCIQueueHead* _pAsyncReclaimQueueHead;

    friend class EHCIManager;
};

#endif
