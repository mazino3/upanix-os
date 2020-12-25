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
#ifndef _PORTCOM_H_
#define _PORTCOM_H_

#include <Global.h>
#include <ustring.h>

void PortCom_SendByte(const unsigned short portAddress, const byte data) ;
byte PortCom_ReceiveByte(const unsigned short portAddress) ;
void PortCom_SendWord(const unsigned short portAddress, const unsigned short data) ;
unsigned short PortCom_ReceiveWord(const unsigned short portAddress) ;
void PortCom_SendDoubleWord(const unsigned short portAddress, const unsigned data) ;
unsigned PortCom_ReceiveDoubleWord(const unsigned short portAddress) ;

class COM
{
  public:
    void Write(byte data);
    void Write(const upan::string& msg);

  protected:
    byte IsTransmitReady();
    COM(unsigned short port);
    const unsigned short _port;
};

class COM1 final : public COM
{
  private:
    COM1() : COM(0x3F8) {}
  public:
    static COM1& Instance()
    {
      static COM1 instance;
      return instance;
    }
};

#endif
