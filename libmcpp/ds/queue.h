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
#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <exception.h>

namespace upan {

template <typename T>
class queue
{
  public:
    queue(unsigned size);
    ~queue();

    bool empty() const;
    bool full() const;
    T& front();
    const T& front() const;
    bool pop_front();
    bool push_back(const T& data);
    void clear();

  private:
    unsigned short _readEnd;
    unsigned short _writeEnd;
    unsigned       _size;
    T*             _buffer;
};

template <typename T>
queue<T>::queue(unsigned size) : _readEnd(0), _writeEnd(0), _size(size), _buffer(nullptr)
{
  _buffer = new T[_size];
}

template <typename T>
queue<T>::~queue()
{
  delete[] _buffer;
}

template <typename T>
bool queue<T>::empty() const
{
  return _readEnd == _writeEnd;
}

template <typename T>
bool queue<T>::full() const
{
  return ((_writeEnd + 1) % _size) == _readEnd;
}

template <typename T>
T& queue<T>::front()
{
  if(empty())
    throw exception(XLOC, "queue is empty");
  return _buffer[_readEnd];
}

template <typename T>
const T& queue<T>::front() const
{
  const_cast<T*>(this)->front();
}

template <typename T>
bool queue<T>::pop_front()
{
  if(empty())
    return false;
	_readEnd = (_readEnd + 1) % _size;
  return true;
}

template <typename T>
bool queue<T>::push_back(const T& data)
{
  if(full())
    return false;
	_buffer[_writeEnd] = data;	
	_writeEnd = (_writeEnd + 1) % _size;
	return true;
}

template <typename T>
void queue<T>::clear()
{
  _readEnd = _writeEnd = 0;
}

};

#endif
