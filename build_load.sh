#!/bin/bash
#	 Upanix - An x86 based Operating System
#	 Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa_prajwal@yahoo.co.in'
#																			 
#	 This program is free software: you can redistribute it and/or modify
#	 it under the terms of the GNU General Public License as published by
#	 the Free Software Foundation, either version 3 of the License, or
#	 (at your option) any later version.
#																			 
#	 This program is distributed in the hope that it will be useful,
#	 but WITHOUT ANY WARRANTY; without even the implied warranty of
#	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	 GNU General Public License for more details.
#																			 
#	 You should have received a copy of the GNU General Public License
#	 along with this program.  If not, see <http://www.gnu.org/licenses/
. setup.sh

SUDO_PW=""
if [ -f .sudopw ]
then
  SUDO_PW=`cat .sudopw`
else
  echo "Don't find .sudopw file. Create a (secure) file .sudopw containing sudo password"
  exit 1
fi

#BOOT_FLOPPY_IMG=GrubFloppy_ext.img
if [ "$BOOT_FLOPPY_IMG" != "" ]
then
  echo $SUDO_PW | sudo -S mount floppy/${BOOT_FLOPPY_IMG} floppy/MntFloppy -o loop 

  if [ $? -ne 0 ]
  then
    exit 100
  fi

  echo $SUDO_PW | sudo -S cp -f bin/upanix.elf floppy/MntFloppy/boot

  echo $SUDO_PW | sudo -S umount floppy/MntFloppy 
fi

if [ "$BOOT_USB_DEVICE_NAME" = "" -a "$BOOT_USB_DEVICE_ID" != "" ]
then
  devicename=`udevadm info --query=all -n disk/by-id/$BOOT_USB_DEVICE_ID | grep "^N:" | cut -d":" -f2 | tr -d " "`
  if [ $? -eq 0 ]
  then
    BOOT_USB_DEVICE_NAME=$devicename
  fi
fi

if [ "$BOOT_USB_DEVICE_NAME" = "" ]
then
  DEV="USBImage/300MUSB.img"
  if [ "$LOOP" = "" ]
  then
    MOUNTP="loop0p1"
  else
    MOUNTP="${LOOP}p1"
  fi
else
  DEV="/dev/$BOOT_USB_DEVICE_NAME"
  bus=`udevadm info --query=all -n $DEV | grep ID_BUS | cut -d"=" -f2`
  if [ "$bus" != "usb" ]
  then
    echo "ERROR: $DEV ($bus) is not a USB disk!!"
    exit 200
  fi
  MOUNTP="${BOOT_USB_DEVICE_NAME}1"
fi

echo $SUDO_PW | sudo -S kpartx -d "$DEV"
MOUNTP=`echo $SUDO_PW | sudo -S kpartx -av "$DEV" | head -1 | cut -d" " -f3`
echo "MOUNT DEVICE: $MOUNTP"
echo "BIN FILE: `ls -l bin/upanix.elf`"

sleep 2
echo $SUDO_PW | sudo -S mount /dev/mapper/$MOUNTP usb_boot/MntUSB

if [ $? -ne 0 ]
then
  exit 100
fi

echo $SUDO_PW | sudo -S cp -f bin/upanix.elf usb_boot/MntUSB/efi/boot/
sleep 2
echo $SUDO_PW | sudo -S umount /dev/mapper/$MOUNTP

echo "installed..."
