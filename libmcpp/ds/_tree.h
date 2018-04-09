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
#ifndef _UPAN_TREE_H_
#define _UPAN_TREE_H_

#include <exception.h>
#include <pair.h>
#include <algorithm.h>

class TestSuite;

namespace upan {

template <typename _c_type>
class _tree
{
  protected:
    class _tree_iterator;
    class _const_tree_iterator;
    typename _c_type::_key_accessor get_key;
    typename _c_type::_element_assigner element_assignee;
    typedef typename _c_type::value_type value_type;
    typedef typename _c_type::key_type key_type;
    using insert_result_t = upan::pair<_tree_iterator, bool>;

  public:
    using iterator = _tree_iterator;
    using const_iterator = _const_tree_iterator;

    insert_result_t insert(const value_type& element)
    {
      insert_result_t r(end(), false);
      _root = insert(_root, element, r);
      if(_root) _root->parent(nullptr);
      if(r.second) ++_size;
      return r;
    }

    bool erase(iterator it)
    {
      return erase(get_key(*it));
    }

    bool erase(const key_type& key)
    {
      bool deleted = false;
      _root = erase(_root, key, deleted);
      if(_root) _root->parent(nullptr);
      if(deleted) --_size;
      return deleted;
    }

    iterator find(key_type& key) { return iterator(find_node(key)); }
    const_iterator find(key_type& key) const { return const_iterator(find_node(key)); }
    bool exists(key_type& key) const { return find_node(key) != nullptr; }

    iterator begin() { return iterator(first_node()); }
    iterator end() { return iterator(nullptr); }

    const_iterator cbegin() { return const_iterator(first_node()); }
    const_iterator cend() { return const_iterator(nullptr); }

    const_iterator cbegin() const { return const_iterator(first_node()); }
    const_iterator cend() const { return const_iterator(nullptr); }

    const_iterator begin() const { return cbegin(); }
    const_iterator end() const { return cend(); }

    void clear() 
    { 
      clear(_root);
      _root = nullptr;
      _size = 0;
    }
    bool empty() const { return _size == 0; }
    int size() const { return _size; }

    bool verify_balance_factor();

    void print_diagnosis() { print_diagnosis(_root, 0); }
    int height() const { return height(_root); }

  protected:
    _tree() : _root(nullptr), _size(0) 
    {
    }

    ~_tree() 
    {
      clear();
    }

    class node
    {
      public:
        node(const value_type& element) : 
          _element(element),
          _height(1),
          _parent(nullptr),
          _left(nullptr), 
          _right(nullptr)
        {
        }

        value_type& element() { return _element; }
        node* parent() { return _parent; }
        node* left() { return _left; }
        node* right() { return _right; }
        int height() { return _height; }

        void parent(node* p) { _parent = p; }

        void left(node* l) 
        { 
          _left = l; 
          if(_left)
            _left->parent(this);
        }

        void right(node* r) 
        { 
          _right = r;
          if(_right)
            _right->parent(this);
        }

        int balance_factor() const { return _tree::height(_left) - _tree::height(_right); }
        void update_height() 
        { 
          _height = upan::max(_tree::height(_left), _tree::height(_right)) + 1; 
        }
        void update_value(const value_type& element, const typename _c_type::_element_assigner& ea)
        {
          ea(_element, element);
        }

        node* successor()
        {
          if(_right)
          {
            auto cur = _right;
            while(cur->left() != nullptr) cur = cur->left();
            return cur;
          }

          auto p = _parent;
          auto c = this;
          while(p && p->right() == c) 
          {
            c = p;
            p = p->parent();
          }
          return p;
        }

        node* predecessor()
        {
          if(_left)
          {
            auto cur = _left;
            while(cur->right() != nullptr) cur = cur->right();
            return cur;
          }

          auto p = _parent;
          auto c = this;
          while(p && p->left() == c)
          {
            c = p;
            p = p->parent();
          }
          return p;
        }

      private:
        value_type _element;
        int   _height;
        node* _parent;
        node* _left;
        node* _right;
    };

    class _base_tree_iterator
    {
      protected:
        _base_tree_iterator(node* n) : _node(n) { }

        inline void check_end()
        {
          if(!_node) throw exception(XLOC, "accessing end iterator");
        }

        value_type& value()
        {
          check_end();
          return _node->element();
        }

        node* pre_inc()
        {
          check_end();
          _node = _node->successor();
          return _node;
        }

        node* post_inc()
        {
          auto tmp = _node;
          pre_inc();
          return tmp;
        }

        node* pre_dec()
        {
          check_end();
          _node = _node->predecessor();
          return _node;
        }

        node* post_dec()
        {
          auto tmp = _node;
          pre_dec();
          return tmp;
        }

        node* _node;
    };

    class _tree_iterator : public _base_tree_iterator
    {
      public:
        _tree_iterator(node* n) : _base_tree_iterator(n) { }

        bool operator==(const _tree_iterator& r) { return this->_node == r._node; }
        bool operator!=(const _tree_iterator& r) { return !(*this == r); }
      
        value_type& operator*() { return this->value(); }
        value_type* operator->() { return &this->value(); }
        _tree_iterator operator++() { return this->pre_inc(); }
        _tree_iterator operator++(int) { return this->post_inc(); }
        _tree_iterator operator--() { return this->pre_dec(); }
        _tree_iterator operator--(int) { return this->post_dec(); }

      friend class _tree;
      friend class ::TestSuite;
    };

    class _const_tree_iterator : public _base_tree_iterator
    {
      public:
        _const_tree_iterator(node* n) : _base_tree_iterator(n) { }

        bool operator==(const _const_tree_iterator& r) { return this->_node == r._node; }
        bool operator!=(const _const_tree_iterator& r) { return !(*this == r); }
      
        const value_type& operator*() { return this->value(); }
        const value_type* operator->() { return &this->value(); }
        _const_tree_iterator operator++() { return this->pre_inc(); }
        _const_tree_iterator operator++(int) { return this->post_inc(); }
        _const_tree_iterator operator--() { return this->pre_dec(); }
        _const_tree_iterator operator--(int) { return this->post_dec(); }

      friend class _tree;
      friend class ::TestSuite;
    };

    node* first_node() const
    {
      if(_root == nullptr)
        return nullptr;
      auto cur = _root;
      while(cur->left() != nullptr) cur = cur->left();
      return cur;
    }

    node* rotate_left(node* n)
    {
      auto r = n->right();
      n->right(r->left());
      r->left(n);

      n->update_height();
      r->update_height();

      return r;
    }

    node* rotate_right(node* n)
    {
      auto l = n->left();
      n->left(l->right());
      l->right(n);

      n->update_height();
      l->update_height();

      return l;
    }

    node* rebalance(node* n)
    {
      if(n == nullptr)
        return n;

      n->update_height();
      int bf = n->balance_factor();

      //left - left
      if(bf > 1 && n->left()->balance_factor() >= 0)
        return rotate_right(n);

      //right - right
      if(bf < -1 && n->right()->balance_factor() <= 0)
        return rotate_left(n);

      //left - right
      if(bf > 1 && n->left()->balance_factor() < 0)
      {
        n->left(rotate_left(n->left()));
        return rotate_right(n);
      }

      //right - left
      if(bf < -1 && n->right()->balance_factor() > 0)
      {
        n->right(rotate_right(n->right()));
        return rotate_left(n);
      }

      return n;
    }

    node* insert(node* n, const value_type& element, insert_result_t& r)
    {
      if(n == nullptr)
      {
        auto nw = new node(element);
        r = insert_result_t(_tree_iterator(nw), true);
        return nw;
      }

      if(get_key(element) < get_key(n->element()))
        n->left(insert(n->left(), element, r));
      else if(get_key(n->element()) < get_key(element))
        n->right(insert(n->right(), element, r));
      else
        r = insert_result_t(_tree_iterator(n), false);

      if(!r.second)
        return n;

      return rebalance(n);
    }
    
    node* erase(node* n, const key_type& key, bool& deleted)
    {
      if(n == nullptr)
      {
        deleted = false;
        return n;
      }
      
      if(get_key(n->element()) < key)
        n->left(erase(n->left(), key, deleted));
      else if(key < get_key(n->element()))
        n->right(erase(n->right(), key, deleted));
      else
      {
        deleted = true;
        if(n->left() == nullptr || n->right() == nullptr)
        {
          auto c = n->left() ? n->left() : n->right();
          delete n;
          return c;
        }
        else
        {
          auto c = n->successor();
          n->update_value(c->element(), element_assignee);
          n->right(erase(n->right(), get_key(c->element()), deleted));
        }
      }

      if(!deleted)
        return n;

      return rebalance(n);
    }

    //post-order traversal to delete the nodes
    void clear(node* n)
    {
      if(n == nullptr) return;
      clear(n->left());
      clear(n->right());
      delete n;
    }

    void print_diagnosis(node* n, int depth);
    static int height(node* n) { return n ? n->height() : 0; }

    node* find_node(key_type& key) const
    {
      node* cur = _root;
      while(cur)
      {
        if(key < get_key(cur->element()))
          cur = cur->left();
        else if(get_key(cur->element()) < key)
          cur = cur->right();
        else
          break;
      }
      return cur;
    }

  private:
    node* _root;
    int   _size;
};

template <typename _c_type>
bool _tree<_c_type>::verify_balance_factor()
{
  for(auto it = begin(); it != end(); ++it)
  {
    auto n = it._node;
    if(n->balance_factor() != (height(n->left()) - height(n->right())))
      return false;
  }
  return true;
}

template <typename _c_type>
void _tree<_c_type>::print_diagnosis(node* n, int depth)
{
  if(n == nullptr)
    return;
  if(n->left() == nullptr && n->right() == nullptr)
  {
    printf("\n Leaf node: %d - %d", get_key(n->element()), depth);
    return;
  }
  printf("\n %x : %x", n->left(), n->right());
  print_diagnosis(n->left(), depth + 1);
  print_diagnosis(n->right(), depth + 1);

  if(n->balance_factor() > 1 || n->balance_factor() < -1)
    printf("\n INBALANCE NODE: %d - %d", get_key(n->element()), depth);
}

}

#endif
