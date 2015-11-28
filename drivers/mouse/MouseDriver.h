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
#ifndef _MOUSE_DRIVER_H_
#define _MOUSE_DRIVER_H_

#include <DSUtil.h>

#define MouseDriver_SUCCESS				0
#define MouseDriver_ERR_BUFFER_FULL		1
#define MouseDriver_ERR_BUFFER_EMPTY	2

class MouseDriver
{
	public:
		void Process(unsigned data) ;

	private:
		MouseDriver() ;
		void Initialize() ;
		static void Handler() ;

	private:
		bool SendCommand1(byte command) ;
		bool SendCommand2(byte command) ;
		bool WaitForAck() ;
		bool ReceiveData(byte& data) ;
		bool ReceiveIRQData(byte& data) ;
		bool SendData(byte data) ;

	private:
		bool m_bProcessToggle ;

	friend class KC ;
} ;

#endif
