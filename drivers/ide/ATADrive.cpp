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
# include <ATADrive.h>
# include <ATACommandManager.h>
# include <MemConstants.h>

void ATADrive_Read(ATAPort* pPort, unsigned uiSectorAddress, byte* pBuffer, unsigned uiNoOfSectorsToTransfer)
{
	ATACommand ataCommand ;
	unsigned uiBytesTransfered = 0 ;
	unsigned uiLength = uiNoOfSectorsToTransfer * 512 ;
	unsigned uiChunkSize = 16 KB ;

	while(uiBytesTransfered < uiLength)
	{
		//Prepare Read Command
		ATACommandManager_InitCommand(&ataCommand, pPort) ;

		ataCommand.pTransferBuffer = pBuffer ;
		ataCommand.uiTransferLength = (uiLength - uiBytesTransfered) < uiChunkSize ?  (uiLength - uiBytesTransfered) : uiChunkSize ;

		ataCommand.bCommand[ATA_REG_CONTROL] = ATA_CONTROL_DEFAULT ;
		ataCommand.bCommand[ATA_REG_COMMAND] = ATA_CMD_READ_PIO ;
		ataCommand.bCommand[ATA_REG_DEVICE] = ATA_DEVICE_LBA ;
		ataCommand.bCommand[ATA_REG_COUNT] = ataCommand.uiTransferLength / 512 ;

		ataCommand.bCommand[ATA_REG_LBA_LOW] = uiSectorAddress & 0xFF ;
		ataCommand.bCommand[ATA_REG_LBA_MID] = (uiSectorAddress >> 8) & 0xFF ;
		ataCommand.bCommand[ATA_REG_LBA_HIGH] = (uiSectorAddress >> 16) & 0xFF ;

		if(pPort->bLBA48Bit)
			ataCommand.bCommand[11] = (uiSectorAddress >> 24) & 0xFF ;
		else
			ataCommand.bCommand[ATA_REG_DEVICE] |= (uiSectorAddress >> 24) & 0xFF ;

		ATACommandManager_Queue(&ataCommand) ;

		if(ataCommand.iStatus != ATACommandManager_SUCCESS)
      throw upan::exception(XLOC, "ATA read failed with status code:%d", ataCommand.iStatus);
	
		uiSectorAddress++ ;
		uiBytesTransfered += ataCommand.uiTransferLength ;
		pBuffer += ataCommand.uiTransferLength ;
	}
}

void ATADrive_Write(ATAPort* pPort, unsigned uiSectorAddress, byte* pBuffer, unsigned uiNoOfSectorsToTransfer)
{
	ATACommand ataCommand ;
	unsigned uiBytesTransfered = 0 ;
	unsigned uiLength = uiNoOfSectorsToTransfer * 512 ;
	unsigned uiChunkSize = 16 KB ;

	while(uiBytesTransfered < uiLength)
	{
		//Prepare Write Command
		ATACommandManager_InitCommand(&ataCommand, pPort) ;

		ataCommand.pTransferBuffer = pBuffer ;
		ataCommand.uiTransferLength = (uiLength - uiBytesTransfered) < uiChunkSize ? 
										(uiLength - uiBytesTransfered) : uiChunkSize ;

		ataCommand.bCommand[ATA_REG_CONTROL] = ATA_CONTROL_DEFAULT ;
		ataCommand.bCommand[ATA_REG_COMMAND] = ATA_CMD_WRITE_PIO ;
		ataCommand.bCommand[ATA_REG_DEVICE] = ATA_DEVICE_LBA ;
		ataCommand.bCommand[ATA_REG_COUNT] = ataCommand.uiTransferLength / 512 ;

		ataCommand.bCommand[ATA_REG_LBA_LOW] = uiSectorAddress & 0xFF ;
		ataCommand.bCommand[ATA_REG_LBA_MID] = (uiSectorAddress >> 8) & 0xFF ;
		ataCommand.bCommand[ATA_REG_LBA_HIGH] = (uiSectorAddress >> 16) & 0xFF ;

		if(pPort->bLBA48Bit)
			ataCommand.bCommand[11] = (uiSectorAddress >> 24) & 0xFF ;
		else
			ataCommand.bCommand[ATA_REG_DEVICE] |= (uiSectorAddress >> 24) & 0xFF ;

		ATACommandManager_Queue(&ataCommand) ;

		if(ataCommand.iStatus != ATACommandManager_SUCCESS)
      throw upan::exception(XLOC, "ATA write failed with status code:%d", ataCommand.iStatus);
	
		uiSectorAddress++ ;
		uiBytesTransfered += ataCommand.uiTransferLength ;
		pBuffer += ataCommand.uiTransferLength ;
	}
}
