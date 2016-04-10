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
# include <ATADeviceController.h>
# include <ATAPortOperation.h>
# include <ATAPortManager.h>
# include <Display.h>
# include <DMM.h>
# include <StringUtil.h>
# include <PortCom.h>
# include <ATAIntel.h>
# include <ATAAMD.h>
# include <ATAVIA.h>
# include <ATASIS.h>
# include <AsmUtil.h>
# include <ATADrive.h>
# include <MemUtil.h>
# include <PartitionManager.h>
# include <FileSystem.h>
# include <stdio.h>

static unsigned ATADeviceController_uiHDDDeviceID ;
static unsigned ATADeviceController_uiCDDeviceID ;

static bool ATADeviceController_bInitStatus = false ;
static const IRQ* HD_PRIMARY_IRQ ;
static const IRQ* HD_SECONDARY_IRQ ;

byte ATADeviceController_bLegacyControllerFound ;
unsigned ATADeviceController_uiPortIDSequence ;
ATAController* ATADeviceController_pFirstController ;

ATAPCIDevice ATADeviceController_Devices[] = {
	/* VIA */
	{ 0x1106, 0x1571, ATAVIA_InitController },
	{ 0x1106, 0x0571, ATAVIA_InitController },
	//{ 0x1106, 0x3149, NULL }, //Disabled, SATA
	/* Intel */
	{ 0x8086, 0x122E, ATAIntel_InitController },
	{ 0x8086, 0x1230, ATAIntel_InitController },
	{ 0x8086, 0x1234, ATAIntel_InitController },
	{ 0x8086, 0x7010, ATAIntel_InitController },
	{ 0x8086, 0x7111, ATAIntel_InitController },
	{ 0x8086, 0x7198, ATAIntel_InitController },
	{ 0x8086, 0x7601, ATAIntel_InitController },
	{ 0x8086, 0x84CA, ATAIntel_InitController },
	{ 0x8086, 0x2421, ATAIntel_InitController },
	{ 0x8086, 0x2411, ATAIntel_InitController },
	{ 0x8086, 0x244B, ATAIntel_InitController },
	{ 0x8086, 0x244A, ATAIntel_InitController },
	{ 0x8086, 0x248A, ATAIntel_InitController },
	{ 0x8086, 0x248B, ATAIntel_InitController },
	{ 0x8086, 0x24CB, ATAIntel_InitController },
	{ 0x8086, 0x24DB, ATAIntel_InitController },
	{ 0x8086, 0x245B, ATAIntel_InitController },
	{ 0x8086, 0x24CA, ATAIntel_InitController },
	{ 0x8086, 0x24D1, ATAIntel_InitController },
	
	// Beta Check
	//{ 0x8086, 0x2651, ATAIntel_InitController },
	//{ 0x8086, 0x266A, ATAIntel_InitController },

	/* SIS */
	{ 0x1039, 0x5513, ATASIS_InitController },
	/* AMD */
	{ 0x1022, 0x7401, ATAAMD_InitController },
	{ 0x1022, 0x7409, ATAAMD_InitController },
	{ 0x1022, 0x7411, ATAAMD_InitController },
	{ 0x1022, 0x7441, ATAAMD_InitController },
	{ 0x1022, 0x7469, ATAAMD_InitController },
	/* nVIDIA */
	{ 0x10DE, 0x01BC, ATAAMD_InitController },
	{ 0x10DE, 0x0065, ATAAMD_InitController },
	{ 0x10DE, 0x0085, ATAAMD_InitController },
	{ 0x10DE, 0x008E, ATAAMD_InitController },
	{ 0x10DE, 0x00D5, ATAAMD_InitController },
	{ 0x10DE, 0x00E5, ATAAMD_InitController },
	{ 0x10DE, 0x00E3, ATAAMD_InitController },
	{ 0x10DE, 0x00EE, ATAAMD_InitController }
} ;

/******************************** static functions **************************************************/

static void ATADeviceController_PrimaryIRQHandler()
{
	unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	AsmUtil_SET_KERNEL_DATA_SEGMENTS

  HD_PRIMARY_IRQ->Signal();

	PIC::Instance().SendEOI(*HD_PRIMARY_IRQ) ;
	
	AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
	AsmUtil_RESTORE_GPR(GPRStack) ;

	__asm__ __volatile__("leave") ;
	__asm__ __volatile__("IRET") ;
}

static void ATADeviceController_SecondaryIRQHandler()
{
	unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	AsmUtil_SET_KERNEL_DATA_SEGMENTS

  HD_SECONDARY_IRQ->Signal();
	
	PIC::Instance().SendEOI(*HD_SECONDARY_IRQ) ;

	AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
	AsmUtil_RESTORE_GPR(GPRStack) ;

	asm("leave") ;
	asm("IRET") ;
}

static unsigned ATADeviceController_GetNextPortID()
{
	return ATADeviceController_uiPortIDSequence++ ;
}
static ATAController* ATADeviceController_AllocateController(unsigned uiChannels, unsigned uiPortsPerChannel)
{
	ATAController* pController = (ATAController*)DMM_AllocateForKernel(sizeof(ATAController)) ;

	pController->uiChannels = uiChannels ;
	pController->uiPortsPerChannel = uiPortsPerChannel ;
	strcpy(pController->szName, "ATA Controller") ;
	
	return pController ;
}

static ATAPort* ATADeviceController_AllocatePort(ATAController* pController)
{
	ATAPort* pPort = (ATAPort*)DMM_AllocateForKernel(sizeof(ATAPort)) ;

	// Set Default values
	pPort->pController = pController ;
	pPort->portOperation.Reset = NULL ;
	pPort->portOperation.Select = ATAPortOperation_PortSelect ;
	pPort->portOperation.Configure = ATAPortOperation_PortConfigure ;
	pPort->portOperation.PrepareDMARead = ATAPortOperation_PortPrepareDMARead ;
	pPort->portOperation.PrepareDMAWrite = ATAPortOperation_PortPrepareDMAWrite ;
	pPort->portOperation.StartDMA = ATAPortOperation_StartDMA ;
	pPort->portOperation.FlushRegs = NULL ;

	pPort->pPRDTable = (ATAPRD*)DMM_AllocateAlignForKernel(DMA_MEM_BLOCK_SIZE, 4) ;
	pPort->pDMATransferAddr = DMM_AllocateAlignForKernel(HDD_DMA_BUFFER_SIZE, 4) ;

	return pPort ;
}

static byte ATADeviceController_EnableDisabledController(const PCIEntry* pPCIEntry)
{
	unsigned short usCommand ;

	RETURN_X_IF_NOT(
	pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand), Success, ATADeviceController_FAILURE) ;

	if((usCommand & (PCI_COMMAND_IO | PCI_COMMAND_MASTER)) != (PCI_COMMAND_IO | PCI_COMMAND_MASTER))
	{
		RETURN_X_IF_NOT(pPCIEntry->WritePCIConfig(PCI_COMMAND, 2, (usCommand | PCI_COMMAND_IO | PCI_COMMAND_MASTER)), Success, ATADeviceController_FAILURE) ;

		RETURN_X_IF_NOT(pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand), Success, ATADeviceController_FAILURE) ;

		if((usCommand & (PCI_COMMAND_IO | PCI_COMMAND_MASTER)) != (PCI_COMMAND_IO | PCI_COMMAND_MASTER))
		{
			KC::MDisplay().Message("\n\tFailed	to Enable Controller", ' ') ;
			return ATADeviceController_ERR_ENABLE_CONTROLLER ;
		}
		KC::MDisplay().Message("\n\tController Enabled", Display::WHITE_ON_BLACK()) ;
	}

	return ATADeviceController_SUCCESS ;
}

static byte ATADeviceController_CheckControllerMode(const PCIEntry* pPCIEntry, 
					VendorSpecificATAController_Initialize* pInitFunc, ATAController** pController)
{
	unsigned uiPrimaryCommand, uiSecondaryCommand ;
	unsigned uiPrimaryControl, uiSecondaryControl ;
	unsigned uiPrimaryDMA, uiSecondaryDMA ;
	byte bPrimaryIRQ, bSecondaryIRQ ;

	byte bDMAPossible ;
	byte bNativeMode ;
	byte bMMIO ;

	byte bInterface ;

	RETURN_X_IF_NOT(
	pPCIEntry->ReadPCIConfig(PCI_INTERFACE, 1, &bInterface), Success, ATADeviceController_FAILURE) ;
	
	bNativeMode = false ;
	bDMAPossible = true ;
	bMMIO = false ;

	if((bInterface & 0x05) == 0x00)
	{
		KC::MDisplay().Message("\n\tController is in Legacy Mode", Display::WHITE_ON_BLACK()) ;

		if(ATADeviceController_bLegacyControllerFound == true)
		{
			KC::MDisplay().Message("\n\tAlready a Controller is in Legacy Mode... \n\tTrying to Switch to Native Mode",
								Display::WHITE_ON_BLACK()) ;

			if((bInterface & 0x0A) != 0x0A)
			{
				KC::MDisplay().Message("\n\tSwitch to Native Mode Failed...", Display::WHITE_ON_BLACK()) ;
				return ATADeviceController_ERR_SWITCH_NATIVE_MODE ;
			}

			RETURN_X_IF_NOT(pPCIEntry->WritePCIConfig(PCI_INTERFACE, 1, (bInterface | 0x05)), Success, ATADeviceController_FAILURE) ;

			bNativeMode = true ;
		}
		else
		{
			uiPrimaryCommand = 0x1F0 ;
			uiPrimaryControl = 0x3F6 ;
			uiSecondaryCommand = 0x170 ;
			uiSecondaryControl = 0x376 ;

			bPrimaryIRQ = 14 ;
			bSecondaryIRQ = 15 ;

			uiPrimaryDMA = uiSecondaryDMA = 0 ;

			RETURN_X_IF_NOT(
			pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 16, 4, &uiPrimaryDMA), Success, ATADeviceController_FAILURE) ;
	
			if(!((uiPrimaryDMA & 0x01) && (uiPrimaryDMA & PCI_ADDRESS_IO_MASK) ))
			{
				KC::MDisplay().Message("\n\tDMA Registers in MMIO Mode or Not Present", Display::WHITE_ON_BLACK()) ;
				KC::MDisplay().Message("\n\tDisabling DMA...", Display::WHITE_ON_BLACK()) ;
				bDMAPossible = false ;
				uiPrimaryDMA = 0 ;
			}
			else
			{
				uiPrimaryDMA &= PCI_ADDRESS_IO_MASK ;
				uiSecondaryDMA = uiPrimaryDMA + 0x8 ;
			}

			ATADeviceController_bLegacyControllerFound = true ;
		}
	}
	else
	{
		bNativeMode = true ;
	}

	if(bNativeMode == true)
	{
		RETURN_X_IF_NOT(
		pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 0, 4, &uiPrimaryCommand), Success, ATADeviceController_FAILURE) ;

		RETURN_X_IF_NOT(
		pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 4, 4, &uiPrimaryControl), Success, ATADeviceController_FAILURE) ;

		RETURN_X_IF_NOT(
		pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 8, 4, &uiSecondaryCommand), Success, ATADeviceController_FAILURE) ;

		RETURN_X_IF_NOT(
		pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 12, 4, &uiSecondaryControl), Success, ATADeviceController_FAILURE) ;

		RETURN_X_IF_NOT(
		pPCIEntry->ReadPCIConfig(PCI_INTERRUPT_LINE, 1, &bPrimaryIRQ), Success, ATADeviceController_FAILURE) ;
		bSecondaryIRQ = bPrimaryIRQ;
		
		unsigned uiMask ;
		if(!(uiPrimaryCommand & 0x01))
		{
			KC::MDisplay().Message("\n\tController uses MMIO registers", Display::WHITE_ON_BLACK()) ;
			bMMIO = true ;
			uiMask = PCI_ADDRESS_MEMORY_32_MASK ;
		}
		else
		{
			KC::MDisplay().Message("\n\tController uses PIO registers", Display::WHITE_ON_BLACK()) ;
			uiMask = PCI_ADDRESS_IO_MASK ;
		}

		if(bPrimaryIRQ != 14)
		{
			KC::MDisplay().Message("\n\tPrimary IRQ != 14. Disabling DMA", Display::WHITE_ON_BLACK()) ;
			bDMAPossible = false ;
		}

		if(bPrimaryIRQ < 5 || bPrimaryIRQ > 24 || bSecondaryIRQ < 5 || bSecondaryIRQ > 24)
		{
			KC::MDisplay().Message("\n\tInvalid IRQs. Disabling DMA", Display::WHITE_ON_BLACK()) ;
			bDMAPossible = false ;
		}

		uiPrimaryCommand &= uiMask ;
		uiSecondaryCommand &= uiMask ;
		uiPrimaryControl = (uiPrimaryControl & uiMask) | 2 ;
		uiSecondaryControl = (uiSecondaryControl & uiMask) | 2 ;

		uiPrimaryDMA = uiSecondaryDMA = 0 ;

		RETURN_X_IF_NOT(pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 16, 4, &uiPrimaryDMA), Success, ATADeviceController_FAILURE) ;
		
		if( (!(uiPrimaryDMA & 0x01) != bMMIO) || !(uiPrimaryDMA & uiMask))
		{
			KC::MDisplay().Message("\n\t DMA registers in different mode than Base registers", Display::WHITE_ON_BLACK()) ;
			KC::MDisplay().Message("\n\tDisabling DMA...", Display::WHITE_ON_BLACK()) ;
			bDMAPossible = false ;
			uiPrimaryDMA = 0 ;
		}
		else
		{
			uiPrimaryDMA &= uiMask ;
			uiSecondaryDMA = uiPrimaryDMA + 0x08 ;
		}
	}

	KC::MDisplay().Message("\n\tChannel 0: ", Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message((bMMIO) ? "MMIO " : "PIO ", Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Address(" Cmd - ", uiPrimaryCommand) ;
	KC::MDisplay().Address(" Ctrl - ", uiPrimaryControl) ;
	KC::MDisplay().Address(" DMA - ", uiPrimaryDMA) ;
	KC::MDisplay().Address(" IRQ - ", bPrimaryIRQ) ;

	KC::MDisplay().Message("\n\tChannel 1: ", Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Message((bMMIO) ? "MMIO " : "PIO ", Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Address(" Cmd - ", uiSecondaryCommand) ;
	KC::MDisplay().Address(" Ctrl - ", uiSecondaryControl) ;
	KC::MDisplay().Address(" DMA - ", uiSecondaryDMA) ;
	KC::MDisplay().Address(" IRQ - ", bSecondaryIRQ) ;

	*pController = ATADeviceController_AllocateController(2, 2) ;
	strcpy((*pController)->szName, "PCI ATA Controller") ;

	unsigned i ;
	for(i = 0; i < (*pController)->uiChannels * (*pController)->uiPortsPerChannel; i++)
	{
		ATAPort* pPort = ATADeviceController_AllocatePort(*pController) ;
		
		unsigned uiRegBase = (i & 2) ? uiSecondaryCommand : uiPrimaryCommand ;
		unsigned uiDMARegBase = (i & 2) ? uiSecondaryDMA : uiPrimaryDMA ;
		byte bDMAMask = (i & 1) ? ATA_DMA_STATUS_1_EN : ATA_DMA_STATUS_0_EN ;
		//TODO: UnderStand
// enableSpeeds = 0 -- > Global Variable
		unsigned uiDMASpeedMask = 0 ? 0xFFFFFFFF : ((1 << ATA_SPEED_PIO) | (1 << ATA_SPEED_DMA)) ;

		pPort->portOperation.Reset = ATAPortOperation_PortReset ;
		pPort->bMMIO = bMMIO ;

		if(bDMAPossible && bMMIO && (*((byte*)uiDMARegBase + 2) & bDMAMask))
			pPort->uiSupportedPortSpeed = uiDMASpeedMask ;
		else if(bDMAPossible && !bMMIO && (PortCom_ReceiveByte(uiDMARegBase + 2) & bDMAMask))
			pPort->uiSupportedPortSpeed = uiDMASpeedMask ;
		else
			pPort->uiSupportedPortSpeed = (1 << ATA_SPEED_PIO) ;
		
		pPort->uiType = ATA_PATA ;
		pPort->uiCable = ATA_CABLE_UNKNOWN ;

		unsigned j ;
		for(j = 0; j < ATA_TOTAL_REGS - 1; j++)
			pPort->uiRegs[j] = uiRegBase + j ;

		pPort->uiRegs[ATA_REG_CONTROL] = (i & 2) ? uiSecondaryControl : uiPrimaryControl ;

		pPort->uiDMARegs[ATA_REG_DMA_CONTROL] = uiDMARegBase ;
		pPort->uiDMARegs[ATA_REG_DMA_STATUS] = uiDMARegBase + 2 ;
		pPort->uiDMARegs[ATA_REG_DMA_TABLE] = uiDMARegBase + 4 ;

		(*pController)->pPort[i] = pPort ;
	}

	if(bDMAPossible)
	{
		HD_PRIMARY_IRQ = PIC::Instance().RegisterIRQ(bPrimaryIRQ, (unsigned)&ATADeviceController_PrimaryIRQHandler) ;
		if(!HD_PRIMARY_IRQ)
		{
			printf("\n Failed to register HD Primary IRQ %d", bPrimaryIRQ) ;
			return ATADeviceController_FAILURE ;
		}
		PIC::Instance().EnableInterrupt(*HD_PRIMARY_IRQ);

		if(bPrimaryIRQ != bSecondaryIRQ)
		{
			HD_SECONDARY_IRQ = PIC::Instance().RegisterIRQ(bSecondaryIRQ, (unsigned)&ATADeviceController_SecondaryIRQHandler) ;
			if(!HD_SECONDARY_IRQ)
			{
				printf("\n Failed to register HD Secondary IRQ %d", bSecondaryIRQ) ;
				return ATADeviceController_FAILURE ;
			}
			PIC::Instance().EnableInterrupt(*HD_SECONDARY_IRQ);
		}
	}

	if(pInitFunc && bDMAPossible)
	{
		pInitFunc(pPCIEntry, *pController) ;
	}

	return ATADeviceController_SUCCESS ;
}

static void ATADeviceController_AddATADrive(RawDiskDrive* pDisk)
{
	char driveCh = 'a' ;
	char driveName[5] = { 'h', 'd', 'd', '\0', '\0' } ;

	PartitionTable partitionTable ;
	byte bStatus ;

	ATAPort* pPort = (ATAPort*)pDisk->Device();
	bStatus = pDisk->ReadPartitionTable(&partitionTable) ;
	if(bStatus == PartitionManager_FAILURE)
	{
		KC::MDisplay().Message("\n Fatal Error:- Reading Partition Table", ' ') ;
		return ;
	}
	else if(bStatus == PartitionManager_ERR_NOT_PARTITIONED)
	{
		KC::MDisplay().Number("\n Disk Not Partitioned: ", bStatus) ;
		return ;
	}

  const auto& priPartitions = partitionTable.GetPrimaryPartitions();
  const auto& extPartitions = partitionTable.GetExtPartitions();
	unsigned uiTotalPartitions = priPartitions.size() + extPartitions.size();
	const PartitionInfo* pPartitionInfo ;

	/*** Calculate MountSpacePerPartition, TableCacheSize *****/
	const unsigned uiSectorsInFreePool = 4096 ;
	unsigned uiSectorsInTableCache = 1024 ;
	
	const unsigned uiMinMountSpaceRequired =  FileSystem_GetSizeForTableCache(uiSectorsInTableCache) ;
	const unsigned uiTotalMountSpaceAvailable = MEM_HDD_FS_END - MEM_HDD_FS_START ;
	
	unsigned uiNoOfParitions = uiTotalPartitions ;
	unsigned uiMountSpaceAvailablePerDrive = 0 ;
	while(true)
	{
		if(uiNoOfParitions == 0)
			break ;
		uiMountSpaceAvailablePerDrive = uiTotalMountSpaceAvailable / uiNoOfParitions ;
		if(uiMountSpaceAvailablePerDrive > uiMinMountSpaceRequired)
			break ;	
		uiNoOfParitions--;
	}
	
	if( uiMountSpaceAvailablePerDrive > uiMinMountSpaceRequired )
	{
		uiSectorsInTableCache = uiMountSpaceAvailablePerDrive / FileSystem_GetSizeForTableCache(1) ;
	}
	/*** DONE - Calculating mount stuff ***/
  auto extPartitionIt = extPartitions.begin();
  auto priPartitionIt = priPartitions.begin();
	for(unsigned i = 0; i < uiTotalPartitions; i++)
	{
    unsigned uiMountPointStart = 0;
    unsigned uiMountPointEnd = 0;
		if(i < uiNoOfParitions)
		{
			uiMountPointStart = MEM_HDD_FS_START + uiMountSpaceAvailablePerDrive * i ;
			uiMountPointEnd = MEM_HDD_FS_START + uiMountSpaceAvailablePerDrive * (i + 1) ;
		}

    unsigned uiLBAStartSector;
		if(priPartitionIt != priPartitions.end())
		{
			pPartitionInfo = *priPartitionIt;
			uiLBAStartSector = pPartitionInfo->LBAStartSector;
      ++priPartitionIt;
		}
		else if(extPartitionIt != extPartitions.end())
		{
			pPartitionInfo = &extPartitionIt->CurrentPartition();
			uiLBAStartSector = extPartitionIt->ActualStartSector() + pPartitionInfo->LBAStartSector;
      ++extPartitionIt;
		}
    else
      break;

		driveName[3] = driveCh + ATADeviceController_uiHDDDeviceID++ ;
    DiskDriveManager::Instance().Create(driveName, DEV_ATA_IDE, HDD_DRIVE0,
      uiLBAStartSector,
      pPartitionInfo->LBANoOfSectors,
      pPort->id.usSectors,
      pPort->id.usCylinders,
      pPort->id.usHead,
      pPort,
      pDisk,
      uiSectorsInFreePool,
      uiSectorsInTableCache,
      uiMountPointStart,
      uiMountPointEnd);
	}
}

static void ATADeviceController_AddATAPIDrive(ATAPort* pPort)
{
	char driveCh = 'a' ;
	char driveName[4] = { 'c', 'd', '\0', '\0' } ;
	driveName[2] = driveCh + ATADeviceController_uiCDDeviceID++;

	const unsigned uiSectorsInFreePool = 4096 ;
	unsigned uiSectorsInTableCache = 1024 ;
	
	const unsigned uiMinMountSpaceRequired =  FileSystem_GetSizeForTableCache(uiSectorsInTableCache) ;
	const unsigned uiTotalMountSpaceAvailable = MEM_CD_FS_END - MEM_CD_FS_START;
	
  if(uiTotalMountSpaceAvailable < uiMinMountSpaceRequired)
    printf("\n insufficient mount space available for %s", driveName);
	
  unsigned uiSizeInSectors;
	if(pPort->bLBA48Bit)
		uiSizeInSectors = pPort->id.uiLBACapacity48 ;
	else
		uiSizeInSectors = pPort->id.uiLBASectors ;

	DiskDriveManager::Instance().Create(driveName, DEV_ATAPI, CD_DRIVE0,
    0,
    uiSizeInSectors, 
    pPort->id.usSectors, pPort->id.usCylinders, pPort->id.usHead,
    pPort, nullptr, uiSectorsInFreePool, uiSectorsInTableCache,
    MEM_CD_FS_START, MEM_CD_FS_END);
}

static byte ATADeviceController_Add(ATAController* pController)
{
	unsigned i, j ;
	
	for(i = 0; i < pController->uiChannels; i++)
	{
		for(j = 0; j < pController->uiPortsPerChannel; j++)
		{
			ATAPort* pPort = pController->pPort[i * pController->uiPortsPerChannel + j] ;

			if(pPort->portOperation.Reset == NULL)
			{
				KC::MDisplay().Message("\n\tNo Reset Function provided...", Display::WHITE_ON_BLACK()) ;
				return ATADeviceController_SUCCESS ;
			}

			if(pPort->portOperation.Configure == NULL)
			{
				KC::MDisplay().Message("\n\tNo Configure Function provided...", Display::WHITE_ON_BLACK()) ;
				return ATADeviceController_SUCCESS ;
			}

			pPort->uiID = ATADeviceController_GetNextPortID() ;
			pPort->uiChannel = i ;
			pPort->uiPort = j ;
// TODO: Remove Data Buffer if not required
//			pPort->pDataBuffer = (byte*)DMM_AllocateForKernel(16 * PAGE_SIZE + 4) ;
		}
	}

	// Add ATA Controller to List
	pController->pNextATAController = ATADeviceController_pFirstController ;
	ATADeviceController_pFirstController = pController ;

	// Probe Ports
	for(i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
		ATAPortManager_Probe(pController->pPort[i]) ;

	// Configure Ports
	for(i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
	{
		ATAPortManager_ConfigureDrive(pController->pPort[i]) ;

		if(pController->pPort[i]->portOperation.Configure != NULL)
			pController->pPort[i]->portOperation.Configure(pController->pPort[i]) ;

		if(0) //TODO: ForcePIO Kernel Param
			pController->pPort[i]->uiCurrentSpeed = ATA_SPEED_PIO ;
	}

	char szName[10] = "hddisk0" ;
	int iCntIndex = 6 ;

	for(i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
	{	
		if(pController->pPort[i]->uiDevice == ATA_DEV_ATA)
		{
			szName[iCntIndex] += i ;
			RawDiskDrive* pDisk = DiskDriveManager::Instance().CreateRawDisk(szName, ATA_HARD_DISK, pController->pPort[ i ]) ;
			ATADeviceController_AddATADrive(pDisk) ;
		}
		else if(pController->pPort[i]->uiDevice == ATA_DEV_ATAPI)
			ATADeviceController_AddATAPIDrive(pController->pPort[i]) ;
	}

	return ATADeviceController_SUCCESS ;
}

static byte ATADeviceController_InitController(const PCIEntry* pPCIEntry, VendorSpecificATAController_Initialize* pInitFunc)
{
	KC::MDisplay().Message("\n\nATA PCI Controller found on", Display::WHITE_ON_BLACK()) ;
	KC::MDisplay().Address(" Bus: ", pPCIEntry->uiBusNumber) ;
	KC::MDisplay().Address(" Device: ", pPCIEntry->uiDeviceNumber) ;
	KC::MDisplay().Address(" Function: ", pPCIEntry->uiFunction) ;
	KC::MDisplay().Address(" VendorID: ", pPCIEntry->usVendorID) ;
	KC::MDisplay().Address(" DeviceID: ", pPCIEntry->usDeviceID) ;

	byte bStatus ;
	ATAController* pController ;

	RETURN_IF_NOT(bStatus, ATADeviceController_EnableDisabledController(pPCIEntry), ATADeviceController_SUCCESS) ;

	RETURN_IF_NOT(bStatus, ATADeviceController_CheckControllerMode(pPCIEntry, pInitFunc, &pController), ATADeviceController_SUCCESS) ;

	RETURN_IF_NOT(bStatus, ATADeviceController_Add(pController), ATADeviceController_SUCCESS) ;

	return ATADeviceController_SUCCESS ;
}

/******************************** static functions **************************************************/

void ATADeviceController_Initialize()
{
	ATADeviceController_bInitStatus = false ;

	ATADeviceController_uiHDDDeviceID = 0 ;
	ATADeviceController_uiCDDeviceID = 0 ;

	ATADeviceController_bLegacyControllerFound = false ;
	ATADeviceController_uiPortIDSequence = 0 ;
	ATADeviceController_pFirstController = NULL ;

	byte bControllerFound = false ;
	unsigned uiDeviceIndex ;

	for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())
	{
		if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		for(uiDeviceIndex = 0; 
			uiDeviceIndex < (sizeof(ATADeviceController_Devices) / sizeof(ATAPCIDevice)); uiDeviceIndex++)
		{
			if(pPCIEntry->usVendorID == ATADeviceController_Devices[uiDeviceIndex].usVendorID
				&& pPCIEntry->usDeviceID == ATADeviceController_Devices[uiDeviceIndex].usDeviceID)
			{
				if(ATADeviceController_InitController(pPCIEntry, ATADeviceController_Devices[uiDeviceIndex].InitFunc) == 
					ATADeviceController_SUCCESS)
				{
					bControllerFound = true ;
				}
			}
		}

		if(bControllerFound == false && pPCIEntry->bClassCode == PCI_MASS_STORAGE 
			&& pPCIEntry->bSubClass == PCI_IDE && pPCIEntry->usDeviceID != 0x3149)
		{
			if(ATADeviceController_InitController(pPCIEntry, NULL) == ATADeviceController_SUCCESS)
				bControllerFound = true ;
		}
	}

	ATADeviceController_bInitStatus = bControllerFound ;
	KC::MDisplay().LoadMessage("IDE Initialization", ATADeviceController_bInitStatus ? Success : Failure);
}

bool ATADeviceController_GetInitStatus()
{
	return ATADeviceController_bInitStatus ;
}

unsigned ATADeviceController_GetDeviceSectorLimit(ATAPort* pPort)
{
	return (pPort->bLBA48Bit) ? pPort->id.uiLBACapacity48 : pPort->id.uiLBASectors ;
}

const IRQ& ATADeviceController_GetHDInterruptNo(ATAPort* pPort)
{
	if(pPort->uiDevice == ATA_DEV_ATAPI)
		return *HD_SECONDARY_IRQ ;
	return *HD_PRIMARY_IRQ ;
}

