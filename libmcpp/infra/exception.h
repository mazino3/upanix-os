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
#ifndef _EXERR_H_
#define _EXERR_H_

#include <cstring.h>
#include <StringUtil.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

namespace upan {

#define XLOC __FILE__, __LINE__

class exception
{
  public:
    exception(const upan::string& fileName, unsigned lineNo, const char * __restrict fmsg, ...) :
      _fileName(fileName), _lineNo(lineNo)
    {
      va_list arg;

      const int BSIZE = 1024;
      char buf[BSIZE];

      va_start(arg, fmsg);

      vsnprintf(buf, BSIZE, fmsg, arg);
      _msg = buf;
      _error = _fileName + ":" + ToString(_lineNo) + " " + _msg;

      va_end(arg);
    }

    const upan::string& File() const { return _fileName; }
    unsigned LineNo() const { return _lineNo; }
    const upan::string& Msg() const { return _msg; }
    const upan::string& Error() const 
    {
      return _error;
    }
    void Print() const
    {
      printf("\n%s\n", _error.c_str());
    }
  private:
    const upan::string _fileName;
    const unsigned _lineNo;
    upan::string _msg;
    upan::string _error;
};

};

#endif
