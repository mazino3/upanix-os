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
#ifndef _UHCI_STRUCTURES_H_
#define _UHCI_STRUCTURES_H_

#include <Global.h>
#include <USBConstants.h>

// UHCI IO Registers
// Command Register
#define USBCMD_REG		0
// Status Register
#define USBSTS_REG		2
// Interrupt Register
#define USBINTR_REG		4
// Frame Number Register
#define FRNUM_REG		6
// Frame List Base Address Register
#define FLBASEADD_REG	8
// Start of Frame Modify Register
#define SOF_REG			12
// Port Status and Control Register - Every Port is 2 byte and there can min of 2 and max of N ports
// So, this is the base of Port register and should be used as an array of 2 byte blocks
// The size of this array will be maximum ports available 
#define PORTSC_REG		16

// UHCI IO Register Control Bits

// USBCMD_REG
/** Run/Stop */
#define USBCMD_RS		0x0001
/** Host reset */
#define USBCMD_HCRESET	0x0002
/** Global reset */
#define USBCMD_GRESET	0x0004
/** Global Suspend Mode */
#define USBCMD_EGSM		0x0008
/** Force Global Resume */
#define USBCMD_FGR		0x0010
/** SW Debug mode */
#define USBCMD_SWDBG	0x0020
/** Config Flag (sw only) */
#define USBCMD_CF		0x0040
/** Max Packet (0 = 32, 1 = 64) */
#define USBCMD_MAXP		0x0080

// USBINTR_REG
// 3rd Bit - Short Packet Interrupt Enable
#define USBINTR_SHORT_PACKET	0x08
// 2nd Bit - Interrupt On Complete (IOC) Enable
#define USBINTR_IOC				0x04
// 1st Bit - Resume Interrupt Enable
#define USBINTR_RESUME			0x02
// 0th Bit - Timeout/CRC Interrupt Enable
#define USBINTR_TIMEOUT_CRC		0x01

// USB Mouse and Keyboard Legacy Support for 8042
#define USB_LEGSUP			0xC0
#define USB_LEGSUP_DEFAULT	0x2000

#define TOKEN_PID_SETUP	0x2D
#define TOKEN_PID_IN	0x69
#define TOKEN_PID_OUT	0xE1

// Transfer Desciptor - Tokens
// Link Pointer
#define TD_LINK_VERTICAL_FLAG	0x4
#define TD_LINK_QH_POINTER		0x2
#define TD_LINK_TERMINATE		0x1

// Control and Status
#define TD_CONTROL_SPD		0x20000000

	// Error Level to be set and get with POS only
#define TD_CONTROL_ERR_POS	27
#define TD_CONTROL_NERR		0x0
#define TD_CONTROL_ERR1		0x1
#define TD_CONTROL_ERR2		0x2
#define TD_CONTROL_ERR3		0x3

#define TD_CONTROL_LSD		0x04000000
#define TD_CONTROL_IOS		0x02000000
#define TD_CONTROL_IOC 		0x01000000

#define TD_STATUS_ACTIVE	0x00800000
#define TD_STATUS_STALLED	0x00400000
#define TD_STATUS_DBERR		0x00200000
#define TD_STATUS_BABBLE	0x00100000
#define TD_STATUS_NAK		0x00080000
#define TD_STATUS_CRC_TO	0x00040000
#define TD_STATUS_BSTUFF	0x00020000

#define TD_ACT_LEN_MASK		0x3FF

// Token
#define TD_MAXLEN_POS		21
#define TD_MAXLEN_BLK		0x7FF
#define TD_DATA_TOGGLE_SET	0x00080000
#define TD_DATA_TOGGLE_POS	19
#define TD_ENDPT_POS		15
#define TD_ENDPT_MASK		0xF
#define TD_DEV_ADDR_POS		8
#define TD_DEV_ADDR_MASK	0x7F
#define TD_PID_MASK			0xFF

#define UHCI_DESC_ADDR(addr) ( addr & ~(0xF) )

enum UHCIDescAttrType
{
	// QH Link Pointer
	UHCI_ATTR_QH_TO_TD_HEAD_LINK,
	UHCI_ATTR_QH_TO_QH_HEAD_LINK,
	UHCI_ATTR_QH_TO_TD_ELEM_LINK,
	UHCI_ATTR_QH_TO_QH_ELEM_LINK,
	UHCI_ATTR_QH_HEAD_LINK_TERMINATE,
	UHCI_ATTR_QH_ELEM_LINK_TERMINATE,

	// TD Link Pointer
	UHCI_ATTR_TD_VERTICAL_LINK,
	UHCI_ATTR_TD_HORIZONTAL_LINK,
	UHCI_ATTR_TD_QH_LINK,
	UHCI_ATTR_TD_TERMINATE,

	// TD Control And Status
	UHCI_ATTR_TD_CONTROL_SPD,
	UHCI_ATTR_TD_CONTROL_ERR_LEVEL,
	UHCI_ATTR_TD_CONTROL_LSD,
	UHCI_ATTR_TD_CONTROL_IOS,
	UHCI_ATTR_TD_CONTROL_IOC,
	UHCI_ATTR_TD_STATUS_ACTIVE,
	UHCI_ATTR_TD_RESET_LEN,
	UHCI_ATTR_TD_ACT_LEN,

	// TD Token - Address
	UHCI_ATTR_TD_MAXLEN,
	UHCI_ATTR_TD_DATA_TOGGLE,
	UHCI_ATTR_TD_ENDPT_ADDR,
	UHCI_ATTR_TD_DEVICE_ADDR,
	UHCI_ATTR_TD_PID,

	UHCI_ATTR_CTLSTAT_FULL,
	UHCI_ATTR_BUF_PTR,

};

class UHCITransferDesc
{
  private:
    UHCITransferDesc();
  public:
    static UHCITransferDesc* Create();
    void Clear();
    void SetTDLink(unsigned uiNextD, byte bHorizontal);
    void SetTDLinkToQueueHead(unsigned uiNextQH);
    void ControlnStatus(unsigned uiValue) { _uiControlnStatus = uiValue; }
    void ControlnStatus(unsigned uiValue, bool bSet);
    void Token(unsigned uiValue, bool bSet);
    void SetTDAttribute(UHCIDescAttrType bAttrType, unsigned uiValue);
    unsigned GetTDAttribute(UHCIDescAttrType bAttrType);
    void SetTDControl(byte bShortPacket, byte bErrLevel, 
                      byte bLowSpeed, byte bIOS, byte bIOC, 
                      byte bActive, byte bResetActLen);
    void SetTDToken(unsigned short usMaxLen, byte bSetToggle, 
                    byte bEndPt, byte bDeviceAddr, byte bPID);
    bool IsTDLinkTerminated();
    void GetTDLink(UHCIDescAttrType& bAttrType, unsigned& uiValue);
    unsigned LinkPointer() const { return _uiLinkPointer; }
    unsigned BufferPointer() const { return _uiBufferPointer; }
    unsigned ControlnStatus() const { return _uiControlnStatus; }
    unsigned Token() const { return _uiToken; }
  private:
    bool IsControlnStatusSet(unsigned tag) 
    {
    	return (_uiControlnStatus & tag) ? true : false;
    }

    unsigned _uiLinkPointer;
    unsigned _uiControlnStatus;
    unsigned _uiToken;
    unsigned _uiBufferPointer;
} PACKED;

class UHCIQueueHead
{
  private:
    UHCIQueueHead();
  public:
    static UHCIQueueHead* Create();
    bool PollWait();
    void SetQHAttribute(UHCIDescAttrType bAttrType, unsigned uiValue);
    bool IsQHHeadLinkTerminated();
    bool IsQHElemLinkTerminated();
    void GetQHHeadLink(UHCIDescAttrType& bAttrType, unsigned& uiValue);
    void GetQHElemLink(UHCIDescAttrType& bAttrType, unsigned& uiValue);
    void GetTDLink(UHCIDescAttrType& bAttrType, unsigned& uiValue);
    unsigned HeadLinkPointer() const { return _uiHeadLinkPointer; }
    unsigned ElementLinkPointer() const { return _uiElementLinkPointer; }
  private:
    void SetElementLink(unsigned uiNextD);
    void SetHeadLink(unsigned uiNextD, bool isQueueHead);

    unsigned _uiHeadLinkPointer;
    unsigned _uiElementLinkPointer;
} PACKED;

#endif
