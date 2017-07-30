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
#ifndef _UPAN_VECTOR_H_
#define _UPAN_VECTOR_H_

#ifdef __LOCAL_TEST__
#include "/usr/include/string.h"
#else
#include <cstring.h>
#include <Alloc.h>
#endif
#include <exception.h>
#include <algorithm.h>

namespace upan {

template <typename T>
class vector
{
  public:
    class vector_iterator;
    typedef vector_iterator iterator;
    typedef const vector_iterator const_iterator;

    vector();
    vector(const vector<T>& rhs);
    ~vector();

    vector& operator=(const vector& rhs);

    void insert(const int index, const T& v);
    void insert(const_iterator pos, const T& v);
    void push_back(const T& value);

    iterator erase(const T& value);
    iterator erase(const_iterator& it);
    iterator erase(const_iterator& first, const_iterator& last);
    iterator erase(const int first, const int last);
    void pop_back();

    unsigned size() const { return _size; }
    bool empty() const { return _size == 0; }

    iterator begin() const;
    iterator end() const;

    void clear();
    void swap(vector& other);
    T& operator[](int index) const;

  public:
    class vector_iterator
    {
      private:
        vector_iterator(const vector<T>& parent, const int index) : _parent(&parent), _index(index)
        {
        }
      public:
        typedef T value_type;

        T& operator*() { return const_cast<T&>(value()); }
        const T& operator*() const { return value(); }

        T& operator->() { return const_cast<T&>(value()); }
        const T& operator->() const { return value(); }

        vector_iterator& operator++() { return const_cast<vector_iterator&>(pre_inc()); }
        const vector_iterator& operator++() const { return pre_inc(); }

        vector_iterator operator++(int) { return post_inc(); }
        vector_iterator operator++(int) const { return post_inc(); }

        vector_iterator& operator--() { return const_cast<vector_iterator&>(pre_dec()); }
        const vector_iterator& operator--() const { return pre_dec(); }

        vector_iterator operator--(int) { return post_dec(); }
        vector_iterator operator--(int) const { return post_dec(); }

        vector_iterator operator+(const int delta) const
        {
          const int newIndex = _index + delta;
          validate_index(newIndex);
          return vector_iterator(*_parent, newIndex);
        }

        vector_iterator operator-(const int delta) const
        {
          const int newIndex = _index - delta;
          validate_index(newIndex);
          return vector_iterator(*_parent, newIndex);
        }

        bool operator==(const vector_iterator& rhs) const { return _index == rhs._index; }
        bool operator!=(const vector_iterator& rhs) const { return !operator==(rhs); }
        bool operator<(const vector_iterator& rhs) const { return _index < rhs._index; }
        bool operator>(const vector_iterator& rhs) const { return _index > rhs._index; }

        vector_iterator& operator=(const vector_iterator& rhs)
        {
          _index = rhs._index;
          _parent = rhs._parent;
          return *this;
        }

      private:
        void validate_index(const int index) const
        {
          if(index < 0 || index > _parent->size())
            throw upan::exception(XLOC, "vector index %d out of range", index);
        }
        const T& value() const
        {
          if(_index == _parent->size())
            throw upan::exception(XLOC, "accessing end iterator on vector");
          return (*_parent)[_index];
        }
        const vector_iterator& pre_inc() const
        {
          const int newIndex = _index + 1;
          validate_index(newIndex);
          _index = newIndex;
          return *this;
        }
        vector_iterator post_inc() const
        {
          vector_iterator tmp(*_parent, _index);
          pre_inc();
          return tmp;
        }
        const vector_iterator& pre_dec() const
        {
          const int newIndex = _index - 1;
          validate_index(newIndex);
          _index = newIndex;
          return *this;
        }
        vector_iterator post_dec() const
        {
          vector_iterator tmp(*_parent, _index);
          pre_dec();
          return tmp;
        }
      private:
        const vector<T>* _parent;
        mutable int _index;
        friend class vector<T>;
    };
    friend class vector_iterator;

#ifdef __LOCAL_TEST__
  public:
#else
  private:
#endif
    void expandIfNeeded();

    int    _size;
    int    _capacity;
    char*  _buffer;
};

template <typename T>
vector<T>::vector() : _size(0), _capacity(0), _buffer(nullptr)
{
  _buffer = new char[_capacity];
}

template <typename T>
vector<T>::vector(const vector<T>& rhs) : _size(rhs._size), _capacity(rhs._capacity), _buffer(nullptr)
{
  _buffer = new char[_capacity];
  memcpy(_buffer, rhs._buffer, _size * sizeof(T));
}

template <typename T>
vector<T>::~vector()
{
  delete[] _buffer;
}

template <typename T>
vector<T>& vector<T>::operator=(const vector<T>& rhs)
{
  vector<T> temp(rhs);
  this->swap(temp);
}

template <typename T>
void vector<T>::swap(vector<T>& other)
{
  int tSize = other._size;
  other._size = this->_size;
  this->_size = tSize;

  int tCapacity = other._capacity;
  other._capacity = this->_capacity;
  this->_capacity = tCapacity;

  char* tBuffer = other._buffer;
  other._buffer = this->_buffer;
  this->_buffer = tBuffer;
}

template <typename T>
void vector<T>::expandIfNeeded()
{
  if((_size * sizeof(T)) >= _capacity)
  {
    const uint32_t newCapacity = sizeof(T) * (int)(_size == 0 ? 2 : _size * 1.5);
    char* tempBuffer = new char[newCapacity];
    memcpy(tempBuffer, _buffer, _capacity);
    delete[] _buffer;
    _buffer = tempBuffer;
    _capacity = newCapacity;
  }
}

template <typename T>
void vector<T>::insert(const int index, const T& v)
{
  expandIfNeeded();
  if(index < 0 || index > _size)
    throw upan::exception(XLOC, "invalid index %d for insert", index);
  if(index < _size)
    memmove(_buffer + (index + 1) * sizeof(T), _buffer + index * sizeof(T), (_size - index) * sizeof(T));
  new (_buffer + index * sizeof(T))T(v);
  ++_size;
}

template <typename T>
void vector<T>::insert(iterator pos, const T& v)
{
  insert(pos._index, v);
}

template <typename T>
void vector<T>::push_back(const T& value)
{
  expandIfNeeded();
  insert(_size, value);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(vector<T>::const_iterator& it)
{
  return erase(it, it + 1);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(vector<T>::const_iterator& first, vector<T>::const_iterator& last)
{
  return erase(first._index, last._index);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const int first, const int last)
{
  if(first < 0 || first >= _size || last < 0 || last > _size || last <= first)
    throw upan::exception(XLOC, "vector index first:%d and last:%d out of bounds while deleting elements", first, last);

  const int remaining = _size - last;
  memcpy(_buffer + first * sizeof(T), _buffer + last * sizeof(T), remaining * sizeof(T));
  _size -= (last - first);
  return vector_iterator(*this, first);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const T& v)
{
  return erase(find(begin(), end(), v));
}

template <typename T>
void vector<T>::pop_back()
{
  erase(end() - 1);
}

template <typename T>
typename vector<T>::iterator vector<T>::begin() const
{
  return vector<T>::iterator(*this, 0);
}

template <typename T>
typename vector<T>::iterator vector<T>::end() const
{
  return vector<T>::iterator(*this, _size);
}

template <typename T>
void vector<T>::clear()
{
  for(int i = 0; i < _size; ++i)
    reinterpret_cast<T*>(_buffer)[i].~T();
  _size = 0;
}

template <typename T>
T& vector<T>::operator[](int index) const
{
  if(index < 0 || index >= _size)
    throw upan::exception(XLOC, "vector index %d is out-of-bounds", index);
  return reinterpret_cast<T*>(_buffer)[index];
}

};

#endif
