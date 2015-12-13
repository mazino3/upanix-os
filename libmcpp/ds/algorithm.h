/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2015 'Prajwala Prabhakar' 'srinivasa_prajwal@yahoo.co.in'
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
#ifndef _ALGO_H_
#define _ALGO_H_

namespace upan {

template <typename T>
class equal_to final
{
  public:
    equal_to(const T& value) : _value(value) {}
    bool operator()(const T& rhs) const { return _value == rhs; }
  private:
    const T _value;
};

template <typename IT>
IT find(IT begin, IT end, const typename IT::value_type& value)
{
  auto i = begin;
  for(; i != end; ++i)
    if(*i == value)
      break;
  return i;
}

template <typename IT, typename MATCH>
IT find_if(IT begin, IT end, MATCH match)
{
  auto i = begin;
  for(; i != end; ++i)
    if(match(*i))
      break;
  return i;
}

}

#endif
