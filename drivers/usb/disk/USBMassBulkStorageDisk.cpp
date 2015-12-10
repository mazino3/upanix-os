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
#include <USBMassBulkStorageDisk.h>
#include <ProcessManager.h>
#include <DMM.h>
#include <stdio.h>
#include <PortCom.h>
#include <string.h>
#include <Display.h>
#include <PartitionManager.h>
#include <FileSystem.h>
#include <StringUtil.h>
#include <USBController.h>
#include <EHCIStructures.h>

static int USB_MBS_DriverId ;
static int USDDeviceId ;

static void USBMassBulkStorageDisk_AddDeviceDrive(RawDiskDrive* pDisk)
{
	char driveCh = 'a' ;
	char driveName[5] = { 'u', 's', 'd', '\0', '\0' } ;

	PartitionTable partitionTable ;
	byte bStatus ;

	SCSIDevice* pDevice = (SCSIDevice*)pDisk->pDevice ;
	bStatus = PartitionManager_ReadPartitionInfo(pDisk, &partitionTable) ;

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

	DriveInfo* pDriveInfo ;
	Drive* pDrive ;
	
	unsigned i ;
	unsigned uiTotalPartitions = partitionTable.uiNoOfPrimaryPartitions + partitionTable.uiNoOfExtPartitions ;
	PartitionInfo* pPartitionInfo ;

	/*** Calculate MountSpacePerPartition, FreePoolSize, TableCacheSize *****/
	const unsigned uiSectorsInFreePool = 4096 ;
	unsigned uiSectorsInTableCache = 1024 ;
	
	const unsigned uiMinMountSpaceRequired =  FileSystem_GetSizeForTableCache(uiSectorsInTableCache) ;
	const unsigned uiTotalMountSpaceAvailable = MEM_USD_FS_END - MEM_USD_FS_START ;
	
	unsigned uiNoOfParitions = uiTotalPartitions ;
	unsigned uiMountSpaceAvailablePerDrive = 0 ;
	while(true)
	{
		if(uiNoOfParitions == 0)
			break ;

		uiMountSpaceAvailablePerDrive = uiTotalMountSpaceAvailable / uiNoOfParitions ;

		if( uiMountSpaceAvailablePerDrive > uiMinMountSpaceRequired )
		{
			break ;	
		}

		uiNoOfParitions-- ;
	}
	
	if( uiMountSpaceAvailablePerDrive > uiMinMountSpaceRequired )
	{
		uiSectorsInTableCache = uiMountSpaceAvailablePerDrive / FileSystem_GetSizeForTableCache(1) ;
	}
	/*** DONE - Calculating mount stuff ***/
	
	for(i = 0; i < uiTotalPartitions; i++)
	{
		pDriveInfo = DeviceDrive_CreateDriveInfo(true) ;
		pDrive = &pDriveInfo->drive ;

		if(i < partitionTable.uiNoOfPrimaryPartitions)
		{
			pPartitionInfo = &partitionTable.PrimaryParitions[i] ;
			pDrive->uiLBAStartSector = pPartitionInfo->LBAStartSector ;
		}
		else
		{
			pPartitionInfo = &partitionTable.ExtPartitions[i - partitionTable.uiNoOfPrimaryPartitions].CurrentPartition ;
			pDrive->uiLBAStartSector = partitionTable.ExtPartitions[i - partitionTable.uiNoOfPrimaryPartitions].uiActualStartSector + pPartitionInfo->LBAStartSector ;
		}

		driveName[3] = driveCh + USDDeviceId++ ;

		String_Copy(pDrive->driveName, driveName) ;

		pDrive->deviceType = DEV_SCSI_USB_DISK ;
		pDrive->fsType = FS_UNKNOWN ; 
		pDrive->driveNumber = USD_DRIVE0 ;

		pDrive->uiStartCynlider = pPartitionInfo->StartCylinder ;
		pDrive->uiStartHead = pPartitionInfo->StartHead ;
		pDrive->uiStartSector = pPartitionInfo->StartSector ;
		
		pDrive->uiEndCynlider = pPartitionInfo->EndCylinder ;
		pDrive->uiEndHead = pPartitionInfo->EndHead ;
		pDrive->uiEndSector = pPartitionInfo->EndSector ;
		
		pDrive->uiSizeInSectors = pPartitionInfo->LBANoOfSectors ;

		pDrive->uiSectorsPerTrack = 1 ;
		pDrive->uiTracksPerHead = 1 ;
		pDrive->uiNoOfHeads = 1 ;
		
		pDriveInfo->pDevice = pDevice ;

		pDriveInfo->uiMaxSectorsInFreePoolCache = uiSectorsInFreePool ;
		pDriveInfo->uiNoOfSectorsInTableCache = uiSectorsInTableCache ;

		if(i < uiNoOfParitions)
		{
			pDriveInfo->uiMountPointStart = MEM_USD_FS_START + uiMountSpaceAvailablePerDrive * i ;
			pDriveInfo->uiMountPointEnd = MEM_USD_FS_START + uiMountSpaceAvailablePerDrive * (i + 1) ;
		}
		else
		{
			pDriveInfo->uiMountPointStart = 0 ;
			pDriveInfo->uiMountPointEnd = 0 ;
		}

		pDriveInfo->pRawDisk = pDisk ;
		DeviceDrive_AddEntry(pDriveInfo) ;
	}
}

static byte USBMassBulkStorageDisk_DoReset(USBulkDisk* pDisk)
{
	printf("\n Doing Bulk Reset on USB Bulk Mass Storage Device\n") ;

	USBDevice* pUSBDevice = pDisk->pUSBDevice ;

	if(pUSBDevice == NULL)
		return USBMassBulkStorageDisk_FAILURE ;

	KC::MDisplay().Message("\n Doing Command Reset", Display::WHITE_ON_BLACK()) ;

	if(!pUSBDevice->CommandReset(pUSBDevice))
	{
		printf("\n Failed to Reset device. Bulk Mass Storage Reset Failed !\n") ;
		return USBMassBulkStorageDisk_FAILURE ;
	}

	KC::MDisplay().Message("\n Command Reset Complete", Display::WHITE_ON_BLACK()) ;

	// Long Wait For Reset
	ProcessManager::Instance().Sleep( 5 * 1000 );

	KC::MDisplay().Message("\n Doing IN EndPoint Clear Halt", Display::WHITE_ON_BLACK()) ;

	if(!pUSBDevice->ClearHaltEndPoint(pDisk, true))
	{
		printf("\n Failed to Clear Halt EndPointIn. Bulk Mass Storage Reset Failed !\n") ;
		return USBMassBulkStorageDisk_FAILURE ;
	}

	KC::MDisplay().Message("\n IN EndPoint Clear Halt Complete", Display::WHITE_ON_BLACK()) ;

	KC::MDisplay().Message("\n Doing OUT EndPoint Clear Halt", Display::WHITE_ON_BLACK()) ;

	if(!pUSBDevice->ClearHaltEndPoint(pDisk, false))
	{
		printf("\n Failed to Clear Halt EndPointOut. Bulk Mass Storage Reset Failed !\n") ;
		return USBMassBulkStorageDisk_FAILURE ;
	}

	KC::MDisplay().Message("\n OUT EndPoint Clear Halt Complete", Display::WHITE_ON_BLACK()) ;

	printf("\n Bulk soft Reset Complete\n") ;

	return USBMassBulkStorageDisk_SUCCESS ;
}


/***************************************************************************************************************************/

byte USBMassBulkStorageDisk_ReadData(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
	unsigned uiTranLen ;
	while(uiLen > 0)
	{
		uiTranLen = (uiLen > US_BULK_MAX_TRANSFER_SIZE) ? US_BULK_MAX_TRANSFER_SIZE : uiLen ;
		uiLen -= uiTranLen ;

		if(!pDisk->pUSBDevice->BulkRead(pDisk, pDataBuf, uiTranLen))
		{
			printf("\n Bulk Read Data Failed") ;
			return USBMassBulkStorageDisk_FAILURE ;
		}

		pDataBuf = (byte*)pDataBuf + uiTranLen ;
	}

	return USBMassBulkStorageDisk_SUCCESS ;
}

byte USBMassBulkStorageDisk_WriteData(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
	unsigned uiTranLen ;
	while(uiLen > 0)
	{
		uiTranLen = (uiLen > US_BULK_MAX_TRANSFER_SIZE) ? US_BULK_MAX_TRANSFER_SIZE : uiLen ;
		uiLen -= uiTranLen ;

		if(!pDisk->pUSBDevice->BulkWrite(pDisk, pDataBuf, uiTranLen))
		{
			printf("\n Bulk Write Data Failed") ;
			return USBMassBulkStorageDisk_FAILURE ;
		}

		pDataBuf = (byte*)pDataBuf + uiTranLen ;
	}

	return USBMassBulkStorageDisk_SUCCESS ;
}

byte USBMassBulkStorageDisk_SendCommand(USBulkDisk* pDisk, SCSICommand* pCommand)
{
	USBulkCBW oCBW ;
	USBulkCSW oCSW ;

	USBDevice* pUSBDevice = (USBDevice*)(pDisk->pUSBDevice) ;

	if(pUSBDevice == NULL)
		return USBMassBulkStorageDisk_FAILURE ;

	// Setup Command Block Wrapper (CBW)
	oCBW.uiSignature = USBDISK_BULK_CBW_SIGNATURE ;
	oCBW.uiTag = 0 ;
	oCBW.uiDataTransferLen = SCSIHandler_GetTransferLen(pCommand) ;
	oCBW.bFlags = (pCommand->iDirection == SCSI_DATA_READ) ? USBDISK_CBW_DATA_IN : USBDISK_CBW_DATA_OUT ;
	oCBW.bLUN = (pCommand->bCommand[ 1 ] >> 5 ) & 0xF ;

	/*TODO:
	if ( psDisk -> nFlags & US_FL_SCM_MULT_TARG ) {
		sBcb.nLun |= psSrb -> nDevice << 4;
	}*/

	oCBW.bCBLen = pCommand->iCmdLen & 0x1F ;

	/* copy the command payload */
	memset(oCBW.bCommandBlock, 0, sizeof(oCBW.bCommandBlock)) ;
	memcpy(oCBW.bCommandBlock, pCommand->bCommand, oCBW.bCBLen) ;

	/* Send it to out endpoint */
	byte bStatus ;
	RETURN_IF_NOT(bStatus, USBMassBulkStorageDisk_WriteData(pDisk, &oCBW, USB_CBW_SIZE), USBMassBulkStorageDisk_SUCCESS) ;

	if(pCommand->iDirection == SCSI_DATA_READ)
	{
		RETURN_IF_NOT(bStatus, USBMassBulkStorageDisk_ReadData(pDisk, pCommand->pRequestBuffer, oCBW.uiDataTransferLen), USBMassBulkStorageDisk_SUCCESS) ;
	}
	else
	{
		RETURN_IF_NOT(bStatus, USBMassBulkStorageDisk_WriteData(pDisk, pCommand->pRequestBuffer, oCBW.uiDataTransferLen), USBMassBulkStorageDisk_SUCCESS) ;
	}

	RETURN_IF_NOT(bStatus, USBMassBulkStorageDisk_ReadData(pDisk, &oCSW, 13), USBMassBulkStorageDisk_SUCCESS) ;

	/* Checl bulk status */
	if(oCSW.uiSignature != USBDISK_BULK_CSW_SIGNATURE || oCSW.uiTag != oCBW.uiTag || oCSW.bStatus > US_BULK_STAT_PHASE)
	{
		printf("\nUSB Disk: Command Block Status Failed") ;
		printf(" --- Signature: 0x%x, Tag: %d, Status: %d", oCSW.uiSignature, oCSW.uiTag, oCSW.bStatus) ;
		return USBMassBulkStorageDisk_FAILURE ;
	}

	/* based on the status code, we report good or bad */
	switch(oCSW.bStatus)
	{
		case US_BULK_STAT_OK:
			if(oCSW.uiDataResidue > 0)
			{
				printf("\n USB: Short Data Situation - Unprocessed: %d", oCSW.uiDataResidue) ;
				return USBMassBulkStorageDisk_SHORT_DATA ;
			}

			return USBMassBulkStorageDisk_SUCCESS ;
		case US_BULK_STAT_FAIL:
			printf("Command Failed\n") ;
			return USBMassBulkStorageDisk_FAILURE ;
		case US_BULK_STAT_PHASE:
			printf("Error Processing Command\n") ;
			return USBMassBulkStorageDisk_ERROR ;
	}

	return USBMassBulkStorageDisk_SUCCESS ;
}

const char* USBMassBulkStorageDisk_GetName()
{
	return "USB Disk" ;
}

bool USBMassBulkStorageDisk_SCSICommand(SCSICommand* pCommand)
{
	USBulkDisk* pDisk = (USBulkDisk*)pCommand->pHost->pPrivate ;

	if(pCommand->iLun > pDisk->bMaxLun)
		return false ;

	byte bStatus = USBMassBulkStorageDisk_SendCommand(pDisk, pCommand) ;

	if(bStatus == USBMassBulkStorageDisk_ERROR)
	{
		printf("\n Doing Full Reset of Mass Storage Device") ;
		USBMassBulkStorageDisk_DoReset(pDisk) ;
		pCommand->iResult = 1 ;
		return false ;
	}

	pCommand->iResult = 0 ;
	bool bNeedSense = (bStatus == USBMassBulkStorageDisk_FAILURE) ;

	/* Also, if we have a short transfer on a command that can't have
	 * a short transfer, we're going to do this.
	 */
	byte bCode = pCommand->bCommand[ 0 ] ;

	if(!bNeedSense && (bStatus == USBMassBulkStorageDisk_SHORT_DATA))
	{
		switch(bCode)
		{
			case SCSI_REQUEST_SENSE:
			case SCSI_INQUIRY:
			case SCSI_MODE_SENSE:
			case SCSI_LOG_SENSE:
			case SCSI_MODE_SENSE_10:
				break ;
			default:
				bNeedSense = true ;
		}
	}

	/* Now, if we need do the auto-sense, let's do it */
	if(bNeedSense)
	{
		byte* pOldReqBuf ;
		int iOldDataLen ;
		int iOldDirection ;
		byte bOldCommand[ SCSI_CMD_SIZE ] ;

		memcpy(bOldCommand, pCommand->bCommand, SCSI_CMD_SIZE) ;

		/* Set the command and the LUN */
		pCommand->bCommand[ 0 ] = SCSI_REQUEST_SENSE;
		pCommand->bCommand[ 1 ] = pCommand->bCommand[ 1 ] & 0xE0;
		pCommand->bCommand[ 2 ] = 0;
		pCommand->bCommand[ 3 ] = 0;
		pCommand->bCommand[ 4 ] = 18;
		pCommand->bCommand[ 5 ] = 0;

		/* set the transfer direction */
		iOldDirection = pCommand->iDirection ;
		pCommand->iDirection = SCSI_DATA_READ ;

		/* use the new buffer we have */
		pOldReqBuf = pCommand->pRequestBuffer ;
		pCommand->pRequestBuffer = pCommand->u.bSense ;

		/* set the buffer length for transfer */
		iOldDataLen = pCommand->iRequestLen ;
		pCommand->iRequestLen = 18 ;

		/* issue the auto-sense command */
		printf("\nAuto Sensing...") ;
		bStatus = USBMassBulkStorageDisk_SendCommand(pDisk, pCommand) ;

		if(bStatus == USBMassBulkStorageDisk_ERROR)
		{
			printf("Error processing Request Sense. Trying Complete RESET\n") ;
			USBMassBulkStorageDisk_DoReset(pDisk) ;
			pCommand->iResult = 2 ;
			return false ;
		}

		/* we're done here, let's clean up */
		pCommand->pRequestBuffer = pOldReqBuf ;
		pCommand->iRequestLen = iOldDataLen ;
		pCommand->iDirection = iOldDirection ;
		memcpy(pCommand->bCommand, bOldCommand, SCSI_CMD_SIZE) ;

		/* if things are really okay, then let's show that */
		if((pCommand->u.bSense[ 2 ] & 0xF) == 0x00)
		{
			pCommand->iResult = 0 ;
			return false ;
		}
		
		pCommand->iResult = 3 ;
	}

	return (bStatus == USBMassBulkStorageDisk_SUCCESS) ;
}

//sint32 usbDiskAllocateIrq( struct sUsbDisk *psDisk ) {
//	uint32 nPipe;
//	sint32 nMaxP;
//	sint32 nResult = 1;
//
//	kPrint( "Allocating IRQ for CBI transport\n" );
//
//	/* Lock access to the data structure */
//	LOCK( psDisk -> nIrqUrbSem );
//
//	/* Allocate the URB */
//	psDisk -> psIrqUrb = usbAllocPacket( 0 );
//	if ( !psDisk -> psIrqUrb ) {
//		UNLOCK( psDisk -> nIrqUrbSem );
//		kPrint( "[%s]: Couldn't allocate interrupt URB\n", __FUNCTION__ );
//		return 1;
//	}
//
//	/* calculate the pipe and max packet size */
//	nPipe = USB_RCVINTPIPE( psDisk -> psUsbDev, psDisk -> psEpInt -> nEndPointAddress & USB_ENDPOINT_NUMBER_MASK );
//	nMaxP = USB_MAXPACKET( psDisk -> psUsbDev, nPipe, USB_PIPEOUT( nPipe ) );
//	if ( nMaxP > sizeof( psDisk -> anIrqBuf ) ) {
//		nMaxP = sizeof( psDisk -> anIrqBuf );
//	}
//
//	/* fill in the URB with our data */
//	USB_FILL_INT( psDisk -> psIrqUrb, psDisk -> psUsbDev, nPipe,
//		psDisk -> anIrqBuf, nMaxP, usbStorCbiIrq, psDisk, psDisk -> psEpInt -> nInterval );
//
//	/* submit the URB for processing */
//	nResult = usbSubmitPacket( psDisk -> psIrqUrb );
//	if ( nResult ) {
//		usbFreePacket( psDisk -> psIrqUrb );
//		UNLOCK( psDisk -> nIrqUrbSem );
//		return 2;
//	}
//
//	/* unlock the data structure and return success */
//	UNLOCK( psDisk -> nIrqUrbSem );
//
//	return 0;
//}

static void USBMassBulkStorageDisk_Remove(USBDevice* pUSBDevice)
{
	USBulkDisk* pDisk = (USBulkDisk*)pUSBDevice->pPrivate ;
	SCSIHost* pHost = pDisk->pHost ;
	SCSIDevice** pSCSIDeviceList = pDisk->pSCSIDeviceList ;

	class USBDriveRemoveClause : public DriveRemoveClause
	{
		public:
			USBDriveRemoveClause(SCSIDevice** p, int iMaxLun) : m_pSCSIDeviceList(p), m_iMaxLun(iMaxLun) { }

			bool operator()(const DriveInfo* pDriveInfo) const
			{
				int iLun ;
				for(iLun = 0; iLun <= m_iMaxLun; iLun++)
				{
					if(m_pSCSIDeviceList[ iLun ] == NULL)
						break ;

					if(pDriveInfo->pDevice == m_pSCSIDeviceList[ iLun ])
						return true ;
				}

				return false ;
			}

		private:
			SCSIDevice** m_pSCSIDeviceList ;
			const int m_iMaxLun ;
	} ;

	if(pSCSIDeviceList != NULL)
		DeviceDrive_RemoveEntryByCondition(USBDriveRemoveClause(pSCSIDeviceList, pDisk->bMaxLun)) ;

	DMM_DeAllocateForKernel((unsigned)pHost) ;
	DMM_DeAllocateForKernel((unsigned)pDisk->pRawAlignedBuffer) ;
	DMM_DeAllocateForKernel((unsigned)pDisk) ;
}


static bool USBMassBulkStorageDisk_Add(USBDevice* pUSBDevice)
{
	USBulkDisk* pDisk = NULL ;
	bool bFound = false ;
	byte bDeviceSubClass = 0 ;
	byte bDeviceProtocol = 0 ;
	USBStandardInterface* pInterface = NULL ;
	USBStandardEndPt *pEndPointIn = NULL, *pEndPointOut = NULL, *pEndPointInt = NULL ;
	
	USBStandardConfigDesc* pConfig = &(pUSBDevice->pArrConfigDesc[ pUSBDevice->iConfigIndex ]) ;

	if(pUSBDevice->deviceDesc.sVendorID == 0x05DC)
	{
		KC::MDisplay().Message("Lexar Jumpshot USB CF Reader detected\n", Display::WHITE_ON_BLACK()) ;
		KC::MDisplay().Message("Not supported yet !", ' ') ;

		return USBMassBulkStorageDisk_FAILURE ;
	}

	if(bFound == false)
	{
		byte i ;
		for(i = 0; i < pConfig->bNumInterfaces; i++)
		{
			pInterface = &(pConfig->pInterfaces[ i ]) ;
			
			if(pInterface->bInterfaceClass == USB_CLASS_MASS_STORAGE)
			{
				if(pInterface->bInterfaceProtocol == USB_PR_BULK)
				{
					bFound = true ;
					pUSBDevice->iInterfaceIndex = i ;
					pUSBDevice->bInterfaceNumber = pInterface->bInterfaceNumber ;
					bDeviceSubClass = pInterface->bInterfaceSubClass ;
					bDeviceProtocol = pInterface->bInterfaceProtocol ;
					break;
				}
			}
		}
	}

	if( !bFound )
		return false ;

	/* Find the endpoints we need
	 * We are expecting a minimum of 2 endpoints - in and out (bulk).
	 * An optional interrupt is OK (necessary for CBI protocol).
	 * We will ignore any others.
	 */
	int i ;
	for(i = 0; i < pInterface->bNumEndpoints; i++)
	{
		USBStandardEndPt* pEndPoint = &(pInterface->pEndPoints[ i ]) ;

		// Bulk Endpoint
		if( (pEndPoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK )
		{
			// Bulk in/out
			if( pEndPoint->bEndpointAddress & USB_DIR_IN )
				pEndPointIn = pEndPoint ;
			else
				pEndPointOut = pEndPoint ;
		}
		else if( (pEndPoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT )
			pEndPointInt = pEndPoint ;
	}

	/*  Do some basic sanity checks, and bail if we find a problem */
	if( !pEndPointIn || !pEndPointOut || ( bDeviceProtocol == USB_PR_CB && !pEndPointInt ) )
	{
		printf("USBMassBulkStorageDisk: Invalid End Point configuration. In:%x, Out:%x, Int:%x", pEndPointIn, pEndPointOut, pEndPointInt) ;
		return false ;
	}

	/* Create device */
	pDisk = (USBulkDisk*)DMM_AllocateForKernel(sizeof(USBulkDisk)) ;
	if(pDisk == NULL)
	{
		KC::MDisplay().Message("USBMassBulkStorageDisk: Panic error -> Out of memory", ' ') ;
		return false ;
	}

	memset(pDisk, 0, sizeof(USBulkDisk)) ;

	/* copy over the subclass and protocol data */
	pDisk->bDeviceSubClass = bDeviceSubClass ;
	pDisk->bDeviceProtocol = bDeviceProtocol ;

	/* Copy over the endpoint data */
	if(pEndPointIn)
	{
		pDisk->uiEndPointIn = pEndPointIn->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK ;
		pDisk->usInMaxPacketSize = pEndPointIn->wMaxPacketSize ;
		printf("\n Max Bulk Read Size: %d", pDisk->usInMaxPacketSize) ;
	}

	if(pEndPointOut)
	{
		pDisk->uiEndPointOut = pEndPointOut->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK ;
		pDisk->usOutMaxPacketSize = pEndPointOut->wMaxPacketSize ;
		printf("\n Max Bulk Write Size: %d", pDisk->usOutMaxPacketSize) ;
	}

	pDisk->pRawAlignedBuffer = (byte*)DMM_AllocateAlignForKernel(US_BULK_MAX_TRANSFER_SIZE, 16) ;

	pDisk->pEndPointInt = pEndPointInt ;

	pDisk->pUSBDevice = pUSBDevice ;
	pUSBDevice->pPrivate = pDisk ;

	pDisk->bEndPointInToggle = pDisk->bEndPointOutToggle = 0 ;

	/* Set the handler pointers based on the protocol
	 * Again, this data is persistant across reattachments
	 */
	switch(pDisk->bDeviceProtocol)
	{
		case USB_PR_BULK:
			printf("\n Reading MAX LUN for the Bulk Storage Disk\n") ;
			pDisk->bMaxLun = 0 ;
			if(!pUSBDevice->GetMaxLun(pUSBDevice, &(pDisk->bMaxLun)))
			{
				printf("\n Failed to Read MAX LUN from the device") ;
				DMM_DeAllocateForKernel((unsigned)pDisk->pRawAlignedBuffer) ;
				DMM_DeAllocateForKernel((unsigned)pDisk) ;
				return false ;
			}
			printf("\n Max LUN of the device: %d", pDisk->bMaxLun) ;
			break;

		default:
			printf("\n Unsupported USB Disk Device Protocol: %d", pDisk->bDeviceProtocol) ;
			DMM_DeAllocateForKernel((unsigned)pDisk->pRawAlignedBuffer) ;
			DMM_DeAllocateForKernel((unsigned)pDisk) ;
			return false ;
	}

	switch(pDisk->bDeviceSubClass)
	{
		case USB_SC_SCSI:
			pDisk->szProtocolName = "SCSI" ;
			break ;

		default:
			printf("\n Unsupported USB Disk Device SubClass: %d", pDisk->bDeviceSubClass) ;
			DMM_DeAllocateForKernel((unsigned)pDisk->pRawAlignedBuffer) ;
			DMM_DeAllocateForKernel((unsigned)pDisk) ;
			return false ;
	}

	//TODO
	/* allocate an IRQ callback if one is needed */
//	if ( (  psDisk -> nProtocol == US_PR_CBI ) && usbDiskAllocateIrq( psDisk ) ) {
//		AtomicDec( &psDevice -> nRefCount );
//		return FALSE;
//	}

	/* Create SCSI host */
	pDisk->pHost = (SCSIHost*)DMM_AllocateForKernel(sizeof(SCSIHost)) ;
	memset(pDisk->pHost, 0, sizeof(SCSIHost)) ;

	//TODO
	pDisk->pHost->GetName = USBMassBulkStorageDisk_GetName ;
	pDisk->pHost->QueueCommand = USBMassBulkStorageDisk_SCSICommand ;
	pDisk->pHost->pPrivate = pDisk ;
	pDisk->pSCSIDeviceList = NULL ;

	byte bStatus = USBMassBulkStorageDisk_DoReset(pDisk) ;
	if(bStatus != USBMassBulkStorageDisk_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pDisk->pRawAlignedBuffer) ;
		DMM_DeAllocateForKernel((unsigned)pDisk->pHost) ;
		DMM_DeAllocateForKernel((unsigned)pDisk) ;
		return false ;
	}

	pDisk->pSCSIDeviceList = (SCSIDevice**)DMM_AllocateForKernel(sizeof(SCSIDevice**) * (pDisk->bMaxLun + 1)) ;
	int iLun ;
	for(iLun = 0; iLun <= pDisk->bMaxLun; iLun++)
		pDisk->pSCSIDeviceList[ iLun ] = NULL ;

	char szName[10] = "usbdisk0" ;
	int iCntIndex = 7 ;
	bool bDeviceFound = false ;

	//TODO: For the time configuring only LUN 0
	for(iLun = 0; iLun <= 0/*pDisk->bMaxLun*/; iLun++)
	{
		SCSIDevice* pSCSIDevice = SCSIHandler_ScanDevice(pDisk->pHost, 0, 0, iLun) ;
		pDisk->pSCSIDeviceList[ iLun ] = pSCSIDevice ;

		if(pSCSIDevice == NULL)
		{
			printf("\n No SCSI USB Mass Bulk Storage Device @ LUN: %d", iLun) ;
			continue ;
		}

		szName[ iCntIndex ] += iLun ;
		RawDiskDrive* pDisk = DeviceDrive_CreateRawDisk(szName, USB_SCSI_DISK, pSCSIDevice) ;

		if(!pDisk)
		{
			printf("\n Failed to add Raw Disk Drive for %s", szName) ;
			continue ;
		}

		DeviceDrive_AddRawDiskEntry(pDisk) ;
		USBMassBulkStorageDisk_AddDeviceDrive(pDisk) ;

		bDeviceFound = true ;
	}

	if(!bDeviceFound)
	{
		printf("\n No USB Bulk Storage Device Found. Driver setup failed") ;
		return false ;
	}

	return true ;
}

byte USBMassBulkStorageDisk_Initialize()
{
	USDDeviceId = 0 ;
	USBDriver* pDriver = (USBDriver*)DMM_AllocateForKernel(sizeof(USBDriver)) ;

	if( (USB_MBS_DriverId = USBController_RegisterDriver(pDriver)) < 0 )
		return USBMassBulkStorageDisk_FAILURE ;

	String_Copy(pDriver->szName, "USB Mass Storage Disk") ;
	pDriver->AddDevice = USBMassBulkStorageDisk_Add ;
	pDriver->RemoveDevice = USBMassBulkStorageDisk_Remove ;

	return USBMassBulkStorageDisk_SUCCESS ;
}

