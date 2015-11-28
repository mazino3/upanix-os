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
#ifndef _STRING_H_
#define _STRING_H_

#include <Global.h>
#include <string.h>

class String
{
	public:
		String() : _pVal(0), _len(0)
		{
			_sVal[0] = '\0';
		}

		String(const char* v) : _pVal(0), _len(0)
		{
			Value(v);
		}

		~String()
		{
			DeletePtr();
		}

		String(const String& r) : _pVal(0)
		{
			Value(r.Value());
		}

		String& operator=(const String& r)
		{
      Value(r.Value());
			return *this;
		}

		String& operator+=(const String& r)
		{
      _len += r.Length();
			if(Large())
			{
				char* newVal = AllocPtr(_len);
				strcpy(newVal, Value());
				strcat(newVal, r.Value());
				DeletePtr();
				_pVal = newVal;
			}
			else
				strcat(_sVal, r.Value());
			return *this;
		}

		String operator+(const String& r) const
		{
			String temp(*this);
			return temp += r;
		}

		bool operator==(const String& r) const
		{
			return strcmp(Value(), r.Value()) == 0;
		}

		bool operator!=(const String& r) const
		{
			return !(*this == r);
		}

		const char* Value() const
		{
			if(Large())
				return _pVal;
			return _sVal;
		}

		unsigned Length() const { return _len; }

	private:
		static const unsigned FIXED_BUFFER = 64;

		bool Large(unsigned len) const { return len > FIXED_BUFFER - 1; }
		bool Large() const { return Large(_len); }

		void Value(const char* v)
		{
			DeletePtr();
			_len = strlen(v);
			char* dest = _sVal;
			if(Large())
				dest = _pVal = AllocPtr(_len);
			strcpy(dest, v);
		}

		char* AllocPtr(unsigned len)
		{
			return new char[len + 1];
		}

		void DeletePtr()
		{
			delete[] _pVal;
			_pVal = 0;
		}

		char  _sVal[FIXED_BUFFER];
		char* _pVal;
		unsigned _len;
};

#endif
