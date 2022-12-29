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

#include <RootConsole.h>
#include <TextWriter.h>
#include <FrameBuffer.h>
#include <Viewport.h>
#include <BaseFrame.h>
#include <timer_thread.h>
#include <RootFrame.h>
#include <UIObject.h>
#include <DrawBuffer.h>

namespace upanui {
  class MouseEvent;
  class MouseEventHandler;
  class KeyboardEvent;
  class Layout;
}

class RootGUIConsole : public RootConsole {
private:
  static RootGUIConsole* _instance;
  static void Create();
  RootGUIConsole(const upanui::FrameBuffer& frameBuffer, const upanui::Viewport& viewport);

public:
  static RootGUIConsole& Instance();

  void resetFrameBuffer(uint32_t frameBufferAddress);
  void setFontContext(upanui::usfn::Context*);
  RootFrame& frame() {
    return _frame;
  }

private:
  void gotoCursor() override;
  void putChar(int iPos, byte ch, const upanui::CharStyle& attr) override;
  void scrollDown() override;
  void putCursor(bool show);

  class CursorBlink : public upan::timer_thread {
  public:
    explicit CursorBlink(RootGUIConsole& console);
    void on_timer_trigger() override;
    RootGUIConsole& _console;
    bool _showCursorToggle;
  };
  void StartCursorBlink() override;

  class ConsoleUIObject : public upanui::UIObject {
  public:
    explicit ConsoleUIObject(RootFrame& frame) : _frame(frame) {
      _drawBuffer.initLocal(frame.frameBuffer());
    }
    ~ConsoleUIObject() override = default;

    upanui::DrawBuffer& drawBuffer() override { return _drawBuffer; }
    void draw() override { _frame.touch(); }

  private:
    int x() const override { return 0; }
    int y() const override { return 0; }
    uint32_t width() const override { return _frame.frameBuffer().width(); }
    uint32_t height() const override { return _frame.frameBuffer().height(); }
    uint32_t backgroundColor() const override { return 0; }
    uint32_t backgroundColorForDraw() const override { return 0; }
    uint8_t  backgroundColorAlpha() const override { return 100; }
    uint32_t borderColor() const override { return 0; }
    uint8_t  borderColorAlpha() const { return 100; }
    uint32_t borderThickness() const override { return 0; }

    void x(const int) override {}
    void y(const int) override {}
    void xy(int, int) override {}
    void width(const uint32_t) override {}
    void height(const uint32_t) override {}
    void backgroundColor(const uint32_t color) override {}
    void backgroundColorAlpha(const uint8_t) override {}
    void borderColor(const uint32_t) override {}
    void borderColorAlpha(const uint8_t) override {}
    void borderThickness(const uint32_t) override {}

    UIObject& parent() const override {
      throw upan::exception(XLOC, "unsupported parent()");
    }
    const upan::list<UIObject*>& children() override {
      throw upan::exception(XLOC, "unsupported children()");
    }
    upanui::Layout& layout() override {
      throw upan::exception(XLOC, "unsupported layout()");
    }

    void add(UIObject& child) override {}
    void remove() override {}
    void redraw() override {}

    void registerMouseEventHandler(upanui::MouseEventHandler& handler) override {}
    bool hasAlpha() override { return false; }
    upan::option<UIObject&> uiObjectUnderCursor(const int x, const int y) override { return upan::option<UIObject&>::empty(); }

    bool captureMouseEvents() const override {
      return true;
    }

    void captureMouseEvents(bool) {}

    void onKeyboardEvent(const upanui::KeyboardEvent& event) override {}
    void onMouseEvent(const upanui::MouseEvent& event) override {}
    void onMouseFocus() override {}
    void onLoseMouseFocus() override {}

    int drawX() const override { return 0; }
    int drawY() const override { return 0; }
    void drawTopDown() override {}
    void drawToTop() override {}

    void positionChanged() override {}
    void sizeChanged() override {}
    void contentChanged() override {}

  private:
    RootFrame& _frame;
    upanui::DrawBuffer _drawBuffer;
  };

private:
  upanui::TextWriter _textWriter;
  RootFrame _frame;
  int _cursorPos;
  bool _cursorEnabled;
  upan::mutex _cursorMutex;
  ConsoleUIObject _consoleUIObject;

  friend class RootConsole;
  friend class MemManager;
};