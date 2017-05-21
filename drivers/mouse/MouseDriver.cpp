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
#include <Display.h>
#include <AsmUtil.h>
#include <PIC.h>
#include <PortCom.h>
#include <BuiltInKeyboardDriver.h>
#include <ProcessManager.h>
#include <MouseDriver.h>
#include <stdio.h>

MouseDriver::MouseDriver()
{
	m_bProcessToggle = true ;
}

void MouseDriver::Initialize()
{
	IrqManager::Instance().DisableIRQ(StdIRQ::Instance().MOUSE_IRQ) ;
	IrqManager::Instance().RegisterIRQ(StdIRQ::Instance().MOUSE_IRQ, (unsigned)&MouseDriver::Handler) ;

	bool bStatus = true ;
	do
	{
		if(!SendCommand1(0x20))
		{
			bStatus = false ;
			printf("\n Failed to send command 0x20") ;
			break ;
		}

		byte status ;
    BuiltInKeyboardDriver::Instance().WaitForRead() ;
		status = PortCom_ReceiveByte(KB_DATA_PORT) ;

		status |= 0x2 ;
		status &= ~(0x20) ;

		if(!SendCommand1(0x60))
		{
			bStatus = false ;
			printf("\n Failed to send command 0x60") ;
			break ;
		}

    BuiltInKeyboardDriver::Instance().WaitForWrite() ;
		PortCom_SendByte(KB_DATA_PORT, status) ;

		WaitForAck() ;
	} while(false) ;

	byte data ;
	if(SendCommand2(0xF4))
		if(SendCommand2(0xF2))
			if(ReceiveData(data))
				printf("\n Mouse ID: %u ", data) ;

//	if(SendCommand2(0xF3))
//	if(SendData(200))
//	if(SendCommand2(0xF3))
//	if(SendData(100))
//	if(SendCommand2(0xF3))
//	if(SendData(80))
//	if(SendCommand2(0xF2))
//	if(ReceiveData(data))
//		printf("\n Mouse ID: %u ", data) ;
		
	IrqManager::Instance().EnableIRQ(StdIRQ::Instance().MOUSE_IRQ) ;
	KC::MDisplay().LoadMessage("Mouse Initialization", bStatus ? Success : Failure);
}

void MouseDriver::Handler()
{
	unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	AsmUtil_SET_KERNEL_DATA_SEGMENTS

	unsigned data ;
	byte* pData = (byte*)(&data) ;
	if(KC::MMouseDriver().ReceiveIRQData(pData[0]))
	if(KC::MMouseDriver().ReceiveIRQData(pData[1]))
	if(KC::MMouseDriver().ReceiveIRQData(pData[2]))
	{
		KC::MMouseDriver().Process(data) ;
	}
	IrqManager::Instance().SendEOI(StdIRQ::Instance().MOUSE_IRQ);

	AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
	AsmUtil_RESTORE_GPR(GPRStack) ;

	__asm__ __volatile__("LEAVE") ;
	__asm__ __volatile__("IRET") ;
}

bool MouseDriver::SendCommand1(byte command)
{
  if(!BuiltInKeyboardDriver::Instance().WaitForWrite())
		return false ;

	PortCom_SendByte(KB_STAT_PORT, command) ;
	return true ;
}

bool MouseDriver::SendCommand2(byte command)
{
  if(!BuiltInKeyboardDriver::Instance().WaitForWrite())
		return false ;
	PortCom_SendByte(KB_STAT_PORT, 0xD4) ;

  if(!BuiltInKeyboardDriver::Instance().WaitForWrite())
		return false ;
	PortCom_SendByte(KB_DATA_PORT, command) ;

	return WaitForAck() ;
}

bool MouseDriver::WaitForAck()
{
	byte data ;
	if(ReceiveData(data))
		if(data == 0xFA)
			return true ;
	return false ;
}

bool MouseDriver::ReceiveData(byte& data)
{
  if(!BuiltInKeyboardDriver::Instance().WaitForRead())
		return false ;

	data = PortCom_ReceiveByte(KB_DATA_PORT) ;
	return true ;
}

bool MouseDriver::ReceiveIRQData(byte& data)
{
  if(!BuiltInKeyboardDriver::Instance().WaitForRead())
		return false ;

	if(!(PortCom_ReceiveByte(KB_STAT_PORT) & 0x20))
		return false ;

	data = PortCom_ReceiveByte(KB_DATA_PORT) ;
	return true ;
}


bool MouseDriver::SendData(byte data)
{
  if(!BuiltInKeyboardDriver::Instance().WaitForWrite())
		return false ;

	PortCom_SendByte(KB_DATA_PORT, data) ;
	return true ;
}

void MouseDriver::Process(unsigned data)
{
	if(!m_bProcessToggle)
	{
		m_bProcessToggle = true ;
		return ;
	}
	m_bProcessToggle = false ;

	byte b1 ;
	char xMov, yMov ;
	byte* pData = (byte*)(&data) ;
	char iYsign, iXsign ;
	int iCurPos, iX, iY, iNewCurPos ;

	b1 = pData[ 0 ] ;

	if(!(b1 & 0x8) || (b1 & 0xC0) == 0xC0)
		return ;

	xMov = pData[ 1 ] ;
	yMov = pData[ 2 ] ;

	iYsign = (b1 & 0x20) ? -1 : 1 ;
	iXsign = (b1 & 0x10) ? -1 : 1 ;

	/*
	//Middle button
	if(b1 & 0x4)
		;
	//Right button
	if(b1 & 0x2)
		;
	//Left button
	if(b1 & 0x1)
		;
	*/

	iCurPos = KC::MDisplay().GetMouseCursorPos() ;
	iX = iCurPos % KC::MDisplay().MaxColumns() ;
	iY = iCurPos / KC::MDisplay().MaxRows() ;

	// X - Horizontal
	// Y - Vertical
	int sX = 1, sY = 1 ;
	if(xMov < 0) sX = -1 ;
	if(yMov < 0) sY = -1 ;
	xMov = abs(xMov) ;
	yMov = abs(yMov) ;
	if(xMov > 10) xMov /= 2 ;
	if(yMov > 4) yMov /= 2 ;
	if(xMov > 20) xMov = 20 ;
	if(yMov > 8) yMov = 8 ;
	xMov *= sX ;
	yMov *= sY ;

	iX += (xMov) ;
	iY += (-yMov) ;

	if(iX < 0) iX = 0 ;
	if(iY < 0) iY = 0 ;
	if(iX >= (int)KC::MDisplay().MaxColumns()) iX = KC::MDisplay().MaxColumns() - 1 ;
	if(iY >= (int)KC::MDisplay().MaxRows()) iY = KC::MDisplay().MaxRows() - 1 ;

	//printf("\n (%d, %d, %d, %d, %d, %d, %d, %d) = ", iX, iY, iCurPos, iNewCurPos, xMov, yMov, iXsign, iYsign) ;
	iNewCurPos = iY * KC::MDisplay().MaxColumns() + iX;
	KC::MDisplay().SetMouseCursorPos(iNewCurPos);
}

