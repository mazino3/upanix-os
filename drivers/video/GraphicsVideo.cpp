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
#include <DMM.h>

#define SSFN_IMPLEMENTATION
#include <ssfn.h>
#include "GraphicsFont.h"

extern unsigned _binary_fonts_FreeSans_sfn_start;
extern unsigned _binary_unifont_sfn_start;
extern unsigned _binary_u_vga16_sfn_start;

ssfn_t*  _ssfnContext;

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

void GraphicsVideo::InitializeSSFN() {
  // You can as well read a .sfn file in a buffer and call ssfn_load on the same
  // In here, the .sfn file was included as part of the kernel binary
  _ssfnInitialized = false;
  //_ssfnContext = new ssfn_t();
  _ssfnContext = DMM_AllocateForKernel(sizeof(ssfn_t));//= { 0 };                                 /* the renderer context */

  //int r = ssfn_load(_ssfnContext, &_binary_fonts_FreeSans_sfn_start);
  int r = ssfn_load(_ssfnContext, &_binary_unifont_sfn_start);
  //int r = ssfn_load(_ssfnContext, &_binary_u_vga16_sfn_start);
  if (r != SSFN_OK) {
    printf("\n Failed to load SSFN font - error code: %d", r);
    return;
  }
  //SSFN_STYLE_REGULAR | SSFN_STYLE_UNDERLINE
  r = ssfn_select(_ssfnContext, SSFN_FAMILY_MONOSPACE, NULL, SSFN_STYLE_REGULAR, 16);
  if (r != SSFN_OK) {
    printf("\n Failed to select SSFN font - error code: %d", r);
    return;
  }

  _ssfnInitialized = true;
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
    memcpy((void*)_mappedLFBAddress, (void*)_zBuffer, _lfbSize);
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

void GraphicsVideo::PrintSSFNContext() {
  printf("%d %d %d %d\n", _ssfnContext->mx, _ssfnContext->my, _ssfnContext->lx, _ssfnContext->ly);
}

void GraphicsVideo::DrawChar(byte ch, unsigned x, unsigned y, unsigned fg, unsigned bg) {
  if (_ssfnInitialized && ch != ' ') {
    DrawSSFNChar(ch, x, y, fg, bg);
    return;
  }
  const int xScale = 8;
  const int yScale = 16;
  x *= xScale;
  y *= yScale;
  if((y + yScale) >= _height || (x + xScale) >= _width)
    return;
  fg |= 0xFF000000;
  bg |= 0xFF000000;
  const byte* font_data = GraphicsFont::Get(ch);
  bool yr = false;
  for(unsigned f = 0; f < 8; ++y)
  {
    unsigned lfbp = _zBuffer + y * _pitch + x * _bytesPerPixel;
    for(unsigned i = 0x80; i != 0; i >>= 1, lfbp += _bytesPerPixel)
      *(unsigned*)lfbp = font_data[f] & i ? fg : bg;

    if(yr) ++f;
    yr = !yr;
  }

  NeedRefresh();
}

void GraphicsVideo::DrawSSFNChar(byte ch, unsigned x, unsigned y, unsigned fg, unsigned bg) {
  //for SSFN, y is the baseline, the characters are drawn above y and hence add yScale to y
  ++y;
  const int xScale = 8;
  const int yScale = 16;
  x *= xScale;
  y *= yScale;

  if(y >= _height || (x + xScale) >= _width)
    return;

  ssfn_buf_t buf = {                                  /* the destination pixel buffer */
      .ptr = (uint8_t*)_zBuffer,                      /* address of the buffer */
      .w = (int16_t)_width,                             /* width */
      .h = (int16_t)_height,                             /* height */
      .p = (uint16_t)_pitch,                         /* bytes per line */
      .x = (int16_t)x,                                       /* pen position */
      .y = (int16_t)y,
      .fg = 0xFF000000 | fg,
      .bg = 0xFF000000 | bg
  };

  const char s[2] = { (const char)ch, '\0' };
  ssfn_render(_ssfnContext, &buf, s);
  NeedRefresh();
}

//TODO: this is assuming 4 bytes per pixel
//TODO: initialize y and x scale as class members and base all calculations on y/x scale instead of assuming/hardcoding
void GraphicsVideo::ScrollDown()
{
  static const unsigned maxSize = _width * _height;
  //1 line = 16 rows as we are scaling y axis by 16
  static const unsigned oneLine = _width * 16;

  memcpy((void*)_zBuffer, (void*)(_zBuffer + oneLine * _bytesPerPixel), (maxSize - oneLine) * _bytesPerPixel);
  unsigned i = maxSize - oneLine;
  unsigned* lfb = (unsigned*)(_zBuffer);
  for(; i < maxSize; ++i)
    lfb[i] = 0xFF000000;

  NeedRefresh();
}
