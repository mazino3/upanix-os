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
#ifndef _KERNEL_COMP_H_
#define _KERNEL_COMP_H_

class Display ;
class MemManager ;
class KernelService ;
class MouseDriver ;
class NetworkManager ;

class KC
{
	public:
		static Display& MDisplay() ; 
		static MemManager& MMemManager() ;
		static KernelService& MKernelService() ;
		static MouseDriver& MMouseDriver() ;
		static NetworkManager& MNetworkManager() ;
} ;

#endif
