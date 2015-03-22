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
#ifndef _REALTEK_NIC_H_
#define _REALTEK_NIC_H_

#include <NIC.h>
#include <PIC.h>
#include <PortCom.h>
#include <MemConstants.h>

class RealtekNIC : public NIC
{
	public:
		RealtekNIC(const PCIEntry* pPCIEntry) ;

	private:
		bool Setup() ;
		bool Reset() ;
		bool Open() ;
		void InitRing() ;

		static void Handler() ;

	private:
		inline void SendByte(const unsigned& reg, byte data) { PortCom_SendByte(m_uiIOPort+reg, data) ; }
		inline void SendWord(const unsigned& reg, unsigned short data) { PortCom_SendWord(m_uiIOPort+reg, data) ; }
		inline void SendDWord(const unsigned& reg, unsigned data) { PortCom_SendDoubleWord(m_uiIOPort+reg, data) ; }
		inline byte ReceiveByte(const unsigned& reg) { return PortCom_ReceiveByte(m_uiIOPort+reg) ; }
		inline unsigned short ReceiveWord(const unsigned& reg) { return PortCom_ReceiveWord(m_uiIOPort+reg) ; }
		inline unsigned ReceiveDWord(const unsigned& reg) { return PortCom_ReceiveDoubleWord(m_uiIOPort+reg) ; }

		inline unsigned RXDMAAddr() { return (unsigned)m_rxRing + GLOBAL_DATA_SEGMENT_BASE ; }
		inline unsigned TXDMAAddr() { return (unsigned)m_txBufs + GLOBAL_DATA_SEGMENT_BASE ; }
		
	private:
		static const IRQ* m_pIRQ ;

	private:
		byte* m_rxRing ;
		int m_iNextRxIndex ;
		unsigned m_uiRxConfig ;

	//	byte* m_txBuf[ NUM_TX_DESC ] ;
		byte* m_txBufs ;
		unsigned m_iNextTxIndex ;
		unsigned m_uiTxConfig ;
		unsigned m_uiTxFlag ;

		// MII details
} ;

#endif
