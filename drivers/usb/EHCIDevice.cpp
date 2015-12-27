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
#include <Global.h>
#include <DMM.h>
#include <USBDataHandler.h>
#include <EHCIController.h>
#include <EHCIDataHandler.h>
#include <EHCITransaction.h>
#include <EHCIDevice.h>

EHCIDevice::EHCIDevice(EHCIController& controller) 
  : _controller(controller),
    _pControlQH(controller.CreateDeviceQueueHead(64, 0, 0)),
    _bFirstBulkRead(true), _bFirstBulkWrite(true)
{
  if(!SetAddress())
    throw upan::exception(XLOC, "SetAddress Failed");

	USBStandardDeviceDesc devDesc;
  if(!GetDeviceDescriptor(&devDesc))
    throw upan::exception(XLOC, "GetDevDesc Failed");
	USBDataHandler_DisplayDevDesc(&devDesc) ;

	char bConfigValue = 0;
  if(!GetConfigValue(bConfigValue))
    throw upan::exception(XLOC, "GetConfigVal Failed") ;
	printf("\n\n ConfifValue: %d", bConfigValue) ;

  if(!CheckConfiguration(bConfigValue, devDesc.bNumConfigs))
    throw upan::exception(XLOC, "CheckConfig Failed");

  if(!GetConfigDescriptor(devDesc.bNumConfigs, &pArrConfigDesc))
    throw upan::exception(XLOC, "GeConfigDesc Failed");

  if(!GetStringDescriptorZero(&pStrDescZero))
    throw upan::exception(XLOC, "GetStringDescZero Failed");

	USBDataHandler_CopyDevDesc(&deviceDesc, &devDesc, sizeof(USBStandardDeviceDesc)) ;

	USBDataHandler_SetLangId(this);

	iConfigIndex = 0 ;

	for(int i = 0; i < devDesc.bNumConfigs; i++)
	{
		if(pArrConfigDesc[ i ].bConfigurationValue == bConfigValue)
		{
			iConfigIndex = i;
			break ;
		}
	}
	
	GetDeviceStringDetails();
}

bool EHCIDevice::SetConfiguration(char bConfigValue)
{
	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pTDStart) ;
		return false;
	}

	USBDevRequest* pDevRequest = (USBDevRequest*)(KERNEL_VIRTUAL_ADDRESS(pTDStart->uiBufferPointer[ 0 ])) ;
	pDevRequest->bRequestType = 0x00 ;
	pDevRequest->bRequest = 9 ;
	pDevRequest->usWValue = bConfigValue ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 0 ;

	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = KERNEL_REAL_ADDRESS(pTD1) ;
	pTD1->uiNextpTDPointer = 1 ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (3 << 10) | (1 << 8) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);

	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false;
	}
	
  aTransaction.Clear();

	return true;
}

bool EHCIDevice::CheckConfiguration(char& bConfigValue, char bNumConfigs)
{
	if(bNumConfigs <= 0)
	{
		printf("\n Invalid NumConfigs: %d", bNumConfigs) ;
    return false;
	}

	if(bConfigValue <= 0 || bConfigValue > bNumConfigs)
	{
		bool bStatus ;
		printf("\n Current Configuration %d -> Invalid. Setting to Configuration 1", bConfigValue) ;
		RETURN_MSG_IF_NOT(bStatus, SetConfiguration(1), true, "SetConfiguration Failed") ;
		bConfigValue = 1 ;
	}

	return true;
}

bool EHCIDevice::GetConfigDescriptor(char bNumConfigs, USBStandardConfigDesc** pConfigDesc)
{
  bool status = true;
	int index ;

	USBStandardConfigDesc* pCD = (USBStandardConfigDesc*)DMM_AllocateForKernel(sizeof(USBStandardConfigDesc) * bNumConfigs) ;

	for(index = 0; index < (int)bNumConfigs; index++)
		USBDataHandler_InitConfigDesc(&pCD[index]) ;

	for(index = 0; index < (int)bNumConfigs; index++)
	{
		unsigned short usDescValue = (0x2 << 8) | (index & 0xFF) ;

		if(!(status = GetDescriptor(usDescValue, 0, -1, &(pCD[index]))))
			break ;

		int iLen = pCD[index].wTotalLength;

		void* pBuffer = (void*)DMM_AllocateForKernel(iLen) ;

		if(!(status = GetDescriptor(usDescValue, 0, iLen, pBuffer)))
		{
			DMM_DeAllocateForKernel((unsigned)pBuffer) ;
			break ;
		}

		USBDataHandler_CopyConfigDesc(&pCD[index], pBuffer, pCD[index].bLength) ;

		USBDataHandler_DisplayConfigDesc(&pCD[index]) ;

		printf("\n Parsing Interface information for Configuration: %d", index) ;
		printf("\n Number of Interfaces: %d", (int)pCD[ index ].bNumInterfaces) ;

		void* pInterfaceBuffer = (char*)pBuffer + pCD[index].bLength ;

		pCD[index].pInterfaces = (USBStandardInterface*)DMM_AllocateForKernel(sizeof(USBStandardInterface) * pCD[index].bNumInterfaces) ;
		int iI ;

		for(iI = 0; iI < (int)pCD[index].bNumInterfaces; iI++)
		{
			USBDataHandler_InitInterfaceDesc(&(pCD[index].pInterfaces[iI])) ;
		}

		for(iI = 0; iI < (int)pCD[index].bNumInterfaces; iI++)
		{
			USBStandardInterface* pInt = (USBStandardInterface*)(pInterfaceBuffer) ;

			int iIntLen = sizeof(USBStandardInterface) - sizeof(USBStandardEndPt*) ;
			int iCopyLen = pInt->bLength ;

			if(iCopyLen > iIntLen)
				iCopyLen = iIntLen ;

			MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pInt, 
								MemUtil_GetDS(), (unsigned)&(pCD[index].pInterfaces[iI]),
								iCopyLen) ;

			USBDataHandler_DisplayInterfaceDesc(&(pCD[index].pInterfaces[iI])) ;

			USBStandardEndPt* pEndPtBuffer = (USBStandardEndPt*)((char*)pInterfaceBuffer + pInt->bLength) ;

			int iNumEndPoints = pCD[index].pInterfaces[iI].bNumEndpoints ;

			pCD[index].pInterfaces[iI].pEndPoints = (USBStandardEndPt*)DMM_AllocateForKernel(
														sizeof(USBStandardEndPt) * iNumEndPoints) ;

			printf("\n Parsing EndPoints for Interface: %d of Configuration: %d", iI, index) ;

			int iE ;
			for(iE = 0; iE < iNumEndPoints; iE++)
				USBDataHandler_InitEndPtDesc(&(pCD[index].pInterfaces[iI].pEndPoints[iE])) ;

			for(iE = 0; iE < iNumEndPoints; iE++)
			{
				USBStandardEndPt* pEndPt = &pEndPtBuffer[iE] ;

				int iELen = sizeof(USBStandardEndPt) ;
				int iECopyLen = pEndPt->bLength ;
				
				if(iECopyLen > iELen)
					iECopyLen = iELen ;

				MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pEndPt, 
									MemUtil_GetDS(), (unsigned)&(pCD[index].pInterfaces[iI].pEndPoints[iE]),
									iECopyLen) ;

				USBDataHandler_DisplayEndPtDesc(&(pCD[index].pInterfaces[iI].pEndPoints[iE])) ;
			}

			pInterfaceBuffer = (USBStandardInterface*)((char*)pEndPtBuffer + iNumEndPoints * sizeof(USBStandardEndPt)) ;
		}

		DMM_DeAllocateForKernel((unsigned)pBuffer) ;
		pBuffer = NULL ;
	}

	if(!status)
	{
		USBDataHandler_DeAllocConfigDesc(pCD, bNumConfigs) ;
    return false;
	}

	*pConfigDesc = pCD ;

	return true;
}

bool EHCIDevice::GetStringDescriptorZero(USBStringDescZero** ppStrDescZero)
{
	unsigned short usDescValue = (0x3 << 8) ;
	char szPart[ 8 ];

	if(!GetDescriptor(usDescValue, 0, -1, szPart))
    return false;

	int iLen = ((USBStringDescZero*)&szPart)->bLength ;
	printf("\n String Desc Zero Len: %d", iLen) ;

	byte* pStringDescZero = (byte*)DMM_AllocateForKernel(iLen) ;

	if(!GetDescriptor(usDescValue, 0, iLen, pStringDescZero))
	{
		DMM_DeAllocateForKernel((unsigned)pStringDescZero) ;
		return false;
	}

	*ppStrDescZero = (USBStringDescZero*)DMM_AllocateForKernel(sizeof(USBStringDescZero)) ;

	USBDataHandler_CopyStrDescZero(*ppStrDescZero, pStringDescZero) ;
	USBDataHandler_DisplayStrDescZero(*ppStrDescZero) ;

	DMM_DeAllocateForKernel((unsigned)pStringDescZero) ;

	return true;
}

bool EHCIDevice::GetDeviceStringDetails()
{
	if(usLangID == 0)
	{
		strcpy(szManufacturer, "Unknown") ;
		strcpy(szProduct, "Unknown") ;
		strcpy(szSerialNum, "Unknown") ;
    return true;
	}

	unsigned short usDescValue = (0x3 << 8) ;
	char szPart[ 8 ];

	const int index_size = 3 ; // Make sure to change this with index[] below
	char arr_index[] = { deviceDesc.indexManufacturer, deviceDesc.indexProduct, deviceDesc.indexSerialNum } ;
	char* arr_name[] = { szManufacturer, szProduct, szSerialNum } ;

	int i ;
	for(i = 0; i < index_size; i++)
	{
		int index = arr_index[ i ] ;
    if(!GetDescriptor(usDescValue | index, usLangID, -1, szPart))
      return false;

		int iLen = ((USBStringDescZero*)&szPart)->bLength ;
		printf("\n String Index: %u, String Desc Size: %d", index, iLen) ;
		if(iLen == 0)
		{
			strcpy(arr_name[i], "Unknown") ;
			continue ;
		}

		byte* pStringDesc = (byte*)DMM_AllocateForKernel(iLen) ;
    if(!GetDescriptor(usDescValue | index, usLangID, iLen, pStringDesc))
		{
			DMM_DeAllocateForKernel((unsigned)pStringDesc) ;
			return false;
		}

		int j, k ;
		for(j = 0, k = 0; k < USB_MAX_STR_LEN && j < (iLen - 2); k++, j += 2)
		{
			//TODO: Ignoring Unicode 2nd byte for the time.
			arr_name[i][k] = pStringDesc[j + 2] ;
		}

		arr_name[i][k] = '\0' ;
	}

	USBDataHandler_DisplayDeviceStringDetails(this) ;

	return true;
}

bool EHCIDevice::GetMaxLun(byte* bLUN)
{
	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pTDStart) ;
		return false ;
	}

	USBDevRequest* pDevRequest = (USBDevRequest*)(KERNEL_VIRTUAL_ADDRESS(pTDStart->uiBufferPointer[ 0 ])) ;
	pDevRequest->bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN ;
	pDevRequest->bRequest = 0xFE ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = bInterfaceNumber ;
	pDevRequest->usWLength = 1 ;

	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = KERNEL_REAL_ADDRESS(pTD1) ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (1 << 16) | (3 << 10) | (1 << 8) | (1 << 7) ;
	
	if(SetupAllocBuffer(pTD1, 1) != EHCIController_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pTDStart) ;
		DMM_DeAllocateForKernel((unsigned)pTD1) ;
		return false ;
	}

	// It is important to store the data buffer address now for later read as the data buffer address
	// in TD will be incremented by Host Controller with the number of bytes read
	unsigned uiDataBuffer = KERNEL_VIRTUAL_ADDRESS(pTD1->uiBufferPointer[ 0 ]) ;

	EHCIQTransferDesc* pTD2 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTD1->uiNextpTDPointer = KERNEL_REAL_ADDRESS(pTD2) ;
	pTD2->uiNextpTDPointer = 1 ;
	pTD2->uiAltpTDPointer = 1 ;
	pTD2->uipTDToken = (3 << 10) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);

	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false ;
	}
	
	*bLUN = *((char*)uiDataBuffer) ;

  aTransaction.Clear();

	return true ;
}

bool EHCIDevice::CommandReset()
{
	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pTDStart) ;
		return false ;
	}

	USBDevRequest* pDevRequest = (USBDevRequest*)(KERNEL_VIRTUAL_ADDRESS(pTDStart->uiBufferPointer[ 0 ])) ;
	pDevRequest->bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE ;
	pDevRequest->bRequest = 0xFF ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = bInterfaceNumber ;
	pDevRequest->usWLength = 0 ;
	
	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = KERNEL_REAL_ADDRESS(pTD1) ;
	pTD1->uiNextpTDPointer = 1 ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (3 << 10) | (1 << 8) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);
	
	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false ;
	}

  aTransaction.Clear();

	return true ;
}

bool EHCIDevice::ClearHaltEndPoint(USBulkDisk* pDisk, bool bIn)
{
	unsigned uiEndPoint = (bIn) ? pDisk->uiEndPointIn : pDisk->uiEndPointOut ;

	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pTDStart) ;
		return false ;
	}

	USBDevRequest* pDevRequest = (USBDevRequest*)(KERNEL_VIRTUAL_ADDRESS(pTDStart->uiBufferPointer[ 0 ])) ;
	pDevRequest->bRequestType = USB_RECIP_ENDPOINT ;
	pDevRequest->bRequest = 1 ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = (uiEndPoint & 0xF) | ((bIn) ? 0x80 : 0x00) ;
	pDevRequest->usWLength = 0 ;
	
	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = KERNEL_REAL_ADDRESS(pTD1) ;
	pTD1->uiNextpTDPointer = 1 ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (3 << 10) | (1 << 8) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);
	
	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false ;
	}

  aTransaction.Clear();

	return true ;
}

extern void _UpdateReadStat(unsigned, bool) ;
bool EHCIDevice::BulkRead(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
	_UpdateReadStat(uiLen, true) ;
	if(uiLen == 0)
		return true ;

	unsigned uiMaxLen = pDisk->usInMaxPacketSize * MAX_EHCI_TD_PER_BULK_RW ;
	if(uiLen > uiMaxLen)
	{
		printf("\n Max Bulk Transfer per Frame is %d bytes", uiMaxLen) ;
		return false ;
	}

	int iNoOfTDs = uiLen / EHCI_MAX_BYTES_PER_TD ;
	iNoOfTDs += ((uiLen % EHCI_MAX_BYTES_PER_TD) != 0) ;

	if(iNoOfTDs > MAX_EHCI_TD_PER_BULK_RW)
	{
		printf("\n No. of TDs per Bulk Read/Wrtie exceeded limit %d !!", MAX_EHCI_TD_PER_BULK_RW) ;
		return false ;
	}

  const unsigned uiMaxPacketSize = pDisk->usInMaxPacketSize;

	if(_bFirstBulkRead)
	{
		_bFirstBulkRead = false ;

		for(int i = 0; i < MAX_EHCI_TD_PER_BULK_RW; i++)
			_ppBulkReadTDs[ i ] = EHCIDataHandler_CreateAsyncQTransferDesc() ; 

		_pBulkInEndPt = _controller.CreateDeviceQueueHead(uiMaxPacketSize, pDisk->uiEndPointIn, devAddr) ;
	}
	else
	{
		int i ;
		for(i = 0; i < MAX_EHCI_TD_PER_BULK_RW; i++)
			memset((void*)(_ppBulkReadTDs[i]), 0, sizeof(EHCIQTransferDesc)) ;

		EHCIDataHandler_CleanAysncQueueHead(_pBulkInEndPt) ;
	}

	int iIndex ;

	unsigned uiYetToRead = uiLen ;
	unsigned uiCurReadLen ;
	for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
	{
		uiCurReadLen = (uiYetToRead > EHCI_MAX_BYTES_PER_TD) ? EHCI_MAX_BYTES_PER_TD : uiYetToRead ;
		uiYetToRead -= uiCurReadLen ;

		EHCIQTransferDesc* pTD = _ppBulkReadTDs[ iIndex ] ;

		pTD->uiAltpTDPointer = 1 ;
		pTD->uipTDToken = (pDisk->bEndPointInToggle << 31) | (uiCurReadLen << 16) | (3 << 10) | (1 << 8) | (1 << 7) ;
		unsigned toggle = uiCurReadLen / uiMaxPacketSize ;
		if(uiCurReadLen % uiMaxPacketSize)
			toggle++ ;
		if(toggle % 2)
			pDisk->bEndPointInToggle ^= 1 ;

		unsigned uiBufAddr = (unsigned)(pDisk->pRawAlignedBuffer + (iIndex * EHCI_MAX_BYTES_PER_TD)) ;
		if(SetupBuffer(pTD, uiBufAddr, uiCurReadLen)	!= EHCIController_SUCCESS)
		{
			printf("\n EHCI Transfer Buffer setup failed") ;
			return false ;
		}

		if(iIndex > 0)
			_ppBulkReadTDs[ iIndex - 1 ]->uiNextpTDPointer = KERNEL_REAL_ADDRESS((unsigned)pTD) ;
	}

	_ppBulkReadTDs[ iIndex - 1 ]->uiNextpTDPointer = 1 ;

	EHCITransaction aTransaction(_pBulkInEndPt, _ppBulkReadTDs[ 0 ]);

	if(!aTransaction.PollWait())
	{
		printf("\n Bulk Read Transaction Failed: ") ;
		DisplayTransactionState(_pBulkInEndPt, _ppBulkReadTDs[0]) ;
		_controller.DisplayStats() ;
		return false ;
	}

	EHCIDataHandler_CleanAysncQueueHead(_pBulkInEndPt) ;

	memcpy(pDataBuf, pDisk->pRawAlignedBuffer, uiLen) ;

	_UpdateReadStat(uiLen, false) ;
	return true ;
}

bool EHCIDevice::BulkWrite(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
	if(uiLen == 0)
		return true ;

	unsigned uiMaxLen = pDisk->usOutMaxPacketSize * MAX_EHCI_TD_PER_BULK_RW ;
	if(uiLen > uiMaxLen)
	{
		printf("\n Max Bulk Transfer per Frame is %d bytes", uiMaxLen) ;
		return false ;
	}

	int iNoOfTDs = uiLen / EHCI_MAX_BYTES_PER_TD ;
	iNoOfTDs += ((uiLen % EHCI_MAX_BYTES_PER_TD) != 0) ;

	if(iNoOfTDs > MAX_EHCI_TD_PER_BULK_RW)
	{
		printf("\n No. of TDs per Bulk Read/Wrtie exceeded limit %d !!", MAX_EHCI_TD_PER_BULK_RW) ;
		return false ;
	}

  const unsigned uiMaxPacketSize = pDisk->usOutMaxPacketSize;

	if(_bFirstBulkWrite)
	{
		_bFirstBulkWrite = false ;

		for(int i = 0; i < MAX_EHCI_TD_PER_BULK_RW; i++)
			_ppBulkWriteTDs[ i ] = EHCIDataHandler_CreateAsyncQTransferDesc() ; 

		_pBulkOutEndPt = _controller.CreateDeviceQueueHead(uiMaxPacketSize, pDisk->uiEndPointOut, devAddr) ;
	}
	else
	{
		int i ;
		for(i = 0; i < MAX_EHCI_TD_PER_BULK_RW; i++)
			memset((void*)(_ppBulkWriteTDs[i]), 0, sizeof(EHCIQTransferDesc)) ;

		EHCIDataHandler_CleanAysncQueueHead(_pBulkOutEndPt) ;
	}

	int iIndex ;

	memcpy(pDisk->pRawAlignedBuffer, pDataBuf, uiLen) ;

	unsigned uiYetToWrite = uiLen ;
	unsigned uiCurWriteLen ;

	for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
	{
		uiCurWriteLen = (uiYetToWrite > EHCI_MAX_BYTES_PER_TD) ? EHCI_MAX_BYTES_PER_TD : uiYetToWrite ;
		uiYetToWrite -= uiCurWriteLen ;

		EHCIQTransferDesc* pTD = _ppBulkWriteTDs[ iIndex ] ;

		pTD->uiAltpTDPointer = 1 ;
		pTD->uipTDToken = (pDisk->bEndPointOutToggle << 31) | (uiCurWriteLen << 16) | (3 << 10) | (1 << 7) ;
		unsigned toggle = uiCurWriteLen / uiMaxPacketSize ;
		if(uiCurWriteLen % uiMaxPacketSize)
			toggle++ ;
		if(toggle % 2)
			pDisk->bEndPointOutToggle ^= 1 ;

		unsigned uiBufAddr = (unsigned)(pDisk->pRawAlignedBuffer + (iIndex * EHCI_MAX_BYTES_PER_TD)) ;
		if(SetupBuffer(pTD, uiBufAddr, uiCurWriteLen) != EHCIController_SUCCESS)
		{
			printf("\n EHCI Transfer Buffer setup failed") ;
			return false ;
		}

		if(iIndex > 0)
			_ppBulkWriteTDs[ iIndex - 1 ]->uiNextpTDPointer = KERNEL_REAL_ADDRESS((unsigned)pTD) ;
	}

	_ppBulkWriteTDs[ iIndex - 1 ]->uiNextpTDPointer = 1 ;
  
	EHCITransaction aTransaction(_pBulkOutEndPt, _ppBulkWriteTDs[ 0 ]);

	if(!aTransaction.PollWait())
	{
		printf("\n Bulk Write Transaction Failed: ") ;
		DisplayTransactionState(_pBulkOutEndPt, _ppBulkWriteTDs[0]) ;
		_controller.DisplayStats() ;
		return false ;
	}
  
	EHCIDataHandler_CleanAysncQueueHead(_pBulkOutEndPt) ;
  return true;
}

bool EHCIDevice::SetAddress()
{
	devAddr = USBController_GetNextDevNum();
	if(devAddr <= 0)
    throw upan::exception(XLOC, "Invalid Next Dev Addr: %d", devAddr);

	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pTDStart) ;
		return EHCIController_FAILURE ;
	}

	USBDevRequest* pDevRequest = (USBDevRequest*)(KERNEL_VIRTUAL_ADDRESS(pTDStart->uiBufferPointer[ 0 ])) ;
	pDevRequest->bRequestType = 0x00 ;
	pDevRequest->bRequest = 5 ;
	pDevRequest->usWValue = (devAddr & 0xFF) ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 0 ;
	
	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = KERNEL_REAL_ADDRESS(pTD1) ;
	pTD1->uiNextpTDPointer = 1 ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (3 << 10) | (1 << 8) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);
	
	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false;
	}

  aTransaction.Clear();

	_pControlQH->uiEndPointCap_Part1 |= (devAddr & 0x7F) ;

	if(_controller.AsyncDoorBell() != EHCIController_SUCCESS)
	{
		printf("\n Async Door Bell failed while Setting Device Address") ;
		_controller.DisplayStats() ;
    return false;
	}
  return true;
}

bool EHCIDevice::GetDeviceDescriptor(USBStandardDeviceDesc* pDevDesc)
{
	USBDataHandler_InitDevDesc(pDevDesc) ;

  if(!GetDescriptor(0x100, 0, -1, pDevDesc))
    return false;

	byte len = pDevDesc->bLength ;

	if(len >= sizeof(USBStandardDeviceDesc))
		len = sizeof(USBStandardDeviceDesc) ;

  if(!GetDescriptor(0x100, 0, len, pDevDesc))
    return false;

  return true;
}

bool EHCIDevice::GetDescriptor(unsigned short usDescValue, unsigned short usIndex, int iLen, void* pDestDesc)
{
	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pTDStart) ;
		return false;
	}

	if(iLen < 0)
		iLen = DEF_DESC_LEN ;

	USBDevRequest* pDevRequest = (USBDevRequest*)(KERNEL_VIRTUAL_ADDRESS(pTDStart->uiBufferPointer[ 0 ])) ;
	pDevRequest->bRequestType = 0x80 ;
	pDevRequest->bRequest = 6 ;
	pDevRequest->usWValue = usDescValue ;
	pDevRequest->usWIndex = usIndex ;
	pDevRequest->usWLength = iLen ;

	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = KERNEL_REAL_ADDRESS(pTD1) ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (iLen << 16) | (3 << 10) | (1 << 8) | (1 << 7) ;
	
	if(SetupAllocBuffer(pTD1, iLen) != EHCIController_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pTDStart) ;
		DMM_DeAllocateForKernel((unsigned)pTD1) ;
		return false;
	}

	// It is important to store the data buffer address now for later read as the data buffer address
	// in TD will be incremented by Host Controller with the number of bytes read
	unsigned uiDataBuffer = KERNEL_VIRTUAL_ADDRESS(pTD1->uiBufferPointer[ 0 ]) ;

	EHCIQTransferDesc* pTD2 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTD1->uiNextpTDPointer = KERNEL_REAL_ADDRESS(pTD2) ;
	pTD2->uiNextpTDPointer = 1 ;
	pTD2->uiAltpTDPointer = 1 ;
	pTD2->uipTDToken = (3 << 10) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);

	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false;
	}
	
	MemUtil_CopyMemory(MemUtil_GetDS(), uiDataBuffer, MemUtil_GetDS(), (unsigned)pDestDesc, iLen) ;

  aTransaction.Clear();

	return true;
}

bool EHCIDevice::GetConfigValue(char& bConfigValue)
{
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pTDStart) ;
		return false;
	}

	USBDevRequest* pDevRequest = (USBDevRequest*)(KERNEL_VIRTUAL_ADDRESS(pTDStart->uiBufferPointer[ 0 ])) ;
	pDevRequest->bRequestType = 0x80 ;
	pDevRequest->bRequest = 8 ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 1 ;
	
	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = KERNEL_REAL_ADDRESS(pTD1) ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (1 << 16) | (3 << 10) | (1 << 8) | (1 << 7) ;
	
	if(SetupAllocBuffer(pTD1, 1) != EHCIController_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pTDStart) ;
		DMM_DeAllocateForKernel((unsigned)pTD1) ;
		return false;
	}

	EHCIQTransferDesc* pTD2 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTD1->uiNextpTDPointer = KERNEL_REAL_ADDRESS(pTD2) ;
	pTD2->uiNextpTDPointer = 1 ;
	pTD2->uiAltpTDPointer = 1 ;
	pTD2->uipTDToken = (3 << 10) | (1 << 7) ;

	unsigned uiDataBuffer = KERNEL_VIRTUAL_ADDRESS(pTD1->uiBufferPointer[ 0 ]) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);

	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false;
	}
	
	bConfigValue = *((char*)(uiDataBuffer)) ;
		
  aTransaction.Clear();

	return true;
}

byte EHCIDevice::SetupBuffer(EHCIQTransferDesc* pTD, unsigned uiAddress, unsigned uiSize)
{
	unsigned uiFirstPagePart = PAGE_SIZE - (uiAddress % PAGE_SIZE) ;

	pTD->uiBufferPointer[ 0 ] = KERNEL_REAL_ADDRESS(uiAddress) ;

	if(uiFirstPagePart >= uiSize)
		return EHCIController_SUCCESS ;

	pTD->uiBufferPointer[ 1 ] = pTD->uiBufferPointer[ 0 ] + uiFirstPagePart ;

	uiSize -= uiFirstPagePart ;

	unsigned i ;
	for(i = 2; uiSize > PAGE_SIZE && i < 5; i++, uiSize -= PAGE_SIZE)
	{
		pTD->uiBufferPointer[ i ] = pTD->uiBufferPointer[ i - 1 ] + PAGE_SIZE ;
	}

	if(uiSize > PAGE_SIZE)
	{
		printf("\n EHCI TD Buffer Allocation Math is incorrect in page boundary calculation !") ;
		DMM_DeAllocateForKernel(uiAddress) ;
		return EHCIController_FAILURE ;
	}

	return EHCIController_SUCCESS ;
}

byte EHCIDevice::SetupAllocBuffer(EHCIQTransferDesc* pTD, unsigned uiSize)
{
	static const unsigned MAX_LEN = PAGE_SIZE * 4 ;

	if(uiSize > MAX_LEN)
	{
		printf("\n EHCI QTD Buffer Size: %d greater than Limit (PAGE_SIZE * 4): %d", uiSize, MAX_LEN) ;
		return EHCIController_FAILURE ;
	}

	unsigned uiAddress = DMM_AllocateForKernel(uiSize) ;

	return SetupBuffer(pTD, uiAddress, uiSize) ;
}

void EHCIDevice::DisplayTransactionState(EHCIQueueHead* pQH, EHCIQTransferDesc* pTDStart)
{
	printf("\n QH Details:- ECap1: %x, ECap2: %x", pQH->uiEndPointCap_Part1, pQH->uiEndPointCap_Part2) ;
	printf("\n QTD Token: %x, Current: %x, Next: %x", pQH->uipQHToken, pQH->uiCurrrentpTDPointer, pQH->uiNextpTDPointer) ;
	printf("\n Alt_NAK: %x, BufferPtr0: %x", pQH->uiAltpTDPointer_NAKCnt, pQH->uiBufferPointer[ 0 ]) ;

	EHCIQTransferDesc* pTDCur = pTDStart ;
	unsigned i = 1 ;
	for(;(unsigned)pTDCur > 1;)
	{
		printf("\n TD %d Token: %x", i++, pTDCur->uipTDToken) ;
		pTDCur = (EHCIQTransferDesc*)KERNEL_VIRTUAL_ADDRESS(pTDCur->uiNextpTDPointer) ;
	}
}

