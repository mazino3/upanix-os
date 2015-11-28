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
#include <DMM.h>
#include <DSUtil.h>
#include <Display.h>
#include <KernelUtil.h>
#include <USBStructures.h>
#include <UHCIStructures.h>
#include <UHCIDataHandler.h>
#include <stdio.h>
#include <Atomic.h>

static DSUtil_Queue UHCIDataHandler_FrameQueue ;
static unsigned* UHCIDataHandler_pFrameList = NULL ;
static DSUtil_SLL* UHCIDataHandler_pLocalFrameList = NULL ;
__attribute__((unused)) static unsigned short UHCIDataHandler_lSupportedLandIds[] = { 0x409, 0 } ;

/***********************************************************************************************/

Mutex& UHCIDataHandler_GetFrameListMutex()
{
	static Mutex m ;
	return m ;
}

static void UHCIDataHandler_SetValue(unsigned* uiSrc, unsigned uiValue, byte bSet)
{
	if(bSet)
		*uiSrc |= uiValue ;
	else
		*uiSrc &= ~(uiValue) ;
}

static unsigned UHCIDataHandler_IsSet(unsigned uiValue, unsigned tag)
{
	return (uiValue & tag) ? true : false ;
}

static void UHCIDataHandler_SetQHLink(UHCIQueueHead* pQH, unsigned uiNextD, byte bIsHeadLink, byte bIsQueueHead)
{
	unsigned* pLink = (bIsHeadLink) ? &(pQH->uiHeadLinkPointer) : &(pQH->uiElementLinkPointer) ;

	if(uiNextD == NULL)
		*pLink = TD_LINK_TERMINATE ;
	else
		*pLink = (KERNEL_REAL_ADDRESS(uiNextD)) | ((bIsQueueHead) ? TD_LINK_QH_POINTER : 0) ;
}

static void UHCIDataHandler_SetTDLinkToQueueHead(UHCITransferDesc* pTD, unsigned uiNextQH)
{
	if(uiNextQH == NULL)
		pTD->uiLinkPointer = TD_LINK_TERMINATE ;
	else
		pTD->uiLinkPointer = KERNEL_REAL_ADDRESS(uiNextQH) | TD_LINK_QH_POINTER ;
}

static void UHCIDataHandler_SetTDLinkToTransferDesc(UHCITransferDesc* pTD, unsigned uiNextD, byte bHorizontal)
{
	if(uiNextD == NULL)
		pTD->uiLinkPointer = TD_LINK_TERMINATE ;
	else
	{
		pTD->uiLinkPointer = KERNEL_REAL_ADDRESS(uiNextD) ;

		if(!bHorizontal)
			pTD->uiLinkPointer |= TD_LINK_VERTICAL_FLAG ;
	}
}

static byte UHCIDataHandler_GetNextFrameToClean(unsigned* uiFrameNumber)
{
	byte bStatus = UHCIDataHandler_SUCCESS ;

	ResourceManager_Acquire(RESOURCE_UHCI_FRM_BUF, RESOURCE_ACQUIRE_BLOCK) ;

	if(DSUtil_ReadFromQueue(&UHCIDataHandler_FrameQueue, uiFrameNumber) == DSUtil_ERR_QUEUE_EMPTY)
		bStatus = UHCIDataHandler_FRMQ_EMPTY ;

	ResourceManager_Release(RESOURCE_UHCI_FRM_BUF) ;

	return bStatus ;
}

static void UHCIDataHandler_BuildFrameEntryForDeAlloc(unsigned uiFrameNumber, unsigned uiDescAddress, bool bCleanBuffer, bool bCleanDescs)
{
	if(uiDescAddress == NULL)
		return ;

	DSUtil_SLL* pSLL = &(UHCIDataHandler_pLocalFrameList[uiFrameNumber]) ;

	if(uiDescAddress & TD_LINK_QH_POINTER)
	{
		uiDescAddress = UHCI_DESC_ADDR(uiDescAddress) - GLOBAL_DATA_SEGMENT_BASE ;
		
		UHCIQueueHead* pQH = (UHCIQueueHead*)uiDescAddress ;
		unsigned uiElementLinkPointer = pQH->uiElementLinkPointer ;
		unsigned uiHeadLinkPointer = pQH->uiHeadLinkPointer ;
		
		if(!(uiElementLinkPointer & TD_LINK_TERMINATE))
		{
			UHCIDataHandler_BuildFrameEntryForDeAlloc(uiFrameNumber, uiElementLinkPointer, bCleanBuffer, bCleanDescs) ;
		}

		if(!(uiHeadLinkPointer & TD_LINK_TERMINATE))
		{
			UHCIDataHandler_BuildFrameEntryForDeAlloc(uiFrameNumber, uiHeadLinkPointer, bCleanBuffer, bCleanDescs) ;
		}

		DSUtil_InsertSLL(pSLL, uiDescAddress) ;
	}
	else
	{
		uiDescAddress = UHCI_DESC_ADDR(uiDescAddress) - GLOBAL_DATA_SEGMENT_BASE ;

		UHCITransferDesc* pTD = (UHCITransferDesc*)uiDescAddress ;
		unsigned uiLinkPointer = pTD->uiLinkPointer ;

		if(!(uiLinkPointer & TD_LINK_TERMINATE))
		{
			UHCIDataHandler_BuildFrameEntryForDeAlloc(uiFrameNumber, uiLinkPointer, bCleanBuffer, bCleanDescs) ;
		}

		if(bCleanBuffer)
		{
			unsigned uiBufferPointer = pTD->uiBufferPointer ;
			if(uiBufferPointer != NULL)
			{
				uiBufferPointer -= GLOBAL_DATA_SEGMENT_BASE ;

				if(uiBufferPointer != NULL)
				{
					DSUtil_InsertSLL(pSLL, uiBufferPointer) ;
				}
			}
		}

		if(bCleanDescs)
			DSUtil_InsertSLL(pSLL, uiDescAddress) ;
	}
}

void UHCIDataHandler_ReleaseFrameResource(unsigned uiFrameNumber)
{
	DSUtil_SLL* pSLL = &(UHCIDataHandler_pLocalFrameList[uiFrameNumber]) ;

	DSUtil_SLLNode *pCur, *pTemp ;
	
	for(pCur = pSLL->pFirst; pCur != NULL;)
	{
		DMM_DeAllocateForKernel(pCur->val) ;

		pTemp = pCur->pNext ;
		DMM_DeAllocateForKernel((unsigned)pCur) ;
		pCur = pTemp ;
	}

	DSUtil_InitializeSLL(pSLL) ;
}

static void UHCIDataHandler_FrameCleaner()
{
	unsigned uiFrameNumber ;
	while(true)
	{
		if(UHCIDataHandler_GetNextFrameToClean(&uiFrameNumber) != UHCIDataHandler_SUCCESS)
			break ;

		UHCIDataHandler_pFrameList[ uiFrameNumber ] |= TD_LINK_TERMINATE ;

		UHCIDataHandler_ReleaseFrameResource(uiFrameNumber) ;

		UHCIDataHandler_pFrameList[ uiFrameNumber ] = 1 ;
	}
}

/***********************************************************************************************/

UHCITransferDesc* UHCIDataHandler_CreateTransferDesc()
{
	UHCITransferDesc *pTD = (UHCITransferDesc*)DMM_AllocateAlignForKernel( sizeof(UHCITransferDesc), 16 );
	pTD->uiLinkPointer = TD_LINK_TERMINATE;
	pTD->uiControlnStatus = 0;
	pTD->uiToken = 0;
	pTD->uiBufferPointer = 0;

	return pTD ;
}

void UHCIDataHandler_ClearTransferDesc(UHCITransferDesc* pTD)
{
	pTD->uiLinkPointer = TD_LINK_TERMINATE;
	pTD->uiControlnStatus = 0;
	pTD->uiToken = 0;
	pTD->uiBufferPointer = 0;
}

UHCIQueueHead* UHCIDataHandler_CreateQueueHead()
{
	UHCIQueueHead *pQH = (UHCIQueueHead*)DMM_AllocateAlignForKernel( sizeof(UHCIQueueHead), 16 );
	pQH->uiHeadLinkPointer = TD_LINK_TERMINATE ;
	pQH->uiElementLinkPointer = TD_LINK_TERMINATE;
	return pQH ;
}

unsigned UHCIDataHandler_CreateFrameList()
{
	unsigned uiFreePageNo ;

	if(MemManager::Instance().AllocatePhysicalPage(&uiFreePageNo) != Success)
		return NULL ;

	UHCIDataHandler_pFrameList = (unsigned*)(uiFreePageNo * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE) ;

	unsigned i ;
	for(i = 0; i < 1024; i++)
		UHCIDataHandler_pFrameList[i] = 1;

	UHCIDataHandler_pLocalFrameList = (DSUtil_SLL*)DMM_AllocateForKernel(sizeof(DSUtil_SLL) * 1024) ;

	for(i = 0; i < 1024; i++)
		DSUtil_InitializeSLL(&(UHCIDataHandler_pLocalFrameList[i])) ;

	return (unsigned)UHCIDataHandler_pFrameList ;
}

void UHCIDataHandler_SetTDToken(UHCITransferDesc* pTD, unsigned short usMaxLen, byte bSetToggle, 
								byte bEndPt, byte bDeviceAddr, byte bPID)
{
	pTD->uiToken = ((usMaxLen & TD_MAXLEN_BLK) << TD_MAXLEN_POS)
						| ((bSetToggle) ? TD_DATA_TOGGLE_SET : 0)
						| ((bEndPt & TD_ENDPT_MASK) << TD_ENDPT_POS)
						| ((bDeviceAddr & TD_DEV_ADDR_MASK) << TD_DEV_ADDR_POS)
						| (bPID & TD_PID_MASK) ;
}

void UHCIDataHandler_SetTDControlFull(UHCITransferDesc* pTD, unsigned uiControlnStatus)
{
	pTD->uiControlnStatus = uiControlnStatus ;
}

void UHCIDataHandler_SetTDControl(UHCITransferDesc* pTD, byte bShortPacket, byte bErrLevel, byte bLowSpeed, 
								  byte bIOS, byte bIOC, byte bActive, byte bResetActLen)
{
	unsigned uiControlnStatus = 0;
	
	UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_CONTROL_SPD, bShortPacket) ;
	UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_CONTROL_LSD, bLowSpeed) ;
	UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_CONTROL_IOS, bIOS) ;
	UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_CONTROL_IOC, bIOC) ;
	UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_STATUS_ACTIVE, bActive) ;
	UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_RESET_LEN, bResetActLen) ;

	pTD->uiControlnStatus = uiControlnStatus ;
}

void UHCIDataHandler_SetQHAttribute(UHCIQueueHead* pQH, UHCIDescAttrType bAttrType, unsigned uiValue)
{
	byte bIsHeadLink = false ;
	byte bIsQueueHead = false ;

	// QH Link Pointer
	switch((unsigned)bAttrType)
	{
		case UHCI_ATTR_QH_HEAD_LINK_TERMINATE:
			pQH->uiHeadLinkPointer = TD_LINK_TERMINATE ;
			return ;

		case UHCI_ATTR_QH_ELEM_LINK_TERMINATE:
			pQH->uiElementLinkPointer = TD_LINK_TERMINATE ;
			return ;

		case UHCI_ATTR_QH_TO_TD_HEAD_LINK:
			bIsHeadLink = true ;
			break ;

		case UHCI_ATTR_QH_TO_QH_HEAD_LINK:
			bIsHeadLink = true ;
			bIsQueueHead = true ;
			break ;

		case UHCI_ATTR_QH_TO_QH_ELEM_LINK:
			bIsQueueHead = true ;
			break ;
	}

	UHCIDataHandler_SetQHLink(pQH, uiValue, bIsHeadLink, bIsQueueHead) ;
}

void UHCIDataHandler_SetTDAttribute(UHCITransferDesc* pTD, UHCIDescAttrType bAttrType, unsigned uiValue)
{
	switch((unsigned)bAttrType)
	{
		// TD Link Pointer
		case UHCI_ATTR_TD_VERTICAL_LINK:
			UHCIDataHandler_SetTDLinkToTransferDesc(pTD, uiValue, false) ;
			break ;
			
		case UHCI_ATTR_TD_HORIZONTAL_LINK:
			UHCIDataHandler_SetTDLinkToTransferDesc(pTD, uiValue, true) ;
			break ;

		case UHCI_ATTR_TD_QH_LINK:
			UHCIDataHandler_SetTDLinkToQueueHead(pTD, uiValue) ;
			break ;
			
		case UHCI_ATTR_TD_TERMINATE:
			pTD->uiLinkPointer = TD_LINK_TERMINATE ;
			break ;
		
		// TD Control And Status
		case UHCI_ATTR_TD_CONTROL_SPD:
			UHCIDataHandler_SetValue(&(pTD->uiControlnStatus), TD_CONTROL_SPD, uiValue) ;
			break ;

		case UHCI_ATTR_TD_CONTROL_ERR_LEVEL:
			pTD->uiControlnStatus |= ((uiValue & 0x3) << TD_CONTROL_ERR_POS) ;
			break ;
			
		case UHCI_ATTR_TD_CONTROL_LSD:
			UHCIDataHandler_SetValue(&(pTD->uiControlnStatus), TD_CONTROL_LSD, uiValue) ;
			break ;

		case UHCI_ATTR_TD_CONTROL_IOS:
			UHCIDataHandler_SetValue(&(pTD->uiControlnStatus), TD_CONTROL_IOS, uiValue) ;
			break ;

		case UHCI_ATTR_TD_CONTROL_IOC:
			UHCIDataHandler_SetValue(&(pTD->uiControlnStatus), TD_CONTROL_IOC, uiValue) ;
			break ;

		case UHCI_ATTR_TD_STATUS_ACTIVE:
			UHCIDataHandler_SetValue(&(pTD->uiControlnStatus), TD_STATUS_ACTIVE, uiValue) ;
			break ;
		
		case UHCI_ATTR_TD_RESET_LEN:
			if(uiValue) pTD->uiControlnStatus &= ~(TD_ACT_LEN_MASK) ;
			break ;

		// TD Token - Address
		case UHCI_ATTR_TD_MAXLEN:
			pTD->uiToken |= ((uiValue & TD_MAXLEN_BLK) << TD_MAXLEN_POS) ;
			break ;

		case UHCI_ATTR_TD_DATA_TOGGLE:
			UHCIDataHandler_SetValue(&(pTD->uiToken), TD_DATA_TOGGLE_SET, uiValue) ;
			break ;

		case UHCI_ATTR_TD_ENDPT_ADDR:
			pTD->uiToken |= ((uiValue & TD_ENDPT_MASK) << TD_ENDPT_POS) ;
			break ;

		case UHCI_ATTR_TD_DEVICE_ADDR:
			pTD->uiToken |= ((uiValue & TD_DEV_ADDR_MASK) << TD_DEV_ADDR_POS) ;
			break ;

		case UHCI_ATTR_TD_PID:
			pTD->uiToken |= (uiValue & TD_PID_MASK) ;
			break ;

		case UHCI_ATTR_CTLSTAT_FULL:
			pTD->uiControlnStatus = uiValue ;
			break ;

		case UHCI_ATTR_BUF_PTR:
			pTD->uiBufferPointer = KERNEL_REAL_ADDRESS(uiValue) ;
			break ;
	}
}

byte UHCIDataHandler_IsQHHeadLinkTerminated(const UHCIQueueHead* pQH)
{
	return (pQH->uiHeadLinkPointer & TD_LINK_TERMINATE) ;
}

byte UHCIDataHandler_IsQHElemLinkTerminated(const UHCIQueueHead* pQH)
{
	return (pQH->uiElementLinkPointer & TD_LINK_TERMINATE) ;
}

void UHCIDataHandler_GetQHHeadLink(const UHCIQueueHead* pQH, UHCIDescAttrType* bAttrType, unsigned* uiValue)
{
	unsigned uiHeadLinkPointer = pQH->uiHeadLinkPointer ;

	if(uiHeadLinkPointer & TD_LINK_QH_POINTER)
		*bAttrType = UHCI_ATTR_QH_TO_QH_HEAD_LINK ;
	else
		*bAttrType = UHCI_ATTR_QH_TO_TD_HEAD_LINK ;

	*uiValue = UHCI_DESC_ADDR(uiHeadLinkPointer) ;
	return ;
}

void UHCIDataHandler_GetQHElemLink(const UHCIQueueHead* pQH, UHCIDescAttrType* bAttrType, unsigned* uiValue)
{
	unsigned uiElementLinkPointer = pQH->uiElementLinkPointer ;

	if(uiElementLinkPointer & TD_LINK_QH_POINTER)
		*bAttrType = UHCI_ATTR_QH_TO_QH_ELEM_LINK ;
	else
		*bAttrType = UHCI_ATTR_QH_TO_TD_ELEM_LINK ;

	*uiValue = UHCI_DESC_ADDR(uiElementLinkPointer) ;
	return ;
}

byte UHCIDataHandler_IsTDLinkTerminated(const UHCITransferDesc* pTD)
{
	return (pTD->uiLinkPointer & TD_LINK_TERMINATE) ;
}

void UHCIDataHandler_GetTDLink(const UHCITransferDesc* pTD, UHCIDescAttrType* bAttrType, unsigned* uiValue)
{
	unsigned uiLinkPointer = pTD->uiLinkPointer ;

	if(uiLinkPointer & TD_LINK_QH_POINTER)
		*bAttrType = UHCI_ATTR_TD_QH_LINK ;
	else if(uiLinkPointer & TD_LINK_VERTICAL_FLAG)
		*bAttrType = UHCI_ATTR_TD_VERTICAL_LINK ;
	else 
		*bAttrType = UHCI_ATTR_TD_HORIZONTAL_LINK ;

	*uiValue = UHCI_DESC_ADDR(uiLinkPointer) ;
}

unsigned UHCIDataHandler_GetTDAttribute(UHCITransferDesc* pTD, UHCIDescAttrType bAttrType)
{
	switch((unsigned)bAttrType)
	{
		// TD Control And Status
		case UHCI_ATTR_TD_CONTROL_SPD:
			return UHCIDataHandler_IsSet(pTD->uiControlnStatus, TD_CONTROL_SPD) ;

		case UHCI_ATTR_TD_CONTROL_ERR_LEVEL:
			return ((pTD->uiControlnStatus >> TD_CONTROL_ERR_POS) & 0x3) ;
			
		case UHCI_ATTR_TD_CONTROL_LSD:
			return UHCIDataHandler_IsSet(pTD->uiControlnStatus, TD_CONTROL_LSD) ;

		case UHCI_ATTR_TD_CONTROL_IOS:
			return UHCIDataHandler_IsSet(pTD->uiControlnStatus, TD_CONTROL_IOS) ;

		case UHCI_ATTR_TD_CONTROL_IOC:
			return UHCIDataHandler_IsSet(pTD->uiControlnStatus, TD_CONTROL_IOC) ;

		case UHCI_ATTR_TD_STATUS_ACTIVE:
			return UHCIDataHandler_IsSet(pTD->uiControlnStatus, TD_STATUS_ACTIVE) ;
		
		case UHCI_ATTR_TD_ACT_LEN:
			return (pTD->uiControlnStatus & TD_ACT_LEN_MASK) ;

		// TD Token - Address
		case UHCI_ATTR_TD_MAXLEN:
			return ((pTD->uiToken >> TD_MAXLEN_POS) & TD_MAXLEN_BLK) ;

		case UHCI_ATTR_TD_DATA_TOGGLE:
			return UHCIDataHandler_IsSet(pTD->uiToken, TD_DATA_TOGGLE_SET) ;

		case UHCI_ATTR_TD_ENDPT_ADDR:
			return ((pTD->uiToken >> TD_ENDPT_POS) & TD_ENDPT_MASK) ;

		case UHCI_ATTR_TD_DEVICE_ADDR:
			return ((pTD->uiToken >> TD_DEV_ADDR_POS) & TD_DEV_ADDR_MASK) ;

		case UHCI_ATTR_TD_PID:
			return (pTD->uiToken & TD_PID_MASK) ;

		case UHCI_ATTR_CTLSTAT_FULL:
			return pTD->uiControlnStatus ;

		case UHCI_ATTR_BUF_PTR:
			return pTD->uiBufferPointer - GLOBAL_DATA_SEGMENT_BASE ;
	}

	printf("\n Critical Error! Invalid UHCI Desc Attr: %d", (int)bAttrType) ;
	return 0 ;
}

byte UHCIDataHandler_StartFrameCleaner()
{
	unsigned uiFreePageNo ;
	RETURN_X_IF_NOT(MemManager::Instance().AllocatePhysicalPage(&uiFreePageNo), Success, UHCIDataHandler_FAILURE) ;
	
	unsigned uiFrameQBuf = (uiFreePageNo * PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE ;

	DSUtil_InitializeQueue(&UHCIDataHandler_FrameQueue, uiFrameQBuf, PAGE_TABLE_ENTRIES) ;

	KernelUtil::ScheduleTimedTask("UHCIFrmCleaner", 100, (unsigned)UHCIDataHandler_FrameCleaner) ;

	return UHCIDataHandler_SUCCESS ;
}

byte UHCIDataHandler_CleanFrame(unsigned uiFrameNumber)
{
	byte bStatus = UHCIDataHandler_SUCCESS ;

	UHCIDataHandler_GetFrameListMutex().Lock() ;
	// ResourceManager_Acquire(RESOURCE_UHCI_FRM_BUF, RESOURCE_ACQUIRE_BLOCK) ;

	if(DSUtil_WriteToQueue(&UHCIDataHandler_FrameQueue, uiFrameNumber) == DSUtil_ERR_QUEUE_FULL)
		bStatus = UHCIDataHandler_FRMQ_FULL ;
	
	// ResourceManager_Release(RESOURCE_UHCI_FRM_BUF) ;
	UHCIDataHandler_GetFrameListMutex().UnLock() ;

	return bStatus ;
}

unsigned UHCIDataHandler_GetFrameListAddress()
{
	return (unsigned)UHCIDataHandler_pFrameList ;
}

unsigned UHCIDataHandler_GetFrameListEntry(unsigned uiFrameNumber)
{
	return UHCIDataHandler_pFrameList[ uiFrameNumber ] ;
}

void UHCIDataHandler_SetFrameListEntry(unsigned uiFrameNumber, unsigned uiValue, bool bCleanBuffer, bool bCleanDescs)
{
	UHCIDataHandler_BuildFrameEntryForDeAlloc(uiFrameNumber, uiValue, bCleanBuffer, bCleanDescs) ;
	UHCIDataHandler_pFrameList[ uiFrameNumber ] = uiValue ;
}

byte UHCIDataHandler_GetNextFreeFrame(unsigned* uiFreeFrameID)
{
	ResourceManager_Acquire(RESOURCE_UHCI_FRM_BUF, RESOURCE_ACQUIRE_BLOCK) ;

	int i ;
	for(i = 0; i < 1024; i++)
	{
		if(UHCIDataHandler_pFrameList[i] == 1)
		{
			UHCIDataHandler_pFrameList[i] = 0 ;
			*uiFreeFrameID = i ;
			ResourceManager_Release(RESOURCE_UHCI_FRM_BUF) ;
			return UHCIDataHandler_SUCCESS ;
		}	
	}
	
	ResourceManager_Release(RESOURCE_UHCI_FRM_BUF) ;

	return UHCIDataHandler_NO_FREE_FRM ;
}

