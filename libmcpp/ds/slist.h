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

namespace upan {

template <typename T>
class slist
{
  public:
    class slist_iterator;
    typedef slist_iterator iterator;
    typedef const slist_iterator const_iterator;

    slist();
    slist(const slist<T>& rhs);
    ~slist();

    void push_back(const T& value);
    void push_front(const T& value);
    void erase(const_iterator& it);
    iterator begin() const;
    iterator end() const;

  private:
    class node
    {
      public:
        node(const T& value) : _value(value), _next(nullptr)
        {
        }
        node(const node&);

        node* next() { return _next; }
        const node* next() const { return _next; }
        void next(node* n) { _next = n; }

        T& value() { return _value; }
        const T& value() const { return _value; }
        void value(const T& value) { _value = value; }
      private:
        T     _value;
        node* _next;
    };

  public:
    class slist_iterator
    {
      private:
        explicit slist_iterator(node* n) : _node(n)
        {
        }
      public:
        typedef T value_type;
        slist_iterator(const slist_iterator& rhs)
        {
          copy(rhs);
        }
        slist_iterator& operator=(const slist_iterator& rhs)
        {
          copy(rhs);
        }

        T& operator*() { value(); }
        const T& operator*() const { value(); }

        T& operator->() { value(); }
        const T& operator->() const { value(); }

        slist_iterator& operator++() { return const_cast<slist_iterator&>(pre_inc()); }
        const slist_iterator& operator++() const { return pre_inc(); }

        slist_iterator operator++(int) { return post_inc(); }
        slist_iterator operator++(int) const { return post_inc(); }

        bool operator==(const slist_iterator& rhs) const { return _node == rhs._node; }
        bool operator!=(const slist_iterator& rhs) const { return !operator==(rhs); }
      private:
        bool is_end() const { return _node == nullptr; }
        void copy(const slist_iterator& rhs) { _node = rhs._node; }
        const T& value() const
        {
          check_end();
          _node->value();
        }
        void check_end() const
        {
          if(is_end())
            ;//throw Exerr(XLOC, "slist: accessing end iterator");
        }
        const slist_iterator& pre_inc() const
        {
          check_end();
          _node = _node->next();
          return *this;
        }
        slist_iterator post_inc() const
        {
          slist_iterator tmp(*this);
          pre_inc();
          return tmp;
        }
      private:
        mutable node* _node;
        friend class slist<T>;
    };
  private:
    unsigned _size;
    node*    _first;
    node*    _last;
};

template <typename T>
slist<T>::slist() : _size(0), _first(nullptr), _last(nullptr)
{
}

template <typename T>
slist<T>::slist(const slist<T>& rhs)
{
  for(const auto& i : rhs)
    push_back(i);
}

template <typename T>
slist<T>::~slist()
{
  iterator it = begin();
  for(; it != end(); ++it)
    delete it._node;
}

template <typename T>
void slist<T>::push_back(const T& value)
{
  slist<T>::node* n = new slist<T>::node(value);
  if(_last == nullptr)
  {
    _first = _last = n;
  }
  else
  {
    _last->next(n);
    _last = n;
  }
}

template <typename T>
void slist<T>::push_front(const T& value)
{
  slist<T>::node* n = new slist<T>::node(value);
  if(_last == nullptr)
  {
    _first = _last = n;
  }
  else
  {
    n->next(_first);
    _first = n;
  }
}

template <typename T>
void slist<T>::erase(slist<T>::const_iterator& it)
{
  for(slist<T>::node *i = _first, *_prev = nullptr; i != nullptr; _prev = i, i = i->next())
  {
    if(i != it._node)
      continue;
    if(i == _first)
      _first = i->next();
    else
      _prev->next(i->next());
    if(i == _last)
      _last = _prev;
    delete i;
    --_size;
    break;
  }
}

template <typename T>
typename slist<T>::iterator slist<T>::begin() const 
{ 
  return slist<T>::iterator(_first); 
}

template <typename T>
typename slist<T>::iterator slist<T>::end() const 
{ 
  return slist<T>::iterator(nullptr); 
}

};

#endif
