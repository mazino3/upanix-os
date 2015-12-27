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
#ifndef _USB_DATA_HANDLER_H_
#define _USB_DATA_HANDLER_H_

#include <Global.h>
#include <USBStructures.h>

#define DEF_DESC_LEN 8

void USBDataHandler_DisplayDevDesc(const USBStandardDeviceDesc* pDevDesc) ;
void USBDataHandler_CopyDevDesc(void* pDest, const void* pSrc, size_t len);
void USBDataHandler_DisplayConfigDesc(const USBStandardConfigDesc* pConfigDesc) ;
void USBDataHandler_CopyConfigDesc(void* pDest, const void* pSrc, int iLen) ;
void USBDataHandler_InitDevDesc(USBStandardDeviceDesc* pDesc) ;
void USBDataHandler_InitConfigDesc(USBStandardConfigDesc* pDesc) ;
void USBDataHandler_InitInterfaceDesc(USBStandardInterface* pInt) ;
void USBDataHandler_DisplayInterfaceDesc(const USBStandardInterface* pInt) ;
void USBDataHandler_InitEndPtDesc(USBStandardEndPt* pDesc) ;
void USBDataHandler_DisplayEndPtDesc(const USBStandardEndPt* pDesc) ;
void USBDataHandler_CopyStrDescZero(USBStringDescZero* pDest, const void* pSrcv) ;
void USBDataHandler_DisplayStrDescZero(USBStringDescZero* pStringDescZero) ;
void USBDataHandler_SetLangId(USBDevice* pUSBDevice) ;
void USBDataHandler_DisplayDeviceStringDetails(const USBDevice* pUSBDevice) ;
void USBDataHandler_DeAllocConfigDesc(USBStandardConfigDesc* pCD, char bNumConfigs) ;

#endif
