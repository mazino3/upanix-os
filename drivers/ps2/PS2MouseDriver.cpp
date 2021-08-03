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
#include <PS2KeyboardDriver.h>
#include <PS2MouseDriver.h>
#include <PS2Controller.h>
#include <GraphicsVideo.h>

PS2MouseDriver::PS2MouseDriver() {
  _dataCounter = 0;
  _packetSize = 3; //TODO: go with default - 3 bytes per mouse movement

  IrqManager::Instance().DisableIRQ(StdIRQ::Instance().MOUSE_IRQ) ;
	IrqManager::Instance().RegisterIRQ(StdIRQ::Instance().MOUSE_IRQ, (unsigned)&PS2MouseDriver::Handler) ;

	try {
//	if(SendCommand2(0xF3))
//	if(SendData(200))
//	if(SendCommand2(0xF3))
//	if(SendData(100))
//	if(SendCommand2(0xF3))
//	if(SendData(80))
//	if(SendCommand2(0xF2))
//	if(ReceiveData(data))
//		printf("\n Mouse ID: %u ", data) ;

    IrqManager::Instance().EnableIRQ(StdIRQ::Instance().MOUSE_IRQ);
    KC::MConsole().LoadMessage("PS2 Mouse Driver Initialization", Success);
  } catch(upan::exception& e) {
	  printf("\n Failed to initialize mouse driver: %s", e.ErrorMsg().c_str());
    KC::MConsole().LoadMessage("Mouse Initialization", Failure);
	}
}

void PS2MouseDriver::Handler() {
	AsmUtil_STORE_GPR() ;
	AsmUtil_SET_KERNEL_DATA_SEGMENTS

	try {
    PS2MouseDriver::Instance().Process();
  } catch (const upan::exception& e) {
	  printf("\n Failed to get interrupt data from mouse IRQ handler: %s", e.ErrorMsg().c_str());
	}

	IrqManager::Instance().SendEOI(StdIRQ::Instance().MOUSE_IRQ);

	AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
	AsmUtil_RESTORE_GPR() ;

	__asm__ __volatile__("LEAVE") ;
	__asm__ __volatile__("IRET") ;
}

upan::option<uint8_t> PS2MouseDriver::ReceiveIRQData() {
  if(!PS2Controller::Instance().WaitForRead()) {
    throw upan::exception(XLOC, "Mouse Recv IRQ Data wait failed");
  }

  const auto status = PortCom_ReceiveByte(PS2Controller::COMMAND_PORT);
	if(status & 0x20) {
    return PortCom_ReceiveByte(PS2Controller::DATA_PORT);
  }
	// the data at the port is not from mouse
	return upan::option<uint8_t>::empty();
}

void PS2MouseDriver::Process()
{
  try {
    ReceiveIRQData().ifPresent([this](uint8_t data) {
      _packetData[_dataCounter] = data;
      ++_dataCounter;
      if (_dataCounter == _packetSize) {
        _dataCounter = 0;
        const uint8_t status = _packetData[0];
        if (!(status & 0x8) || (status & 0xC0) == 0xC0)
          return;

        const auto xMov = static_cast<int8_t>(_packetData[1]);
        const auto yMov = static_cast<int8_t>(_packetData[2]);

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

        int iX = GraphicsVideo::Instance().GetMouseX() + xMov;
        int iY = GraphicsVideo::Instance().GetMouseY() - yMov;

        GraphicsVideo::Instance().SetMouseCursorPos(iX, iY);
        //printf("\n (%d, %d, %d, %d)", GraphicsVideo::Instance().GetMouseX(), GraphicsVideo::Instance().GetMouseY(), xMov, yMov);
      }
    });
  } catch(const upan::exception& e) {
    printf("\n packet byte: %x, packet count: %d", _packetData[_dataCounter], _dataCounter);
    throw e;
  }
}

