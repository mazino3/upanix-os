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
#pragma once

#include <MultiBoot.h>
#include <KernelUtil.h>
#include <usfncontext.h>
#include <BmpImage.h>
#include <atomicop.h>
#include <list.h>
#include <mutex.h>

class GraphicsVideo : protected KernelUtil::TimerTask
{
  private:
    GraphicsVideo(const framebuffer_info_t&);

  public:
    static void Create();
    static GraphicsVideo& Instance();
    unsigned FlatLFBAddress() const { return _flatLFBAddress; }
    void MappedLFBAddress(unsigned a)
    {
      _mappedLFBAddress = a;
      _zBuffer = a;
    }
    unsigned LFBSize() const { return _lfbSize; }
    uint32_t LFBPageCount() const { return _lfbPageCount; }

    void SetPixel(unsigned x, unsigned y, unsigned color);
    void FillRect(unsigned sx, unsigned sy, unsigned width, unsigned height, unsigned color);
    void CreateRefreshTask();
    void Initialize();

    void SetMouseCursorPos(int x, int y);
    bool switchFGProcessOnMouseClick();

    void addFGProcess(int pid);
    void removeFGProcess(int pid);
    upan::option<int> getActiveFGProcess();

    uint32_t allocateFrameBuffer();

  private:
    bool isDirty();
    bool TimerTrigger() override;
    void NeedRefresh();
    void DrawUSFNChar(byte ch, unsigned x, unsigned y, unsigned fg, unsigned bg);
    void CopyArea(unsigned sx, unsigned sy, uint32_t width, uint32_t height, const uint32_t* src, bool directWrite);
    void DrawMouseCursor();

    static GraphicsVideo* _instance;
    unsigned _flatLFBAddress;
    unsigned _mappedLFBAddress;
    unsigned _zBuffer;
    unsigned _pitch;
    unsigned _width;
    unsigned _height;
    unsigned _lfbSize;
    uint32_t _lfbPageCount;
    upan::atomic::integral<bool> _needRefresh;
    byte     _bpp;
    byte     _bytesPerPixel;

    upanui::usfn::Context* _ssfnContext;
    bool     _initialized;
    uint32_t _xCharScale;
    uint32_t _yCharScale;
    upanui::Image* _mouseCursorImg;
    upan::list<int> _fgProcesses;
    upan::mutex _fgProcessMutex;
};