/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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
#include <mutex.h>
#include <DMM.h>

#include <usfncontext.h>
#include <usfntypes.h>
#include <Pat.h>
#include <BmpImage.h>
#include <ColorPalettes.h>
#include <RootGUIConsole.h>
#include <ImageAlgo.h>

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

GraphicsVideo& GraphicsVideo::Instance() {
  if (_instance == nullptr) {
    throw upan::exception(XLOC, "GraphicsVideo driver is not created yet");
  }
  return *_instance;
}

GraphicsVideo::GraphicsVideo(const framebuffer_info_t& fbinfo)
  : _needRefresh(false), _initialized(false), _mouseCursor(nullptr) {
  _flatLFBAddress = fbinfo.framebuffer_addr;
  _mappedLFBAddress = fbinfo.framebuffer_addr;
  _zBuffer = fbinfo.framebuffer_addr;

  _height = fbinfo.framebuffer_height;
  _width = fbinfo.framebuffer_width;
  _pitch = fbinfo.framebuffer_pitch;
  _bpp = fbinfo.framebuffer_bpp;
  _bytesPerPixel = _bpp / 8;
  _lfbSize = _height * _width * _bytesPerPixel;
  _lfbPageCount = ((_lfbSize - 1) / PAGE_SIZE) + 1;
  _xCharScale = 8;
  _yCharScale = 16;
  _inputEventFGProcess = NO_PROCESS_ID;

  FillRect(0, 0, _width, _height, 0x0);
}

void GraphicsVideo::Initialize() {
  if (_initialized) {
    return;
  }

  if (_lfbPageCount > PAGE_TABLE_ENTRIES) {
    throw upan::exception(XLOC, "Max pages available for user process GUI framebuffer is %u, requested: %u", PAGE_TABLE_ENTRIES, _lfbPageCount);
  }

  const auto wc = Pat::Instance().writeCombiningPageTableFlag();
  if (wc >= 0) {
    //remap the video framebuffer address space with write-combining flag
    MemManager::Instance().MemMapGraphicsLFB(wc);
    Mem_FlushTLB();
  }

  printf("\n Initializing mouse cursor image");
  upanui::BmpImage::Header header;
  upanui::BmpImage::InfoHeader infoHeader;
  auto rawImgBuffer = upanui::BmpImage::parse(&_binary_mouse_cursor_bmp_start, header, infoHeader, ColorPalettes::CP16::Get(ColorPalettes::CP16::FGColor::FG_RED));
  auto mouseImgBuffer = upanui::ImageAlgo::resize(rawImgBuffer, infoHeader._width, infoHeader._height, 16, 16);
  _mouseCursor.reset(new upanui::MouseCursor(mouseImgBuffer, 0, 0, 16, 16));
  delete rawImgBuffer;

  // You can as well read a .sfn file in a buffer and call ssfn_load on the same
  // In here, the .sfn file was included as part of the kernel binary
  try {
    _ssfnContext = new upanui::usfn::Context();
    _ssfnContext->Load(upanui::usfn::Context::GetPreloadedFont(upanui::usfn::PreloadedFonts::VGA16));
    _ssfnContext->Select(upanui::usfn::FAMILY_MONOSPACE, NULL, upanui::usfn::STYLE_REGULAR, 16);
    RootGUIConsole::Instance().setFontContext(_ssfnContext);
  } catch(upan::exception& e) {
    printf("\n Failed to load USFN font: %s", e.ErrorMsg().c_str());
    while(true);
  }

  _initialized = true;
}

void GraphicsVideo::CreateRefreshTask() {
  _zBuffer = KERNEL_VIRTUAL_ADDRESS(MEM_GRAPHICS_Z_BUFFER_START);
  memset((void*)_zBuffer, 0, _lfbSize);
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
  if(isDirty()) {
    upan::mutex_guard g(_fgProcessMutex);
    ProcessSwitchLock p;
    for(auto i = 0u; i < _fgProcesses.size(); ++i) {
      auto process = ProcessManager::Instance().GetProcess(_fgProcesses[i]);
      process.ifPresent([&](Process& p) {
        const auto& frame = p.getGuiFrame().value();
        const auto& viewport = frame.viewport();
        const auto buffer = (uint32_t)frame.frameBuffer().buffer();
        if (viewport.x1() == 0 && viewport.y1() == 0 && viewport.width() == _width && viewport.height() == _height) {
          optimized_memcpy(_zBuffer, (uint32_t)buffer, _lfbSize);
        } else {
          int x1 = upan::max(viewport.x1(), 0);
          int y1 = upan::max(viewport.y1(), 0);
          int x2 = upan::min(viewport.x2(), (int)(_width - 1));
          int y2 = upan::min(viewport.y2(), (int)(_height - 1));

          if (x1 >= 0 && x2 < (int)_width && y1 >= 0 && y2 < (int)_height) {
            auto xoffset = x1 * _bytesPerPixel;
            auto bytesPerLine = (x2 - x1 + 1) * _bytesPerPixel;
            for(int y = y1, sy = 0; y <= y2; ++y, ++sy) {
              auto destOffset = y * _pitch + xoffset;
              auto srcOffset = sy * _pitch;
              memcpy((void*)(_zBuffer + destOffset), (void*)(buffer + srcOffset), bytesPerLine);
            }
          }
        }
      });
    }
    optimized_memcpy(_mappedLFBAddress, _zBuffer, _lfbSize);
    DrawMouseCursor();
  }
  return true;
}

void GraphicsVideo::NeedRefresh() {
  _needRefresh.set(true);
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

  if (newX != _mouseCursor->x() || newY != _mouseCursor->y()) {
    _mouseCursor->x(newX);
    _mouseCursor->y(newY);
    NeedRefresh();
  }
}

upan::option<int> GraphicsVideo::getFGProcessUnderMouseCursor() {
  upan::mutex_guard g(_fgProcessMutex);

  for(auto it = _fgProcesses.rbegin(); it != _fgProcesses.rend(); ++it) {
    const auto pid = *it;
    auto process = ProcessManager::Instance().GetProcess(pid);
    if (process.isEmpty()) {
      removeFGProcess(pid);
    } else {
      if (!process.value().getGuiFrame().isEmpty()) {
        const auto& f = process.value().getGuiFrame().value();
        if (f.viewport().x1() <= _mouseCursor->x() && _mouseCursor->x() <= f.viewport().x2()
        && f.viewport().y1() <= _mouseCursor->y() && _mouseCursor->y() <= f.viewport().y2()) {
          return upan::option<int>(pid);
        }
      }
    }
  }
  return upan::option<int>::empty();
}

void GraphicsVideo::switchFGProcess(int pid) {
  upan::mutex_guard g(_fgProcessMutex);

  auto process = ProcessManager::Instance().GetProcess(pid);
  if (process.isEmpty()) {
    removeFGProcess(pid);
  } else {
    if (pid != _fgProcesses.back() && !process.value().isGuiBase()) {
      _fgProcesses.erase(pid);
      _fgProcesses.push_back(pid);
    }
    _inputEventFGProcess = pid;
  }
}

void GraphicsVideo::DrawMouseCursor() {
  CopyArea(_mouseCursor->x(), _mouseCursor->y(), _mouseCursor->width(), _mouseCursor->height(), _mouseCursor->data(), true);
  //printf("\n%d:%d:%d:%d", _mouseCursor->x(), _mouseCursor->y(), _mouseCursor->width(), _mouseCursor->height());
  //_needRefresh.set(false);
}

void GraphicsVideo::CopyArea(unsigned sx, unsigned sy, uint32_t width, uint32_t height, const uint32_t* src, bool directWrite) {
  const uint32_t frameBuffer = directWrite ? _mappedLFBAddress : _zBuffer;
  for(unsigned y = sy; y < (sy + height) && y < _height; ++y) {
    const auto y_offset = y * _pitch;
    const auto src_y_offset = (y - sy) * width;
    for(unsigned x = sx; x < (sx + width) && x < _width; ++x) {
      auto p = (unsigned*)(frameBuffer + y_offset + x * _bytesPerPixel);
      auto v = src[src_y_offset + (x - sx)];

      //transparent color alpha --> TODO: blend with alpha of the underlying canvas
      const auto alpha = v >> 24;
      if (alpha < 0xAA)
        continue;

      *p = v;
    }
  }

  if (!directWrite) {
    NeedRefresh();
  }
}

bool GraphicsVideo::isDirty() {
  upan::mutex_guard g(_fgProcessMutex);

  if (_needRefresh.get()) {
    _needRefresh.set(false);
    return true;
  }

  bool dirty = false;

  for(auto pid : _fgProcesses) {
    auto process = ProcessManager::Instance().GetProcess(pid);
    if (process.isEmpty()) {
      removeFGProcess(pid);
    } else {
      process.value().getGuiFrame().ifPresent([&dirty](RootFrame& f) {
        dirty |= f.isDirty();
        f.clean();
      });
    }
  }
  return dirty;
}

void GraphicsVideo::addFGProcess(int pid) {
  upan::mutex_guard g(_fgProcessMutex);
  _fgProcesses.push_back(pid);
  _inputEventFGProcess = pid;
}

void GraphicsVideo::removeFGProcess(int pid) {
  upan::mutex_guard g(_fgProcessMutex);
  _fgProcesses.erase(pid);
  if (pid == _inputEventFGProcess) {
    if (_fgProcesses.size() > 0) {
      _inputEventFGProcess = _fgProcesses.back();
    } else {
      _inputEventFGProcess = NO_PROCESS_ID;
    }
  }
}

upan::option<int> GraphicsVideo::getDisplayFGProcess() {
  upan::mutex_guard g(_fgProcessMutex);
  if (_fgProcesses.empty()) {
    return upan::option<int>::empty();
  }
  return upan::option<int>(_fgProcesses[_fgProcesses.size() - 1]);
}

uint32_t GraphicsVideo::allocateFrameBuffer() {
  auto addr = DMM_AllocateForKernel(_lfbPageCount * PAGE_SIZE, PAGE_SIZE);
  memset((void*)addr, 0, _lfbSize);
  return addr;
}
