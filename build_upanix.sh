#	other Operating System - An x86 based Operating System
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

cmd=""
exitcode=0
if [ "$KDEV_2_PARAM" = "1" ]
then
	cmd=$2
else
	cmd=$1
fi

if [ -z "$cmd" ]
then
	make elf
	exitcode=$?
	cmd="elf"
else
	make $cmd
	exitcode=$?
fi

(cd test && ./compile.sh)

if [ $exitcode -eq 0 ]
then
	if [ "$cmd" = "elf" ]
	then
		./build_load.sh
	fi
fi
