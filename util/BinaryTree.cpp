/*
 *	Mother Operating System - An x86 based Operating System
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
#include <BinaryTree.h>
#include <DMM.h>
#include <SimAlgos.h>
#include <stdio.h>

static bool BinaryTree_LessThanCompare(BinaryTreeHeader* pHeader, unsigned key1, unsigned key2)
{
	return (pHeader->LessThan) ? pHeader->LessThan(key1, key2) : (key1 < key2) ;
}

static bool BinaryTree_DestroyKeyValue(BinaryTreeHeader* pHeader, BinaryTreeNode* pNode)
{
	return (pHeader->DestroyKeyValue) ? pHeader->DestroyKeyValue(pNode->Key, pNode->Value) : true ;
}

static void BinaryTree_CopyNodeData(BinaryTreeNode* pDest, BinaryTreeNode* pSrc)
{
	pDest->Key = pSrc->Key ;
	pDest->Value = pSrc->Value ;
}

static void BinaryTree_DoInOrderTraversal(BinaryTreeNode* pNode, BinaryTreeHeader* pHeader, const TraversalVisitor& InOrderVisitorFunc)
{
	if(pHeader->bStopInOrderTraversal)
		return ;

	if(pNode == NULL)
		return ;

	BinaryTree_DoInOrderTraversal(pNode->pLeftNode, pHeader, InOrderVisitorFunc) ;
	BinaryTreeNode* pTemp = pNode->pRightNode ;
	InOrderVisitorFunc(pHeader, pNode) ;
	BinaryTree_DoInOrderTraversal(pTemp, pHeader, InOrderVisitorFunc) ;
}

static int BinaryTree_FindHeight(BinaryTreeNode* pNode)
{
	if(pNode == NULL)
		return 0 ;

	pNode->iHeight = SimAlgos_Max(BinaryTree_FindHeight(pNode->pLeftNode), BinaryTree_FindHeight(pNode->pRightNode)) + 1 ;
	return pNode->iHeight ;
}

static bool BinaryTree_Balance(BinaryTreeHeader* pHeader, BinaryTreeNode* pNode, BinaryTreeNode* pParentNode, bool bFindHeight)
{
	if(pNode == NULL)
		return true ;

	int iLeftHeight, iRightHeight ;

	if(!bFindHeight)
	{
		iLeftHeight = (pNode->pLeftNode != NULL) ? pNode->pLeftNode->iHeight : 0 ;
		iRightHeight = (pNode->pRightNode != NULL) ? pNode->pRightNode->iHeight : 0 ;
	}
	else
	{
		iLeftHeight = BinaryTree_FindHeight(pNode->pLeftNode) ;
		iRightHeight = BinaryTree_FindHeight(pNode->pRightNode) ;
	}

	BinaryTreeNode *pTempNode = pNode ;

	bFindHeight = false ;

	if(SimAlgos_AbsDiff(iLeftHeight, iRightHeight) >= 2)
	{
		bFindHeight = true ;

		BinaryTreeNode* pCurNode ;
		BinaryTreeNode *pTempPNode ;

		// Left is heavy. Move from left to right
		if(iLeftHeight > iRightHeight)
		{
			pCurNode = pNode->pLeftNode ;
			if(pCurNode == NULL)
			{
				printf("\n BTree is corrupted... Left Child of a Left unbalanced node cannot be null !") ;
				return false ;
			}

			// Left-Left case... less work
			if(pCurNode->pRightNode == NULL)
			{
				if(pCurNode->pLeftNode == NULL)
				{
					printf("\n BTree is corrupted... Both child nodes of an unbalanced node's child cannot be null !") ;
					return false ;
				}

				pNode->pLeftNode = NULL ;
				pCurNode->pRightNode = pNode ;
				pTempNode = pCurNode ;
			}
			// Left-Right case... more work
			else
			{
				// Find InOrder Predecessor of pNode
				pTempNode = pCurNode ;
				while(pTempNode->pRightNode != NULL)
				{
					pTempPNode = pTempNode ;
					pTempNode = pTempNode->pRightNode ;
				}

				pTempPNode->pRightNode = pTempNode->pLeftNode ;

				pTempNode->pLeftNode = pCurNode ;
				pTempNode->pRightNode = pNode ;
				pNode->pLeftNode = NULL ;
			}
		}
		// Right is heavy. Move from right to left
		else
		{
			pCurNode = pNode->pRightNode ;
			if(pCurNode == NULL)
			{
				printf("\n BTree is corrupted... Right Child of an Right unbalanced node cannot be null !") ;
				return false ;
			}

			// Right-Right case... less work
			if(pCurNode->pLeftNode == NULL)
			{
				if(pCurNode->pRightNode == NULL)
				{
					printf("\n BTree is corrupted... Both child nodes of an unbalanced node's child cannot be null !") ;
					return false ;
				}

				pNode->pRightNode = NULL ;
				pCurNode->pLeftNode = pNode ;
				pTempNode = pCurNode ;
			}
			// Right-Left case... more work
			else
			{
				// Find InOrder Successor of pNode
				pTempNode = pCurNode ;
				while(pTempNode->pLeftNode != NULL)
				{
					pTempPNode = pTempNode ;
					pTempNode = pTempNode->pLeftNode ;
				}

				pTempPNode->pLeftNode = pTempNode->pRightNode ;

				pTempNode->pRightNode = pCurNode ;
				pTempNode->pLeftNode = pNode ;
				pNode->pRightNode = NULL ;
			}
		}

		if(pParentNode != NULL)
		{
			if((unsigned)pParentNode->pLeftNode == (unsigned)pNode)
				pParentNode->pLeftNode = pTempNode ;
			else
				pParentNode->pRightNode = pTempNode ;
		}
		else
		{
			pHeader->pRootNode = pTempNode ;
		}
	}

	if(!BinaryTree_Balance(pHeader, pTempNode->pLeftNode, pTempNode, bFindHeight))
		return false ;

	if(!BinaryTree_Balance(pHeader, pTempNode->pRightNode, pTempNode, bFindHeight))
		return false ;

	return true ;
}

/*************************************************************************************************************/

void BinaryTree_InitNode(BinaryTreeNode* pNode, unsigned key, void* value)
{
	pNode->pLeftNode = pNode->pRightNode = NULL ;

	pNode->Key = key ;
	pNode->Value = value ;
}

BinaryTreeNode* BinaryTree_CreateNode(unsigned key, void* value)
{
	BinaryTreeNode* pNode = (BinaryTreeNode*)DMM_AllocateForKernel(sizeof(BinaryTreeNode)) ;
	BinaryTree_InitNode(pNode, key, value) ;
	return pNode ;
}

void BinaryTree_InitHeader(BinaryTreeHeader* pHeader, void* pInfo, LessThanPtr fLessThan, DestroyKeyValuePtr fDestroyKeyValue)
{
	pHeader->iNodeCount = 0 ;
	pHeader->bDeleteFromLeft = true ;
	pHeader->LessThan = fLessThan ;
	pHeader->DestroyKeyValue = fDestroyKeyValue ;
	pHeader->pRootNode = NULL ;
	pHeader->pInfo = pInfo ;
}

BinaryTreeHeader* BinaryTree_CreateHeader(void* pInfo, LessThanPtr fLessThan, DestroyKeyValuePtr fDestroyKeyValue)
{
	BinaryTreeHeader* pHeader = (BinaryTreeHeader*)DMM_AllocateForKernel(sizeof(BinaryTreeHeader)) ;
	BinaryTree_InitHeader(pHeader, pInfo, fLessThan, fDestroyKeyValue) ;
	return pHeader ;
}

bool BinaryTree_FindKeyWithParent(BinaryTreeHeader* pHeader, unsigned key, BinaryTreeNode** pCurNode, BinaryTreeNode** pParentNode)
{
	*pParentNode = NULL ;
	*pCurNode = pHeader->pRootNode ;

	while((*pCurNode))
	{
		if(BinaryTree_LessThanCompare(pHeader, key, (*pCurNode)->Key))
		{
			*pParentNode = *pCurNode ;
			*pCurNode = (*pCurNode)->pLeftNode ;
		}
		else if(BinaryTree_LessThanCompare(pHeader, (*pCurNode)->Key, key))
		{
			*pParentNode = *pCurNode ;
			*pCurNode = (*pCurNode)->pRightNode ;
		}
		else
		{
			return true ;
		}
	}

	return false ;
}

void* BinaryTree_FindKey(BinaryTreeHeader* pHeader, unsigned key)
{
	BinaryTreeNode *pCurNode, *pParentNode ;

	if(!BinaryTree_FindKeyWithParent(pHeader, key, &pCurNode, &pParentNode))
		return NULL ;

	return pCurNode->Value ;
}

void BinaryTree_InsertNode(BinaryTreeHeader* pHeader, unsigned key, void* value)
{
	pHeader->iNodeCount++ ;

	BinaryTreeNode* pNewNode = BinaryTree_CreateNode(key, value) ;

	if(!pHeader->pRootNode)
	{
		pHeader->pRootNode = pNewNode ;
		return ;
	}

	BinaryTreeNode* pCurNode = pHeader->pRootNode ;
	BinaryTreeNode* pParentNode = NULL ;
	bool bLeft = false ;

	while(pCurNode)
	{
		pParentNode = pCurNode ;
		if(BinaryTree_LessThanCompare(pHeader, pNewNode->Key, pCurNode->Key))
		{
			pCurNode = pCurNode->pLeftNode ;
			bLeft = true ;
		}
		else
		{
		   	pCurNode = pCurNode->pRightNode ;
			bLeft = false ;
		}
	}

	if(bLeft)
		pParentNode->pLeftNode = pNewNode ;
	else
		pParentNode->pRightNode = pNewNode ;

	if(!BinaryTree_Balance(pHeader, pHeader->pRootNode, NULL, true))
	{
		printf("\n Tree Balancing Failed!") ;
	}
}

bool BinaryTree_DeleteNode(BinaryTreeHeader* pHeader, unsigned key)
{
	pHeader->iNodeCount-- ;

	BinaryTreeNode *pCurNode, *pParentNode ;
	
	if(!BinaryTree_FindKeyWithParent(pHeader, key, &pCurNode, &pParentNode))
		return false ;

	//Case 1: Leaf Node
	//Case 2: One of the Child is NULL
	if(pCurNode->pLeftNode == NULL || pCurNode->pRightNode == NULL)
	{
		BinaryTreeNode* pNextNode = pCurNode->pLeftNode ;

		if(pCurNode->pLeftNode == NULL)
			pNextNode = pCurNode->pRightNode ;

		if(pParentNode)
		{
			if((unsigned)(pParentNode->pLeftNode) == (unsigned)(pCurNode))
				pParentNode->pLeftNode = pNextNode ;
			else
				pParentNode->pRightNode = pNextNode ;
		}
		else
		{
			pHeader->pRootNode = pNextNode ;
		}

		BinaryTree_DestroyKeyValue(pHeader, pCurNode) ;

		DMM_DeAllocateForKernel((unsigned)pCurNode) ;
	}
	else
	{
		//Case 3: Neither of the child is NULL. So, find either InOrder Predecessor or InOrder Successor as NextNode
		BinaryTreeNode* pNextNode ;
		BinaryTreeNode* pNextParentNode ;

		if(pHeader->bDeleteFromLeft)
		{
			pHeader->bDeleteFromLeft = false ;

			pNextNode = pCurNode->pLeftNode ;
			pNextParentNode = pCurNode ;

			while(pNextNode->pRightNode)
			{
				pNextParentNode = pNextNode ;
				pNextNode = pNextNode->pRightNode ;
			}

			BinaryTree_DestroyKeyValue(pHeader, pCurNode) ;
			BinaryTree_CopyNodeData(pCurNode, pNextNode) ;

			if((unsigned)pNextParentNode == (unsigned)pCurNode)
				pNextParentNode->pLeftNode = pNextNode->pLeftNode ;
			else
				pNextParentNode->pRightNode = pNextNode->pLeftNode ;

			DMM_DeAllocateForKernel((unsigned)pNextNode) ;
		}
		else
		{
			pHeader->bDeleteFromLeft = true ;

			pNextNode = pCurNode->pRightNode ;
			pNextParentNode = pCurNode ;

			while(pNextNode->pLeftNode)
			{
				pNextParentNode = pNextNode ;
				pNextNode = pNextNode->pLeftNode ;
			}

			BinaryTree_DestroyKeyValue(pHeader, pCurNode) ;
			BinaryTree_CopyNodeData(pCurNode, pNextNode) ;

			if((unsigned)pNextParentNode == (unsigned)pCurNode)
				pNextParentNode->pRightNode = pNextNode->pRightNode ;
			else
				pNextParentNode->pLeftNode = pNextNode->pRightNode ;

			DMM_DeAllocateForKernel((unsigned)pNextNode) ;
		}
	}

	BinaryTree_Balance(pHeader, pHeader->pRootNode, NULL, true) ;
	return true ;
}

void BinaryTree_InOrderTraversal(BinaryTreeHeader* pHeader, const TraversalVisitor& InOrderVisitorFunc)
{
	pHeader->bStopInOrderTraversal = false ;
	BinaryTree_DoInOrderTraversal(pHeader->pRootNode, pHeader, InOrderVisitorFunc) ;	
	pHeader->bStopInOrderTraversal = false ;
}

