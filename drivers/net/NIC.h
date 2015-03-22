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
#ifndef _NIC_H_
#define _NIC_H_

#include <PCIBusHandler.h>

class NIC
{
	public:
		NIC(const PCIEntry* pPCIEntry) : m_bSetup(false), m_pPCIEntry(pPCIEntry) { }

		static NIC* Create(const PCIEntry* pPCIEntry) ;

	public:
		bool IsSetup() { return m_bSetup ; }

	protected:
		bool m_bSetup ;
		unsigned m_uiIOPort ;
		const PCIEntry* m_pPCIEntry ;
		byte m_bInterrupt ;
		struct 
		{
			int m_iLen ;
			byte m_address[ 10 ] ;
		} m_macAddress ; 
} ;

#endif
