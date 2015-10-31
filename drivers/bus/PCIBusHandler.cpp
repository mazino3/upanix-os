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
# include <PCIBusHandler.h>
# include <PortCom.h>
# include <Display.h>
# include <DMM.h>
# include <KernelService.h>

unsigned PCIBusHandler_uiDeviceCount ;

unsigned PCIBusHandler_uiType ;
unsigned PCIBusHandler_uiNoOfPCIBusess ;
PCIEntry* PCIBusHandler_pDevices[MAX_PCI_DEVICES] ;

/************************* static functions ***********************************/

static Result PCIBusHandler_Find()
{
	unsigned uiData ;

    /* Check if Configuration 1 is Supported */
	PortCom_SendByte(PCI_REG_1, 0x01) ;

	uiData = PortCom_ReceiveDoubleWord(PCI_REG_2) ;
	PortCom_SendDoubleWord(PCI_REG_2, 0x80000000) ;

	if(PortCom_ReceiveDoubleWord(PCI_REG_2) == 0x80000000)
	{
		PortCom_SendDoubleWord(PCI_REG_2, uiData) ;
		PCIBusHandler_uiType = PCI_TYPE_ONE ;
		return Success ;
	}

	PortCom_SendDoubleWord(PCI_REG_2, uiData) ;

    /* Check if Configuration 2 is Supported */
	PortCom_SendByte(PCI_REG_1, 0x00) ;
	PortCom_SendByte(PCI_REG_2, 0x00) ;
	PortCom_SendByte(PCI_REG_3, 0x00) ;

	if(PortCom_ReceiveByte(PCI_REG_2) == 0x00 && PortCom_ReceiveByte(PCI_REG_3) == 0x00)
	{
		PCIBusHandler_uiType = PCI_TYPE_TWO ;
		return Success ;
	}

	return NoPCIInstalled;
}

static Result PCIBusHandler_ReadNonBridgePCIHeader(PCIEntry* pPCIEntry, unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction)
{
	KC::MDisplay().Address("\n PCI: ", (unsigned)(pPCIEntry)) ;
	pPCIEntry->uiBusNumber = uiBusNumber ;
	pPCIEntry->uiDeviceNumber = uiDeviceNumber ;
	pPCIEntry->uiFunction = uiFunction ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_VENDOR_ID, 2, &pPCIEntry->usVendorID),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_DEVICE_ID, 2, &pPCIEntry->usDeviceID),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, &pPCIEntry->usCommand),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_STATUS, 2, &pPCIEntry->usStatus),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_REVISION, 1, &pPCIEntry->bRevisionID),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_INTERFACE, 1, &pPCIEntry->bInterface),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_CLASS_CODE, 1, &pPCIEntry->bClassCode),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_SUB_CLASS, 1, &pPCIEntry->bSubClass),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_HEADER_TYPE, 1, &pPCIEntry->bHeaderType),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BASE_REGISTERS + 0, 4, &pPCIEntry->BusEntity.NonBridge.uiBaseAddress0),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BASE_REGISTERS + 4, 4, &pPCIEntry->BusEntity.NonBridge.uiBaseAddress1),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BASE_REGISTERS + 8, 4, &pPCIEntry->BusEntity.NonBridge.uiBaseAddress2),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BASE_REGISTERS + 12, 4, &pPCIEntry->BusEntity.NonBridge.uiBaseAddress3),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BASE_REGISTERS + 16, 4, &pPCIEntry->BusEntity.NonBridge.uiBaseAddress4),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BASE_REGISTERS + 20, 4, &pPCIEntry->BusEntity.NonBridge.uiBaseAddress5),
	Success, Failure) ;
	
	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_INTERRUPT_LINE, 1, &pPCIEntry->BusEntity.NonBridge.bInterruptLine),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_INTERRUPT_PIN, 1, &pPCIEntry->BusEntity.NonBridge.bInterruptPin),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_MIN_GRANT, 1, &pPCIEntry->BusEntity.NonBridge.bMinDMAGrant),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_MAX_LATENCY, 1, &pPCIEntry->BusEntity.NonBridge.bMaxDMALatency),
	Success, Failure) ;

	return Success ;
}

static byte PCIBusHandler_ReadBridgePCIHeader(PCIEntry* pPCIEntry, unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction)
{
	pPCIEntry->uiBusNumber = uiBusNumber ;
	pPCIEntry->uiDeviceNumber = uiDeviceNumber ;
	pPCIEntry->uiFunction = uiFunction ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_VENDOR_ID, 2, &pPCIEntry->usVendorID),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_DEVICE_ID, 2, &pPCIEntry->usDeviceID),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, &pPCIEntry->usCommand),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_STATUS, 2, &pPCIEntry->usStatus),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_REVISION, 1, &pPCIEntry->bRevisionID),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_INTERFACE, 1, &pPCIEntry->bInterface),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_CLASS_CODE, 1, &pPCIEntry->bClassCode),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_SUB_CLASS, 1, &pPCIEntry->bSubClass),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_HEADER_TYPE, 1, &pPCIEntry->bHeaderType),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BASE_REGISTERS + 0, 4, &pPCIEntry->BusEntity.Bridge.uiBaseAddress0),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BASE_REGISTERS + 4, 4, &pPCIEntry->BusEntity.Bridge.uiBaseAddress1),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_PRIMARY, 1, &pPCIEntry->BusEntity.Bridge.bPrimaryBus),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_SECONDARY, 1, &pPCIEntry->BusEntity.Bridge.bSecondaryBus),
	Success, Failure) ;

	RETURN_X_IF_NOT(
	PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_SUBORDINATE, 1, &pPCIEntry->BusEntity.Bridge.bSubordinateBus),
	Success, Failure) ;

	return Success ;
}

static byte PCIBusHandler_ScanBus(unsigned uiBusNumber)
{
	PCIEntry* pPCIEntry ;
	unsigned uiDevicePerBus = (PCIBusHandler_uiType == PCI_TYPE_ONE) ? 32 : 16 ;
	unsigned uiDeviceNumber, uiFunction ;
	unsigned short usCommand ;
	unsigned short usVendorID ;
	byte bHeaderType = 0 ;

	PCIBusHandler_uiNoOfPCIBusess++ ;

	for(uiDeviceNumber = 0; uiDeviceNumber < uiDevicePerBus; uiDeviceNumber++)
	{
		for(uiFunction = 0; uiFunction < 8; uiFunction++)
		{
			RETURN_X_IF_NOT(
			PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_VENDOR_ID, 2, &usVendorID),
			Success, Failure) ;
		
			if(usVendorID != 0xFFFF && usVendorID != 0x0000) 
			{
				if(uiFunction == 0)
				{
					RETURN_X_IF_NOT(
					PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_HEADER_TYPE, 1, &bHeaderType),
					Success, Failure) ;
				}
				else
				{
					if((bHeaderType & PCI_MULTIFUNCTION) == 0)
					{
						continue ;
					}
				}

				if(bHeaderType & PCI_HEADER_BRIDGE)
				{
					RETURN_X_IF_NOT(
					PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, &usCommand), 
					Success, Failure) ;

					RETURN_X_IF_NOT(
					PCIBusHandler_WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2,
						(usCommand & ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY))),
					Success, Failure) ;

					RETURN_X_IF_NOT(
					PCIBusHandler_WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_PRIMARY, 1, uiBusNumber),
					Success, Failure) ;

					RETURN_X_IF_NOT(
					PCIBusHandler_WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_SECONDARY, 1, 
					PCIBusHandler_uiNoOfPCIBusess),
					Success, Failure) ;

					RETURN_X_IF_NOT(
					PCIBusHandler_WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_SUBORDINATE, 1, 0xFF),
					Success, Failure) ;

					RETURN_X_IF_NOT(
					PCIBusHandler_ScanBus(PCIBusHandler_uiNoOfPCIBusess),
					Success, Failure) ;

					RETURN_X_IF_NOT(
					PCIBusHandler_WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_SUBORDINATE, 1, 
					PCIBusHandler_uiNoOfPCIBusess - 1),
					Success, Failure) ;
	
					RETURN_X_IF_NOT(
					PCIBusHandler_ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, &usCommand), 
					Success, Failure) ;

					RETURN_X_IF_NOT(
					PCIBusHandler_WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2,
					(usCommand | PCI_COMMAND_IO | PCI_COMMAND_MEMORY)),
					Success, Failure) ;
				}

				pPCIEntry = (PCIEntry*)DMM_AllocateForKernel(sizeof(PCIEntry)) ;
				if(pPCIEntry != NULL)
				{
					if(bHeaderType & PCI_HEADER_BRIDGE)
					{
						RETURN_X_IF_NOT(
						PCIBusHandler_ReadBridgePCIHeader(pPCIEntry, uiBusNumber, uiDeviceNumber, uiFunction),
						Success, Failure) ;
					}
					else
					{
						RETURN_X_IF_NOT(
						PCIBusHandler_ReadNonBridgePCIHeader(pPCIEntry, uiBusNumber, uiDeviceNumber, uiFunction),
						Success, Failure) ;
					}

					if(PCIBusHandler_uiDeviceCount < MAX_PCI_DEVICES)
					{
						PCIBusHandler_pDevices[PCIBusHandler_uiDeviceCount++] = pPCIEntry ;
					}
					else
					{
						KC::MDisplay().Message("\n Too many PCI Devices", ' ') ;
					}
				}
	    	}
		}
    }

	return Success ;
}

static unsigned PCIBusHandler_PCISize(unsigned uiBase, unsigned uiMask)
{
	unsigned uiSize = uiBase & uiMask ;
    return uiSize & ~( uiSize - 1 );
	//return uiSize ;
}

/*****************************************************************************/

void PCIBusHandler_Initialize()
{
	Result result = Success ;
	
	PCIBusHandler_uiType = 0 ;
	PCIBusHandler_uiNoOfPCIBusess = 0 ;
	PCIBusHandler_uiDeviceCount = 0 ;

	if((result = PCIBusHandler_Find()) == Success)
	{
		KC::MDisplay().Address("\n\tPCI Configuration Mechanism# ", PCIBusHandler_uiType) ;
		KC::MDisplay().Message("\n\tScanning PCI Bus for Devices...", Display::WHITE_ON_BLACK()) ;

		if((result = PCIBusHandler_ScanBus(0)) == Success)
		{		
			KC::MDisplay().Message("\n\tPCI Bus Scan Successfull", Display::WHITE_ON_BLACK()) ;
			KC::MDisplay().Message("\n\tFollowing Devices are found: ", Display::WHITE_ON_BLACK()) ;

			unsigned i ;
			for(i = 0; i < PCIBusHandler_uiDeviceCount; i++)
			{
				KC::MDisplay().Address("\n", i + 1) ;
				KC::MDisplay().Address(") Vendor = ", PCIBusHandler_pDevices[i]->usVendorID) ;
				KC::MDisplay().Address(" Device = ", PCIBusHandler_pDevices[i]->usDeviceID) ;
				KC::MDisplay().Address(" ClassCode = ", PCIBusHandler_pDevices[i]->bClassCode) ;
			}
		}
	}

	KC::MDisplay().LoadMessage("PCI Bus Initialization", result) ;
}

Result PCIBusHandler_GetPCIEntry(PCIEntry** pPCIEntry, unsigned uiIndex)
{
	if(!(uiIndex >= 0 && uiIndex < PCIBusHandler_uiDeviceCount))
		return InvalidDeviceIndex;

	*pPCIEntry = (PCIEntry*)(PCIBusHandler_pDevices[uiIndex]) ;
	return Success;
}

Result PCIBusHandler_ReadPCIConfig(unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction, unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, void* pValue)
{
//TODO: PCI TYPE = 0 => Using BIOS PCI Interface
	if(PCIBusHandler_uiType == PCI_TYPE_ONE)
	{
		PortCom_SendDoubleWord(PCI_REG_2, (0x80000000 
			| (uiBusNumber << 16) 
			| (uiDeviceNumber << 11)
			| (uiFunction << 8)
			| (uiPCIEntryOffset & ~3))) ;

		switch(uiPCIEntrySize)
		{
			case 1:
				*((byte*)pValue) = PortCom_ReceiveByte(PCI_REG_4 + (uiPCIEntryOffset & 0x3)) ;
				break ;

			case 2:
				*((unsigned short*)pValue) = PortCom_ReceiveWord(PCI_REG_4 + (uiPCIEntryOffset & 0x2)) ;
				break ;
		
			case 4:
				*((unsigned*)pValue) = PortCom_ReceiveDoubleWord(PCI_REG_4) ;
				break ;

			default:
				return Failure ;
		}

		return Success ;
	}
	else if(PCIBusHandler_uiType == PCI_TYPE_TWO)
	{
		PortCom_SendByte(PCI_REG_2, 0xF0 | (uiFunction << 1)) ;
		PortCom_SendByte(PCI_REG_3, uiBusNumber) ;

		switch(uiPCIEntrySize)
		{
			case 1:
				*((byte*)pValue) = PortCom_ReceiveByte(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset) ;
				break ;

			case 2:
				*((unsigned short*)pValue) = PortCom_ReceiveWord(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset) ;
				break ;

			case 4:
				*((unsigned*)pValue) = PortCom_ReceiveDoubleWord(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset) ;
				break ;

			default:
				return Failure ;
		}

		PortCom_SendByte(PCI_REG_2, 0x00) ;
		return Success ;
	}

	return Failure ;
}

Result PCIBusHandler_WritePCIConfig(unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction, 
				unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, unsigned uiValue)
{
//TODO: PCI TYPE = 0 => Using BIOS PCI Interface

	if(PCIBusHandler_uiType == PCI_TYPE_ONE) 
	{
		PortCom_SendDoubleWord(PCI_REG_2, (0x80000000
			| (uiBusNumber << 16)
			| (uiDeviceNumber << 11)
			| (uiFunction << 8)
			| (uiPCIEntryOffset & ~0x3))) ;
	
		switch(uiPCIEntrySize)
		{
			case 1:
				PortCom_SendByte(PCI_REG_4 + (uiPCIEntryOffset & 0x3), uiValue) ;
				break ;

			case 2:
				PortCom_SendWord(PCI_REG_4 + (uiPCIEntryOffset & 0x2), uiValue) ;
				break ;

			case 4:
				PortCom_SendDoubleWord(PCI_REG_4, uiValue) ;
				break ;
			
			default:
				return Failure ;
		}

		return Success ;
	}
	else if(PCIBusHandler_uiType == PCI_TYPE_TWO)
	{
		PortCom_SendByte(PCI_REG_2, 0xF0 | (uiFunction << 1)) ;
		PortCom_SendByte(PCI_REG_3, uiBusNumber) ;

		switch(uiPCIEntrySize)
		{
			case 1:
				PortCom_SendByte(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset, uiValue) ;
				break ;
			
			case 2:
				PortCom_SendWord(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset, uiValue) ;
				break ;

			case 4:
				PortCom_SendDoubleWord(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset, uiValue) ;
				break ;
	
			default:
				return Failure ;
		}

		PortCom_SendByte(PCI_REG_2, 0x00) ;
		return Success ;
	}

	return Failure ;
}

unsigned PCIBusHandler_GetPCIMemSize(PCIEntry* pPCIEntry, int iAddressIndex)
{
    unsigned uiOffset = PCI_BASE_REGISTERS + iAddressIndex * 4;
	unsigned uiBase, uiSize ;

	PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
								uiOffset, 4, &uiBase) ;

	PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
								uiOffset, 4, 0xFFFFFFFF) ;

	PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
								uiOffset, 4, &uiSize) ;

	PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
								uiOffset, 4, uiBase) ;

    if(!uiSize || uiSize == 0xFFFFFFFF)
        return 0 ;

	// Why is this required !!
    if(uiBase == 0xFFFFFFFF)
        uiBase = 0;

    if(!(uiSize & PCI_IO_ADDRESS_SPACE))
        return PCIBusHandler_PCISize(uiSize, PCI_ADDRESS_MEMORY_32_MASK) ;
    else
        return PCIBusHandler_PCISize(uiSize, PCI_ADDRESS_IO_MASK & 0xFFFF) ;
}

void PCIBusHandler_EnableBusMaster(const PCIEntry* pPCIEntry)
{
	unsigned short usCmd ;
	PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
			PCI_COMMAND, 2, &usCmd) ;
	if(!(usCmd & PCI_COMMAND_MASTER))
	{
		printf("\nPCI: Enabling Bus Master for Device: %x", pPCIEntry->uiDeviceNumber) ;
		usCmd |= PCI_COMMAND_MASTER ;
		PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
								PCI_COMMAND, 2, usCmd) ;
	}
	// SetBios Bus Master
	byte bLat ;
	PCIBusHandler_ReadPCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
			PCI_LATENCY, 1, &bLat) ;
	if(bLat > 255)
		bLat = 255 ;
	else if(bLat < 16)
		bLat = 64 ;
	else
		return ;
	printf("\nPCI: Setting latency timer of device: %x to %u", pPCIEntry->uiDeviceNumber, bLat) ;
	PCIBusHandler_WritePCIConfig(pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction,
							PCI_LATENCY, 1, bLat) ;
}

