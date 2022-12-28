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
#include <mosstd.h>
#include <MemManager.h>
#include <GraphicsVideo.h>
#include <ProcessManager.h>
#include <mutex.h>
#include <DMM.h>

#include <usfncontext.h>
#include <usfntypes.h>
#include <Pat.h>
#include <BmpImage.h>
#include <ColorPalettes.h>
#include <RootGUIConsole.h>
#include <GCoreFunctions.h>
#include <metrics.h>

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
  : _needRefresh(false), _initialized(false),
    _mouseCursor(nullptr), _mouseChange(0), _mousePrevX(0), _mousePrevY(0) {
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
  auto mouseImgBuffer = upanui::GCoreFunctions::resize(rawImgBuffer, infoHeader._width, infoHeader._height, 16, 16);
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
  KernelUtil::ScheduleTimedTask("xgrefresh", 10, *this);
}

bool GraphicsVideo::TimerTrigger() {
  RedrawInfo redrawInfo = isDirty();
  if (!redrawInfo._processChanged && !redrawInfo._mouseChanged) {
    return true;
  }

  ProcessSwitchLock p;
  auto& gvtStats = upan::metrics::instance().get("gvt");
  auto& lfbStats = upan::metrics::instance().get("lfb");
  gvtStats.start();
  if (redrawInfo._processChanged) {
    memset((void *) _zBuffer, 0, _lfbSize);
    for (auto i = 0u; i < _fgProcesses.size(); ++i) {
      auto process = ProcessManager::Instance().GetProcess(_fgProcesses[i]);
      process.ifPresent([&](Process &p) {
        const auto &frame = p.getGuiFrame().value();
        const auto &viewport = frame.viewport();
        const auto buffer = frame.buffer();
        const auto srcBufferWidth = frame.bufferLineWidth();

        const int destX1 = upan::min(upan::max(viewport.x1(), 0), (int) _width);
        const int destX2 = upan::min(upan::max(viewport.x2(), 0), (int) _width);

        const int destY1 = upan::min(upan::max(viewport.y1(), 0), (int) _height);
        const int destY2 = upan::min(upan::max(viewport.y2(), 0), (int) _height);

        if (destX1 < destX2 && destY1 < destY2) {
          const int srcX1 = destX1 - viewport.x1();
          const int srcY1 = destY1 - viewport.y1();
          CopyArea(destX1, destY1, srcX1, srcY1, srcBufferWidth, (destX2 - destX1), (destY2 - destY1), buffer, frame.hasAlpha());
        }
      });
    }
  } else if (redrawInfo._mouseChanged) {
    const int drawMinX = upan::min(_mousePrevX, _mouseCursor->x());
    const int drawMaxX = upan::max(_mousePrevX, _mouseCursor->x()) + _mouseCursor->width();

    const int drawMinY = upan::min(_mousePrevY, _mouseCursor->y());
    const int drawMaxY = upan::max(_mousePrevY, _mouseCursor->y()) + _mouseCursor->height();

    for (auto i = 0u; i < _fgProcesses.size(); ++i) {
      auto process = ProcessManager::Instance().GetProcess(_fgProcesses[i]);
      process.ifPresent([&](Process& p) {
        const auto& frame = p.getGuiFrame().value();
        const auto& viewport = frame.viewport();
        const auto buffer = frame.buffer();
        const auto srcBufferWidth = frame.bufferLineWidth();

        const int destX1 = upan::min(upan::max(upan::max(viewport.x1(), 0), drawMinX), (int)_width);
        const int destX2 = upan::min(upan::min(upan::max(viewport.x2(), 0), drawMaxX), (int)_width);

        const int destY1 = upan::min(upan::max(upan::max(viewport.y1(), 0), drawMinY), (int)_height);
        const int destY2 = upan::min(upan::min(upan::max(viewport.y2(), 0), drawMaxY), (int)_height);

        if (destX1 < destX2 && destY1 < destY2) {
          const int srcX1 = destX1 - viewport.x1();
          const int srcY1 = destY1 - viewport.y1();
          CopyArea(destX1, destY1, srcX1, srcY1, srcBufferWidth, (destX2 - destX1), (destY2 - destY1), buffer, frame.hasAlpha());
        }
      });
    }
  }

  _mousePrevX = _mouseCursor->x();
  _mousePrevY = _mouseCursor->y();

  DrawMouseCursor();
  lfbStats.start();
  optimized_memcpy(_mappedLFBAddress, _zBuffer, _lfbSize);
  lfbStats.end();
  gvtStats.end();

  return true;
}

void GraphicsVideo::DebugPrint() {
}

void GraphicsVideo::NeedRefresh() {
  _needRefresh.set(true);
}

void GraphicsVideo::FillRect(unsigned sx, unsigned sy, unsigned width, unsigned height, unsigned color) {
  unsigned y_offset;
  for(unsigned y = sy; y < (sy + height) && y < _height; ++y)
  {
    y_offset = y * _pitch;
    for(unsigned x = sx; x < (sx + width) && x < _width; ++x)
    {
      auto p = (unsigned*)(_zBuffer + y_offset + x * _bytesPerPixel);
      *p = (color | upanui::GCoreFunctions::ALPHA_MASK);
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
    _mouseChange.set(true);
  }
}

upan::option<int> GraphicsVideo::getFGProcessUnderMouseCursor() {
  upan::mutex_guard g(_fgProcessMutex);

  for(auto it = _fgProcesses.rbegin(); it != _fgProcesses.rend();) {
    const auto pid = *it;
    ++it;
    auto process = ProcessManager::Instance().GetProcess(pid);
    if (process.isEmpty()) {
      removeFGProcess(pid);
    } else {
      if (!process.value().getGuiFrame().isEmpty()) {
        const auto& f = process.value().getGuiFrame().value();
        if (f.viewport().x1() <= _mouseCursor->x() && _mouseCursor->x() < f.viewport().x2()
        && f.viewport().y1() <= _mouseCursor->y() && _mouseCursor->y() < f.viewport().y2()) {
          return upan::option<int>(pid);
        }
      }
    }
  }
  return upan::option<int>::empty();
}

void GraphicsVideo::addGuiBase(int pid) {
  upan::mutex_guard g(_fgProcessMutex);

  const auto firstPid = _fgProcesses.front();

  ProcessManager::Instance().GetProcess(firstPid).ifPresent([&](Process& p) {
    if (p.isGuiBase()) {
      _guiBaseStack.push_back(firstPid);
      if (_inputEventFGProcess == firstPid) {
        _inputEventFGProcess = pid;
      }
      _fgProcesses.pop_front();
    }
  });

  _fgProcesses.erase(pid);
  _fgProcesses.push_front(pid);
}

void GraphicsVideo::removeGuiBase(int pid) {
  upan::mutex_guard g(_fgProcessMutex);

  _guiBaseStack.erase(pid);
  if (!_guiBaseStack.empty()) {
    _fgProcesses.push_front(_guiBaseStack.back());
    _guiBaseStack.pop_back();
    if (_inputEventFGProcess == pid) {
      _inputEventFGProcess = _fgProcesses.front();
    }
  }
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
  CopyArea(_mouseCursor->x(), _mouseCursor->y(),
           0, 0, _mouseCursor->width(),
           _mouseCursor->width(), _mouseCursor->height(),
           _mouseCursor->data(), true);
}

void GraphicsVideo::CopyArea(const uint32_t destX, const uint32_t destY,
                             const uint32_t srcX, const uint32_t srcY,
                             const uint32_t srcBufferWidth,
                             const uint32_t drawWidth, const uint32_t drawHeight,
                             const uint32_t* src, const bool checkAlpha) {
  upanui::GCoreFunctions::PixelCache pixelCache;

  for(auto sy = srcY, dy = destY; dy < (destY + drawHeight) && dy < _height; ++dy, ++sy) {
    const auto destOffset = dy * _width;
    const auto srcOffset = sy * srcBufferWidth;

    bool hasAlpha = false;
    if (checkAlpha) {
      for (auto sx = srcX, dx = destX; sx < (srcX + drawWidth) && dx < _width; ++sx, ++dx) {
        if (src[sx + srcOffset] >> 24 != upanui::GCoreFunctions::MAX_ALPHA) {
          hasAlpha = true;
          break;
        }
      }
    }

    if (hasAlpha) {
      for (auto sx = srcX, dx = destX; sx < (srcX + drawWidth) && dx < _width; ++sx, ++dx) {
        upanui::GCoreFunctions::setPixel(((uint32_t *) _zBuffer)[dx + destOffset], src[sx + srcOffset], pixelCache, false);
      }
    } else {
      const auto lineWidth = upan::min(_width - destX, drawWidth) * _bytesPerPixel;
      memcpy(&((uint32_t *) _zBuffer)[destX + destOffset], &src[srcX + srcOffset], lineWidth);
    }
  }
}

GraphicsVideo::RedrawInfo GraphicsVideo::isDirty() {
  upan::mutex_guard g(_fgProcessMutex);
  RedrawInfo redrawInfo;

  redrawInfo._mouseChanged = _mouseChange.set(false);
  redrawInfo._processChanged = _needRefresh.set(false);
  if (redrawInfo._processChanged) {
    return redrawInfo;
  }

  bool dirty = false;

  for(auto it = _fgProcesses.begin(); it != _fgProcesses.end();) {
    const auto pid = *it;
    ++it;
    auto process = ProcessManager::Instance().GetProcess(pid);
    if (process.isEmpty()) {
      removeFGProcess(pid);
    } else {
      process.value().getGuiFrame().ifPresent([&dirty](RootFrame &f) {
        dirty |= f.isDirty();
        f.clean();
      });
    }
  }
  redrawInfo._processChanged = dirty;
  return redrawInfo;
}

void GraphicsVideo::addFGProcess(int pid) {
  upan::mutex_guard g(_fgProcessMutex);
  _fgProcesses.push_back(pid);
  _inputEventFGProcess = pid;
}

void GraphicsVideo::removeFGProcess(int pid) {
  upan::mutex_guard g(_fgProcessMutex);

  if (ProcessManager::Instance().GetProcess(pid).map<bool>([](Process& p) { return p.isGuiBase(); }).valueOrElse(false)) {
    removeGuiBase(pid);
  }
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
