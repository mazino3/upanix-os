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
#ifndef _USB_CONSTANTS_H_
#define _USB_CONSTANTS_H_

#define USB_TYPE_CLASS 0x20

#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT 0x02

#define USB_MAX_STR_LEN 31

#define USB_ENDPOINT_XFERTYPE_MASK	0x03
#define USB_ENDPOINT_XFER_CONTROL	0
#define USB_ENDPOINT_XFER_BULK		2
#define USB_ENDPOINT_XFER_INT		3

#define USB_ENDPOINT_NUMBER_MASK 0x0F

#define USB_DIR_IN 0x80
#define USB_DIR_OUT 0x00

#endif
