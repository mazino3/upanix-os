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

#include <usfncontext.h>
#include <usfntypes.h>
#include <Pat.h>
#include <BmpImage.h>

extern unsigned _binary_fonts_FreeSans_sfn_start;
extern unsigned _binary_unifont_sfn_start;
extern unsigned _binary_u_vga16_sfn_start;
extern unsigned _binary_mouse_cursor_bmp_start;
//make below extern as you load bmp files during testing
unsigned _binary_p16_bmp_start;
unsigned _binary_p256_bmp_start;
unsigned _binary_p24_bmp_start;
unsigned _binary_s24_bmp_start;

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

GraphicsVideo::GraphicsVideo(const framebuffer_info_t& fbinfo) : _needRefresh(0), _mouseCursorImg(nullptr)
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
  _xCharScale = 8;
  _yCharScale = 16;
  _mouseX = 0;
  _mouseY = 0;

  FillRect(0, 0, _width, _height, 0x0);
}

void GraphicsVideo::Initialize() {
  const auto wc = Pat::Instance().writeCombiningPageTableFlag();
  if (wc >= 0) {
    //remap the video framebuffer address space with write-combining flag
    MemManager::Instance().MemMapGraphicsLFB(wc);
    Mem_FlushTLB();
  }
  _mouseCursorImg.reset(new upanui::BmpImage(&_binary_mouse_cursor_bmp_start));
  InitializeUSFN();
}

void GraphicsVideo::InitializeUSFN() {
  // You can as well read a .sfn file in a buffer and call ssfn_load on the same
  // In here, the .sfn file was included as part of the kernel binary
  _usfnInitialized = false;
  try {
    _ssfnContext = new usfn::Context();
    _ssfnContext->Load((uint8_t *) &_binary_u_vga16_sfn_start);
    _ssfnContext->Select(usfn::FAMILY_MONOSPACE, NULL, usfn::STYLE_REGULAR, 16);
    _usfnInitialized = true;
  } catch(upan::exception& e) {
    printf("\n Failed to load USFN font: %s", e.ErrorMsg().c_str());
    while(1);
  }
}

void GraphicsVideo::CreateRefreshTask()
{
  _zBuffer = KERNEL_VIRTUAL_ADDRESS(MEM_GRAPHICS_Z_BUFFER_START);
  FillRect(0, 0, _width, _height, 0x0);
  KernelUtil::ScheduleTimedTask("xgrefresh", 50, *this);
}

static void optimized_memcpy(uint32_t dest, uint32_t src, int len) {
  const int inc = 16 * 8; // number of bytes copied per iteration = 16 bytes per xmm register * 8 xmm registers
  for(int i = 0; i < len; i += inc) {
    __asm__ __volatile__ (
        "prefetchnta 128(%0);"
        "prefetchnta 160(%0);"
        "prefetchnta 192(%0);"
        "prefetchnta 224(%0);"
        "movdqa 0(%0), %%xmm0;"
        "movdqa 16(%0), %%xmm1;"
        "movdqa 32(%0), %%xmm2;"
        "movdqa 48(%0), %%xmm3;"
        "movdqa 64(%0), %%xmm4;"
        "movdqa 80(%0), %%xmm5;"
        "movdqa 96(%0), %%xmm6;"
        "movdqa 112(%0), %%xmm7;"
        "movntdq %%xmm0, 0(%1);"
        "movntdq %%xmm1, 16(%1);"
        "movntdq %%xmm2, 32(%1);"
        "movntdq %%xmm3, 48(%1);"
        "movntdq %%xmm4, 64(%1);"
        "movntdq %%xmm5, 80(%1);"
        "movntdq %%xmm6, 96(%1);"
        "movntdq %%xmm7, 112(%1);"
        : : "r"(src), "r"(dest) : "memory");
    src += inc;
    dest += inc;
  }
}

bool GraphicsVideo::TimerTrigger() {
  if(_needRefresh) {
    ProcessSwitchLock p;
    optimized_memcpy(_mappedLFBAddress, _zBuffer, _lfbSize);
    //memcpy((void *) _mappedLFBAddress, (void *) _zBuffer, _lfbSize);
    DrawMouseCursor();
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
  auto p = (unsigned*)(_zBuffer + y * _pitch + x * _bytesPerPixel);
  *p = (color | 0xFF000000);

  NeedRefresh();
}

void GraphicsVideo::FillRect(unsigned sx, unsigned sy, unsigned width, unsigned height, unsigned color) {
  unsigned y_offset;
  for(unsigned y = sy; y < (sy + height) && y < _height; ++y)
  {
    y_offset = y * _pitch;
    for(unsigned x = sx; x < (sx + width) && x < _width; ++x)
    {
      auto p = (unsigned*)(_zBuffer + y_offset + x * _bytesPerPixel);
      *p = (color | 0xFF000000);
    }
  }

  NeedRefresh();
}

void GraphicsVideo::DrawCursor(uint32_t x, uint32_t y, uint32_t color) {
  x *= _xCharScale;
  y *= _yCharScale;
  if(y >= _height || (x + _xCharScale) >= _width)
    return;
  FillRect(x + 1, y + _yCharScale - 1, _xCharScale - 1, 1, color);
}

void GraphicsVideo::DrawChar(byte ch, unsigned x, unsigned y, unsigned fg, unsigned bg) {
  if (_usfnInitialized) {
    try {
      DrawUSFNChar(ch, x, y, fg, bg);
      return;
    } catch(upan::exception& e) {
      _usfnInitialized = false;
      printf("failed to render character using usfn: %s", e.ErrorMsg().c_str());
    }
  }
  x *= _xCharScale;
  y *= _yCharScale;
  if((y + _yCharScale) >= _height || (x + _xCharScale) >= _width)
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

void GraphicsVideo::DrawUSFNChar(byte ch, unsigned x, unsigned y, unsigned fg, unsigned bg) {
  //for SSFN, y is the baseline, the characters are drawn above y and hence add yScale to y --> this is only for Render() which is used for GUI
  //we don't have to do it for a standard text console display using RenderCharacter()
  //++y;
  x *= _xCharScale;
  y *= _yCharScale;

  if(y >= _height || (x + _xCharScale) >= _width)
    return;

  usfn::FrameBuffer buf = {                                  /* the destination pixel buffer */
      .ptr = (uint8_t*)_zBuffer,                      /* address of the buffer */
      .w = (int16_t)_width,                             /* width */
      .h = (int16_t)_height,                             /* height */
      .p = (uint16_t)_pitch,                         /* bytes per line */
      .x = (int16_t)x,                                       /* pen position */
      .y = (int16_t)y,
      .fg = 0xFF000000 | fg,
      .bg = 0xFF000000 | bg
  };

//    const char s[2] = { (const char)ch, '\0' };
//    _ssfnContext->Render(buf, s, true);
  _ssfnContext->RenderCharacter(buf, ch);
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

void GraphicsVideo::SetMouseCursorPos(int x, int y) {
  int newX;
  if (x < 0) {
    newX = 0;
  } else if (x >= _width) {
    newX = _width - 1;
  } else {
    newX = x;
  }

  int newY;
  if (y < 0) {
    newY = 0;
  } else if (y >= _height) {
    newY = _height - 1;
  } else {
    newY = y;
  }

  if (newX != _mouseX || newY != _mouseY) {
    _mouseX = newX;
    _mouseY = newY;
    NeedRefresh();
  }
}
void GraphicsVideo::ExperimentWithMouseCursor(int i) {
  void* img;
  if (i == 0) {
    img = &_binary_mouse_cursor_bmp_start;
  } else if (i == 1) {
    img = &_binary_p16_bmp_start;
  } else if (i == 2) {
    img = &_binary_p256_bmp_start;
  } else if (i == 3) {
    img = &_binary_p24_bmp_start;
  } else if (i == 4) {
    img = &_binary_s24_bmp_start;
  } else {
    printf("\n pass in the image id");
    return;
  }

  upanui::BmpImage image(img);
  image.DebugPrint();
  CopyArea(200, 200, image.width(), image.height(), image.frameBuffer(), false);
}

void GraphicsVideo::DrawMouseCursor() {
  CopyArea(_mouseX, _mouseY, _mouseCursorImg->width(), _mouseCursorImg->height(), _mouseCursorImg->frameBuffer(), true);
}

void GraphicsVideo::CopyArea(unsigned sx, unsigned sy, uint32_t width, uint32_t height, const uint32_t* src, bool directWrite) {
  const uint32_t frameBuffer = directWrite ? _mappedLFBAddress : _zBuffer;
  for(unsigned y = sy; y < (sy + height) && y < _height; ++y) {
    const auto y_offset = y * _pitch;
    const auto src_y_offset = (y - sy) * width;
    for(unsigned x = sx; x < (sx + width) && x < _width; ++x) {
      auto p = (unsigned*)(frameBuffer + y_offset + x * _bytesPerPixel);
      auto v = src[src_y_offset + (x - sx)];
      if ((v & 0xFF000000) == 0)
        continue;
      *p = v;
    }
  }

  NeedRefresh();
}
