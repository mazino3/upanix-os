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
#ifndef _PCI_BUS_HANDLER_H_
#define _PCI_BUS_HANDLER_H_

# include <Global.h>

#define PCIBusHandler_SUCCESS					0
#define PCIBusHandler_ERR_NO_PCI_INSTALLED		1
#define PCIBusHandler_ERR_INVALID_DEVICE_INDEX	2
#define PCIBusHandler_FAILURE					3

#define PCI_TYPE_ONE 1
#define PCI_TYPE_TWO 2

#define PCI_REG_1	0xCFB
#define PCI_REG_2	0xCF8
#define PCI_REG_3	0xCFA
#define PCI_REG_4	0xCFC

#define PCI_VENDOR_ID		0x00
#define PCI_DEVICE_ID		0x02
#define PCI_COMMAND			0x04
#define PCI_STATUS			0x06
#define PCI_REVISION		0x08
#define PCI_INTERFACE		0x09
#define PCI_SUB_CLASS		0x0A
#define PCI_CLASS_CODE		0x0B
#define PCI_LATENCY			0x0D
#define PCI_HEADER_TYPE		0x0E
#define PCI_BASE_REGISTERS	0x10
#define PCI_INTERRUPT_LINE	0x3C
#define PCI_INTERRUPT_PIN	0x3D
#define PCI_MIN_GRANT		0x3E
#define PCI_MAX_LATENCY		0x3F

#define PCI_COMMAND_IO		0x001
#define PCI_COMMAND_MEMORY	0x002
#define PCI_COMMAND_MASTER	0x004

#define PCI_IO_ADDRESS_SPACE		0x01
#define PCI_ADDRESS_IO_MASK		0xFFFFFFFC
#define PCI_ADDRESS_MEMORY_32_MASK 	0xFFFFFFF0
#define PCI_RESOURCE_TYPE_IO	0x100

#define PCI_HEADER_BRIDGE   0x01
#define PCI_MULTIFUNCTION   0x80

#define PCI_BUS_PRIMARY		0x18
#define PCI_BUS_SECONDARY	0x19
#define PCI_BUS_SUBORDINATE	0x1A

#define MAX_PCI_DEVICES		255

#define PCI_MASS_STORAGE	0x01
#define PCI_IDE				0x01

#define PCI_SERIAL_BUS		0x0C
#define PCI_USB				0x03

typedef struct
{
	unsigned uiBusNumber ;
	unsigned uiDeviceNumber ;
	unsigned uiFunction ;

	unsigned short usVendorID ;
	unsigned short usDeviceID ;
	unsigned short usCommand ;
	unsigned short usStatus ;

	byte bRevisionID ;

	byte bInterface ;
	byte bClassCode ;
	byte bSubClass ;

	byte bCacheLineSize ;
	byte bLatency ;
	byte bHeaderType ;
	byte bSelfTestResult ;

	union
	{
		struct 
		{
			unsigned uiBaseAddress0 ;
			unsigned uiBaseAddress1 ;
			unsigned uiBaseAddress2 ;
			unsigned uiBaseAddress3 ;
			unsigned uiBaseAddress4 ;
			unsigned uiBaseAddress5 ;

			unsigned uiCardBusCIS ;

			unsigned short usSubsystemVendorID ;
			unsigned short usSubsystemDeviceID ;
		
			unsigned uiExpansionROM ;
			
			byte bCapabilityPointer ;
			
			byte bReserved1[3] ;
			unsigned uiReserved2[1] ;

			byte bInterruptLine ;
			byte bInterruptPin ;
			byte bMinDMAGrant ;
			byte bMaxDMALatency ;

			unsigned uiDeviceSpecific[48] ;
		} NonBridge ;
		
		struct
		{
			unsigned uiBaseAddress0 ;
			unsigned uiBaseAddress1 ;
			
			byte bPrimaryBus ;
			byte bSecondaryBus ;
			byte bSubordinateBus ;
			
			byte bSecondaryLatency ;

			byte bIOBaseLow ;
			byte bIOLimitLow ;
	
			unsigned short usSecondaryStatus ;

			unsigned short usMemoryBaseLow ;
			unsigned short usMemoryLimitLow ;
			unsigned short usPrefetchBaseLow ;
			unsigned short usPrefetchLimitLow ;		

			unsigned uiPrefetchBaseHigh ;
			unsigned uiPrefetchLimitHigh ;

			unsigned short usIOBaseHigh ;
			unsigned short usIOLimitHigh ;

			unsigned uiReserved[1] ;

			byte bInterruptLine ;
			byte bInterruptPin ;
			
			unsigned short usBridgeControl ;
			unsigned uiDeviceSpecifix[48] ;
		} Bridge ;
			
		struct
		{
			unsigned uiExCaBase ;

			byte bCapabilityPointer ;
			byte bReserved05 ;
			
			unsigned short usSecondaryStatus ;
			
			byte bPCIBus ;
			byte bCardBus ;
			byte bSubordinateBus ;

			byte bLatencyTimer ;
			
			unsigned uiMemoryBase0 ;
			unsigned uiMemoryLimit0 ;
			unsigned uiMemoryBase1 ;
			unsigned uiMemoryLimit1 ;

			unsigned short usIOBase0Low ;
			unsigned short usIOBase0High ;
			unsigned short usIOLimit0Low ;
			unsigned short usIOLimit0High ;

			unsigned short usIOBase1Low ;
			unsigned short usIOBase1High ;
			unsigned short usIOLimit1Low ;
			unsigned short usIOLimit1High ;

			byte bInterruptLine ;
			byte bInterruptPin ;
	
			unsigned short usBridgeControl ;
			unsigned short usSubsystemVendorID ;
			unsigned short usSubsystemDeviceID ;

			unsigned uiLegacyBaseAddr ;
			unsigned uiCardBusReserved[14] ;
			unsigned uiVendorSpecific[32] ;
		} CardBus ;
	} BusEntity ;
} PCIEntry ; 

extern unsigned PCIBusHandler_uiDeviceCount ;

void PCIBusHandler_Initialize() ;
byte PCIBusHandler_GetPCIEntry(PCIEntry** pPCIEntry, unsigned uiIndex) ;
byte PCIBusHandler_ReadPCIConfig(unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction, 
						unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, void* pValue) ;
byte PCIBusHandler_WritePCIConfig(unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction, 
				unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, unsigned uiValue) ;
unsigned PCIBusHandler_GetPCIMemSize(PCIEntry* pPCIEntry, int iAddressIndex) ;
void PCIBusHandler_EnableBusMaster(const PCIEntry* pPCIEntry) ;

#endif
