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
#ifndef _M_RESULT_H_
#define _M_RESULT_H_

#include <string.h>
#include <exception.h>

namespace upan {

template <typename Good>
class result
{
  protected:
    bool _isBad;
    Good _value;
    upan::error _error;

  public:
    result(const Good& value) : _isBad(false), _value(value) {}
    result(const upan::error& error) : _isBad(true), _error(error) {}

    static result<Good> bad(const char * __restrict fmsg, ...)
    {
      va_list arg;
      va_start(arg, fmsg);
      auto r = result<Good>(upan::error(fmsg, arg));
      va_end(arg);
      return r;
    }
    
    bool isBad() const { return _isBad; }
    bool isGood() const { return !isBad(); }

    const Good& goodValue() const
    {
      if(_isBad)
        throw exception(XLOC, "Result is bad - can't get Good value");
      return _value;
    }

    const Good& goodValueOrThrow(const upan::string& fileName, unsigned lineNo) const
    {
      if(_isBad)
        throw exception(fileName, lineNo, _error);
      return _value;
    }

    template <typename LAMBDA>
    bool goodMap(const LAMBDA& lambdaf)
    {
      if(isBad())
        return false;
      lambdaf(_value);
      return true;
    }

    const upan::error& badValue() const
    {
      if(!_isBad)
        throw exception(XLOC, "result is Good - can't get Error");
      return _error;
    }

    template <typename LAMBDA>
    bool badMap(const LAMBDA& lambdaf)
    {
      if(isGood())
        return false;
      lambdaf(_error);
      return true;
    }
};

template <typename Good>
upan::result<Good> good(const Good& value) { return upan::result<Good>(value); }

}

#endif
