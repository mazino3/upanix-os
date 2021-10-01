/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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
#ifndef _ATA_COMMAND_MANAGER_H_
#define _ATA_COMMAND_MANAGER_H_

# include <ATADeviceController.h>

#define ATACommandManager_SUCCESS		0
#define ATACommandManager_ERR_IO		1
#define ATACommandManager_FAILURE		2

typedef struct ATACommand ATACommand ;

typedef struct
{
	byte bErrorCode ;
	byte bValid ;
	byte bSegmentNumber ;
	byte bSenseKey ;
	byte bReserved1 ;
	byte ILI ;
	byte bReserved2 ;
	byte bInformation[4] ;
	byte bAddSenseLen ;
	byte bCommandInfo[4] ;
	byte bASC ;
	byte bASCQ ;
	byte bFruc ;
	byte bSKS[3] ;
	byte bASB[46] ;
} ATAPISense ;	

struct ATACommand
{
	ATACommand* pNext ;
	ATACommand* pPrev ;
	
	ATAPort* pPort ;
	unsigned uiFlags ;
	byte bCommand[14] ;
	int iStatus ;
	int iError ;
	ATAPISense atapiSense ;
	byte bCanDMA ;
	byte *pTransferBuffer ;
	unsigned uiTransferLength ;
} ;
	
void ATACommandManager_ExecuteATACommand(ATACommand* pCommand) ;
void ATACommandManager_ExecuteATAPICommand(ATACommand* pCommand) ;
void ATACommandManager_InitCommand(ATACommand* pCommand, ATAPort* pPort) ;
void ATACommandManager_Queue(ATACommand* pCommand) ;

#endif
