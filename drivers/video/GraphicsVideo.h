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

#include <MultiBoot.h>
#include <KernelUtil.h>
#include <usfncontext.h>
#include <BmpImage.h>

class GraphicsVideo : protected KernelUtil::TimerTask
{
  private:
    GraphicsVideo(const framebuffer_info_t&);

  public:
    static void Create();
    static GraphicsVideo* Instance() { return _instance; }
    unsigned FlatLFBAddress() const { return _flatLFBAddress; }
    void MappedLFBAddress(unsigned a)
    {
      _mappedLFBAddress = a;
      _zBuffer = a;
    }
    unsigned LFBSize() const { return _lfbSize; }
    void SetPixel(unsigned x, unsigned y, unsigned color);
    void FillRect(unsigned sx, unsigned sy, unsigned width, unsigned height, unsigned color);
    void DrawChar(byte ch, unsigned x, unsigned y, unsigned fg, unsigned bg);
    void ScrollDown();
    void CreateRefreshTask();
    void Initialize();
    void DrawCursor(uint32_t x, uint32_t y, uint32_t color);


    int GetMouseX() {
      return _mouseX;
    }
    int GetMouseY() {
      return _mouseY;
    }
    void SetMouseCursorPos(int x, int y);
    void ExperimentWithMouseCursor(int i);

  private:
    void InitializeUSFN();
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
    uint32_t _needRefresh;
    byte     _bpp;
    byte     _bytesPerPixel;

    usfn::Context* _ssfnContext;
    bool     _usfnInitialized;
    uint32_t _xCharScale;
    uint32_t _yCharScale;
    int _mouseX;
    int _mouseY;
    upan::uniq_ptr<upanui::BmpImage> _mouseCursorImg;
};