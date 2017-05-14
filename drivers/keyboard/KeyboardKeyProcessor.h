#ifndef _KEYBOARDKEYPROCESSOR_H
#define _KEYBOARDKEYPROCESSOR_H

#include <Global.h>
#include <exception.h>

class KeyboardKeyProcessor
{
protected:
  KeyboardKeyProcessor() {}
  virtual ~KeyboardKeyProcessor() {}

  KeyboardKeyProcessor(const KeyboardKeyProcessor&) = delete;
  KeyboardKeyProcessor& operator=(const KeyboardKeyProcessor&) = delete;

public:
  virtual byte Data(byte rawKey) = 0;
  virtual byte MapChar(byte rawKey) = 0;
  virtual byte ShiftedMapChar(byte rawKey) = 0;
  virtual bool IsKeyRelease(byte rawKey) = 0;
  virtual bool IsExtraKey(byte rawKey) = 0;
  virtual int ExtraKey(byte rawKey) = 0;
  virtual bool IsKey(byte rawKey, byte val) = 0;
  virtual bool IsNormal(byte rawKey) = 0;
  virtual bool IsSpecial(byte rawKey) = 0;
};

class BuiltInKeyboardKeyProcessor : public KeyboardKeyProcessor
{
private:
  BuiltInKeyboardKeyProcessor() { }
public:
  static BuiltInKeyboardKeyProcessor& Instance()
  {
    static BuiltInKeyboardKeyProcessor instance;
    return instance;
  }

  byte Data(byte rawKey);
  byte MapChar(byte rawKey);
  byte ShiftedMapChar(byte rawKey);
  bool IsKeyRelease(byte rawKey);
  bool IsExtraKey(byte rawKey);
  int ExtraKey(byte rawKey);
  bool IsKey(byte rawKey, byte val);
  bool IsNormal(byte rawKey);
  bool IsSpecial(byte rawKey);
};

class USBKeyboardKeyProcessor : public KeyboardKeyProcessor
{
private:
  USBKeyboardKeyProcessor() { }
public:
  static USBKeyboardKeyProcessor& Instance()
  {
    static USBKeyboardKeyProcessor instance;
    return instance;
  }
  byte Data(byte rawKey) { throw upan::exception(XLOC, "unimplemented"); }
  byte MapChar(byte rawKey) { throw upan::exception(XLOC, "unimplemented"); }
  byte ShiftedMapChar(byte rawKey) { throw upan::exception(XLOC, "unimplemented"); }
  bool IsKeyRelease(byte rawKey) { throw upan::exception(XLOC, "unimplemented"); }
  bool IsExtraKey(byte rawKey) { throw upan::exception(XLOC, "unimplemented"); }
  int ExtraKey(byte rawKey) { throw upan::exception(XLOC, "unimplemented"); }
  bool IsKey(byte rawKey, byte val) { throw upan::exception(XLOC, "unimplemented"); }
  bool IsNormal(byte rawKey) { throw upan::exception(XLOC, "unimplemented"); }
  bool IsSpecial(byte rawKey) { throw upan::exception(XLOC, "unimplemented"); }
};

#endif
