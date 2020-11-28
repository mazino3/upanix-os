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
#ifndef _UPAN_STRING_H_
#define _UPAN_STRING_H_

#ifdef __LOCAL_TEST__
#include "/usr/include/string.h"
#else
#include <Global.h>
#include <cstring.h>
#endif

#include <stdio.h>

namespace upan {

class string
{
public:
  string()
  {
    init();
  }

  string(const char* v)
  {
    if(v == nullptr)
      init();
    else
    {
      _len = strlen(v);
      _capacity = _len + 1;
      _buffer = new char[_capacity];
      strcpy(_buffer, v);
    }
  }

  string(const char* v, int len)
  {
    _len = len;
    _capacity = _len + 1;
    _buffer = new char[_capacity];
    strncpy(_buffer, v, len);
    _buffer[_len] = '\0';
  }

  string(const string& r)
  {
    _len = r.length();
    _capacity = _len + 1;
    _buffer = new char[_capacity];
    strcpy(_buffer, r.c_str());
  }

  string(string&& r) : _buffer(r._buffer), _len(r.length()), _capacity(r._capacity)
  {
    r._buffer = nullptr;
  }

  ~string()
  {
    delete[] _buffer;
  }

  string& operator=(const string& r)
  {
    _len = r.length();
    if(_capacity <= _len)
    {
      delete[] _buffer;
      _capacity = _len + 1;
      _buffer = new char[_capacity];
    }
    strcpy(_buffer, r.c_str());
    return *this;
  }

  string& operator=(string&& r)
  {
    delete[] _buffer;

    _len = r.length();
    _capacity = r._capacity;
    _buffer = r._buffer;

    r._buffer = nullptr;
    return *this;
  }

  string operator+(const string& r) const
  {
    if (r.length() == 0)
      return *this;
    string temp(*this);
    return temp += r;
  }

  string& operator+=(const string& r)
  {
    if (r.length() > 0)
    {
      _len += r.length();
      if(_capacity <= _len)
      {
        _capacity = (_len + 1) * 1.5;
        char* temp = new char[_capacity];
        strcpy(temp, _buffer);
        delete[] _buffer;
        _buffer = temp;
      }
      strcat(_buffer, r.c_str());
    }
    return *this;
  }

  bool operator<(const string& r) const
  {
    return strcmp(c_str(), r.c_str()) < 0;
  }

  bool operator==(const string& r) const
  {
    return strcmp(c_str(), r.c_str()) == 0;
  }

  bool operator!=(const string& r) const
  {
    return !(*this == r);
  }

  const char* c_str() const
  {
    return _buffer;
  }

  int length() const { return _len; }

  const char operator[](int index) const;

private:
  void init()
  {
    _capacity = 8;
    _len = 0;
    _buffer = new char[_capacity];
    _buffer[0] = '\0';
  }

private:
  char* _buffer;
  int   _len;
  int   _capacity;

public:
  static upan::string to_string(uint32_t uiNumber) {
    char strNumber[128];
    unsigned i = 0;

    do {
      strNumber[i++] = (uiNumber % 10) + 0x30;
      uiNumber /= 10;
      if(i == 128)
        return "";
    }
    while(uiNumber) ;

    strNumber[i] = '\0';
    strreverse(strNumber);
    return strNumber;
  }
};

};

#endif
