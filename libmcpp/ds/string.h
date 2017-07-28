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
		string() : _pVal(0), _len(0)
		{
			_sVal[0] = '\0';
		}

		string(const char* v) : _pVal(0), _len(0)
		{
			Value(v);
		}

		~string()
		{
			DeletePtr();
		}

		string(const string& r) : _pVal(0)
		{
			Value(r.c_str());
		}

    const char operator[](unsigned index) const;

		string& operator=(const string& r)
		{
      Value(r.c_str());
			return *this;
		}

    string operator+(const string& r) const
    {
      string temp(*this);
      return temp += r;
    }

		string& operator+=(const string& r)
		{
      unsigned newLen = _len + r.length();
			if(Large(newLen))
			{
				char* newVal = AllocPtr(newLen);
				strcpy(newVal, c_str());
				strcat(newVal, r.c_str());
				DeletePtr();
				_pVal = newVal;
			}
			else
				strcat(_sVal, r.c_str());
      _len = newLen;
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
			if(Large())
				return _pVal;
			return _sVal;
		}

		unsigned length() const { return _len; }

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

};

#endif
