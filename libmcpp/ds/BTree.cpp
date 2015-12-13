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
#include <BTree.h>
#include <Display.h>
#include <stdio.h>

#include <DiskCache.h>
//class BTree definition
BTree::BTree(int iMaxElements) :
	m_iMaxElementsPerNode(MAX_ELEMENTS_PER_NODE), 
	m_iMinElementsPerNode(m_iMaxElementsPerNode / 2),
	m_iMaxElements(iMaxElements),
	m_bLeftDeletion(true)
{
	m_pRootNode = new BTreeNode() ;
	// No Limit
	m_iTotalElements = 0 ;
	m_pDestroyKeyValue = NULL ;
	
	if(iMaxElements < 1)
		iMaxElements = 1024 ;

	m_pElementMemPool = &MemPool<BTreeElement>::CreateMemPool(iMaxElements) ;
}

BTree::~BTree()
{
	DestroyTree(m_pRootNode) ;
	delete m_pElementMemPool ;
}

void BTree::DestroyTree(BTreeNode* pNode)
{
	if(pNode == NULL)
		return ;

	BTreeNode& node = *pNode ;

	int i ;
	for(i = 0; i < node.GetNoOfElements(); i++)
		DestroyTree(node[i]->GetLeftNode()) ;

	if(i > 0)
		DestroyTree(node[i - 1]->GetRightNode()) ;

	for(int i = 0; i < node.GetNoOfElements(); i++)
		DestroyElement(node[i]) ;

	delete pNode ;
}

void BTree::DestroyKey(BTreeKey* pKey)
{
	if(m_pDestroyKeyValue)
		m_pDestroyKeyValue->DestroyKey(pKey) ;
}

void BTree::DestroyValue(BTreeValue* pValue)
{
	if(m_pDestroyKeyValue)
		m_pDestroyKeyValue->DestroyValue(pValue) ;
}

void BTree::DestroyElement(BTreeElement* pElement)
{
	DestroyKey(pElement->GetKeyPtr()) ;
	DestroyValue(pElement->GetValue()) ;
	m_pElementMemPool->Release(pElement) ;
}

BTreeElement* BTree::CreateElement(BTreeKey* pKey, BTreeValue* pValue)
{
	BTreeElement* pElement = m_pElementMemPool->Create() ;
	pElement->SetKey(pKey) ;
	pElement->SetValue(pValue) ;

	return pElement ;
}

bool BTree::SetValue(const BTreeKey& rKey, BTreeValue* pValue)
{
	BTreeNodeElement kv = FindElement(m_pRootNode, rKey) ;
	BTreeElement* pElement = kv.second ;

	if(pElement)
	{
	   	DestroyValue(pElement->GetValue()) ;

		pElement->SetValue(pValue) ;

		return true ;
	}

	return false ;
}

bool BTree::ValidateTreeSize()
{	
	if(m_iMaxElements < 0)
		return true ;

	return (m_iTotalElements < m_iMaxElements) ;
}

bool BTree::Insert(BTreeKey* pKey, BTreeValue* pValue)
{
	if(!ValidateTreeSize())
	{
		printf("\n BTree Size Limit: %d reached\n", m_iMaxElements) ;
		return false ;
	}

	BTreeElement* pElement = CreateElement(pKey, pValue) ;

	BTreeNode* pNode = m_pRootNode ;

	while(!pNode->IsLeaf())
		pNode = pNode->GetInsertionNode(*pKey) ;

	if(InsertElement(pNode, pElement))
	{
		m_iTotalElements++ ;
		return true ;
	}

	return false ;
}

BTreeNodeElement BTree::Highest(BTreeNode* pNode)
{
	if(!pNode)
	{
		printf("\n BTree Highest: NULL Node. %d", __LINE__) ;
		return BTreeNodeElement(NULL, NULL) ;
	}

	BTreeNode& node = *pNode ;
	int iIndex = node.GetNoOfElements() ;
	if(!iIndex)
		return BTreeNodeElement(NULL, NULL) ;
	--iIndex ;
	
	BTreeElement* pElement = node[ iIndex ] ;

	if(pElement->GetRightNode())
		return Highest(pElement->GetRightNode()) ;

	if(!node.IsLeaf())
	{
		printf("\n Finding Highest. Node with Highest element is not the leaf node. BTree is corrupted!") ;
		return BTreeNodeElement(NULL, NULL) ;
	}

	return BTreeNodeElement(pNode, pElement) ;
}

BTreeNodeElement BTree::Lowest(BTreeNode* pNode)
{
	if(!pNode)
	{
		printf("\n BTree Lowest: NULL Node. %d", __LINE__) ;
		return BTreeNodeElement(NULL, NULL) ;
	}

	BTreeNode& node = *pNode ;
	if(!node.GetNoOfElements())
		return BTreeNodeElement(NULL, NULL) ;
	
	BTreeElement* pElement = node[ 0 ] ;

	if(pElement->GetLeftNode())
		return Lowest(pElement->GetLeftNode()) ;

	if(!node.IsLeaf())
	{
		printf("\n Finding Lowest. Node with Lowest element is not the leaf node. BTree is corrupted!") ;
		return BTreeNodeElement(NULL, NULL) ;
	}

	return BTreeNodeElement(pNode, pElement) ;
}

void BTree::SwapElement(BTreeElement* e1, BTreeElement* e2)
{
	Pair<BTreeKey*, BTreeValue*> kv(e1->GetKeyPtr(), e1->GetValue()) ;

	e1->SetKey(e2->GetKeyPtr()) ;
	e1->SetValue(e2->GetValue()) ;

	e2->SetKey(kv.first) ;
	e2->SetValue(kv.second) ;
}

bool BTree::Rebalance(BTreeNode* pNode)
{
	if(pNode == m_pRootNode)
		return true ;

	// No need to balance if there are sufficient (minelementspernode) elements in the Node
	if(pNode->GetNoOfElements() >= m_iMinElementsPerNode)
		return true ;

	// Find Parent Element
	BTreeNode& parent = *(pNode->GetParentNode()) ;
	bool bLeftChild = true ;

	int iParentIndex ;
	for(iParentIndex = 0; iParentIndex < parent.GetNoOfElements(); iParentIndex++)
	{
		if(parent[iParentIndex]->GetLeftNode() == pNode)
			break ;

		if(iParentIndex == (parent.GetNoOfElements() - 1))
		{
			if(parent[iParentIndex]->GetRightNode() == pNode)
			{
				bLeftChild = false ;
				break ;
			}
		}
	}

	if(iParentIndex == parent.GetNoOfElements())
	{
		printf("\n %d, %d, %x, %x", parent.GetNoOfElements(), pNode->GetNoOfElements(), pNode, m_pRootNode) ;
		return false ;
	}

	BTreeElement* pParentElement = parent[iParentIndex] ;

	if(bLeftChild)
	{
		BTreeNode& leftNode = *pNode ;
		BTreeNode& rightNode = *(pParentElement->GetRightNode()) ;
		if(rightNode.GetNoOfElements() > m_iMinElementsPerNode)
		{
			BTreeElement* pRightElement = rightNode.RemoveByIndex(0) ;
			SwapElement(pParentElement, pRightElement) ;

			leftNode[ leftNode.GetNoOfElements() ] = pRightElement ;

			pRightElement->SetRightNode(pRightElement->GetLeftNode()) ;
			pRightElement->SetLeftNode(leftNode[ leftNode.GetNoOfElements() - 1 ]->GetRightNode()) ;
			leftNode.SetNoOfElements(leftNode.GetNoOfElements() + 1) ;	

			pRightElement->GetRightNode()->SetParentNode(&leftNode) ;
			
			return true ;
		}
		else
		{
			leftNode[ leftNode.GetNoOfElements() ] = pParentElement ;
			for(int i = leftNode.GetNoOfElements() + 1, j = 0; j < rightNode.GetNoOfElements(); j++, i++)
			{
				leftNode[i] = rightNode[j] ;

				rightNode[j]->GetLeftNode()->SetParentNode(&leftNode) ;
				if(j == rightNode.GetNoOfElements() - 1)
					rightNode[j]->GetRightNode()->SetParentNode(&leftNode) ;
			}

			pParentElement->SetLeftNode(leftNode[ leftNode.GetNoOfElements() - 1 ]->GetRightNode()) ;
			pParentElement->SetRightNode(leftNode[ leftNode.GetNoOfElements() + 1 ]->GetLeftNode()) ;

			leftNode.SetNoOfElements(leftNode.GetNoOfElements() + rightNode.GetNoOfElements() + 1 ) ;

			for(int i = iParentIndex; i < parent.GetNoOfElements() - 1; i++)
				parent[ i ] = parent[ i + 1 ] ;
			parent.SetNoOfElements(parent.GetNoOfElements() - 1) ;

			if(parent.GetNoOfElements() == 0)
			{
				m_pRootNode = pNode ;
				delete (&rightNode) ;
				return true ;
			}

			// If parent element is not the right most element
			if(iParentIndex != parent.GetNoOfElements())
				parent[ iParentIndex ]->SetLeftNode(pNode) ;
			
			delete (&rightNode) ;
			return Rebalance(&parent) ;
		}
	}
	else
	{
		BTreeNode& rightNode = *pNode ;
		BTreeNode& leftNode = *(pParentElement->GetLeftNode()) ;
		if(leftNode.GetNoOfElements() > m_iMinElementsPerNode)
		{
			BTreeElement* pLeftElement = leftNode.RemoveByIndex(leftNode.GetNoOfElements() - 1) ;
			SwapElement(pParentElement, pLeftElement) ;

			for(int i = rightNode.GetNoOfElements(); i > 0; --i)
				rightNode[i] = rightNode[i-1] ;
			rightNode[0] = pLeftElement ;

			pLeftElement->SetLeftNode(pLeftElement->GetRightNode()) ;
			pLeftElement->SetRightNode(rightNode[1]->GetLeftNode()) ;
			rightNode.SetNoOfElements(rightNode.GetNoOfElements() + 1) ;	

			pLeftElement->GetLeftNode()->SetParentNode(&rightNode) ;

			return true ;
		}
		else
		{
			leftNode[ leftNode.GetNoOfElements() ] = pParentElement ;
			for(int i = leftNode.GetNoOfElements() + 1, j = 0; j < rightNode.GetNoOfElements(); j++, i++)
			{
				leftNode[i] = rightNode[j] ;

				rightNode[j]->GetLeftNode()->SetParentNode(&leftNode) ;
				if(j == rightNode.GetNoOfElements() - 1)
					rightNode[j]->GetRightNode()->SetParentNode(&leftNode) ;
			}

			pParentElement->SetLeftNode(leftNode[ leftNode.GetNoOfElements() - 1 ]->GetRightNode()) ;
			pParentElement->SetRightNode(leftNode[ leftNode.GetNoOfElements() + 1 ]->GetLeftNode()) ;

			leftNode.SetNoOfElements(leftNode.GetNoOfElements() + rightNode.GetNoOfElements() + 1 ) ;

			// This case will arise when Parent Element is the right most element. So, no elements shifts should be required in parent node
			parent.SetNoOfElements(parent.GetNoOfElements() - 1) ;

			if(parent.GetNoOfElements() == 0)
			{
				m_pRootNode = pNode ;
				delete (&rightNode) ;
				return true ;
			}

			// more so for validation
			if(iParentIndex != parent.GetNoOfElements())
			{
				printf("\n Rebalance failed. Inconsistent state of BTree!") ;
				return false ;
			}

			delete (&rightNode) ;
			return Rebalance(&parent) ;
		}
	}
}

bool BTree::DeleteFromLeaf(BTreeNode* pNode, const BTreeKey& rKey)
{
	if(!pNode)
		return false ;
	
	if(!pNode->IsLeaf())
	{
		printf("\n Node is not a Leaf Node. Error in DeleteFromLeaf:%d", __LINE__) ;
		return false ;
	}

	// TODO: If Index is available then we can directly remove that w/o search for the key
	BTreeElement* pElement = pNode->Remove(rKey) ;

	if(!pElement)
	{
		printf("\n BTree corrupted. Deletetion failed at: %d", __LINE__) ;
		return false ;	
	}

	DestroyElement(pElement) ;
	--m_iTotalElements ;

	return Rebalance(pNode) ;
}

bool BTree::Delete(const BTreeKey& rKey)
{
	BTreeNodeElement kv = FindElement(m_pRootNode, rKey) ;
	BTreeNode* pNode = kv.first ;
	BTreeElement* pElement = kv.second ;

	if(!pElement)
		return false ;

	BTreeNode* pNodeForDelete = NULL ;
	// Case1: Leaf Node Deletion
	if(pNode->IsLeaf())
	{
		pNodeForDelete = pNode ;
	}
	// Case2: Internal Node Deletion
	else
	{
		BTreeNodeElement child(NULL, NULL) ;
		if(m_bLeftDeletion)
		{
			child = Highest(pElement->GetLeftNode()) ;
			m_bLeftDeletion = false ;
		}
		else
		{
			child = Lowest(pElement->GetRightNode()) ;
			m_bLeftDeletion = true ;
		}
		
		if(child.first == NULL || child.second == NULL)
			return false ;

		SwapElement(pElement, child.second) ;

		pNodeForDelete = child.first ;
	}

	return DeleteFromLeaf(pNodeForDelete, rKey) ;
}

BTreeValue* BTree::Find(const BTreeKey& rKey)
{
	BTreeNodeElement kv = FindElement(m_pRootNode, rKey) ;
	BTreeElement* pElement = kv.second ;

	if(pElement)
		return pElement->GetValue() ;

	return NULL ;
}

bool BTree::InsertElement(BTreeNode* pNode, BTreeElement* pNewElement)
{
	if(pNode == NULL)
		return false ;

	BTreeNode& node = *pNode ;

	if(node.GetNoOfElements() < m_iMaxElementsPerNode)
		return node.Insert(pNewElement) >= 0 ;

	// Node is full, we need to split the node
	BTreeNode* pNewNode ;
	BTreeElement* pMidElement = node.SplitNode(&pNewNode, pNewElement) ;

	// Can happen only for root. So a new root node created
	if(!node.GetParentNode())
	{
		BTreeNode* pRootNode = new BTreeNode() ;
		m_pRootNode = pRootNode ;
		node.SetParentNode(pRootNode) ;
		pNewNode->SetParentNode(pRootNode) ;		
	}

	return InsertElement(node.GetParentNode(), pMidElement) ;
}

BTreeNodeElement BTree::FindElement(BTreeNode* pNode, const BTreeKey& rKey)
{
	if(pNode == NULL)
		return BTreeNodeElement(NULL, NULL) ;

	BTreeNode& node = *pNode ;

	if(node.GetNoOfElements() <= 0)
		return BTreeNodeElement(NULL, NULL) ;

	int iDirection ;
	BTreeElement* pElement = node.Search(rKey, iDirection) ;

	if(!pElement)
		return BTreeNodeElement(NULL, NULL) ;

	switch(iDirection)
	{
		case 0:
			return BTreeNodeElement(&node, pElement) ;

		case 1:
			return FindElement(pElement->GetLeftNode(), rKey) ;

		case 2:
			return FindElement(pElement->GetRightNode(), rKey) ;
	}

	return BTreeNodeElement(NULL, NULL) ;
}

void BTree::InOrderTraverse(BTreeNode* pNode, InOrderVisitor& visitor)
{
	if(visitor.Abort())
		return ;

	if(pNode == NULL)
		return ;

	BTreeNode& rNode = *pNode ;

	for(int i = 0; i < rNode.GetNoOfElements(); i++)
	{
		InOrderTraverse(rNode[i]->GetLeftNode(), visitor) ;

		if(visitor.Abort())
			return ;

		visitor(rNode[i]->GetKey(), rNode[i]->GetValue()) ;	

		// If last element of this node then visit right else the right node is visited as the left node of the next element
		if(i == rNode.GetNoOfElements() - 1)
			InOrderTraverse(rNode[i]->GetRightNode(), visitor) ;
	}
}

BTree::DestroyKeyValue* BTree::SetDestoryKeyValueCallBack(DestroyKeyValue* pDestroyKeyValue)
{
	DestroyKeyValue* pT = m_pDestroyKeyValue ;
	m_pDestroyKeyValue = pDestroyKeyValue ;
	return pT ;
}

BTreeValue::~BTreeValue()
{
}

// class BTreeElement definition
BTreeElement::BTreeElement()
{
	m_pKey = NULL ;
	m_pValue = NULL ;
	m_pLeftNode = NULL ;
	m_pRightNode = NULL ;
}

BTreeElement::BTreeElement(BTreeKey* pKey, BTreeValue* pValue) : m_pKey(pKey), m_pValue(pValue)
{
	m_pLeftNode = NULL ;
	m_pRightNode = NULL ;
}

BTreeElement::~BTreeElement()
{
}

void BTreeElement::SetLeftNode(BTreeNode* pNode) { m_pLeftNode = pNode ; }

void BTreeElement::SetRightNode(BTreeNode* pNode) { m_pRightNode = pNode ; }

const BTreeNode* BTreeElement::GetLeftNode() const { return m_pLeftNode ; }
const BTreeNode* BTreeElement::GetRightNode() const { return m_pRightNode ; }

BTreeNode* BTreeElement::GetLeftNode() { return m_pLeftNode ; }
BTreeNode* BTreeElement::GetRightNode() { return m_pRightNode ; }

void BTreeElement::SetKey(BTreeKey* pKey) { m_pKey = pKey ; }
void BTreeElement::SetValue(BTreeValue* pValue) { m_pValue = pValue ; }

const BTreeKey& BTreeElement::GetKey() const { return *m_pKey ; }
const BTreeValue* BTreeElement::GetValue() const { return m_pValue ; }

BTreeKey* BTreeElement::GetKeyPtr() { return m_pKey ; }
BTreeValue* BTreeElement::GetValue() { return m_pValue ; }

// class BTreeNode definition
BTreeNode::BTreeNode() : m_iNoOfElements(0), m_pParentNode(NULL)
{
	for(int i = 0; i < BTree::MAX_ELEMENTS_PER_NODE + 1; i++)
		m_pElementList[i] = NULL ;
}

BTreeNode* BTreeNode::GetParentNode() { return m_pParentNode ; }
void BTreeNode::SetParentNode(BTreeNode* pNode) { m_pParentNode = pNode ; }
BTreeNode::BTreeElementProxy  BTreeNode::operator[](int iIndex) { return BTreeElementProxy(this, iIndex) ; } 
const BTreeNode::BTreeElementProxy  BTreeNode::operator[](int iIndex) const { return BTreeElementProxy(const_cast<BTreeNode*>(this), iIndex) ; }
BTreeElement** BTreeNode::GetElemenList() { return m_pElementList ; }
int BTreeNode::GetNoOfElements() const { return m_iNoOfElements ; }
void BTreeNode::SetNoOfElements(int iVal) { m_iNoOfElements = iVal ; }

int BTreeNode::Insert(BTreeElement* pElement)
{
	if(m_iNoOfElements == BTree::MAX_ELEMENTS_PER_NODE + 1)
		return -1 ;

	if(m_iNoOfElements == 0)
	{
		m_pElementList[0] = pElement ;
		m_iNoOfElements = 1 ;
		return 0 ;
	}

	const BTreeKey& lastKey = m_pElementList[m_iNoOfElements - 1]->GetKey() ;
	const BTreeKey& inKey = pElement->GetKey() ;

	if(lastKey < inKey || lastKey == inKey)
	{
		m_pElementList[ m_iNoOfElements ] = pElement ;
		m_pElementList[ m_iNoOfElements - 1 ]->SetRightNode(pElement->GetLeftNode()) ;
		m_iNoOfElements++ ;
		return m_iNoOfElements - 1 ;
	}

	for(int i = 0; i < m_iNoOfElements; i++)
	{
		if(inKey < m_pElementList[i]->GetKey())
		{
			for(int j = m_iNoOfElements; j > i; j--)
				m_pElementList[j] = m_pElementList[j-1] ;

			m_pElementList[i] = pElement ;

			if(i > 0)
				m_pElementList[i-1]->SetRightNode(pElement->GetLeftNode()) ;

			if(i < m_iNoOfElements)
				m_pElementList[i+1]->SetLeftNode(pElement->GetRightNode()) ;

			m_iNoOfElements++ ;
			return i ;
		}
	}

	return -2 ;
}

// Bald Deletion... Doesn't care about child nodes if any
BTreeElement* BTreeNode::Remove(const BTreeKey& rKey)
{
	if(m_iNoOfElements > 0)
	{
		if(rKey < m_pElementList[0]->GetKey() || m_pElementList[m_iNoOfElements-1]->GetKey() < rKey)
			return NULL ;

		for(int i = 0; i < m_iNoOfElements; i++)
		{
			if(rKey == m_pElementList[i]->GetKey())
			{
				BTreeElement* pElement = m_pElementList[i] ;
				
				for(; i < m_iNoOfElements - 1; i++)
					m_pElementList[i] = m_pElementList[i+1] ;

				m_iNoOfElements-- ;

				return pElement ;
			}
		}
	}

	return NULL ;
}

BTreeElement* BTreeNode::RemoveByIndex(int iIndex)
{
	if(iIndex < 0 || iIndex >= m_iNoOfElements)
		return NULL ;

	BTreeElement* pElement = m_pElementList[iIndex] ;
				
	for(int i = iIndex; i < m_iNoOfElements - 1; i++)
		m_pElementList[i] = m_pElementList[i+1] ;

	m_iNoOfElements-- ;

	return pElement ;
}

BTreeNode* BTreeNode::GetInsertionNode(const BTreeKey& rKey)
{
	if(IsLeaf())
		return this ;

	const BTreeKey& lastKey = m_pElementList[ m_iNoOfElements - 1 ]->GetKey() ;

	if(lastKey < rKey || lastKey == rKey)
		return m_pElementList[m_iNoOfElements-1]->GetRightNode() ;

	for(int i = 0; i < m_iNoOfElements; i++)
	{
		if(rKey < m_pElementList[i]->GetKey())
			return m_pElementList[i]->GetLeftNode() ;
	}

	return NULL ;
}

BTreeElement* BTreeNode::HasElement(const BTreeKey& rKey)
{
	for(int i = 0; i < m_iNoOfElements; i++)
	{
		if(m_pElementList[i]->GetKey() == rKey)
			return m_pElementList[i] ;
	}

	return NULL ;
}

// 0 = Found, 1 = Left, 2 = Right
BTreeElement* BTreeNode::Search(const BTreeKey& rKey, int& iDirection)
{
	return Search(rKey, iDirection, 0, m_iNoOfElements - 1) ;
}

BTreeElement* BTreeNode::Search(const BTreeKey& rKey, int& iDirection, int iStart, int iEnd)
{
	if(iStart == iEnd)
	{
		iDirection = (rKey == m_pElementList[iStart]->GetKey()) ? 0 : ((rKey < m_pElementList[iStart]->GetKey()) ? 1 : 2) ;
		return m_pElementList[iStart] ;
	}

	if(iStart < iEnd)
	{
		int iMid = iStart + (iEnd - iStart) / 2 ;

		if(rKey == m_pElementList[iMid]->GetKey())
		{
			iDirection = 0 ;
			return m_pElementList[iMid] ;
		}

		if(m_pElementList[iMid]->GetKey() < rKey)
		{
			if(iMid < iEnd)
				++iMid ;

			return Search(rKey, iDirection, iMid, iEnd) ;
		}
		else
		{
			if(iStart < iMid)
				--iMid ;

			return Search(rKey, iDirection, iStart, iMid) ;
		}
	}

	return NULL ;
}

bool BTreeNode::IsLeaf()
{
	for(int i = 0; i < m_iNoOfElements; i++)
	{
		if(i == m_iNoOfElements - 1)
		{
			if(m_pElementList[i]->GetRightNode() != NULL)
				return false ;
		}

		if(m_pElementList[i]->GetLeftNode() != NULL)
			return false ;
	}
	
	return true ;
}

BTreeElement* BTreeNode::SplitNode(BTreeNode** pNewNode, BTreeElement* pNewElement)
{
	if(Insert(pNewElement) < 0)
		return NULL ;

	*pNewNode = new BTreeNode() ;
	BTreeNode& newNode = **pNewNode ;

	int iMid = m_iNoOfElements / 2 ;
	BTreeElement* pMidElement = m_pElementList[ iMid ] ;

	for(int j = iMid + 1; j < m_iNoOfElements; j++)
		newNode[ j - (iMid + 1) ] = m_pElementList[ j ] ;

	newNode.SetNoOfElements(m_iNoOfElements - iMid - 1) ;

	pMidElement->SetLeftNode(this) ;
	pMidElement->SetRightNode(*pNewNode) ;

	newNode.SetParentNode(m_pParentNode) ;

	for(int i = 0; i < newNode.GetNoOfElements(); i++)
	{
		BTreeNode* pChild = newNode[i]->GetLeftNode() ;
		
		if(pChild)
			pChild->SetParentNode(*pNewNode) ;

		if(i == newNode.GetNoOfElements() - 1)
		{
			pChild = newNode[i]->GetRightNode() ;
			if(pChild)
				pChild->SetParentNode(*pNewNode) ;
		}
	}

	m_iNoOfElements = iMid ;
	return pMidElement ;
}

