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

#include <string.h>
#include <exception.h>

namespace upan {

template <typename T>
class option
{
  protected:
    bool _isEmpty;
    T _value;
    option() : _isEmpty(true) {}

  public:
    option(const T& value) : _isEmpty(false), _value(value) {}
    
    static option<T> empty() {
      return option<T>();
    }
    bool isEmpty() const { return _isEmpty; }

    const T& value() const
    {
      if(_isEmpty)
        throw exception(XLOC, "Option is empty");
      return _value;
    }

    const T& valueOrThrow(const upan::string& fileName, unsigned lineNo, const upan::string& error) const
    {
      if(_isEmpty)
        throw exception(fileName, lineNo, error);
      return _value;
    }

    const T& valueOrElse(const T& defaultValue) const
    {
      if(_isEmpty)
        return defaultValue;
      return _value;
    }

    template <typename LAMBDA>
    bool ifPresent(const LAMBDA& lambdaf)
    {
      if(isEmpty())
        return false;
      lambdaf(_value);
      return true;
    }
};

template <typename T>
upan::option<T> toOption(const T& value) { return upan::option<T>(value); }

}