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
#ifndef _UPAN_MAP_H_
#define _UPAN_MAP_H_

#include <exception.h>
#include <pair.h>

namespace upan {

template <typename K, typename V>
class map
{
  public:
    class map_iterator;
    typedef map_iterator iterator;
    typedef const map_iterator const_iterator;
    typedef pair<const K, V> value_type;

    map();
    ~map();

    pair<iterator, bool> insert(const pair<K, V>& element);
    iterator find(const K& key);
    const_iterator find(const K& key) const { return const_cast<map<K, V>*>(this)->find(key); }
    V& operator[](const K& key);
    void clear() { clear(_root); }
    bool empty() const { return _size == 0; }
    int size() const { return _size; }

    iterator begin() { return iterator(this, first_node()); }
    iterator end() { return iterator(this, nullptr); }
    const_iterator begin() const { return const_cast<map<K, V>*>(this)->begin(); }
    const_iterator end() const { return const_cast<map<K, V>*>(this)->end(); }

    void print_diagnosis() { print_diagnosis(_root, 0); }
    int height() const { return height(_root); }

  private:
    class node
    {
      public:
        node(const pair<K, V>& element) : 
          _element(element.first, element.second),
          _balance_factor(0),
          _parent(nullptr),
          _left(nullptr), 
          _right(nullptr)
        {
        }

        value_type& element() { return _element; }

        node* parent() { return _parent; }
        void parent(node* p) { _parent = p; }

        node* left() { return _left; }
        void left(node* l) { _left = l; }

        node* right() { return _right; }
        void right(node* r) { _right = r; }

        int balance_factor() const { return _balance_factor; }
        void balance_factor(int f) { _balance_factor = f; }

        node* next();

      private:
        value_type _element;
        int   _balance_factor;
        node* _parent;
        node* _left;
        node* _right;
    };

    node* first_node();
    void rotate_left(node& n);
    void rotate_right(node& n);
    pair<node*, bool> find_node(const K& key);
    pair<iterator, bool> insert_at_node(node* parent, const pair<K, V>& element);
    void clear(node* n);
    void rebalance(node& node);
    void print_diagnosis(node* n, int depth);
    int height(node* n) const;

  public:
    class map_iterator
    {
      public:
        map_iterator(map<K, V>* parent, node* n) : _parent(parent), _node(n)
        {
        }
      
        value_type& operator*() { return value(); }
        const value_type& operator*() const { return const_cast<map_iterator*>(this)->value(); }

        value_type* operator->() { return &value(); }
        const value_type* operator->() const { return &const_cast<map_iterator*>(this)->value(); }

        iterator& operator++() { return pre_inc(); }
        const_iterator& operator++() const { return const_cast<map_iterator*>(this)->pre_inc(); }

        iterator operator++(int) { return post_inc(); }
        const_iterator operator++(int) const { return const_cast<map_iterator*>(this)->post_inc(); }

        bool operator==(const map_iterator& rhs) const 
        { 
          return _parent == rhs._parent && _node == rhs._node;
        }
        bool operator!=(const map_iterator& rhs) const { return !operator==(rhs); }
      private:
        void check_end()
        {
          if(*this == _parent->end())
            throw exception(XLOC, "map: accessing end iterator");
        }
        
        value_type& value()
        {
          check_end();
          return _node->element();
        }

        iterator& pre_inc()
        {
          check_end();
          _node = _node->next();
          return *this;
        }

        iterator post_inc()
        {
          iterator tmp(*this);
          pre_inc();
          return tmp;
        }
      private:
        map<K, V>* _parent; 
        node*      _node;
    };

  private:
    node* _root;
    int   _size;
};

template <typename K, typename V>
map<K, V>::map() : _root(nullptr), _size(0)
{
}

template <typename K, typename V>
map<K, V>::~map()
{
  clear();
}

//post-order traversal to delete the nodes
template <typename K, typename V>
void map<K, V>::clear(node* n)
{
  if(n == nullptr)
    return;
  clear(n->left());
  clear(n->right());
  delete n;
}

template <typename K, typename V>
typename map<K, V>::node* map<K, V>::first_node()
{
  if(_root == nullptr)
    return nullptr;
  node* cur = _root;
  while(cur->left() != nullptr)
    cur = cur->left();
  return cur;
}

template <typename K, typename V>
pair<typename map<K, V>::node*, bool> map<K, V>::find_node(const K& key)
{
  node* cur = _root;
  node* parent = nullptr;
  while(cur != nullptr)
  {
    node* next;
    if(key < cur->element().first)
      next = cur->left();
    else if(cur->element().first < key)
      next = cur->right();
    else
      return pair<node*, bool>(cur, true);
    parent = cur;
    cur = next;
  }
  return pair<node*, bool>(parent, false);
}

template <typename K, typename V>
pair<typename map<K, V>::iterator, bool> map<K, V>::insert(const pair<K, V>& element)
{
  pair<node*, bool> ret = find_node(element.first);
  if(ret.second)
    return pair<iterator, bool>(iterator(this, ret.first), false);
  return insert_at_node(ret.first, element);
}

template <typename K, typename V>
pair<typename map<K, V>::iterator, bool> map<K, V>::insert_at_node(node* parent, const pair<K, V>& element)
{
  if(parent == nullptr)
  {
    _root = new node(element);
    ++_size;
    return pair<iterator, bool>(iterator(this, _root), true);
  }
  node* new_node = new node(element);
  new_node->parent(parent);
  if(element.first < parent->element().first)
    parent->left(new_node);
  else
    parent->right(new_node);
  ++_size;
  rebalance(*new_node);
  return pair<iterator, bool>(iterator(this, new_node), true);
}

template <typename K, typename V>
void map<K, V>::rebalance(node& child)
{
  if(child.parent() == nullptr)
    return;
  node& parent = *child.parent();
  if(&child == parent.left())
  {
    if(parent.balance_factor() == 1)
    {
      if(child.balance_factor() == -1) //left-right case
        rotate_left(child);
      //left-left case
      rotate_right(parent);
      return;
    }
    else if(parent.balance_factor() == -1)
    {
      parent.balance_factor(0);
      return;
    }
    else
      parent.balance_factor(1);
  }
  else
  {
    if(parent.balance_factor() == -1)
    {
      if(child.balance_factor() == 1) //right-left case
        rotate_right(child);
      //right-right case
      rotate_left(parent);
      return;
    }
    else if(parent.balance_factor() == 1)
    {
      parent.balance_factor(0);
      return;
    }
    else
      parent.balance_factor(-1);
  }
  rebalance(parent);
}

template <typename K, typename V>
void map<K, V>::rotate_left(node& n)
{
  node* right = n.right();
  if(right == nullptr)
    throw exception(XLOC, "map tree is corrupt. right child of left rotating node is null!");

  node* parent = n.parent();
  right->parent(parent);
  if(parent == nullptr)
    _root = right;
  else
  {
    if(&n == parent->left())
      parent->left(right);
    else
      parent->right(right);
  }
  n.right(right->left());
  if(right->left() != nullptr)
    right->left()->parent(&n);
  n.parent(right);
  right->left(&n);

  n.balance_factor(n.balance_factor() + 1);
  right->balance_factor(right->balance_factor() + 1);
}

template <typename K, typename V>
void map<K, V>::rotate_right(node& n)
{
  node* left = n.left();
  if(left == nullptr)
    throw exception(XLOC, "map tree is corrupt. left child of right rotating node is null!");

  node* parent = n.parent();
  left->parent(parent);
  if(parent == nullptr)
    _root = left;
  else
  {
    if(&n == parent->left())
      parent->left(left);
    else
      parent->right(left);
  }
  n.left(left->right());
  if(left->right() != nullptr)
    left->right()->parent(&n);
  n.parent(left);
  left->right(&n);

  n.balance_factor(n.balance_factor() - 1);
  left->balance_factor(left->balance_factor() - 1);
}

template <typename K, typename V>
typename map<K, V>::iterator map<K, V>::find(const K& key)
{
  pair<node*, bool> ret = find_node(key);
  if(ret.second)
    return iterator(this, ret.first);
  return end();
}

template <typename K, typename V>
V& map<K, V>::operator[](const K& key)
{
  pair<node*, bool> ret = find_node(key);
  if(ret.second)
    return ret.first->element().second;
  return insert_at_node(ret.first, pair<K, V>(key, V())).first->second;
}

//in-order successor of a given node
template <typename K, typename V>
typename map<K, V>::node* map<K, V>::node::next()
{
  if(_right != nullptr)
  {
    node* cur = _right;
    while(cur->left() != nullptr)
      cur = cur->left();
    return cur;
  }

  if(_parent == nullptr)
    return nullptr;

  if(this == _parent->left())
    return _parent;

  node* cur = _parent;
  while(cur->parent() != nullptr)
  {
    if(cur == cur->parent()->left())
      return cur->parent();
    cur = cur->parent();
  }
  return nullptr;
}

template <typename K, typename V>
void map<K, V>::print_diagnosis(node* n, int depth)
{
  if(n == nullptr)
    return;
  if(n->left() == nullptr && n->right() == nullptr)
  {
    printf("\n Leaf node: %d - %d", n->element().first, depth);
    return;
  }
  print_diagnosis(n->left(), depth + 1);
  print_diagnosis(n->right(), depth + 1);

  if(n->balance_factor() > 1 || n->balance_factor() < -1)
    printf("\n INBALANCE NODE: %d - %d", n->element().first, depth);
}

template <typename K, typename V>
int map<K, V>::height(node* n) const
{
  if(n == nullptr)
    return 0;
  int lh = height(n->left());
  int rh = height(n->right());
  return (lh > rh ? lh : rh) + 1;
}

};

#endif
