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
#ifndef _BINARY_TREE_H_
#define _BINARY_TREE_H_

#include <Global.h>

typedef struct BinaryTreeNode_S BinaryTreeNode ;

struct BinaryTreeNode_S
{
	BinaryTreeNode* pLeftNode ;
	BinaryTreeNode* pRightNode ;

	int iHeight ;

	unsigned Key ;
	void* Value ;
} ;

typedef bool (*LessThanPtr)(unsigned key1, unsigned key2) ;
typedef bool (*DestroyKeyValuePtr)(unsigned key, void* value) ;

typedef struct
{
	LessThanPtr LessThan ;
	DestroyKeyValuePtr DestroyKeyValue ;

	bool bDeleteFromLeft ;
	int iNodeCount ;

	BinaryTreeNode* pRootNode ;
	void *pInfo ;

	bool bStopInOrderTraversal ;
} BinaryTreeHeader ;

class TraversalVisitor
{
	public:
		virtual void operator()(BinaryTreeHeader* pHeader, BinaryTreeNode* pNode) const = 0 ;
} ;

void BinaryTree_InitNode(BinaryTreeNode* pNode, unsigned key, void* value) ;
BinaryTreeNode* BinaryTree_CreateNode(unsigned key, void* value) ;
void BinaryTree_InitHeader(BinaryTreeHeader* pHeader, void* pInfo, LessThanPtr fLessThan, DestroyKeyValuePtr fDestroyKeyValue) ;
BinaryTreeHeader* BinaryTree_CreateHeader(void* pInfo, LessThanPtr fLessThan, DestroyKeyValuePtr fDestroyKeyValue) ;
bool BinaryTree_FindKeyWithParent(BinaryTreeHeader* pHeader, unsigned key, BinaryTreeNode** pCurNode, BinaryTreeNode** pParentNode) ;
void* BinaryTree_FindKey(BinaryTreeHeader* pHeader, unsigned key) ;
void BinaryTree_InsertNode(BinaryTreeHeader* pHeader, unsigned key, void* value) ;
bool BinaryTree_DeleteNode(BinaryTreeHeader* pHeader, unsigned key) ;
void BinaryTree_InOrderTraversal(BinaryTreeHeader* pHeader, const TraversalVisitor& InOrderVisitorFunc) ;

#endif
