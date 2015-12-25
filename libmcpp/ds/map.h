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

class TestSuite;

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

    pair<iterator, bool> insert(const value_type& element);
    iterator find(const K& key);
    const_iterator find(const K& key) const { return const_cast<map<K, V>*>(this)->find(key); }
    V& operator[](const K& key);
    bool erase(iterator it);
    bool erase(const K& key) { return erase(find(key)); }
    void clear() 
    { 
      clear(_root);
      _root = nullptr;
      _size = 0;
    }
    bool empty() const { return _size == 0; }
    int size() const { return _size; }

    iterator begin() { return iterator(this, first_node()); }
    iterator end() { return iterator(this, nullptr); }
    const_iterator begin() const { return const_cast<map<K, V>*>(this)->begin(); }
    const_iterator end() const { return const_cast<map<K, V>*>(this)->end(); }

    void print_diagnosis() { print_diagnosis(_root, 0); }
    bool verify_balance_factor();
    int height() const { return height(_root); }

  private:
    class node
    {
      public:
        node(const value_type& element) : 
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
        bool is_leaf() const { return _left == nullptr && _right == nullptr; }

        node* next();

      private:
        value_type _element;
        int   _balance_factor;
        node* _parent;
        node* _left;
        node* _right;
    };

    node* first_node();
    void rotate_left(node* n);
    void rotate_right(node* n);
    pair<node*, bool> find_node(const K& key);
    pair<iterator, bool> insert_at_node(node* parent, const value_type& element);
    void clear(node* n);
    void rebalance_on_insert(node*);
    void rebalance_on_delete(bool is_right_child, node*);
    void print_diagnosis(node* n, int depth);
    int height(node* n) const;
    void refresh_balance_factor(node* n) const;

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

      friend class map;
      friend class ::TestSuite;
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
pair<typename map<K, V>::iterator, bool> map<K, V>::insert(const value_type& element)
{
  pair<node*, bool> ret = find_node(element.first);
  if(ret.second)
    return pair<iterator, bool>(iterator(this, ret.first), false);
  return insert_at_node(ret.first, element);
}

template <typename K, typename V>
pair<typename map<K, V>::iterator, bool> map<K, V>::insert_at_node(node* parent, const value_type& element)
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
  rebalance_on_insert(new_node);
  return pair<iterator, bool>(iterator(this, new_node), true);
}

template <typename K, typename V>
void map<K, V>::rebalance_on_insert(node* child)
{
  if(child == nullptr)
    return;
  node* parent = child->parent();
  while(parent != nullptr)
  {
    if(child == parent->left())
    {
      if(parent->balance_factor() == 1)
      {
        if(child->balance_factor() == -1) //left-right case
          rotate_left(child);
        //left-left case
        rotate_right(parent);
        break;
      }
      if(parent->balance_factor() == -1)
      {
        parent->balance_factor(0);
        break;
      }
      parent->balance_factor(1);
    }
    else
    {
      if(parent->balance_factor() == -1)
      {
        if(child->balance_factor() == 1) //right-left case
          rotate_right(child);
        //right-right case
        rotate_left(parent);
        break;
      }
      if(parent->balance_factor() == 1)
      {
        parent->balance_factor(0);
        break;
      }
      parent->balance_factor(-1);
    }
    child = parent;
    parent = child->parent();
  }
}

template <typename K, typename V>
void map<K, V>::rotate_left(node* n)
{
  if(n == nullptr)
    return;
  node* right = n->right();
  if(right == nullptr)
    throw exception(XLOC, "map tree is corrupt. right child of left rotating node is null!");

  node* parent = n->parent();
  right->parent(parent);
  if(parent == nullptr)
    _root = right;
  else
  {
    if(n == parent->left())
      parent->left(right);
    else
      parent->right(right);
  }
  n->right(right->left());
  if(right->left() != nullptr)
    right->left()->parent(n);
  n->parent(right);
  right->left(n);

  refresh_balance_factor(n);
  refresh_balance_factor(right);
}

template <typename K, typename V>
void map<K, V>::rotate_right(node* n)
{
  if(n == nullptr)
    return;
  node* left = n->left();
  if(left == nullptr)
    throw exception(XLOC, "map tree is corrupt. left child of right rotating node is null!");

  node* parent = n->parent();
  left->parent(parent);
  if(parent == nullptr)
    _root = left;
  else
  {
    if(n == parent->left())
      parent->left(left);
    else
      parent->right(left);
  }
  n->left(left->right());
  if(left->right() != nullptr)
    left->right()->parent(n);
  n->parent(left);
  left->right(n);

  refresh_balance_factor(n);
  refresh_balance_factor(left);
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
  return insert_at_node(ret.first, value_type(key, V())).first->second;
}

template <typename K, typename V>
bool map<K, V>::erase(iterator it)
{
  if(it == end())
    return false;

  node* n = it._node;
  
  if((n->left() && n->right())
    || (n->left() && !n->left()->is_leaf())
    || (n->right() && !n->right()->is_leaf()))
  {
    node* y;
    bool deleteFromLeft;
    if(n->left() && !n->right())
      deleteFromLeft = true;
    else if(n->right() && !n->left())
      deleteFromLeft = false;
    else
      deleteFromLeft = n->left() && n->right() && n->balance_factor() > 0;
    //find in-order predecessor
    if(deleteFromLeft)
    {
      y = n->left();
      while(y->right() != nullptr)
        y = y->right();
    }
    else //find in-order successor
    {
      y = n->right();
      while(y->left() != nullptr)
        y = y->left();
    }

    node* n_parent = n->parent();
    node* n_left = n->left();
    node* n_right = n->right();

    node* y_parent = y->parent();
    node* y_left = y->left();
    node* y_right = y->right();

    y->parent(n_parent);
    if(n_parent != nullptr)
    {
      if(n_parent->left() == n)
        n_parent->left(y);
      else
        n_parent->right(y);
    }

    n->left(y_left);
    if(y_left)
      y_left->parent(n);
    n->right(y_right);
    if(y_right)
      y_right->parent(n);

    if(y_parent == n)
    {
      n->parent(y);
      if(n_left == y)
      {
        y->left(n);
        y->right(n_right);
        if(n_right)
          n_right->parent(y);
      }
      else
      {
        y->right(n);
        y->left(n_left);
        if(n_left)
          n_left->parent(y);
      }
    }
    else
    {
      y->left(n_left);
      if(n_left)
        n_left->parent(y);
      y->right(n_right);
      if(n_right)
        n_right->parent(y);

      n->parent(y_parent);
      if(y_parent != nullptr)
      {
        if(y_parent->left() == y)
          y_parent->left(n);
        else
          y_parent->right(n);
      }
    }

    y->balance_factor(n->balance_factor());
    if(n == _root)
      _root = y;
  }

  node* parent = n->parent();

  node* subtree = nullptr;
  if(n->left())
    subtree = n->left();
  else if(n->right())
    subtree = n->right();

  if(subtree)
    subtree->parent(parent);

  if(parent == nullptr)
  {
    _root = subtree;
  }
  else
  {
    bool is_right_child = parent->right() == n;
    if(is_right_child)
      parent->right(subtree);
    else
      parent->left(subtree);
    rebalance_on_delete(is_right_child, parent);
  }
  delete n;
  --_size;
  return true;
}

template <typename K, typename V>
void map<K, V>::rebalance_on_delete(bool is_right_child, node* parent)
{
  while(parent != nullptr)
  {
    if(is_right_child)
    {
      if(parent->balance_factor() == 1) //going to be 2
      {
        node* sibling = parent->left();
        if(!sibling)
          throw exception(XLOC, "no left node for parent with balance factor 1 - map tree is corrupted!");
        int bf_sibling = sibling->balance_factor();
        if(bf_sibling == -1)
          rotate_left(sibling);
        rotate_right(parent);
        if(bf_sibling == 0)
        {
          parent->balance_factor(1);
          break;
        }
        parent = parent->parent();
      }
      else if(parent->balance_factor() == 0)
      {
        parent->balance_factor(1);
        break;
      }
      else
        parent->balance_factor(0);
    } 
    else
    {
      if(parent->balance_factor() == -1)
      {
        node* sibling = parent->right();
        if(!sibling)
          throw exception(XLOC, "no right node for parent with balance factor -1 - map tree is corrupted!");
        int bf_sibling = sibling->balance_factor();
        if(bf_sibling == 1)
          rotate_right(sibling);
        rotate_left(parent);
        if(bf_sibling == 0)
        {
          parent->balance_factor(-1);
          break;
        }
        parent = parent->parent();
      }
      else if(parent->balance_factor() == 0)
      {
        parent->balance_factor(-1);
        break;
      }
      else
        parent->balance_factor(0);
    }
    if(parent->parent())
      is_right_child = parent->parent()->right() == parent;
    parent = parent->parent();
  }
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
  printf("\n %x : %x", n->left(), n->right());
  print_diagnosis(n->left(), depth + 1);
  print_diagnosis(n->right(), depth + 1);

  if(n->balance_factor() > 1 || n->balance_factor() < -1)
    printf("\n INBALANCE NODE: %d - %d", n->element().first, depth);
}

template <typename K, typename V>
bool map<K, V>::verify_balance_factor()
{
  for(auto it = begin(); it != end(); ++it)
  {
    auto n = it._node;
    if(n->balance_factor() != (height(n->left()) - height(n->right())))
      return false;
  }
  return true;
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

template <typename K, typename V>
void map<K, V>::refresh_balance_factor(node* n) const
{
  n->balance_factor(height(n->left()) - height(n->right()));
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

};

#endif
