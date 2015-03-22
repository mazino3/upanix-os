/*
 *	Mother Operating System - An x86 based Operating System
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
# include <ATAIntel.h>
# include <String.h>
# include <DMM.h>
# include <Display.h>
# include <MemUtil.h>

static IntelIDE IntelIDEList[] = {
    { "PIIXa",     0x122E, 0 },
    { "PIIXb",     0x1230, 0 },
    { "MPIIX",     0x1234, 0 },
    { "PIIX3",     0x7010, 0 },
    { "PIIX4",     0x7111, INTEL_UDMA_33 },
    { "PIIX4",     0x7198, INTEL_UDMA_33 },
    { "PIIX4",     0x7601, INTEL_UDMA_66 },
    { "PIIX4",     0x84CA, INTEL_UDMA_33 },
    { "ICHO",      0x2421, INTEL_UDMA_33 | INTEL_NO_MWDMA0 | INTEL_INIT },
    { "ICH",       0x2411, INTEL_UDMA_66 | INTEL_NO_MWDMA0 | INTEL_INIT | INTEL_80W },
    { "ICH2",      0x244B, INTEL_UDMA_100 | INTEL_NO_MWDMA0 | INTEL_INIT | INTEL_80W },
    { "ICH2M",     0x244A, INTEL_UDMA_100 | INTEL_NO_MWDMA0 | INTEL_INIT | INTEL_80W },
    { "ICH3M",     0x248A, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
    { "ICH3",      0x248B, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
    { "ICH4",      0x24CB, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
    { "ICH5",      0x24DB, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
    { "C-ICH",     0x245B, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
    { "ICH4",      0x24CA, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
    { "ICH5 SATA", 0x24D1, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
    //{ "ICHBeta",   0x2651, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
    //{ "ICHBeta2",  0x266A, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W }
} ;

/************************************** Static Functions **********************************************/
static void ATAIntel_PortConfigurePIO(ATAPort *pPort)
{
	IntelIDEInfo* pIntelIDEInfo = (IntelIDEInfo*)pPort->pVendorSpecInfo ;
	PCIEntry* pPCIEntry = &pIntelIDEInfo->pciEntry ;

	unsigned uiMasterPort = pPort->uiChannel ? 0x42 : 0x40 ;
	unsigned uiSlavePort = 0x44 ;
	byte bIsSlave = (pPort->uiPort == 1) ;

	byte bTimings[][2] = {
		{ 0, 0 },
		{ 0, 0 },
		{ 1, 0 },
		{ 2, 1 },
		{ 2, 3 },
	} ;

	int iPIO = -1 ;
	unsigned i ;

	for(i = ATA_SPEED_PIO_5; i >= 0; i--)
	{
		if( (pPort->uiSupportedPortSpeed & ( 1 << i)) 
			&& (pPort->uiSupportedPortSpeed & (1 << i))) // TODO: Why Same Check again.... 
		{
			iPIO = i + 2 ;
			break ;
		}
	}

	if(iPIO < 0)
		return ;

	unsigned short usMasterData ;
	byte bSlaveData ;

	if(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
		uiMasterPort, 2, &usMasterData) != PCIBusHandler_SUCCESS)
		return ;

	if(bIsSlave)
	{
		usMasterData |= 0x4000 ;

	if(iPIO > 1) // Enable PPE, IE and TIME
		usMasterData |= 0x0070 ;

	if(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
		uiSlavePort, 1, &bSlaveData) != PCIBusHandler_SUCCESS)
		return ;

	bSlaveData = bSlaveData & ( pPort->uiChannel ? 0x0F : 0xF0) ;
	bSlaveData = bSlaveData | ( ( ( bTimings[iPIO][0] << 2 ) | 
								bTimings[iPIO][1] ) << ( pPort->uiChannel ? 4 : 0 ) ) ;
	}
	else
	{
		usMasterData |= 0xCCF8 ;

		if(iPIO > 1) // Enable PPE, IE and TIME
			usMasterData |= 0x0007 ;

		usMasterData = usMasterData | ( bTimings[iPIO][0] << 12 ) | ( bTimings[iPIO][1] << 8 ) ;
	}

	PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
		uiMasterPort, 2, usMasterData) ;

	PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
		uiSlavePort, 1, bSlaveData) ;
}
/**********************************************************************************************************/

byte ATAIntel_PortConfigure(ATAPort* pPort)
{
	IntelIDEInfo* pIntelIDEInfo = (IntelIDEInfo*)pPort->pVendorSpecInfo ;
	PCIEntry* pPCIEntry = &pIntelIDEInfo->pciEntry ;
	
	byte bMaSlave = pPort->uiChannel ? 0x42 : 0x40 ;
	unsigned uiCurrentSpeed = pPort->uiCurrentSpeed ;

	unsigned uiDriveNumber = pPort->uiChannel * pPort->pController->uiPortsPerChannel + pPort->uiPort ;
	unsigned uiSpeedA = 3 << ( uiDriveNumber * 4 ) ;
	unsigned uiFlagU = 1 << uiDriveNumber ;
	unsigned uiFlagV = 1 << uiDriveNumber ;
	unsigned uiFlagW = 0x10 << uiDriveNumber ;
	unsigned uiSpeedU = 0 ;

	unsigned short usReg4042, usReg44, usReg48, usReg4A, usReg54 ;
	byte bReg55 ;

	RETURN_X_IF_NOT(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
			bMaSlave, 2, &usReg4042), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;

	RETURN_X_IF_NOT(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
			0x44, 2, &usReg44), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;

	RETURN_X_IF_NOT(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
			0x48, 2, &usReg48), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;
	
	RETURN_X_IF_NOT(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
			0x4A, 2, &usReg4A), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;

	RETURN_X_IF_NOT(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
			0x54, 2, &usReg54), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;

	RETURN_X_IF_NOT(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
			0x55, 1, &bReg55), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;

	switch(uiCurrentSpeed)
	{
		case ATA_SPEED_UDMA_4:
		case ATA_SPEED_UDMA_2:
			uiSpeedU = 2 << ( uiDriveNumber * 4 ) ;
			break ;

		case ATA_SPEED_UDMA_5:
		case ATA_SPEED_UDMA_3:
		case ATA_SPEED_UDMA_1:
			uiSpeedU = 1 << ( uiDriveNumber * 4 ) ;
			break ;

		case ATA_SPEED_UDMA_0:
			uiSpeedU = 0 << ( uiDriveNumber * 4 ) ; // TODO: 0 << N ?
			break ;
	
		case ATA_SPEED_MWDMA_0:
		case ATA_SPEED_MWDMA_1:
		case ATA_SPEED_MWDMA_2:
				break ;

		case ATA_SPEED_PIO_5:
		case ATA_SPEED_PIO_4:
		case ATA_SPEED_PIO_3:
		case ATA_SPEED_PIO:
				break ;

		default:
			return ATAIntel_FAILURE ;
	}

	if(uiCurrentSpeed >= ATA_SPEED_UDMA_0)
	{
		if(! ( usReg48 & uiFlagU) )
		{
			RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, 0x48, 2, usReg48 | uiFlagU), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;
		}

		if(uiCurrentSpeed == ATA_SPEED_UDMA_5)
		{
			RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, 0x55, 1, (byte) bReg55 | uiFlagW), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;
		}
		else
		{
			RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, 0x55, 1, (byte) bReg55 & ~uiFlagW), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;
		}

		if(! ( usReg4A & uiSpeedU) )
		{
			RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, 0x4A, 2, usReg4A & ~uiSpeedA), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;

			RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, 0x4A, 2, usReg4A | uiSpeedU), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;
		}

		if(uiCurrentSpeed > ATA_SPEED_UDMA_2)
		{
			if(! ( usReg54 & uiFlagV) )
			{
				RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
					pPCIEntry->uiFunction, 0x54, 2, usReg54 | uiFlagV), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;
			}
		}
		else
		{
			RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, 0x54, 2, usReg54 & ~uiFlagV), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;
		}
	}
	else
	{
		if( usReg48 & uiFlagU)
		{
			RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, 0x48, 2, usReg48 & ~uiFlagU), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;
		}

		if(usReg4A & uiSpeedA)
		{
			RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, 0x4A, 2, usReg4A & ~uiSpeedA), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;
		}

		if(usReg54 & uiFlagV)
		{
			RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, 0x54, 2, usReg54 & ~uiFlagV), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;
		}
	
		if(bReg55 & uiFlagW)
		{
			RETURN_X_IF_NOT(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, 
				pPCIEntry->uiFunction, 0x55, 2, bReg55 & ~uiFlagW), PCIBusHandler_SUCCESS, ATAIntel_FAILURE) ;
		}	
	}

	ATAIntel_PortConfigurePIO(pPort) ;
	return ATAIntel_SUCCESS ;
}

void ATAIntel_InitController(const PCIEntry* pPCIEntry, ATAController* pController)
{
	// Check for SATA Controller
	if(pPCIEntry->usDeviceID == INTEL_SATA_DEVICE_ID)
	{
		String_Copy(pController->szName, "Intel Serial ATA Controller") ;
		
		DMM_DeAllocateForKernel((unsigned)pController->pPort[1]) ;
		DMM_DeAllocateForKernel((unsigned)pController->pPort[3]) ;
		
		pController->pPort[1] = pController->pPort[2] ;
		pController->pPort[2] = NULL ;

		pController->uiPortsPerChannel = 1 ;

		pController->pPort[0]->uiCable = pController->pPort[1]->uiCable = ATA_CABLE_SATA ;
		return ;
	}

//TODO: Don't know why again PCI Bus needs to be Scanned as pPCIEntry is sent as parameter already
	// Scan PCI Bus
	unsigned uiPCIIndex, uiDeviceIndex ;
	byte bIDEFound = false ;
	PCIEntry* pIDE ;
	IntelIDE* pIntelIDE = NULL ;

	for(uiPCIIndex = 0; uiPCIIndex < PCIBusHandler_uiDeviceCount; uiPCIIndex++)
	{
		if(PCIBusHandler_GetPCIEntry(&pIDE, uiPCIIndex) != PCIBusHandler_SUCCESS)
			break ;
	
		if(pIDE->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		for(uiDeviceIndex = 0; uiDeviceIndex < (sizeof(IntelIDEList) / sizeof(IntelIDE)); uiDeviceIndex++)
		{
			if(pIDE->usVendorID == INTEL_VENDOR_ID && pIDE->usDeviceID == IntelIDEList[uiDeviceIndex].usDeviceID)
			{
				pIntelIDE = &IntelIDEList[uiDeviceIndex] ;
				bIDEFound = true ;
				break ;
			}
        }

		if(bIDEFound == true)
			break ;
    }

	if(bIDEFound == false)
	{
		KC::MDisplay().Message("\n\tUnknown Intel ATA Controller detected", Display::WHITE_ON_BLACK()) ;
		return ;
	}
	
	KC::MDisplay().Message("\n\tIntel ", Display::WHITE_ON_BLACK()) ;	
	KC::MDisplay().Message(pIntelIDE->szName, Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message(" ATA Controller Detected", Display::WHITE_ON_BLACK()) ;

	if(pIntelIDE->usFlags & INTEL_INIT)
	{
		unsigned uiExtra ;
		if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x54, 4, &uiExtra)
			!= PCIBusHandler_SUCCESS)
		{
			KC::MDisplay().Message("\n\tFailed to Init Intel Controller", Display::WHITE_ON_BLACK()) ;
			return ;
		}

		if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x54, 4, uiExtra | 0x400) != PCIBusHandler_SUCCESS)
		{
			KC::MDisplay().Message("\n\tFailed to Init Intel Controller", Display::WHITE_ON_BLACK()) ;
			return ;
		}
	}	

	unsigned ui80W = 0 ;
	if(pIntelIDE->usFlags & INTEL_80W)
	{
		byte b54, b55 ;

		if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x54, 1, &b54)
			!= PCIBusHandler_SUCCESS)
		{
			KC::MDisplay().Message("\n\tFailed to Init Intel Controller", Display::WHITE_ON_BLACK()) ;
			return ;
		}

		if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x55, 1, &b55)
			!= PCIBusHandler_SUCCESS)
		{
			KC::MDisplay().Message("\n\tFailed to Init Intel Controller", Display::WHITE_ON_BLACK()) ;
			return ;
		}

		if(b54 & 0x30)
			ui80W |= 0x01 ;

		if(b55 & 0xC0) //TODO: Verify that it is b55 and not b54
			ui80W |= 0x02 ;
	}

	pController->pPort[0]->uiCable = pController->pPort[1]->uiCable = 
		(ui80W & 0x01) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;
	pController->pPort[2]->uiCable = pController->pPort[3]->uiCable = 
		(ui80W & 0x02) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;

	IntelIDEInfo* pIntelIDEInfo = (IntelIDEInfo*)DMM_AllocateForKernel(sizeof(IntelIDEInfo)) ;

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pPCIEntry, MemUtil_GetDS(), (unsigned)&pIntelIDEInfo->pciEntry, sizeof(PCIEntry)) ;
	
	unsigned i ;
	for(i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
	{
		ATAPort* pPort = pController->pPort[i] ;

		pPort->uiSupportedPortSpeed = 0xFF ;
		byte b80Bit = ( i & 0x02 ) ? ( ui80W & 0x02 ) : ( ui80W & 0x01) ;
	
		if(pIntelIDE->usFlags & INTEL_NO_MWDMA0)
			pPort->uiSupportedPortSpeed = pPort->uiSupportedPortSpeed & ~0x20 ;

		if(pIntelIDE->usFlags & INTEL_UDMA)
			pPort->uiSupportedPortSpeed |= 0x700 ;
	
		if(pIntelIDE->usFlags & INTEL_UDMA_66 & b80Bit)
			pPort->uiSupportedPortSpeed |= 0x1F00 ;

		if(pIntelIDE->usFlags & INTEL_UDMA_100 & b80Bit)
			pPort->uiSupportedPortSpeed |= 0x3FFF ;

		if(pIntelIDE->usFlags & INTEL_UDMA_133 & b80Bit)
			pPort->uiSupportedPortSpeed |= 0x7FFF ;

		pPort->portOperation.Configure = ATAIntel_PortConfigure ;
		pPort->pVendorSpecInfo = pIntelIDEInfo ;
	}

	String_Copy(pController->szName, "Intel ") ;
	String_CanCat(pController->szName, pIntelIDE->szName) ;
}
