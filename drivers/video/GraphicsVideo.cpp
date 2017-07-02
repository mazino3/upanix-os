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
#include <exception.h>
#include <MemManager.h>
#include <GraphicsVideo.h>
#include <GraphicsFont.h>
#include <ProcessManager.h>
#include <Atomic.h>

GraphicsVideo* GraphicsVideo::_instance = nullptr;

void GraphicsVideo::Create()
{
  static bool _done = false;
  //can be called only once
  if(_done)
    throw upan::exception(XLOC, "cannot create GraphicsVideo driver!");
  _done = true;
  auto f = MultiBoot::Instance().VideoFrameBufferInfo();
  if(f)
  {
    static GraphicsVideo v(*f);
    _instance = &v;
  }
}

GraphicsVideo::GraphicsVideo(const framebuffer_info_t& fbinfo) : _needRefresh(0)
{
  _flatLFBAddress = fbinfo.framebuffer_addr;
  _mappedLFBAddress = fbinfo.framebuffer_addr;
  _zBuffer = fbinfo.framebuffer_addr;

  _height = fbinfo.framebuffer_height;
  _width = fbinfo.framebuffer_width;
  _pitch = fbinfo.framebuffer_pitch;
  _bpp = fbinfo.framebuffer_bpp;
  _bytesPerPixel = _bpp / 8;
  _lfbSize = _height * _width * _bytesPerPixel;

  FillRect(0, 0, _width, _height, 0x0);
}

void GraphicsVideo::CreateRefreshTask()
{
  _zBuffer = KERNEL_VIRTUAL_ADDRESS(MEM_GRAPHICS_Z_BUFFER_START);
  FillRect(0, 0, _width, _height, 0x0);
  KernelUtil::ScheduleTimedTask("xgrefresh", 50, *this);
}

bool GraphicsVideo::TimerTrigger()
{
  if(_needRefresh)
  {
    ProcessSwitchLock p;
    memcpy(_mappedLFBAddress, _zBuffer, _lfbSize);
    Atomic::Swap(_needRefresh, 0);
  }
  return true;
}

void GraphicsVideo::NeedRefresh()
{
  Atomic::Swap(_needRefresh, 1);
}

void GraphicsVideo::SetPixel(unsigned x, unsigned y, unsigned color)
{
  if(y >= _height || x >= _width)
    return;
  unsigned* p = (unsigned*)(_zBuffer + y * _pitch + x * _bytesPerPixel);
  *p = (color | 0xFF000000);

  NeedRefresh();
}

void GraphicsVideo::FillRect(unsigned sx, unsigned sy, unsigned width, unsigned height, unsigned color)
{
  unsigned y_offset;
  for(unsigned y = sy; y < (sy + height) && y < _height; ++y)
  {
    y_offset = y * _pitch;
    for(unsigned x = sx; x < (sx + width) && x < _width; ++x)
    {
      unsigned* p = (unsigned*)(_zBuffer + y_offset + x * _bytesPerPixel);
      *p = (color | 0xFF000000);
    }
  }

  NeedRefresh();
}

void GraphicsVideo::DrawChar(byte ch, unsigned x, unsigned y, unsigned fg, unsigned bg)
{
  const int xScale = 8;
  const int yScale = 16;
  x *= xScale;
  y *= yScale;
  if((y + yScale) >= _height || (x + xScale) >= _width)
    return;
  fg |= 0xFF000000;
  bg |= 0xFF000000;
  const byte* font_data = GraphicsFont::Get(ch);
  unsigned yr = 0;
  for(unsigned f = 0; f < 8; ++y)
  {
    unsigned lfbp = _zBuffer + y * _pitch + x * _bytesPerPixel;
    for(unsigned i = 0x80; i != 0; i >>= 1, lfbp += _bytesPerPixel)
      *(unsigned*)lfbp = font_data[f] & i ? fg : bg;

    if(yr == 1)
    {
      ++f;
      yr = 0;
    }
    else
      ++yr;
  }

  NeedRefresh();
}

//TODO: this is assuming 4 bytes per pixel
//TODO: initialize y and x scale as class members and base all calculations on y/x scale instead of assuming/hardcoding
void GraphicsVideo::ScrollDown()
{
  static const unsigned maxSize = _width * _height;
  //1 line = 16 rows as we are scaling y axis by 16
  static const unsigned oneLine = _width * 16;

  memcpy(_zBuffer, _zBuffer + oneLine * _bytesPerPixel, (maxSize - oneLine) * _bytesPerPixel);
  unsigned i = maxSize - oneLine;
  unsigned* lfb = (unsigned*)(_zBuffer);
  for(; i < maxSize; ++i)
    lfb[i] = 0xFF000000;

  NeedRefresh();
}
