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
# include <ATAVIA.h>
# include <ATATimingManager.h>
# include <DMM.h>
# include <Display.h>
# include <StringUtil.h>
# include <MemUtil.h>

static VIAIDE VIAIDEList[] = {
    { "VT8237",    0x3227, 0x00, 0x2F, VIA_UDMA_133 | VIA_BAD_AST	},
    { "VT8235",    0x3177, 0x00, 0x2F, VIA_UDMA_133 | VIA_BAD_AST	},
    { "VT8233a",   0x3147, 0x00, 0x2F, VIA_UDMA_133 | VIA_BAD_AST	},
    { "VT8233c",   0x3109, 0x00, 0x2F, VIA_UDMA_100			},
    { "VT8233",    0x3074, 0x00, 0x2F, VIA_UDMA_100			},
    { "VT8231",    0x8231, 0x00, 0x2F, VIA_UDMA_100			},
    { "VT82C686b", 0x0686, 0x40, 0x4F, VIA_UDMA_100			},
    { "VT82C686a", 0x0686, 0x10, 0x2F, VIA_UDMA_66			},
    { "VT82C686",  0x0686, 0x00, 0x0F, VIA_UDMA_33 | VIA_BAD_CLK66	},
    { "VT82C596b", 0x0596, 0x10, 0x2F, VIA_UDMA_66			},
    { "VT82C596a", 0x0596, 0x00, 0x0F, VIA_UDMA_33 | VIA_BAD_CLK66	},
    { "VT82C586b", 0x0586, 0x47, 0x4F, VIA_UDMA_33 | VIA_SET_FIFO	},
    { "VT82C586b", 0x0586, 0x40, 0x46, VIA_UDMA_33 | VIA_SET_FIFO | VIA_BAD_CLK66 },
    { "VT82C586b", 0x0586, 0x30, 0x3F, VIA_UDMA_33 | VIA_SET_FIFO	},
    { "VT82C586a", 0x0586, 0x20, 0x2F, VIA_UDMA_33 | VIA_SET_FIFO	},
    { "VT82C586",  0x0586, 0x00, 0x0F, VIA_UDMA_NONE| VIA_SET_FIFO	},
    { "VT82C576b", 0x0576, 0x00, 0x2F, VIA_UDMA_NONE | VIA_SET_FIFO | VIA_NO_UNMASK }
} ;

/*************************************** Static Functions **************************************************/

static byte ATAVIA_SetPortSpeed(VIAIDE* pVIAIDE, PCIEntry* pPCIEntry, ATAPort* pPort, ATATiming* pTiming)
{
	unsigned uiDriveNumber = pPort->uiChannel * pPort->pController->uiPortsPerChannel + pPort->uiPort ;
	byte bTemp ;

	if(!(pVIAIDE->usFlags & VIA_BAD_AST))
	{
		RETURN_X_IF_NOT(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, VIA_ADDRESS_SETUP, 1, &bTemp), Success, ATAVIA_FAILURE) ;

		bTemp = (bTemp & ~(3 << ((3 - uiDriveNumber) << 1))) |
				((FIT(pTiming->usSetup, 1, 4) - 1) << ((3 - uiDriveNumber) << 1)) ;

		RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, VIA_ADDRESS_SETUP, 1, bTemp), Success, ATAVIA_FAILURE) ;
	}

	RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
			pPCIEntry->uiFunction, VIA_8BIT_TIMING + (1 - (uiDriveNumber >> 1)), 1, 
			((FIT(pTiming->usAct8b, 1, 16) - 1) << 4) | 
			(FIT(pTiming->usRec8b, 1, 16) - 1)), Success, ATAVIA_FAILURE) ;

	RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
			pPCIEntry->uiFunction, VIA_DRIVE_TIMING + (3 - uiDriveNumber), 1, 
			((FIT(pTiming->usAct, 1, 16) - 1) << 4) | 
			(FIT(pTiming->usRec, 1, 16) - 1)), Success, ATAVIA_FAILURE) ;

	switch(pVIAIDE->usFlags & VIA_UDMA)
	{
		case VIA_UDMA_33:
			bTemp = pTiming->usUDMA ? (0xE0 | (FIT(pTiming->usUDMA, 2, 5) - 2)) : 0x03 ;
			break ;

		case VIA_UDMA_66:
			bTemp = pTiming->usUDMA ? (0xE8 | (FIT(pTiming->usUDMA, 2, 9) - 2)) : 0x0F ;
			break ;

		case VIA_UDMA_100:
			bTemp = pTiming->usUDMA ? (0xE0 | (FIT(pTiming->usUDMA, 2, 9) - 2)) : 0x07 ;
			break ;

		case VIA_UDMA_133:
			bTemp = pTiming->usUDMA ? (0xE0 | (FIT(pTiming->usUDMA, 2, 9) - 2)) : 0x07 ;
			break ;

		default:
			return ATAVIA_SUCCESS ;
	}

	RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
		pPCIEntry->uiFunction, VIA_UDMA_TIMING + (3 - uiDriveNumber), 1, bTemp), Success, ATAVIA_FAILURE) ;

	return ATAVIA_SUCCESS ;
}

/***************************************************************************************************/

byte ATAVIA_PortConfigure(ATAPort* pPort)
{
	ATAController* pController = pPort->pController ;
	ATAPort* pFirstPort = NULL ;
	ATAPort* pSecondPort = NULL ;

	ATATiming ataTimingResult1, ataTimingResult2 ;
	unsigned uiTiming, uiUDMATiming ;

	VIAIDEInfo* pVIAIDEInfo = (VIAIDEInfo*)pPort->pVendorSpecInfo ;
	VIAIDE* pVIAIDE = pVIAIDEInfo->pVIAIDE ;

	// Check Configuration
	if(pPort->uiPort == 0 && !(pPort->uiDevice >= ATA_DEV_ATA &&
		pController->pPort[pPort->uiChannel * pController->uiPortsPerChannel + 1]->uiDevice < ATA_DEV_ATA))
		return ATAVIA_SUCCESS ;

	if(pPort->uiPort == 0)
		pFirstPort = pPort ;

	if(pPort->uiPort == 1)
	{
		if(pPort->uiDevice < ATA_DEV_ATA)
			return ATAVIA_SUCCESS ;

		if(pController->pPort[pPort->uiChannel * pController->uiPortsPerChannel]->uiDevice < ATA_DEV_ATA)
		{
			pFirstPort = pPort ;
		}
		else
		{
			pFirstPort = pController->pPort[pPort->uiChannel * pController->uiPortsPerChannel] ;
			pSecondPort = pPort ;
		}
	}

	// Calculate Timing 
	uiTiming = 1000000000 / VIA_CLOCK ;

	switch(pVIAIDE->usFlags & VIA_UDMA)
	{
		case VIA_UDMA_33:
			uiUDMATiming = uiTiming ;
			break ;

		case VIA_UDMA_66:
			uiUDMATiming = uiTiming / 2 ;
			break ;

		case VIA_UDMA_100:
			uiUDMATiming = uiTiming / 3 ;
			break ;

		case VIA_UDMA_133:
			uiUDMATiming = uiTiming / 4 ;
			break ;

		default: 
			uiUDMATiming = uiTiming ;
	}

	// Calculate Timing of First Port
	RETURN_X_IF_NOT(ATATimingManager_Compute(pFirstPort, pFirstPort->uiCurrentSpeed, &ataTimingResult1, 
			uiTiming, uiUDMATiming), ATATimingManager_SUCCESS, ATAVIA_FAILURE) ;

	// Calculate Timing of Second Port and Merge Both
	if(pSecondPort)
	{
		RETURN_X_IF_NOT(ATATimingManager_Compute(pSecondPort, pSecondPort->uiCurrentSpeed, &ataTimingResult2, 
				uiTiming, uiUDMATiming), ATATimingManager_SUCCESS, ATAVIA_FAILURE) ;
		
		ATATimingManager_Merge(&ataTimingResult2, &ataTimingResult1, &ataTimingResult1, ATA_TIMING_8BIT) ;
	}

	// Set Speed
	if(pSecondPort)
	{
		RETURN_X_IF_NOT(ATAVIA_SetPortSpeed(pVIAIDE, &pVIAIDEInfo->pciEntry, pSecondPort, &ataTimingResult2),
			ATAVIA_SUCCESS, ATAVIA_FAILURE) ;
	}

	if(pFirstPort)
	{
		RETURN_X_IF_NOT(ATAVIA_SetPortSpeed(pVIAIDE, &pVIAIDEInfo->pciEntry, pFirstPort, &ataTimingResult1),
			ATAVIA_SUCCESS, ATAVIA_FAILURE) ;
	}

	return ATAVIA_SUCCESS ;
}

void ATAVIA_InitController(const PCIEntry* pPCIEntry, ATAController* pController)
{
	// Check for VIA SATA
	if(pPCIEntry->usVendorID == VIA_VENDOR_ID && pPCIEntry->usDeviceID == 0x3149)
	{
		String_Copy(pController->szName, "VIA Serial ATA Controller") ;

		DMM_DeAllocateForKernel((unsigned)pController->pPort[1]) ;
		DMM_DeAllocateForKernel((unsigned)pController->pPort[3]) ;

		pController->pPort[1] = pController->pPort[2] ;
		pController->pPort[2] = NULL ;
		pController->uiPortsPerChannel = 1 ;
		pController->pPort[0]->uiCable = pController->pPort[1]->uiCable = ATA_CABLE_SATA ;

		return ;
	}

	// Scan PCI Bus
	unsigned uiPCIIndex, uiDeviceIndex ;
	byte bIDEFound = false ;
	PCIEntry* pIDE ;
	VIAIDE* pVIAIDE = NULL ;

	for(uiPCIIndex = 0; uiPCIIndex < PCIBusHandler_uiDeviceCount; uiPCIIndex++)
	{
		if(PCIBusHandler_GetPCIEntry(&pIDE, uiPCIIndex) != Success)
			break ;
	
		if(pIDE->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		for(uiDeviceIndex = 0; uiDeviceIndex < (sizeof(VIAIDEList) / sizeof(VIAIDE)); uiDeviceIndex++)
		{
			if(pIDE->usVendorID == VIA_VENDOR_ID && pIDE->usDeviceID == VIAIDEList[uiDeviceIndex].usDeviceID
				&& pIDE->bRevisionID >= VIAIDEList[uiDeviceIndex].bMinRevision
				&& pIDE->bRevisionID <= VIAIDEList[uiDeviceIndex].bMaxRevision)
			{
				pVIAIDE = &VIAIDEList[uiDeviceIndex] ;
				bIDEFound = true ;
				break ;
			}
        }

		if(bIDEFound == true)
			break ;
    }

	if(bIDEFound == false)
	{
		KC::MDisplay().Message("\n\tUnKnown VIA ATA Controller Detected", Display::WHITE_ON_BLACK()) ;
		return ;
	}

	KC::MDisplay().Message("\n\tVIA ", Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message(pVIAIDE->szName, Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message(" Controller Detected", Display::WHITE_ON_BLACK()) ;

	// Check Cable Type
	unsigned uiUDMATiming, ui80W = 0 ;
	int i ;
	switch(pVIAIDE->usFlags & VIA_UDMA)
	{
		case VIA_UDMA_66:
			if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 
				VIA_UDMA_TIMING, 4, &uiUDMATiming)
				!= Success)
			{
				KC::MDisplay().Message("\n\tFailed to Init VIA Controller", Display::WHITE_ON_BLACK()) ;
				return ;
			}

			if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 
				VIA_UDMA_TIMING, 4, uiUDMATiming | 0x80008)
				!= Success)
			{
				KC::MDisplay().Message("\n\tFailed to Init VIA Controller", Display::WHITE_ON_BLACK()) ;
				return ;
			}

			for(i = 24; i >= 0; i -= 8)
			{
				if(((uiUDMATiming >> (i & 16)) & 8) && ((uiUDMATiming >> i) & 0x20) && (((uiUDMATiming >> i) & 7) < 2))
				{
					//BIOS 80-wire bit or UDMA w/ < 60ns/cycle
					ui80W |= (1 << (1 - (i >> 4))) ;	
				}
			}
			break ;

		case VIA_UDMA_100:
			if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 
				VIA_UDMA_TIMING, 4, &uiUDMATiming)
				!= Success)
			{
				KC::MDisplay().Message("\n\tFailed to Init VIA Controller", Display::WHITE_ON_BLACK()) ;
				return ;
			}

			for(i = 24; i >= 0; i -= 8)
			{
				if(((uiUDMATiming >> i) & 0x10) && ((uiUDMATiming >> i) & 0x20) && (((uiUDMATiming >> i) & 7) < 4))
				{
					//BIOS 80-wire bit or UDMA w/ < 60ns/cycle
					ui80W |= (1 << (1 - (i >> 4))) ;	
				}
			}
			break ;

		case VIA_UDMA_133:
			if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 
				VIA_UDMA_TIMING, 4, &uiUDMATiming)
				!= Success)
			{
				KC::MDisplay().Message("\n\tFailed to Init VIA Controller", Display::WHITE_ON_BLACK()) ;
				return ;
			}

			for(i = 24; i >= 0; i -= 8)
			{
				if(((uiUDMATiming >> i) & 0x10) || (((uiUDMATiming >> i) & 0x20) && (((uiUDMATiming >> i) & 7) < 6)))
				{
					//BIOS 80-wire bit or UDMA w/ < 60ns/cycle
					ui80W |= (1 << (1 - (i >> 4))) ;	
				}
			}
			break ;
	}
				
	pController->pPort[0]->uiCable = pController->pPort[1]->uiCable =
		(ui80W & 0x01) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;

	pController->pPort[2]->uiCable = pController->pPort[3]->uiCable =
		(ui80W & 0x02) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;

	if(pVIAIDE->usFlags & VIA_BAD_CLK66)
	{
		if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction,
				VIA_UDMA_TIMING, 4, &uiUDMATiming) != Success)
		{
			KC::MDisplay().Message("\n\tFailed to Init VIA Controller", Display::WHITE_ON_BLACK()) ;
			return ;
		}

		if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction,
				VIA_UDMA_TIMING, 4, uiUDMATiming & ~0x80008) != Success)
		{
			KC::MDisplay().Message("\n\tFailed to Init VIA Controller", Display::WHITE_ON_BLACK()) ;
			return ;
		}
	}
	
	// Check if Interfaces are Enabled
	byte bIDEEnabled, bFIFOConfig ;

	if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction,
			VIA_IDE_ENABLE, 1, &bIDEEnabled) != Success)
	{
		KC::MDisplay().Message("\n\tFailed to Init VIA Controller", Display::WHITE_ON_BLACK()) ;
		return ;
	}

	if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction,
			VIA_FIFO_CONFIG, 1, &bFIFOConfig) != Success)
	{
		KC::MDisplay().Message("\n\tFailed to Init VIA Controller", Display::WHITE_ON_BLACK()) ;
		return ;
	}

	if(pVIAIDE->usFlags & VIA_BAD_PREQ)
		bFIFOConfig &= 0x7F ;

	// Set FIFO Sizes
	if(pVIAIDE->usFlags & VIA_SET_FIFO)
	{
		bFIFOConfig &= (bFIFOConfig & 0x9F) ;
		switch(bIDEEnabled & 0x3)
		{
			case 1:
				bFIFOConfig |= 0x60 ;
				break ;

			case 2:
				bFIFOConfig |= 0x00 ;
				break ;

			case 3:
				bFIFOConfig |= 0x20 ;
				break ;
		}
	}	

	if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction,
			VIA_FIFO_CONFIG, 1, bFIFOConfig) != Success)
	{
		KC::MDisplay().Message("\n\tFailed to Init VIA Controller", Display::WHITE_ON_BLACK()) ;
		return ;
	}

	// Add Speeds
	VIAIDEInfo* pVIAIDEInfo = (VIAIDEInfo*)DMM_AllocateForKernel(sizeof(VIAIDEInfo)) ;

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&pVIAIDEInfo->pciEntry, MemUtil_GetDS(), (unsigned)pPCIEntry, 
						sizeof(PCIEntry)) ;
	pVIAIDEInfo->pVIAIDE = pVIAIDE ;

	for(unsigned i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
	{
		ATAPort* pPort = pController->pPort[i] ;
		byte b80Bit = (i & 0x02) ? (ui80W & 0x02) : (ui80W & 0x01) ;

		//Add all MultiWord DMA and PIO Modes
		pPort->uiSupportedPortSpeed = 0xFF ;

		if(pVIAIDE->usFlags & VIA_UDMA)
			pPort->uiSupportedPortSpeed |= 0x700 ;

		if(pVIAIDE->usFlags & VIA_UDMA_66 & b80Bit)
			pPort->uiSupportedPortSpeed |= 0x1F00 ;
		
		if(pVIAIDE->usFlags & VIA_UDMA_100 & b80Bit)
			pPort->uiSupportedPortSpeed |= 0x3FFF ;

		if(pVIAIDE->usFlags & VIA_UDMA_133 & b80Bit)
			pPort->uiSupportedPortSpeed |= 0x7FFF ;

		pPort->portOperation.Configure = ATAVIA_PortConfigure ;
		pPort->pVendorSpecInfo = pVIAIDEInfo ;
	}

	String_Copy(pController->szName, "VIA ") ;
	String_CanCat(pController->szName, pVIAIDE->szName) ;
}
