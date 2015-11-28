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
#include <KernelComponents.h>
#include <Display.h>
#include <MemManager.h>
#include <KernelService.h>
#include <MouseDriver.h>
#include <NetworkManager.h>

Display& KC::MDisplay()
{
	static Display kDisplay ;
	return kDisplay ;
}

KernelService& KC::MKernelService()
{
	static KernelService kKernelService ;
	return kKernelService ;
}

MouseDriver& KC::MMouseDriver()
{
	static MouseDriver kMouseDriver ;
	static bool bInit = false;
	if(!bInit)
	{
		bInit = true ;
		kMouseDriver.Initialize() ;
	}
	return kMouseDriver ;
}

NetworkManager& KC::MNetworkManager()
{
	static NetworkManager kNetworkManager ;
	return kNetworkManager ;
}

