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
# include <PCIBusHandler.h>
# include <PortCom.h>
# include <Display.h>
# include <DMM.h>
# include <KernelService.h>

PCIBusHandler::PCIBusHandler() : _type(PCI_TYPE_ONE), _uiNoOfPCIBuses(0)
{
}

void PCIBusHandler::Initialize()
{
  for(auto p : _pciEntries)
    delete p;
  _uiNoOfPCIBuses = 0;
	Result result = Success;

	if((result = Find()) == Success)
	{
    printf("\n\tPCI Configuration Mechanism# %d", _type);
    printf("\n\tScanning PCI Bus for Devices...");

		if((result = ScanBus(0)) == Success)
		{
      printf("\n\tPCI Bus Scan Successfull");
      printf("\n\tFollowing Devices are found: ");

      int i = 0;
			for(auto p : _pciEntries)
        printf("\n%d) Vendor = %u Dev = %u If = %u CCode = %u SubCCode = %u", ++i, p->usVendorID, p->usDeviceID, p->bInterface, p->bClassCode, p->bSubClass);
		}
	}
	KC::MDisplay().LoadMessage("PCI Bus Initialization", result) ;
}

Result PCIBusHandler::Find()
{
	unsigned uiData ;

    /* Check if Configuration 1 is Supported */
	PortCom_SendByte(PCI_REG_1, 0x01) ;

	uiData = PortCom_ReceiveDoubleWord(PCI_REG_2) ;
	PortCom_SendDoubleWord(PCI_REG_2, 0x80000000) ;

	if(PortCom_ReceiveDoubleWord(PCI_REG_2) == 0x80000000)
	{
		PortCom_SendDoubleWord(PCI_REG_2, uiData) ;
		_type = PCI_TYPE_ONE ;
		return Success ;
	}

	PortCom_SendDoubleWord(PCI_REG_2, uiData) ;

    /* Check if Configuration 2 is Supported */
	PortCom_SendByte(PCI_REG_1, 0x00) ;
	PortCom_SendByte(PCI_REG_2, 0x00) ;
	PortCom_SendByte(PCI_REG_3, 0x00) ;

	if(PortCom_ReceiveByte(PCI_REG_2) == 0x00 && PortCom_ReceiveByte(PCI_REG_3) == 0x00)
	{
		_type = PCI_TYPE_TWO ;
		return Success ;
	}

	return NoPCIInstalled;
}

Result PCIBusHandler::ScanBus(unsigned uiBusNumber)
{
	unsigned uiDevicePerBus = (_type == PCI_TYPE_ONE) ? 32 : 16 ;
	unsigned uiDeviceNumber, uiFunction ;
	unsigned short usCommand ;
	unsigned short usVendorID ;
	byte bHeaderType = 0 ;

	_uiNoOfPCIBuses++ ;

	for(uiDeviceNumber = 0; uiDeviceNumber < uiDevicePerBus; uiDeviceNumber++)
	{
		for(uiFunction = 0; uiFunction < 8; uiFunction++)
		{
			RETURN_X_IF_NOT(ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_VENDOR_ID, 2, &usVendorID), Success, Failure);
		
			if(usVendorID != 0xFFFF && usVendorID != 0x0000) 
			{
				if(uiFunction == 0)
				{
					RETURN_X_IF_NOT(ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_HEADER_TYPE, 1, &bHeaderType), Success, Failure) ;
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
					RETURN_X_IF_NOT(ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, &usCommand), Success, Failure) ;

					RETURN_X_IF_NOT(WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, (usCommand & ~(PCI_COMMAND_IO | PCI_COMMAND_MMIO))), Success, Failure) ;

					RETURN_X_IF_NOT(WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_PRIMARY, 1, uiBusNumber), Success, Failure) ;

					RETURN_X_IF_NOT(WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_SECONDARY, 1, _uiNoOfPCIBuses), Success, Failure) ;

					RETURN_X_IF_NOT(WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_SUBORDINATE, 1, 0xFF), Success, Failure) ;

					RETURN_X_IF_NOT(ScanBus(_uiNoOfPCIBuses), Success, Failure) ;

					RETURN_X_IF_NOT(WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_SUBORDINATE, 1, _uiNoOfPCIBuses - 1), Success, Failure) ; 

					RETURN_X_IF_NOT(ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, &usCommand), Success, Failure) ;

					RETURN_X_IF_NOT(WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, (usCommand | PCI_COMMAND_IO | PCI_COMMAND_MMIO)), Success, Failure) ;
				}

        if(_pciEntries.size() < MAX_PCI_DEVICES)
          _pciEntries.push_back(new PCIEntry(uiBusNumber, uiDeviceNumber, uiFunction, bHeaderType));
        else
          printf("\n Too many PCI Devices!!!");
	   	}
		}
  }
	return Success;
}

Result PCIBusHandler::ReadPCIConfig(unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction, unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, void* pValue)
{
//TODO: PCI TYPE = 0 => Using BIOS PCI Interface
	if(_type == PCI_TYPE_ONE)
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
	else if(_type == PCI_TYPE_TWO)
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

Result PCIBusHandler::WritePCIConfig(unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction, 
				unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, unsigned uiValue)
{
//TODO: PCI TYPE = 0 => Using BIOS PCI Interface

	if(_type == PCI_TYPE_ONE) 
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
	else if(_type == PCI_TYPE_TWO)
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

PCIEntry::PCIEntry(unsigned uiBusNo, unsigned uiDeviceNo, unsigned uiFunc, byte bHeaderType)
  : uiBusNumber(uiBusNo), uiDeviceNumber(uiDeviceNo), uiFunction(uiFunc)
{
  Result r;
  if(bHeaderType & PCI_HEADER_BRIDGE)
    r = ReadBridgePCIHeader();
  else
    r = ReadNonBridgePCIHeader();
  if(r != Success)
    throw upan::exception(XLOC, "failed to read PCI configuration for Bus:%u, Device:%u, Function:%u, HeaderType:%u", uiBusNumber, uiDeviceNumber, uiFunction, bHeaderType);
}

Result PCIEntry::ReadPCIConfig(unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, void* pValue) const
{
  return PCIBusHandler::Instance().ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, uiPCIEntryOffset, uiPCIEntrySize, pValue);
}

Result PCIEntry::WritePCIConfig(unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, unsigned uiValue) const
{
  return PCIBusHandler::Instance().WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, uiPCIEntryOffset, uiPCIEntrySize, uiValue);
}

Result PCIEntry::ReadNonBridgePCIHeader()
{
	RETURN_X_IF_NOT(ReadPCIConfig(PCI_VENDOR_ID, 2, &usVendorID), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_DEVICE_ID, 2, &usDeviceID), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_COMMAND, 2, &usCommand), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_STATUS, 2, &usStatus), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_REVISION, 1, &bRevisionID), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_INTERFACE, 1, &bInterface), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_CLASS_CODE, 1, &bClassCode), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_SUB_CLASS, 1, &bSubClass), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_HEADER_TYPE, 1, &bHeaderType), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_BASE_REGISTERS + 0, 4, &BusEntity.NonBridge.uiBaseAddress0), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_BASE_REGISTERS + 4, 4, &BusEntity.NonBridge.uiBaseAddress1), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_BASE_REGISTERS + 8, 4, &BusEntity.NonBridge.uiBaseAddress2), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_BASE_REGISTERS + 12, 4, &BusEntity.NonBridge.uiBaseAddress3), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_BASE_REGISTERS + 16, 4, &BusEntity.NonBridge.uiBaseAddress4), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_BASE_REGISTERS + 20, 4, &BusEntity.NonBridge.uiBaseAddress5), Success, Failure);
	
	RETURN_X_IF_NOT(ReadPCIConfig(PCI_INTERRUPT_LINE, 1, &BusEntity.NonBridge.bInterruptLine), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_INTERRUPT_PIN, 1, &BusEntity.NonBridge.bInterruptPin), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_MIN_GRANT, 1, &BusEntity.NonBridge.bMinDMAGrant), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_MAX_LATENCY, 1, &BusEntity.NonBridge.bMaxDMALatency), Success, Failure);

	return Success;
}

Result PCIEntry::ReadBridgePCIHeader()
{
	RETURN_X_IF_NOT(ReadPCIConfig(PCI_VENDOR_ID, 2, &usVendorID), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_DEVICE_ID, 2, &usDeviceID), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_COMMAND, 2, &usCommand), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_STATUS, 2, &usStatus), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_REVISION, 1, &bRevisionID), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_INTERFACE, 1, &bInterface), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_CLASS_CODE, 1, &bClassCode), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_SUB_CLASS, 1, &bSubClass), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_HEADER_TYPE, 1, &bHeaderType), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_BASE_REGISTERS + 0, 4, &BusEntity.Bridge.uiBaseAddress0), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_BASE_REGISTERS + 4, 4, &BusEntity.Bridge.uiBaseAddress1), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_BUS_PRIMARY, 1, &BusEntity.Bridge.bPrimaryBus), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_BUS_SECONDARY, 1, &BusEntity.Bridge.bSecondaryBus), Success, Failure);

	RETURN_X_IF_NOT(ReadPCIConfig(PCI_BUS_SUBORDINATE, 1, &BusEntity.Bridge.bSubordinateBus), Success, Failure);

	return Success;
}

unsigned PCIEntry::GetPCIMemSize(int iAddressIndex) const
{
  unsigned uiOffset = PCI_BASE_REGISTERS + iAddressIndex * 4;
	unsigned uiBase, uiSize ;

	ReadPCIConfig(uiOffset, 4, &uiBase);
	WritePCIConfig(uiOffset, 4, 0xFFFFFFFF);

	ReadPCIConfig(uiOffset, 4, &uiSize);
	WritePCIConfig(uiOffset, 4, uiBase);

  if(!uiSize || uiSize == 0xFFFFFFFF)
    return 0 ;

  // Why is this required !!
  if(uiBase == 0xFFFFFFFF)
    uiBase = 0;

  if(!(uiSize & PCI_IO_ADDRESS_SPACE))
    return PCISize(uiSize, PCI_ADDRESS_MEMORY_32_MASK) ;
  else
    return PCISize(uiSize, PCI_ADDRESS_IO_MASK & 0xFFFF) ;
}

unsigned PCIEntry::PCISize(unsigned uiBase, unsigned uiMask) const
{
  unsigned uiSize = uiBase & uiMask ;
  return uiSize & ~( uiSize - 1 );
}

void PCIEntry::EnableBusMaster() const
{
	unsigned short usCmd ;
	ReadPCIConfig(PCI_COMMAND, 2, &usCmd);
	if(!(usCmd & PCI_COMMAND_MASTER))
	{
		printf("\nPCI: Enabling Bus Master for Device: %x", uiDeviceNumber) ;
		usCmd |= PCI_COMMAND_MASTER ;
		WritePCIConfig(PCI_COMMAND, 2, usCmd);
	}
	// SetBios Bus Master
	byte bLat ;
	ReadPCIConfig(PCI_LATENCY, 1, &bLat);
	if(bLat > 255)
		bLat = 255 ;
	else if(bLat < 16)
		bLat = 64;
	else
		return;
	printf("\nPCI: Setting latency timer of device: %x to %u", uiDeviceNumber, bLat);
	WritePCIConfig(PCI_LATENCY, 1, bLat);
}
