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
#ifndef _GRAPHICS_VIDEO_H_
#define _GRAPHICS_VIDEO_H_

#include <MultiBoot.h>

class GraphicsVideo
{
  private:
    GraphicsVideo(const framebuffer_info_t&);
  public:
    static void Create();
    static GraphicsVideo* Instance() { return _instance; }
    unsigned FlatLFBAddress() const { return _flatLFBAddress; }
    void MappedLFBAddress(unsigned a) { _mappedLFBAddress = a; }
    unsigned LFBSize() const { return _lfbSize; }
    void SetPixel(unsigned x, unsigned y, unsigned color);
    void FillRect(unsigned sx, unsigned sy, unsigned width, unsigned height, unsigned color);
    void DrawChar(byte ch, unsigned x, unsigned y, unsigned fg, unsigned bg);
  private:
    static GraphicsVideo* _instance;
    unsigned _flatLFBAddress;
    unsigned _mappedLFBAddress;
    unsigned _pitch;
    unsigned _width;
    unsigned _height;
    unsigned _lfbSize;
    byte     _bpp;
    byte     _bytesPerPixel;
};

#endif
