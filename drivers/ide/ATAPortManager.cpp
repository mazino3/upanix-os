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
# include <ATAPortManager.h>
# include <ATAPortOperation.h>
# include <PIT.h>
# include <DMM.h>
# include <Display.h>
# include <StringUtil.h>
# include <PortCom.h>
# include <MemUtil.h>
# include <KernelUtil.h>

// Cable Types
char ATAPortManager_szCable[][50] = {
	"None",
	"Standard",
	"40Pin",
	"80Pin",
	"SATA"
} ;

/*********************************************** Static Functions *************************************/

static void ATAPortManager_SwapBytes(char *szString, int iLen)
{
	int i ;
	char ch ;

	for(i = 0; i < iLen; i += 2)
	{
		ch = szString[i] ;
		szString[i] = szString[i + 1] ;
		szString[i + 1] = ch ;
	}
}

static void ATAPortManager_ReadModelID(char* szDest, char* szSrc)
{
	int i ;

	ATAPortManager_SwapBytes(szSrc, 40) ;
	
	for(i = 39; i >= 0; i--)
	{
		if(szSrc[i] != ' ')
			break ;
	}

	String_RawCopy((byte*)szDest, (byte*)szSrc, i + 1) ;
	szDest[i + 1] = '\0' ;
}

static byte ATAPortManager_ResetATAPI(ATAPort* pPort)
{
	ATA_WRITE_REG(pPort, ATA_REG_FEATURE, 0) ;
	ATA_WRITE_REG(pPort, ATAPI_REG_IRR, 0) ;
	ATA_WRITE_REG(pPort, ATAPI_REG_SAMTAG, 0) ;
	ATA_WRITE_REG(pPort, ATAPI_REG_COUNT_LOW, 0) ;
	ATA_WRITE_REG(pPort, ATAPI_REG_COUNT_HIGH, 0) ;
	ATA_WRITE_REG(pPort, ATA_REG_COMMAND, ATAPI_CMD_RESET) ;

	byte bStatus ;
	RETURN_IF_NOT(bStatus, ATAPortManager_IOWait(pPort, ATA_STATUS_BUSY, 0), ATAPortManager_SUCCESS) ;

	return ATAPortManager_SUCCESS ;
}

/*****************************************************************************************************/

void ATAPortManager_Probe(ATAPort* pPort)
{
	byte bLBAHigh, bLBAMid, bStatus ;
	char ID[512] ;
	ATAIdentifyInfo* pATAIdentifyInfo ;

	if(pPort->bConfigured == true)
		return ;

	bStatus = pPort->portOperation.Reset(pPort) ;
	if(bStatus != ATAPortOperation_SUCCESS)
	{
		KC::MDisplay().Address("\n\tNo Device connected on ", pPort->uiChannel) ;
		KC::MDisplay().Address(":", pPort->uiPort) ;
		KC::MDisplay().Address(" E = ", bStatus) ;

		pPort->uiDevice = ATA_DEV_NONE ;
		pPort->uiCable = ATA_CABLE_NONE ;

		return ;
	}

	// Read Registers
    ATA_READ_REG(pPort, ATA_REG_LBA_HIGH, bLBAHigh) ;
	ATA_READ_REG(pPort, ATA_REG_LBA_MID, bLBAMid) ;
	ATA_READ_REG(pPort, ATA_REG_STATUS, bStatus) ;

	if(((bLBAMid == 0x00 && bLBAHigh == 0x00) || (bLBAMid == 0xC3 && bLBAHigh == 0xC3)) && bStatus != 0)
	{
		pPort->uiDevice = ATA_DEV_ATA ;
		KC::MDisplay().Message("\n\tATA Device Connected on ", Display::WHITE_ON_BLACK()) ;
		KC::MDisplay().Message(ATAPortManager_szCable[pPort->uiCable], Display::WHITE_ON_BLACK()) ;
		KC::MDisplay().Message(" Cable", Display::WHITE_ON_BLACK()) ;
	}
	else if((bLBAMid == 0x14 && bLBAHigh == 0xEB) || (bLBAMid == 0x69 && bLBAHigh == 0x96))
	{	
		pPort->uiDevice = ATA_DEV_ATAPI ;
		KC::MDisplay().Message("\n\tATAPI Device Connected on ", Display::WHITE_ON_BLACK()) ;
		KC::MDisplay().Message(ATAPortManager_szCable[pPort->uiCable], Display::WHITE_ON_BLACK()) ;
		KC::MDisplay().Message(" Cable", Display::WHITE_ON_BLACK()) ;
	}
	else
	{
		pPort->uiDevice = ATA_DEV_NONE ;
		pPort->uiCable = ATA_CABLE_NONE ;

		KC::MDisplay().Address("\n\tNo Device connected on ", pPort->uiChannel) ;
		KC::MDisplay().Address(":", pPort->uiPort) ;
		
		return ;
	}

	pPort->bConfigured = true ;

	if(pPort->uiDevice == ATA_DEV_UNKNOWN)
		return ;
	
	pPort->portOperation.Select(pPort, 0) ;
	if(ATAPortManager_IOWait(pPort, ATA_STATUS_BUSY, 0) != ATAPortManager_SUCCESS)
	{
		KC::MDisplay().Message("\n\tCould not get Drive Data", Display::WHITE_ON_BLACK()) ;
		pPort->uiDevice = ATA_DEV_UNKNOWN ;
		return ;
	}
	
	// Send Identification Command and Read the Response
	if(pPort->uiDevice == ATA_DEV_ATAPI)
		ATA_WRITE_REG(pPort, ATA_REG_COMMAND, ATAPI_CMD_IDENTIFY) 
	else
		ATA_WRITE_REG(pPort, ATA_REG_COMMAND, ATA_CMD_IDENTIFY) 

//	if(KERNEL_MODE)
		KernelUtil::WaitOnInterrupt(ATADeviceController_GetHDInterruptNo(pPort)) ;

	if(ATAPortManager_IORead(pPort, (void*)ID, 512) != ATAPortManager_SUCCESS)
	{
		KC::MDisplay().Message("\n\tCould not get Drive Data", Display::WHITE_ON_BLACK()) ;
		pPort->uiDevice = ATA_DEV_UNKNOWN ;
		return ;
	}

	pATAIdentifyInfo = (ATAIdentifyInfo*)&ID[0] ;

	ATAPortManager_ReadModelID(pPort->szDeviceName, pATAIdentifyInfo->szModelID) ;
	KC::MDisplay().Message("\n\tName: ", Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message(pPort->szDeviceName, Display::WHITE_ON_BLACK()) ;

	// Check for LBA Support
	if(pPort->uiDevice == ATA_DEV_ATA && !(pATAIdentifyInfo->bCapabilities & 0x02))
	{
		KC::MDisplay().Message("\n\tError: Device has no LBA Support", Display::WHITE_ON_BLACK()) ;
		pPort->uiDevice = ATA_DEV_UNKNOWN ;
		return ;
	}

	// Set Supported Drive Speeds 
	pPort->uiSupportedDriveSpeed = 1 << ATA_SPEED_PIO ;

	if(pATAIdentifyInfo->usValid & 0x02)
	{
		pPort->uiSupportedDriveSpeed |= (pATAIdentifyInfo->usEIDEPIOModes & 0x07) < ATA_SPEED_PIO_3 ;
		pPort->uiSupportedDriveSpeed |= (pATAIdentifyInfo->usMultiWordDMAInfo & 0x07) < ATA_SPEED_MWDMA_0 ;
		
		if(pATAIdentifyInfo->usMultiWordDMAInfo & 0x07)
			pPort->uiSupportedDriveSpeed |= 1 << ATA_SPEED_DMA ;
	}

	if(pATAIdentifyInfo->usValid & 0x04)
	{
		pPort->uiSupportedDriveSpeed |= 1 << ATA_SPEED_DMA ;
		pPort->uiSupportedDriveSpeed |= (pATAIdentifyInfo->usUltraDMAModes & 0xFF) << ATA_SPEED_UDMA_0 ;
	}

	//TODO: ataPrintDriveSpeed( port );

	// Check 48 bit Addressing
	if(pATAIdentifyInfo->usCommandSet2 & 0x400)
	{
		KC::MDisplay().Message("\n\tDrive uses 48 bit Addressing", Display::WHITE_ON_BLACK()) ;
		KC::MDisplay().Address("\n\tCapacity: ", (pATAIdentifyInfo->uiLBACapacity48 / 1000 * 512 / 1000)) ;
		
		pPort->bLBA48Bit = true ;

		KC::MDisplay().Number("\n\tCHS = ", pATAIdentifyInfo->usCylinders) ;
		KC::MDisplay().Number(" ", pATAIdentifyInfo->usHead) ;
		KC::MDisplay().Number(" ", pATAIdentifyInfo->usSectors) ;

		if(!1) //TODO: Why  enable 48 bit check
		{
			pPort->uiDevice = ATA_DEV_UNKNOWN ;
			return ;
		}
	}
	else if(pATAIdentifyInfo->uiLBASectors)
	{
		KC::MDisplay().Address("\n\tLBA Sectors = ", pATAIdentifyInfo->uiLBASectors) ;
		KC::MDisplay().Address("\n\tCHS = ", pATAIdentifyInfo->usCylinders) ;
		KC::MDisplay().Address(" ", pATAIdentifyInfo->usHead) ;
		KC::MDisplay().Address(" ", pATAIdentifyInfo->usSectors) ;
		KC::MDisplay().Address("\n\tCapacity: ", (pATAIdentifyInfo->uiLBASectors / 1000 * 512 / 1000)) ;
	}

	pPort->bPIO32Bit = true ; // TODO: Read as kernel param

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pATAIdentifyInfo, MemUtil_GetDS(), (unsigned)&pPort->id, sizeof(ATAIdentifyInfo)) ;
}

byte ATAPortManager_ConfigureDrive(ATAPort* pPort)
{
	byte bStatus ;
	pPort->uiCurrentSpeed = ATA_SPEED_PIO ;

	if(!(pPort->uiDevice == ATA_DEV_ATAPI || pPort->uiDevice == ATA_DEV_ATA))
		return ATAPortManager_SUCCESS ;

	// Select Drive
	pPort->portOperation.Select(pPort, 0) ;

	RETURN_IF_NOT(bStatus, ATAPortManager_IOWait(pPort, ATA_STATUS_BUSY, 0), ATAPortManager_SUCCESS) ;

	// Reset ATAPI Drices
	if(pPort->uiDevice == ATA_DEV_ATAPI)
	{
		RETURN_IF_NOT(bStatus, ATAPortManager_ResetATAPI(pPort), ATAPortManager_SUCCESS) ;
	}

	unsigned i ;
	unsigned uiSpeed = 0 ;
	for(i = ATA_SPEED_HIGHEST; i >= 0; i--)
	{
		if((pPort->uiSupportedPortSpeed & ( 1 << i )) && (pPort->uiSupportedDriveSpeed & (1 << i)))
		{
			if(i == ATA_SPEED_PIO)
			{
				KC::MDisplay().Message("\n\tUsing standard PIO mode", Display::WHITE_ON_BLACK()) ;
				return ATAPortManager_SUCCESS ;
			}	

			if(i == ATA_SPEED_DMA)
			{
				pPort->uiCurrentSpeed = ATA_SPEED_DMA ;
				KC::MDisplay().Message("\n\tUsing standard DMA mode", Display::WHITE_ON_BLACK()) ;
				return ATAPortManager_SUCCESS ;
			}

			if(i >= ATA_SPEED_UDMA_0)
			{
				KC::MDisplay().Message("\n\tUsing Ultra DMA mode", Display::WHITE_ON_BLACK()) ;
				uiSpeed = ATA_XFER_UDMA_0 + ( i - ATA_SPEED_UDMA_0) ;
			}
			else if(i >= ATA_SPEED_MWDMA_0)
			{
				KC::MDisplay().Message("\n\tUsing Multiword Ultra DMA mode", Display::WHITE_ON_BLACK()) ;
				uiSpeed = ATA_XFER_MWDMA_0 + ( i - ATA_SPEED_MWDMA_0) ;
			}
			else if(i >= ATA_SPEED_PIO_3)
			{
				KC::MDisplay().Message("\n\tUsing PIO mode", Display::WHITE_ON_BLACK()) ;
				uiSpeed = ATA_XFER_PIO_3 + ( i - 1 ) ;
			}
			
			ATA_WRITE_REG(pPort, ATA_REG_COUNT, (byte)uiSpeed) ;
			ATA_WRITE_REG(pPort, ATA_REG_LBA_LOW, 0) ;
			ATA_WRITE_REG(pPort, ATA_REG_LBA_MID, 0) ;
			ATA_WRITE_REG(pPort, ATA_REG_LBA_HIGH, 0) ;
			ATA_WRITE_REG(pPort, ATA_REG_FEATURE, 0x03) ;
			ATA_WRITE_REG(pPort, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT | ATA_CONTROL_INTDISABLE) ;
			ATA_WRITE_REG(pPort, ATA_REG_COMMAND, ATA_CMD_SET_FEATURES) ;

			RETURN_IF_NOT(bStatus, ATAPortManager_IOWait(pPort, ATA_STATUS_BUSY, 0), ATAPortManager_SUCCESS) ;
			
			pPort->uiCurrentSpeed = i ;
			return ATAPortManager_SUCCESS ;
		}
	}

	return ATAPortManager_SUCCESS ;
}

byte ATAPortManager_IOWait(ATAPort* pPort, unsigned uiMask, unsigned uiValue)
{
	byte bStatus = 0 ;
	byte i ;

	for(i = 0; i < 20; i++)
	{
		ATA_READ_REG(pPort, ATA_REG_STATUS, bStatus) ;

		if((bStatus & uiMask) == uiValue)
			return ATAPortManager_SUCCESS ;

		KernelUtil::Wait(ATA_CMD_TIMEOUT) ;
	}

	return ATAPortManager_ERR_TIMEOUT ;
}

byte ATAPortManager_IOWaitAlt(ATAPort* pPort, unsigned uiMask, unsigned uiValue)
{
	byte bStatus = 0 ;
	byte i ;

	for(i = 0; i < 200; i++)
	{
		KernelUtil::Wait(ATA_CMD_TIMEOUT) ;

		ATA_READ_REG(pPort, ATA_REG_CONTROL, bStatus) ;

		if((bStatus & uiMask) == uiValue)
			return ATAPortManager_SUCCESS ;
	}

	return ATAPortManager_ERR_TIMEOUT ;
}

byte ATAPortManager_IORead(ATAPort* pPort, void* pBuffer, unsigned uiLength)
{
	byte bStatus = 0 ;
	byte bError = 0 ;
	unsigned uiTransfered = 0 ;

	byte to ;
	for(to = 0; to < 10; to++)
	{
		KernelUtil::Wait(ATA_CMD_TIMEOUT) ;
		ATA_READ_REG(pPort, ATA_REG_ALTSTATUS, bStatus) ;
		
		if(!(bStatus & ATA_STATUS_DRQ) && !(bStatus & ATA_STATUS_ERROR))
		{
			continue ;
		}
		else
		{
			if(bStatus & ATA_STATUS_ERROR)
			{
				ATA_READ_REG(pPort, ATA_STATUS_ERROR, bError) ;
				break ;
			}

			if(bStatus & ATA_STATUS_DRQ)
			{
				unsigned i ;

				if(pPort->bPIO32Bit)
				{
					unsigned* uiData = (unsigned*)((unsigned)pBuffer + uiTransfered) ;

					for(i = 0; i < 128; i++)
						ATA_READ_REG32(pPort, ATA_REG_DATA, uiData[i]) ;
				}
				else
				{
					unsigned short *usData = (unsigned short*)((unsigned)pBuffer + uiTransfered) ;

					for(i = 0; i < 256; i++)
						ATA_READ_REG16(pPort, ATA_REG_DATA, usData[i]) ;
				}

				uiTransfered += 512 ;

				if(uiTransfered >= uiLength)
					return ATAPortManager_SUCCESS ;

				ATA_READ_REG(pPort, ATA_REG_STATUS, bStatus) ;
				KernelUtil::Wait(ATA_CMD_DELAY) ;
				to = 0 ;
				continue ;
			}
		}
	}

	return ATAPortManager_ERR_TIMEOUT ;
}

byte ATAPortManager_IOWrite(ATAPort* pPort, void* pBuffer, unsigned uiLength)
{
	byte bStatus = 0 ;
	byte bError = 0 ;
	unsigned uiTransfered = 0 ;
	unsigned i ;
	byte* bData = (byte*)pBuffer ;

	byte to ;
	for(to = 0; to < 10; to++)
	{
		KernelUtil::Wait(ATA_CMD_TIMEOUT) ;
		
		ATA_READ_REG(pPort, ATA_REG_ALTSTATUS, bStatus) ;
		
		if(!(bStatus & ATA_STATUS_DRQ) && !(bStatus & ATA_STATUS_ERROR))
			continue ;
		else
		{
			if(bStatus & ATA_STATUS_ERROR)
			{
				ATA_READ_REG(pPort, ATA_REG_ERROR, bError) ;
				break ;
			}

			if(bStatus & ATA_STATUS_DRQ)
			{
				if(pPort->bPIO32Bit)
				{
					unsigned* uiData = (unsigned*)pBuffer ;
					for(i = 0; i < 128; i++, uiData++)
					{
						ATA_WRITE_REG32(pPort, ATA_REG_DATA, *uiData) ;
					}
				}
				else
				{
					unsigned short *usData = (unsigned short*)pBuffer ;

					for(i = 0; i < 256; i++, usData++)
					{
						ATA_WRITE_REG16(pPort, ATA_REG_DATA, *usData) ;
					}
				}

				bData += 512 ;
				uiTransfered += 512 ;

				if(uiTransfered >= uiLength)
					return ATAPortManager_SUCCESS ;

				ATA_READ_REG(pPort, ATA_REG_STATUS, bStatus) ;
				KernelUtil::Wait(ATA_CMD_DELAY) ;
				to = 0 ;
				continue ;
			}
		}
	}

	return ATAPortManager_ERR_TIMEOUT ;
}
