#!/bin/bash
#	 Upanix - An x86 based Operating System
#	 Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
#
# I am making my contributions/submissions to this project solely in
# my personal capacity and am not conveying any rights to any
# intellectual property of any third parties.
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

#BOOT_FLOPPY_IMG=GrubFloppy_ext.img
if [ "$BOOT_FLOPPY_IMG" != "" ]
then
  sudo mount floppy/${BOOT_FLOPPY_IMG} floppy/MntFloppy -o loop 

  if [ $? -ne 0 ]
  then
    exit 100
  fi

  sudo cp -f bin/upanix.elf floppy/MntFloppy/boot
  sudo umount floppy/MntFloppy 
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

# Manually remove loop device
# sudo dmsetup remove /dev/mapper/loop0p1 /dev/mapper/loop1p1
# sudo losetup --detach /dev/loop0
LOOP_DEVICE=`losetup | grep 300MUSB.img | cut -d" " -f1`
if [ "$LOOP_DEVICE" != "" ]
then
  LOOP_DEVICE=`basename $LOOP_DEVICE`
  MOUNTP=${LOOP_DEVICE}p1
  echo "Reuse existing device mount: $MOUNTP"
fi

if [ "$LOOP_DEVICE" = "" ]
then
  sudo kpartx -d "$DEV"
  MOUNTP=`sudo kpartx -av "$DEV" | head -1 | cut -d" " -f3`
  echo "Create new device mount: $MOUNTP"
fi

echo "BIN FILE: `ls -l bin/upanix.elf`"

sleep 2
sudo mount /dev/mapper/$MOUNTP USBImage/mnt

if [ $? -ne 0 ]
then
  exit 100
fi

sudo cp -f bin/upanix.elf USBImage/mnt/efi/boot/
sudo cp -f boot/grub.cfg USBImage/mnt/efi/boot/

sleep 2
sudo umount /dev/mapper/$MOUNTP

echo "installed..."
