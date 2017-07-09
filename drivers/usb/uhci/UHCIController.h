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

#ifndef _UHCI_HANDLER_H_
#define _UHCI_HANDLER_H_

#include <Global.h>
#include <queue.h>
#include <Atomic.h>
#include <UHCIStructures.h>
#include <USBController.h>
#include <USBMassBulkStorageDisk.h>
#include <KernelUtil.h>

#define UHCIController_SUCCESS				0
#define UHCIController_FAILURE				1
#define UHCIController_ERR_NOT_RUNNING		2
#define UHCIController_ERR_MAX_PKT_SIZE		3
#define UHCIController_ERR_INVALID_MSG_LEN	4
#define UHCIController_ERR_ISO_FAILURE		5
#define UHCIController_ERR_INT_FAILURE		6
#define UHCIController_ERR_PIPE_HALTED		7
#define UHCIController_ERR_NOCNTL_FOUND		8
#define UHCIController_ERR_CONFIG			9
#define UHCIController_INVALID_DEVADDR		10

#define MAX_UHCI_PCI_ENTRIES 32
#define MAX_UHCI_TD_PER_BULK_RW 8

class PCIEntry;

class UHCIController : public KernelUtil::TimerTask
{
  public:
    UHCIController(PCIEntry*, unsigned uiIOBase, unsigned uiIOSize);
    bool Probe();
  private:
    virtual bool TimerTrigger();

    bool GetNextFrameToClean(unsigned& uiFrameNumber);
    bool CleanFrame(unsigned uiFrameNumber);
    void CreateFrameList();
    unsigned GetFrameListEntry(unsigned uiFrameNumber);
    void SetFrameListEntry(unsigned uiFrameNumber, unsigned uiValue, bool bCleanBuffer, bool bCleanDescs);
    unsigned GetNextFreeFrame();
    void BuildFrameEntryForDeAlloc(unsigned uiFrameNumber, unsigned uiDescAddress, bool bCleanBuffer, bool bCleanDescs);
    void ReleaseFrameResource(unsigned uiFrameNumber);
    unsigned IOBase() const { return _uiIOBase; }
    
    PCIEntry* _pPCIEntry;
    unsigned  _uiIOBase;
    unsigned  _uiIOSize;
    upan::queue<unsigned> _frameQueue;
    unsigned* _pFrameList;
    upan::list<unsigned>* _pLocalFrameList;
	  Mutex _m;

    friend class UHCIDevice;
};

#endif
