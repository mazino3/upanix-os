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

#include <PortCom.h>

void PortCom_SendByte(const unsigned short portAddress, const byte data)
{
	__asm__ __volatile__("outb %%al, %%dx" : : "d"(portAddress), "a"(data)) ; 
}

byte PortCom_ReceiveByte(const unsigned short portAddress)
{
	byte data ;
	__asm__ __volatile__("inb %%dx, %%al" : "=a"(data): "d"(portAddress)) ;
	return data ;
}

void PortCom_SendWord(const unsigned short portAddress, const unsigned short data)
{
	__asm__ __volatile__("outw %%ax, %%dx" : : "d"(portAddress), "a"(data)) ; 
}

unsigned short PortCom_ReceiveWord(const unsigned short portAddress)
{
	unsigned short data ;
	__asm__ __volatile__("inw %%dx, %%ax" : "=a"(data): "d"(portAddress)) ;
	return data ;
}

void PortCom_SendDoubleWord(const unsigned short portAddress, const unsigned data)
{
	__asm__ __volatile__("outl %%eax, %%dx" : : "d"(portAddress), "a"(data)) ; 
}

unsigned PortCom_ReceiveDoubleWord(const unsigned short portAddress)
{
	unsigned data ;
	__asm__ __volatile__("inl %%dx, %%eax" : "=a"(data): "d"(portAddress)) ;
	return data ;
}

