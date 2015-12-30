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
#ifndef _UPAN_SET_H_
#define _UPAN_SET_H_

#include <exception.h>
#include <pair.h>
#include <_tree.h>

namespace upan {

template <typename K>
class _set_type
{
  public:
    typedef const K value_type;
    typedef const K key_type;
    class _key_accessor
    {
      public:
        const K& operator()(const K& key) const
        {
          return key;
        }
    };
};

template <typename K>
class set : public _tree<_set_type<K>>
{
};

};

#endif
