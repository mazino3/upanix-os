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
#pragma once

namespace DisplayConstants {
  enum FGColor
  {
    FG_BLACK,
    FG_BLUE,
    FG_GREEN,
    FG_CYAN,
    FG_RED,
    FG_MAGENTA,
    FG_BROWN,
    FG_WHITE,
    FG_DARK_GRAY,
    FG_BRIGHT_BLUE,
    FG_BRIGHT_GREEN,
    FG_BRIGHT_CYAN,
    FG_PINK,
    FG_BRIGHT_MAGENTA,
    FG_YELLOW,
    FG_BRIGHT_WHITE,
  };

  enum BGColor
  {
    BG_BLACK = 0x00,
    BG_BLUE  = 0x10,
    BG_GREEN = 0x20,
    BG_CYAN  = 0x30,
    BG_RED   = 0x40,
    BG_MAGENTA = 0x50,
    BG_BROWN = 0x60,
    BG_WHITE = 0x70,
  };

  enum Blink
  {
    NO_BLINK = 0x00,
    BLINK = 0x80,
  };

  constexpr int NO_BYTES_PER_CHARACTER = 2;
};