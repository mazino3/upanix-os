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
#ifndef _SLIST_H_
#define _SLIST_H_

#include <exception.h>
#include <algorithm.h>

namespace upan {

template <typename T>
class list
{
  public:
    class list_iterator;
    typedef list_iterator iterator;
    typedef const list_iterator const_iterator;

    list();
    list(const list<T>& rhs);
    ~list();

    void push_back(const T& value);
    void push_front(const T& value);
    void pop_front();
    void pop_back();
    T& front();
    const T& front() const;
    T& back();
    const T& back() const;
    bool erase(const_iterator& it);
    bool erase(const T& v);
    iterator begin() const;
    iterator end() const;
    unsigned size() const { return _size; }
    bool empty() const { return _size == 0; }
    iterator insert(iterator pos, const T& v);
    iterator sorted_insert_asc(const T& v);
    iterator sorted_insert_desc(const T& v);

  private:
    void pop(bool front);

    class node
    {
      public:
        node(const T& value) : _value(value), _next(nullptr), _prev(nullptr)
        {
        }
        node(const node&);

        node* next() { return _next; }
        const node* next() const { return _next; }
        void next(node* n) { _next = n; }

        node* prev() { return _prev; }
        const node* prev() const { return _prev; }
        void prev(node* p) { _prev = p; }

        T& value() { return _value; }
        const T& value() const { return _value; }
        void value(const T& value) { _value = value; }
      private:
        T     _value;
        node* _next;
        node* _prev;
    };

  public:
    class list_iterator
    {
      private:
        list_iterator(const list<T>* parent, node* n) : _parent(parent), _node(n)
        {
        }
      public:
        list_iterator() : _parent(nullptr), _node(nullptr)
        {
        }
        typedef T value_type;

        T& operator*() { return const_cast<T&>(value()); }
        const T& operator*() const { return value(); }

        T& operator->() { return const_cast<T&>(value()); }
        const T& operator->() const { return value(); }

        list_iterator& operator++() { return const_cast<list_iterator&>(pre_inc()); }
        const list_iterator& operator++() const { return pre_inc(); }

        list_iterator operator++(int) { return post_inc(); }
        list_iterator operator++(int) const { return post_inc(); }

        bool operator==(const list_iterator& rhs) const { return _node == rhs._node; }
        bool operator!=(const list_iterator& rhs) const { return !operator==(rhs); }
      private:
        bool is_end() const { return _node == nullptr; }
        const T& value() const
        {
          check_end();
          return _node->value();
        }
        void check_end() const
        {
          if(is_end())
            throw exception(XLOC, "list: accessing end iterator");
        }
        const list_iterator& pre_inc() const
        {
          check_end();
          if(_parent->_first == _node->next())
            _node = nullptr;
          else
            _node = _node->next();
          return *this;
        }
        list_iterator post_inc() const
        {
          list_iterator tmp(*this);
          pre_inc();
          return tmp;
        }
      private:
        const list<T>* _parent;
        mutable node*   _node;
        friend class list<T>;
    };
    friend class list_iterator;
  private:
    unsigned _size;
    node*    _first;
};

template <typename T>
list<T>::list() : _size(0), _first(nullptr)
{
}

template <typename T>
list<T>::list(const list<T>& rhs)
{
  for(const auto& i : rhs)
    push_back(i);
}

template <typename T>
list<T>::~list()
{
  iterator it = begin();
  for(; it != end(); ++it)
    delete it._node;
}

template <typename T>
void list<T>::push_back(const T& value)
{
  list<T>::node* n = new list<T>::node(value);
  if(empty())
  {
    _first = n;
    _first->next(n);
    _first->prev(n);
  }
  else
  {
    auto last = _first->prev();
    last->next(n);
    n->prev(last);
    n->next(_first);
    _first->prev(n);
  }
  ++_size;
}

template <typename T>
void list<T>::push_front(const T& value)
{
  push_back(value);
  _first = _first->prev();
}

template <typename T>
bool list<T>::erase(list<T>::const_iterator& it)
{
  if(it == end())
    return false;
  if(empty())
    return false;
  node* cur = it._node;
  if(_size == 1)
  {
    if(_first == cur)
      _first = nullptr;
    else
      return false;
  }
  else
  {
    cur->prev()->next(cur->next());
    cur->next()->prev(cur->prev());
    if(cur == _first)
      _first = cur->next();
  }
  delete cur;
  --_size;
  return true;
}

template <typename T>
bool list<T>::erase(const T& v)
{
  return erase(find(begin(), end(), v));
}

template <typename T>
void list<T>::pop(bool front)
{
  if(empty())
    throw exception(XLOC, "pop_back: list is empty");
  node* cur = front ? _first : _first->prev();
  if(_size == 1)
  {
    _first = nullptr;
  }
  else
  {
    cur->prev()->next(cur->next());
    cur->next()->prev(cur->prev());
    if(front);
      _first = cur->next();
  }
  delete cur;
  --_size;
}

template <typename T>
void list<T>::pop_front()
{
  pop(true);
}

template <typename T>
void list<T>::pop_back()
{
  pop(false);
}

template <typename T>
T& list<T>::front()
{
  if(empty())
    throw exception(XLOC, "list is empty");
  return _first->value();
}

template <typename T>
const T& list<T>::front() const
{
  const_cast<list<T>*>(this)->front();
}

template <typename T>
T& list<T>::back()
{
  if(empty())
    throw exception(XLOC, "list is empty");
  return _first->prev()->value();
}

template <typename T>
const T& list<T>::back() const
{
  const_cast<list<T>*>(this)->back();
}

template <typename T>
typename list<T>::iterator list<T>::insert(iterator pos, const T& v)
{
  ++_size;
  if(pos == end())
  {
    push_back(v);
    return begin();
  }
  if(pos == begin())
  {
    push_front(v);
    return begin();
  }
  node* new_node = new node(v);
  node* cur = pos._node;
  cur->prev()->next(new_node);
  new_node->prev(cur->prev());
  new_node->next(cur);
  cur->prev(new_node);
  return iterator(this, new_node);
}

template <typename T>
typename list<T>::iterator list<T>::sorted_insert_asc(const T& v)
{
  iterator i = begin();
  for(; i != end(); ++i)
    if(v < *i || !(*i < v))
      break;
  return insert(i, v);
}

template <typename T>
typename list<T>::iterator list<T>::sorted_insert_desc(const T& v)
{
  iterator i = begin();
  for(; i != end(); ++i)
    if(*i < v || !(v < *i))
      break;
  return insert(i, v);
}

template <typename T>
typename list<T>::iterator list<T>::begin() const 
{ 
  return list<T>::iterator(this, _first); 
}

template <typename T>
typename list<T>::iterator list<T>::end() const 
{ 
  return list<T>::iterator(); 
}

};

#endif
