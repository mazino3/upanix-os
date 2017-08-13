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
#ifndef TRY_H
#define TRY_H

#include <result.h>

namespace upan {

template <typename LAMBDA>
upan::result<bool> trycall(const LAMBDA& lambdaf)
{
  try
  {
    lambdaf();
    return upan::good(true);
  }
  catch(const upan::exception& e)
  {
    return upan::result<bool>(e.Error());
  }
}

template <typename LAMBDA>
upan::result<typename function_traits<LAMBDA>::return_type> tryreturn(const LAMBDA& lambdaf)
{
  try
  {
    return upan::good(lambdaf());
  }
  catch(const upan::exception& e)
  {
    return upan::result<typename function_traits<LAMBDA>::return_type>(e.Error());
  }
}

}

#endif // TRY_H
