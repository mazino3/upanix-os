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
# include <ATASIS.h>
# include <Display.h>
# include <String.h>
# include <MemUtil.h>
# include <DMM.h>

static SISIDE SISIDEList[] = {
    { "SiS 745",    0x0745, SIS_UDMA_100	},
    { "SiS 735",    0x0735, SIS_UDMA_100	},
    { "SiS 733",    0x0733, SIS_UDMA_100	},
    { "SiS 635",    0x0635, SIS_UDMA_100	},
    { "SiS 633",    0x0633, SIS_UDMA_100	},
    { "SiS 730",    0x0730, SIS_UDMA_100a	},
    { "SiS 550",    0x0550, SIS_UDMA_100a	},
    { "SiS 640",    0x0640, SIS_UDMA_66		},
    { "SiS 630",    0x0630, SIS_UDMA_66		},
    { "SiS 620",    0x0620, SIS_UDMA_66		},
    { "SiS 540",    0x0540, SIS_UDMA_66		},
    { "SiS 530",    0x0530, SIS_UDMA_66		},
    { "SiS 5600",   0x5600, SIS_UDMA_33		},
    { "SiS 5598",   0x5598, SIS_UDMA_33		},
    { "SiS 5597",   0x5597, SIS_UDMA_33		},
    { "SiS 5591/2", 0x5591, SIS_UDMA_33		},
    { "SiS 5582",   0x5582, SIS_UDMA_33		},
    { "SiS 5581",   0x5581, SIS_UDMA_33		},
    { "SiS 5596",   0x5596, SIS_16		},
    { "SiS 5571",   0x5571, SIS_16		},
    { "SiS 551x",   0x5511, SIS_16		}
} ;

// {0, ATA_16, ATA_33, ATA_66, ATA_100a, ATA_100, ATA_133}
static byte ATASIS_bCycleTimeOffset[] = { 0, 0, 5, 4, 4, 0, 0 } ;
static byte ATASIS_bCycleTimeRange[] = { 0, 0, 2, 3, 3, 4, 4 } ;
static byte ATASIS_bCycleTimeValue[][ ATA_SPEED_UDMA_6 - ATA_SPEED_UDMA_0 + 1 ] = {
    {  0,  0, 0, 0, 0, 0, 0 }, /* no udma */
    {  0,  0, 0, 0, 0, 0, 0 }, /* no udma */
    {  3,  2, 1, 0, 0, 0, 0 }, /* ATA_33 */
    {  7,  5, 3, 2, 1, 0, 0 }, /* ATA_66 */
    {  7,  5, 3, 2, 1, 0, 0 }, /* ATA_100a (730 specific), differences are on cycle_time range and offset */
    { 11,  7, 5, 4, 2, 1, 0 }, /* ATA_100 */
    { 15, 10, 7, 5, 3, 2, 1 }, /* ATA_133a (earliest 691 southbridges) */
    { 15, 10, 7, 5, 3, 2, 1 }, /* ATA_133 */
} ;

// CRC Valid Setup Time Vary Across IDE Clock Setting 33/66/100/133
//	Refer SiS962 Data Sheet.
static byte ATASIS_bCVSTimeValue[][ ATA_SPEED_UDMA_6 - ATA_SPEED_UDMA_0 + 1 ] = {
    { 0, 0, 0, 0, 0, 0, 0 }, /* no udma */
    { 0, 0, 0, 0, 0, 0, 0 }, /* no udma */
    { 2, 1, 1, 0, 0, 0, 0 },
    { 4, 3, 2, 1, 0, 0, 0 },
    { 4, 3, 2, 1, 0, 0, 0 },
    { 6, 4, 3, 1, 1, 1, 0 },
    { 9, 6, 4, 2, 2, 2, 2 },
    { 9, 6, 4, 2, 2, 2, 2 },
} ;

//TODO: Tune PIO Modes
byte ATASIS_PortConfigure(ATAPort* pPort)
{
	SISIDEInfo* pSISIDEInfo = (SISIDEInfo*)pPort->pVendorSpecInfo ;
	PCIEntry* pPCIEntry = &pSISIDEInfo->pciEntry ;
	unsigned uiSpeed = pSISIDEInfo->uiSpeed ;

	unsigned uiDriveNumber = pPort->uiChannel * pPort->pController->uiPortsPerChannel + pPort->uiPort ;
	unsigned uiRegDW = 0 ;
	byte bReg = 0 ;

	byte bDrivePCI = 0x40 ;

	unsigned uiNewSpeed = pPort->uiCurrentSpeed ;

	if(uiSpeed >= SIS_UDMA_133)
	{
		unsigned ui54 ;

		if(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction, 0x54, 4, 
			&ui54) != PCIBusHandler_SUCCESS)
		{
			KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
			return ATASIS_FAILURE ;
		}

		if(ui54 & 0x40000000)
			bDrivePCI = 0x70 ;

		bDrivePCI += uiDriveNumber * 0x04 ;

		if(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction, 
			(unsigned)bDrivePCI, 4, &uiRegDW) != PCIBusHandler_SUCCESS)
		{
			KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
			return ATASIS_FAILURE ;
		}

		if(uiNewSpeed < ATA_SPEED_UDMA_0)
		{
			uiRegDW &= 0xFFFFFFFB ;
			if(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction, 
				(unsigned)bDrivePCI, 4, uiRegDW) != PCIBusHandler_SUCCESS)
			{
				KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
				return ATASIS_FAILURE ;
			}
		}
	}
	else
	{
		bDrivePCI += uiDriveNumber * 0x02 ;
		if(PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction, 
			(unsigned)bDrivePCI + 1, 1, &bReg) != PCIBusHandler_SUCCESS)
		{
			KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
			return ATASIS_FAILURE ;
		}
		
		if((uiNewSpeed < ATA_SPEED_UDMA_0) && (uiSpeed > SIS_16))
		{
			bReg &= 0x7F ;
			if(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction, 
				(unsigned)bDrivePCI + 1, 1, bReg) != PCIBusHandler_SUCCESS)
			{
				KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
				return ATASIS_FAILURE ;
			}
		}
	}

	switch(uiNewSpeed)
	{
		case ATA_SPEED_UDMA_6:
		case ATA_SPEED_UDMA_5:
		case ATA_SPEED_UDMA_4:
		case ATA_SPEED_UDMA_3:
		case ATA_SPEED_UDMA_2:
		case ATA_SPEED_UDMA_1:
		case ATA_SPEED_UDMA_0:
			if(uiSpeed >= SIS_UDMA_133)
			{
				uiRegDW |= 0x04 ;
				uiRegDW &= 0xFFFFF00F ;

				if(uiRegDW & 0x08)
				{
					uiRegDW |= (unsigned)ATASIS_bCycleTimeValue[SIS_UDMA_133][uiNewSpeed - ATA_SPEED_UDMA_0] << 4 ;
					uiRegDW |= (unsigned)ATASIS_bCVSTimeValue[SIS_UDMA_133][uiNewSpeed - ATA_SPEED_UDMA_0] << 8 ;
				}
				else
				{
					if(uiNewSpeed > ATA_SPEED_UDMA_5)
						uiNewSpeed = ATA_SPEED_UDMA_5 ;

					uiRegDW |= (unsigned)ATASIS_bCycleTimeValue[SIS_UDMA_100][uiNewSpeed - ATA_SPEED_UDMA_0] << 4 ;
					uiRegDW |= (unsigned)ATASIS_bCVSTimeValue[SIS_UDMA_100][uiNewSpeed - ATA_SPEED_UDMA_0] << 8 ;
				}

				if(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction, 
					(unsigned)bDrivePCI, 4, uiRegDW) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ATASIS_FAILURE ;
				}
			}
			else
			{
				bReg |= 0x80 ;
				bReg &= ~((0xFF >> (8 - ATASIS_bCycleTimeRange[uiSpeed])) << ATASIS_bCycleTimeOffset[uiSpeed]) ;

				// Set Reg Cycle Time Bits

				bReg |= ATASIS_bCycleTimeValue[uiSpeed][uiNewSpeed - ATA_SPEED_UDMA_0] 
							<< ATASIS_bCycleTimeOffset[uiSpeed] ;
	
				if(PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction, 
					(unsigned)bDrivePCI + 1, 1, bReg) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ATASIS_FAILURE ;
				}
			}
			break ;
	}

	return ATASIS_SUCCESS ;
}

void ATASIS_InitController(const PCIEntry* pPCIEntry, ATAController* pController)
{
	PCIEntry* pIDE ;
	unsigned uiSpeed = 0 ;
	unsigned ui80W = 0 ;
		
	// Scan PCI Bus
	unsigned uiPCIIndex, uiDeviceIndex ;
	byte bIDEFound = false ;

	for(uiPCIIndex = 0; uiPCIIndex < PCIBusHandler_uiDeviceCount; uiPCIIndex++)
	{
		if(PCIBusHandler_GetPCIEntry(&pIDE, uiPCIIndex) != PCIBusHandler_SUCCESS)
			break ;
	
		if(pIDE->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		for(uiDeviceIndex = 0; uiDeviceIndex < (sizeof(SISIDEList) / sizeof(SISIDE)); uiDeviceIndex++)
		{
			if(pIDE->usVendorID == SIS_VENDOR_ID && pIDE->usDeviceID == SISIDEList[uiDeviceIndex].usDeviceID)
			{
				uiSpeed = SISIDEList[uiDeviceIndex].usFlags ;
				bIDEFound = true ;

				// Special case for SIS630 : 630S/ET is ATA_100a
				if(pIDE->usDeviceID == 0x0630 && pIDE->bRevisionID >= 0x30)
					uiSpeed = SIS_UDMA_100a ;	

				KC::MDisplay().Message("\n\t", Display::WHITE_ON_BLACK()) ;
				KC::MDisplay().Message(SISIDEList[uiDeviceIndex].szName, Display::WHITE_ON_BLACK()) ;
				KC::MDisplay().Message(" ATA Controller Detected", Display::WHITE_ON_BLACK()) ;
				String_Copy(pController->szName, SISIDEList[uiDeviceIndex].szName) ;
				break ;
			}
        }

		if(bIDEFound == true)
			break ;
    }

	if(bIDEFound == false)
	{
		unsigned uiMisc ;
		unsigned short usTrueID ;
		byte bPrefCtrl ;
		byte bIDEConfig ;
		byte bSBRev ;

		if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x54, 4, &uiMisc)
			!= PCIBusHandler_SUCCESS)
		{
			KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
			return ;
		}

		if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x54, 4, 
			(uiMisc & 0x7FFFFFFF)) != PCIBusHandler_SUCCESS)
		{
			KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
			return ;
		}

		if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, PCI_DEVICE_ID, 2, 
			&usTrueID) != PCIBusHandler_SUCCESS)
		{
			KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
			return ;
		}

		if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x54, 4, uiMisc) 
			!= PCIBusHandler_SUCCESS)
		{
			KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
			return ;
		}
		
		if(usTrueID == 0x5518)
		{
			uiSpeed = SIS_UDMA_133 ;
			KC::MDisplay().Message("\n\tSIS 962/963 MuTIOL IDE UDMA133 Controller Detected", Display::WHITE_ON_BLACK()) ;
		}
		else
		{
			if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x4A, 1, 
				&bIDEConfig) != PCIBusHandler_SUCCESS)
			{
				KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
				return ;
			}

			if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x4A, 1, 
				bIDEConfig | 0x10) != PCIBusHandler_SUCCESS)
			{
				KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
				return ;
			}

			if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, PCI_DEVICE_ID, 2, 
				&usTrueID) != PCIBusHandler_SUCCESS)
			{
				KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
				return ;
			}

			if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x4A, 1, 
				bIDEConfig) != PCIBusHandler_SUCCESS)
			{
				KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
				return ;
			}
			
			if(usTrueID == 0x5517)
			{
				if(PCIBusHandler_ReadPCIConfig(0, 2, 0, PCI_REVISION, 1, &bSBRev) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ;
				}

				if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x49, 1, 
					&bPrefCtrl) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ;
				}
			
				if(bSBRev == 0x10 && (bPrefCtrl & 0x80))
				{
					uiSpeed = SIS_UDMA_133a ;
					KC::MDisplay().Message("\n\tSIS 961B MuTIOL IDE UDMA133 Controller Detected", Display::WHITE_ON_BLACK()) ;
				}
				else
				{
					uiSpeed = SIS_UDMA_100 ;
					KC::MDisplay().Message("\n\tSIS 961 MuTIOL IDE UDMA100 Controller Detected", Display::WHITE_ON_BLACK()) ;
				}
			}
		}

		KC::MDisplay().Message("\n\tunKnown SIS ATA Controller Detected", Display::WHITE_ON_BLACK()) ;
		return ;
	}

	byte bReg ;
	unsigned short usRegW ;

	switch(uiSpeed)
	{
		case SIS_UDMA_133:
				if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x50, 2, 
					&usRegW) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ;
				}
				
				if(usRegW & 0x08)
				{
					if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x50, 2, 
						usRegW & 0xFFF7) != PCIBusHandler_SUCCESS)
					{
						KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
						return ;
					}
				}

				if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x52, 2, 
					&usRegW) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ;
				}

				if(usRegW & 0x08)
				{
					if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x52, 2, 
						usRegW & 0xFFF7) != PCIBusHandler_SUCCESS)
					{
						KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
						return ;
					}
				}
				break ;

		case SIS_UDMA_133a:
		case SIS_UDMA_100:
				if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, PCI_LATENCY, 1, 
					0x80) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ;
				}

				if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x49, 1, 
					&bReg) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ;
				}

				if(!(bReg & 0x01))
				{
					if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x49, 1, 
						bReg | 0x01) != PCIBusHandler_SUCCESS)
					{
						KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
						return ;
					}
				}
				break ;

		case SIS_UDMA_100a:
		case SIS_UDMA_66:
				if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, PCI_LATENCY, 1, 
					0x10) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ;
				}

				if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x52, 1, 
					&bReg) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ;
				}

				if(!(bReg & 0x04))
				{
					if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x52, 1, 
						bReg | 0x04) != PCIBusHandler_SUCCESS)
					{
						KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
						return ;
					}
				}
				break ;

		case SIS_UDMA_33:
				if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x09, 1, 
					&bReg) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ;
				}

				if(bReg & 0x0F)
				{
					if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x09, 1, 
						bReg | 0xF0) != PCIBusHandler_SUCCESS)
					{
						KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
						return ;
					}
				}
				break ;

		case SIS_16:
				if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x52, 1, 
					&bReg) != PCIBusHandler_SUCCESS)
				{
					KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
					return ;
				}

				if(!(bReg & 0x08))
				{
					if(PCIBusHandler_WritePCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x52, 1, 
						bReg | 0x08) != PCIBusHandler_SUCCESS)
					{
						KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
						return ;
					}
				}
				break ;
	}

	unsigned i ;
	unsigned short usRegAddr ;
	byte bMask ;

	if(uiSpeed >= SIS_UDMA_133)
	{
		for(i = 0; i < 2; i++)
		{
			usRegAddr = i ? 0x52 : 0x50 ;

			if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, usRegAddr, 2, 
				&usRegW) != PCIBusHandler_SUCCESS)
			{
				KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
				return ;
			}

			ui80W |= (usRegW & 0x8000) ? 0 : (1 << i) ;
		}
	}
	else if(uiSpeed >= SIS_UDMA_66)
	{
		for(i = 0; i < 2; i++)
		{
			bMask = i ? 0x20 : 0x10 ;

			if(PCIBusHandler_ReadPCIConfig(pIDE->uiBusNumber, pIDE->uiDeviceNumber, pIDE->uiFunction, 0x48, 1, 
				&bReg) != PCIBusHandler_SUCCESS)
			{
				KC::MDisplay().Message("\n\tFailed to Init SIS Controller", Display::WHITE_ON_BLACK()) ;
				return ;
			}
			
			ui80W |= (bReg & bMask) ? 0 : (1 << i) ;		
		}
	}

	pController->pPort[0]->uiCable = pController->pPort[1]->uiCable = 
		(ui80W & 0x01) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;

	pController->pPort[2]->uiCable = pController->pPort[3]->uiCable = 
		(ui80W & 0x02) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;

	//Add Speeds

	SISIDEInfo* pSISIDEInfo = (SISIDEInfo*)DMM_AllocateForKernel(sizeof(SISIDEInfo)) ;

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&pSISIDEInfo->pciEntry, MemUtil_GetDS(), (unsigned)pPCIEntry, 
						sizeof(PCIEntry)) ;
	pSISIDEInfo->uiSpeed = uiSpeed ;

	for(i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
	{
		ATAPort* pPort = pController->pPort[i] ;
		byte b80Bit = (i & 0x02) ? (ui80W & 0x02) : (ui80W & 0x01) ;

		//Add all MultiWord DMA and PIO Modes
		pPort->uiSupportedPortSpeed = 0xFF ;

		if(uiSpeed >= SIS_UDMA_33)
			pPort->uiSupportedPortSpeed |= 0x700 ;

		if(uiSpeed >= SIS_UDMA_66 && b80Bit)
			pPort->uiSupportedPortSpeed |= 0x1F00 ;

		if(uiSpeed >= SIS_UDMA_100a && b80Bit)
			pPort->uiSupportedPortSpeed |= 0x3FFF ;

		if(uiSpeed >= SIS_UDMA_133a && b80Bit)
			pPort->uiSupportedPortSpeed |= 0x7FFF ;

		pPort->portOperation.Configure = ATASIS_PortConfigure ;
		pPort->pVendorSpecInfo = pSISIDEInfo ;
	}

	String_Copy(pController->szName, "SIS ATA Controller") ;
}
