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
#include <UHCIStructures.h>
#include <UHCIManager.h>

UHCITransferDesc* UHCITransferDesc::Create()
{
  return new ((void*)DMM_AllocateAlignForKernel(sizeof(UHCITransferDesc), 16)) UHCITransferDesc();
}

UHCITransferDesc::UHCITransferDesc() : _uiLinkPointer(TD_LINK_TERMINATE),
  _uiControlnStatus(0), _uiToken(0), _uiBufferPointer(0)
{
}

void UHCITransferDesc::Clear()
{
  _uiLinkPointer = TD_LINK_TERMINATE;
  _uiControlnStatus = 0;
  _uiToken = 0;
  _uiBufferPointer = 0;
}

void UHCITransferDesc::SetTDLink(unsigned uiNextD, byte bHorizontal)
{
	if(uiNextD == NULL)
		_uiLinkPointer = TD_LINK_TERMINATE;
	else
	{
		_uiLinkPointer = KERNEL_REAL_ADDRESS(uiNextD);

		if(!bHorizontal)
			_uiLinkPointer |= TD_LINK_VERTICAL_FLAG;
	}
}

void UHCITransferDesc::SetTDLinkToQueueHead(unsigned uiNextQH)
{
	if(uiNextQH == NULL)
		_uiLinkPointer = TD_LINK_TERMINATE;
	else
		_uiLinkPointer = KERNEL_REAL_ADDRESS(uiNextQH) | TD_LINK_QH_POINTER;
}

void UHCITransferDesc::ControlnStatus(unsigned uiValue, bool bSet)
{
	if(bSet)
		_uiControlnStatus |= uiValue;
	else
		_uiControlnStatus &= ~(uiValue);
}

void UHCITransferDesc::Token(unsigned uiValue, bool bSet)
{
	if(bSet)
		_uiToken |= uiValue;
	else
		_uiToken &= ~(uiValue);
}

unsigned UHCITransferDesc::GetTDAttribute(UHCIDescAttrType bAttrType)
{
	switch(bAttrType)
	{
		// TD Control And Status
		case UHCI_ATTR_TD_CONTROL_SPD:
			return IsControlnStatusSet(TD_CONTROL_SPD) ;

		case UHCI_ATTR_TD_CONTROL_ERR_LEVEL:
			return ((_uiControlnStatus >> TD_CONTROL_ERR_POS) & 0x3) ;
			
		case UHCI_ATTR_TD_CONTROL_LSD:
			return IsControlnStatusSet(TD_CONTROL_LSD) ;

		case UHCI_ATTR_TD_CONTROL_IOS:
			return IsControlnStatusSet(TD_CONTROL_IOS) ;

		case UHCI_ATTR_TD_CONTROL_IOC:
			return IsControlnStatusSet(TD_CONTROL_IOC) ;

		case UHCI_ATTR_TD_STATUS_ACTIVE:
			return IsControlnStatusSet(TD_STATUS_ACTIVE) ;
		
		case UHCI_ATTR_TD_ACT_LEN:
			return (_uiControlnStatus & TD_ACT_LEN_MASK) ;

		// TD Token - Address
		case UHCI_ATTR_TD_MAXLEN:
			return ((_uiToken >> TD_MAXLEN_POS) & TD_MAXLEN_BLK) ;

		case UHCI_ATTR_TD_DATA_TOGGLE:
			return _uiToken & TD_DATA_TOGGLE_SET ? true : false;

		case UHCI_ATTR_TD_ENDPT_ADDR:
			return ((_uiToken >> TD_ENDPT_POS) & TD_ENDPT_MASK) ;

		case UHCI_ATTR_TD_DEVICE_ADDR:
			return ((_uiToken >> TD_DEV_ADDR_POS) & TD_DEV_ADDR_MASK) ;

		case UHCI_ATTR_TD_PID:
			return (_uiToken & TD_PID_MASK) ;

		case UHCI_ATTR_CTLSTAT_FULL:
			return _uiControlnStatus ;

		case UHCI_ATTR_BUF_PTR:
			return _uiBufferPointer - GLOBAL_DATA_SEGMENT_BASE ;

    default:  
	    printf("\n Critical Error! Invalid UHCI Desc Attr: %d", (int)bAttrType) ;
      return 0;
	}
}


void UHCITransferDesc::SetTDAttribute(UHCIDescAttrType bAttrType, unsigned uiValue)
{
	switch(bAttrType)
	{
		// TD Link Pointer
		case UHCI_ATTR_TD_VERTICAL_LINK:
			SetTDLink(uiValue, false);
			break;
			
		case UHCI_ATTR_TD_HORIZONTAL_LINK:
			SetTDLink(uiValue, true);
			break;

		case UHCI_ATTR_TD_QH_LINK:
			SetTDLinkToQueueHead(uiValue);
			break;
			
		case UHCI_ATTR_TD_TERMINATE:
			_uiLinkPointer = TD_LINK_TERMINATE;
			break;
		
		// TD Control And Status
		case UHCI_ATTR_TD_CONTROL_SPD:
			ControlnStatus(TD_CONTROL_SPD, uiValue != 0);
			break;

		case UHCI_ATTR_TD_CONTROL_ERR_LEVEL:
			_uiControlnStatus |= ((uiValue & 0x3) << TD_CONTROL_ERR_POS);
			break;
			
		case UHCI_ATTR_TD_CONTROL_LSD:
			ControlnStatus(TD_CONTROL_LSD, uiValue != 0);
			break;

		case UHCI_ATTR_TD_CONTROL_IOS:
			ControlnStatus(TD_CONTROL_IOS, uiValue != 0);
			break;

		case UHCI_ATTR_TD_CONTROL_IOC:
			ControlnStatus(TD_CONTROL_IOC, uiValue != 0);
			break;

		case UHCI_ATTR_TD_STATUS_ACTIVE:
			ControlnStatus(TD_STATUS_ACTIVE, uiValue != 0);
			break;
		
		case UHCI_ATTR_TD_RESET_LEN:
			if(uiValue) _uiControlnStatus &= ~(TD_ACT_LEN_MASK);
			break;

		// TD Token - Address
		case UHCI_ATTR_TD_MAXLEN:
			_uiToken |= ((uiValue & TD_MAXLEN_BLK) << TD_MAXLEN_POS);
			break;

		case UHCI_ATTR_TD_DATA_TOGGLE:
			Token(TD_DATA_TOGGLE_SET, uiValue != 0);
			break;

		case UHCI_ATTR_TD_ENDPT_ADDR:
			_uiToken |= ((uiValue & TD_ENDPT_MASK) << TD_ENDPT_POS);
			break;

		case UHCI_ATTR_TD_DEVICE_ADDR:
			_uiToken |= ((uiValue & TD_DEV_ADDR_MASK) << TD_DEV_ADDR_POS);
			break;

		case UHCI_ATTR_TD_PID:
			_uiToken |= (uiValue & TD_PID_MASK);
			break;

		case UHCI_ATTR_CTLSTAT_FULL:
			_uiControlnStatus = uiValue;
			break;

		case UHCI_ATTR_BUF_PTR:
			_uiBufferPointer = KERNEL_REAL_ADDRESS(uiValue);
			break;

    default:
      break;
	}
}

void UHCITransferDesc::SetTDControl(byte bShortPacket, byte bErrLevel, 
                                    byte bLowSpeed, byte bIOS, byte bIOC, 
                                    byte bActive, byte bResetActLen)
{
	SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, bShortPacket);
	SetTDAttribute(UHCI_ATTR_TD_CONTROL_LSD, bLowSpeed);
	SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOS, bIOS);
	SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, bIOC);
	SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, bActive);
	SetTDAttribute(UHCI_ATTR_TD_RESET_LEN, bResetActLen);

	_uiControlnStatus = 0;
}

void UHCITransferDesc::SetTDToken(unsigned short usMaxLen, byte bSetToggle, 
                                  byte bEndPt, byte bDeviceAddr, byte bPID)
{
	_uiToken = ((usMaxLen & TD_MAXLEN_BLK) << TD_MAXLEN_POS)
						| ((bSetToggle) ? TD_DATA_TOGGLE_SET : 0)
						| ((bEndPt & TD_ENDPT_MASK) << TD_ENDPT_POS)
						| ((bDeviceAddr & TD_DEV_ADDR_MASK) << TD_DEV_ADDR_POS)
						| (bPID & TD_PID_MASK) ;
}

bool UHCITransferDesc::IsTDLinkTerminated()
{
	return (_uiLinkPointer & TD_LINK_TERMINATE) ? true : false;
}

void UHCITransferDesc::GetTDLink(UHCIDescAttrType& bAttrType, unsigned& uiValue)
{
	if(_uiLinkPointer & TD_LINK_QH_POINTER)
		bAttrType = UHCI_ATTR_TD_QH_LINK ;
	else if(_uiLinkPointer & TD_LINK_VERTICAL_FLAG)
		bAttrType = UHCI_ATTR_TD_VERTICAL_LINK ;
	else 
		bAttrType = UHCI_ATTR_TD_HORIZONTAL_LINK ;

	uiValue = UHCI_DESC_ADDR(_uiLinkPointer) ;
}

UHCIQueueHead* UHCIQueueHead::Create()
{
  return new ((void*)DMM_AllocateAlignForKernel(sizeof(UHCIQueueHead), 16)) UHCIQueueHead();
}

UHCIQueueHead::UHCIQueueHead() : _uiHeadLinkPointer(TD_LINK_TERMINATE), _uiElementLinkPointer(TD_LINK_TERMINATE)
{
}

void UHCIQueueHead::SetQHAttribute(UHCIDescAttrType bAttrType, unsigned uiValue)
{
	// QH Link Pointer
	switch(bAttrType)
	{
		case UHCI_ATTR_QH_HEAD_LINK_TERMINATE:
			_uiHeadLinkPointer = TD_LINK_TERMINATE ;
			return ;

		case UHCI_ATTR_QH_ELEM_LINK_TERMINATE:
			_uiElementLinkPointer = TD_LINK_TERMINATE ;
			return ;

		case UHCI_ATTR_QH_TO_TD_HEAD_LINK:
      SetHeadLink(uiValue, false);
			break ;

		case UHCI_ATTR_QH_TO_QH_HEAD_LINK:
      SetHeadLink(uiValue, true);
			break ;

		case UHCI_ATTR_QH_TO_QH_ELEM_LINK:
      SetElementLink(uiValue);
			break ;

    default:
      break;
	}
}

bool UHCIQueueHead::IsQHHeadLinkTerminated()
{
	return (_uiHeadLinkPointer & TD_LINK_TERMINATE) ? true : false;
}

bool UHCIQueueHead::IsQHElemLinkTerminated()
{
	return (_uiElementLinkPointer & TD_LINK_TERMINATE) ? true : false;
}

void UHCIQueueHead::GetQHHeadLink(UHCIDescAttrType& bAttrType, unsigned& uiValue)
{
	if(_uiHeadLinkPointer & TD_LINK_QH_POINTER)
		bAttrType = UHCI_ATTR_QH_TO_QH_HEAD_LINK ;
	else
		bAttrType = UHCI_ATTR_QH_TO_TD_HEAD_LINK ;

	uiValue = UHCI_DESC_ADDR(_uiHeadLinkPointer) ;
}

void UHCIQueueHead::GetQHElemLink(UHCIDescAttrType& bAttrType, unsigned& uiValue)
{
	if(_uiElementLinkPointer & TD_LINK_QH_POINTER)
		bAttrType = UHCI_ATTR_QH_TO_QH_ELEM_LINK ;
	else
		bAttrType = UHCI_ATTR_QH_TO_TD_ELEM_LINK ;

	uiValue = UHCI_DESC_ADDR(_uiElementLinkPointer) ;
}

void UHCIQueueHead::SetElementLink(unsigned uiNextD)
{
	if(uiNextD == NULL)
		_uiElementLinkPointer = TD_LINK_TERMINATE ;
	else
		_uiElementLinkPointer = (KERNEL_REAL_ADDRESS(uiNextD)) | TD_LINK_QH_POINTER;
}

void UHCIQueueHead::SetHeadLink(unsigned uiNextD, bool isQueueHead)
{
	if(uiNextD == NULL)
		_uiHeadLinkPointer = TD_LINK_TERMINATE ;
	else
		_uiHeadLinkPointer = (KERNEL_REAL_ADDRESS(uiNextD)) | (isQueueHead ? TD_LINK_QH_POINTER : 0);
}

bool UHCIQueueHead::PollWait()
{
  return UHCIManager::Instance().PollWait(&_uiElementLinkPointer, 1);
}
