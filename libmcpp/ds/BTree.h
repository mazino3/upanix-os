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
#ifndef _B_P_TREE_H_
#define _B_P_TREE_H_

#include <Global.h>
#include <MemPool.h>
#include <MemUtil.h>
#include <Pair.h>

class BTreeKey
{
	public:
		virtual bool operator<(const BTreeKey& rKey) const = 0 ;

		virtual bool operator==(const BTreeKey& rKey) const { return !(*this < rKey || rKey < *this) ; }

		virtual void Visit() { } ;

		virtual ~BTreeKey() { }
} ;

class BTreeValue
{
	public:
		virtual ~BTreeValue() = 0 ;

		virtual void Visit() { } ;

	private:
		// No definition - assignment not allowed
		BTreeValue& operator=(const BTreeValue&) ;
} ;

class BTreeNode ;
class BTreeElement ;

typedef Pair<BTreeNode*, BTreeElement*> BTreeNodeElement ;

class BTree
{
	public:
		class InOrderVisitor
		{
			public:
				virtual void operator()(const BTreeKey& pKey, BTreeValue* pValue) = 0 ;
				virtual bool Abort() { return false ; }
				virtual ~InOrderVisitor() {}
		} ;

		class DestroyKeyValue
		{
			public:
				virtual void DestroyKey(BTreeKey* pKey) { delete pKey ; }
				virtual void DestroyValue(BTreeValue* pValue) { delete pValue ; }
				virtual ~DestroyKeyValue() {}
		} ;

	public:
		explicit BTree(int iMaxElements = 1024) ;
		~BTree() ;

		static const int MAX_ELEMENTS_PER_NODE = 32 ;

		bool Insert(BTreeKey* pKey, BTreeValue* pValue) ;

		bool SetValue(const BTreeKey& rKey, BTreeValue* pValue) ;

		bool Delete(const BTreeKey& rKey) ;

		BTreeValue* Find(const BTreeKey& rKey) ;

		void InOrderTraverse(InOrderVisitor& visitor) { InOrderTraverse(m_pRootNode, visitor) ; }

		inline void SetMaxElements(int iSize) { if(iSize > 0) m_iMaxElements = iSize ; }
		inline int GetTotalElements() const { return m_iTotalElements ; }

		DestroyKeyValue* SetDestoryKeyValueCallBack(DestroyKeyValue* pDestroyKeyValue) ;

	private:
		void DestroyTree(BTreeNode* pNode) ;
		BTreeNodeElement FindElement(BTreeNode* pNode, const BTreeKey& rKey) ;
		bool InsertElement(BTreeNode* pNode, BTreeElement* pElement) ;
		BTreeElement* SplitNode(BTreeNode* pNode, BTreeNode** pNewNode) ;
		void InOrderTraverse(BTreeNode* pNode, InOrderVisitor& visitor) ;
		bool ValidateTreeSize() ;

		void DestroyKey(BTreeKey* pKey) ;
		void DestroyValue(BTreeValue* pValue) ;
		void DestroyElement(BTreeElement* pElement) ;

		BTreeNodeElement Highest(BTreeNode* pNode) ;
		BTreeNodeElement Lowest(BTreeNode* pNode) ;
		void SwapElement(BTreeElement* e1, BTreeElement* e2) ;
		bool Rebalance(BTreeNode* pNode) ;

		BTreeElement* CreateElement(BTreeKey* pKey, BTreeValue* pValue) ;

		bool DeleteFromLeaf(BTreeNode* pNode, const BTreeKey& rKey) ;

	private:
		const int m_iMaxElementsPerNode ;
		const int m_iMinElementsPerNode ;
		int m_iMaxElements ;
		int m_iTotalElements ;

		DestroyKeyValue* m_pDestroyKeyValue ;

		MemPool<BTreeElement>* m_pElementMemPool ;

		bool m_bLeftDeletion ;
		BTreeNode* m_pRootNode ;
} ;

class BTreeElement
{
	public:
		BTreeElement() ;
		~BTreeElement() ;

	private:
		BTreeElement(BTreeKey* pKey, BTreeValue* pValue) ;

		BTreeKey* m_pKey ;
		BTreeValue* m_pValue ;

		BTreeNode* m_pLeftNode ;
		BTreeNode* m_pRightNode ;

		inline void SetLeftNode(BTreeNode* pNode) ;
		inline void SetRightNode(BTreeNode* pNode) ;

		inline const BTreeNode* GetLeftNode() const ;
		inline const BTreeNode* GetRightNode() const ;

		inline BTreeNode* GetLeftNode() ;
		inline BTreeNode* GetRightNode() ;

		inline void SetKey(BTreeKey* pKey) ;
		inline void SetValue(BTreeValue* pValue) ;

		inline const BTreeKey& GetKey() const ;
		inline const BTreeValue* GetValue() const ;
		inline BTreeKey* GetKeyPtr() ;
		inline BTreeValue* GetValue() ;

	friend class BTree ;
	friend class BTreeNode ;
} ;

class BTreeNode
{
	private:
		int m_iNoOfElements ;
		BTreeNode* m_pParentNode ;
		BTreeElement* m_pElementList[ BTree::MAX_ELEMENTS_PER_NODE + 1 ] ;

		class BTreeElementProxy
		{
			public:
				BTreeNode* m_pNode ;
				int m_iIndex ;

				BTreeElementProxy(BTreeNode* pNode, int iIndex) : m_pNode(pNode), m_iIndex(iIndex) { }
				BTreeElementProxy(const BTreeElementProxy& rhs) : m_pNode(rhs.m_pNode), m_iIndex(rhs.m_iIndex) { }

				BTreeElement* GetElement() { return m_pNode->m_pElementList[ m_iIndex ] ; }
				const BTreeElement* GetElement() const { return m_pNode->m_pElementList[ m_iIndex ] ; }

				inline operator const BTreeElement*() const { return GetElement() ; }
				inline operator BTreeElement*() { return GetElement() ; }
				inline BTreeElement* operator=(BTreeElement* pElement) { m_pNode->m_pElementList[ m_iIndex ] = pElement ; return GetElement() ; }
				inline BTreeElement* operator=(BTreeElementProxy& rhs) { m_pNode->m_pElementList[ m_iIndex ] = rhs.GetElement() ; return GetElement() ; }

				inline BTreeElement* operator ->() { return GetElement() ; } 
				inline const BTreeElement* operator ->() const { return GetElement() ; }
		} ;

	private:
		BTreeNode() ;
		~BTreeNode() { }

		inline BTreeNode* GetParentNode() ;
		inline void SetParentNode(BTreeNode* pNode) ;
		inline BTreeElementProxy operator[](int iIndex) ;
		inline const BTreeElementProxy operator[](int iIndex) const ;
		inline BTreeElement** GetElemenList() ;
		inline int GetNoOfElements() const ;
		inline void SetNoOfElements(int iVal) ;
		
		// Returns the position where the element was inserted, -1 on error
		int Insert(BTreeElement* pElement) ;

		// Remove element with Key = rKey and return ptr to the element that was removed from ElementList
		BTreeElement* Remove(const BTreeKey& rKey) ;
		BTreeElement* RemoveByIndex(int iIndex) ;

		// Get the node where the Element can be inserted
		BTreeNode* GetInsertionNode(const BTreeKey& rKey) ;

		// Returns NULL if not found ;
		BTreeElement* HasElement(const BTreeKey& rKey) ;

		BTreeElement* SplitNode(BTreeNode** pNewNode, BTreeElement* pNewElement) ;

		bool IsLeaf() ;

		BTreeElement* Search(const BTreeKey& rKey, int& iDirection) ;

		BTreeElement* Search(const BTreeKey& rKey, int& iDirection, int iStart, int iEnd) ;

	friend class BTree ;
} ;

#endif

