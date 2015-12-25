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
#define EHCIController_ERR_CONFIG	1
#define EHCIController_FAILURE		2

#define MAX_EHCI_ENTRIES 32
#define MAX_EHCI_TD_PER_BULK_RW 64
#define EHCI_MAX_BYTES_PER_TD 4096

#define QH_TYPE_CONTROL 1

class EHCITransaction
{
  public:
    EHCITransaction(EHCIQueueHead* qh, EHCIQTransferDesc* tdStart);
    bool PollWait();
    void Clear();

  private:
    EHCIQueueHead* _qh;
    EHCIQTransferDesc* _tdStart;
    upan::list<unsigned> _dStorageList;
};

byte EHCIController_Initialize() ;
void EHCIController_ProbeDevice() ;
byte EHCIController_RouteToCompanionController() ;

#endif
