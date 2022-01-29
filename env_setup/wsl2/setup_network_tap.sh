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

gateway=`ip route | grep "^default via" | cut -d" " -f3`
echo "gateway ip $gateway"

sudo brctl addbr br0
echo "created bridge br0"

sudo ip tuntap add tap0 mode tap
echo "created tap device tap0"

sudo brctl addif br0 tap0
echo "added tap0 to br0"

sudo brctl addif br0 eth0
echo "added eth0 to br0"

sudo ip link set dev br0 up
echo "started br0"

sudo ip link set dev tap0 up
echo "started tap0"

bridge_address=`ifconfig | grep -A3 eth0 | grep "inet .*netmask" | tr -s " " | cut -d" " -f3-`
echo "ipconfig of eth0: $bridge_address"

sudo ifconfig br0 $bridge_address
echo "assigned ipconfig of eth0 to br0"

sudo ifconfig eth0 0.0.0.0
echo "remove ip for eth0"

sudo ip route add default via $gateway dev br0
echo "added default route for br0 via gateway $gateway"
