/*
 *	Mother Operating System - An x86 based Operating System
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

#include <String.h>
#include <ctype.h>

#define XWHERE __FILE__, __LINE__

class Exerr
{
  public:
    Exerr(const String& fileName, unsigned lineNo, const String& msg) :
      _fileName(fileName), _lineNo(lineNo), _msg(msg)
    {
      _error = _fileName + ":" + ToString(_lineNo) + " " + _msg;
    }

    const String& File() const { return _fileName; }
    unsigned LineNo() const { return _lineNo; }
    const String& Msg() const { return _msg; }
    const String& Error() const 
    {
      return _error;
    }
  private:
    const String _fileName;
    const unsigned _lineNo;
    const String _msg;
    String _error;
};

#endif
