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
#ifndef _UPAN_EXERR_H_
#define _UPAN_EXERR_H_

#ifdef __LOCAL_TEST__

#include <stdarg.h>
#include <string>

namespace upan {

#define XLOC __FILE__, __LINE__

class exception
{
  public:
    exception(const std::string& fileName, unsigned lineNo, const std::string& msg)
    {
      printf("\n %s:%u - %s", fileName.c_str(), lineNo, msg.c_str());
    }

    exception(const std::string& fileName, unsigned lineNo, const char * __restrict fmsg, ...)
    {
      printf("\n %s:%u - ", fileName.c_str(), lineNo);
      va_list arg;
      va_start(arg, fmsg);
      vprintf(fmsg, arg);
      va_end(arg);
    }
};

};

#else

#include <ustring.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <ustring.h>

namespace upan {

#define XLOC __FILE__, __LINE__

class error
{
  private:
    error() {}

  public:
    error(const char * __restrict fmsg, ...)
    {
      va_list arg;

      const int BSIZE = 1024;
      char buf[BSIZE];

      va_start(arg, fmsg);

      vsnprintf(buf, BSIZE, fmsg, arg);
      _msg = buf;

      va_end(arg);
    }

    error(const char * __restrict fmsg, va_list arg)
    {
      const int BSIZE = 1024;
      char buf[BSIZE];
      vsnprintf(buf, BSIZE, fmsg, arg);
      _msg = buf;
    }

    error(upan::string& msg) : _msg(msg) {}

    const upan::string& Msg() const { return _msg; }

  private:
    upan::string _msg;

    template<typename Good> friend class result;
    friend class exception;
};

class exception
{
  public:
    exception(const upan::string& fileName, unsigned lineNo, const char * __restrict fmsg, ...) :
      _fileName(fileName), _lineNo(lineNo)
    {
      va_list arg;
      va_start(arg, fmsg);
      _error = upan::error(fmsg, arg);
      _msg = _fileName + ":" + string::to_string(_lineNo) + " " + _error.Msg();
      va_end(arg);
    }

    exception(const upan::string& fileName, unsigned lineNo, const upan::error& err) :
      _fileName(fileName), _lineNo(lineNo), _error(err)
    {
      _msg = _fileName + ":" + string::to_string(_lineNo) + " " + _error.Msg();
    }

    const upan::string& File() const { return _fileName; }
    unsigned LineNo() const { return _lineNo; }
    const error& Error() const { return _error; }
    const upan::string& ErrorMsg() const { return _error.Msg(); }
    void Print() const { printf("\n%s\n", _msg.c_str()); }

  private:
    const upan::string _fileName;
    const unsigned _lineNo;
    upan::error _error;
    upan::string _msg;
};

};

#endif

#endif
