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
#ifndef _MCPP_LIST_H_
#define _MCPP_LIST_H_

#include <stdlib.h>
#include <stdio.h>
#include <Display.h>

template<typename T>
class List
{
	private:
		class Node
		{
			private:
				T m_tValue ;
				Node* m_pNext ;
				Node* m_pPrev ;

			public:
				Node() : m_pNext(NULL), m_pPrev(NULL) { }

				inline Node* GetNextNode() { return m_pNext ; }
				inline Node* GetPrevNode() { return m_pPrev ; }
				inline T& GetValue() { return m_tValue ; }
				inline const T& GetValue() const { return m_tValue ; }

				inline void SetValue(const T& v) { m_tValue = v ; }
				inline void SetNextNode(Node* pNext) { m_pNext = pNext ; }
				inline void SetPrevNode(Node* pPrev) { m_pPrev = pPrev ; }
		} ;

	private:
		Node* m_pHeader ;
		Node* m_pCurPos ;
		unsigned m_uiCount ;

	private:
		bool IsHeader(Node* pNode)
		{
			return (pNode == m_pHeader) ;
		}

		void InsertAfter(Node* pNode, const T& v)
		{
			Node* p = new Node() ;

			p->SetValue(v) ;

			Node* pNext = pNode->GetNextNode() ;

			pNext->SetPrevNode(p) ;
			p->SetNextNode(pNext) ;

			pNode->SetNextNode(p) ;
			p->SetPrevNode(pNode) ;

			++m_uiCount ;
			Reset() ;
		}

		void InsertBefore(Node* pNode, const T& v)
		{
			Node* p = new Node() ;

			p->SetValue(v) ;

			Node* pPrev = pNode->GetPrevNode() ;

			pPrev->SetNextNode(p) ;
			p->SetPrevNode(pPrev) ;

			pNode->SetPrevNode(p) ;
			p->SetNextNode(pNode) ;

			++m_uiCount ;
			Reset() ;
		}

		void DeleteNode(Node* pNode)
		{
			if(IsHeader(pNode))
			{
				printf("\n List Deleting Header Node. Critical Error !!") ;
				return ;
			}

			pNode->GetPrevNode()->SetNextNode(pNode->GetNextNode()) ;
			pNode->GetNextNode()->SetPrevNode(pNode->GetPrevNode()) ;

			--m_uiCount ;
			delete pNode ;
		}

	public:
		List() 
		{
			m_pHeader = new Node() ;
			
			m_pHeader->SetNextNode(m_pHeader) ;
			m_pHeader->SetPrevNode(m_pHeader) ;

			m_pCurPos = m_pHeader ;
			m_uiCount = 0 ;
		}

		~List()
		{
			Clear() ;
			delete m_pHeader ;
		}

		void Clear()
		{
			Node* pTemp ;
			Node* pCur = m_pHeader->GetNextNode();

		    while(!IsHeader(pCur))
			{
				pTemp = pCur->GetNextNode() ;
				delete pCur ;
				pCur = pTemp ;
			}
		}

		inline unsigned Size() const 
		{
			return m_uiCount ;
		}

		void Reset()
		{
			m_pCurPos = m_pHeader ;
		}

		bool GetNext()
		{
			m_pCurPos = m_pCurPos->GetNextNode() ;

			if(IsHeader(m_pCurPos))
				return false ;

			return true ;
		}

		bool GetPrev()
		{
			m_pCurPos = m_pCurPos->GetPrevNode() ;

			if(IsHeader(m_pCurPos))
				return false ;

			return true ;
		}

		bool GetCurrentValue(T& v)
		{
			if(IsHeader(m_pCurPos))
				return false ;

			v = m_pCurPos->GetValue() ;

			return true ;
		}

		bool GetValue(unsigned uiIndex, T& v)
		{
			if(uiIndex >= m_uiCount)
				return false ;

			Reset() ;

			for(unsigned i = 0; GetNext(); i++)
			{
				if(i == uiIndex)
				{
					v = m_pCurPos->GetValue() ;
					return true ;
				}
			}

			return false ;
		}

		bool First(T& v)
		{
			Reset() ;

			if(GetNext())
			{
				v = m_pCurPos->GetValue() ;
				return true ;
			}

			return false ;
		}

		bool Last(T& v)
		{
			Reset() ;

			if(GetPrev())
			{
				v = m_pCurPos->GetValue() ;
				return true ;
			}

			return false ;
		}

		void PushBack(const T& v)
		{
			InsertBefore(m_pHeader, v) ;
		}

		void PushFront(const T& v)
		{
			InsertAfter(m_pHeader, v) ;
		}
		
		bool Insert(unsigned uiIndex, const T& v)
		{
			Reset() ;
			unsigned i = 0 ;
			
			do
			{
				if(i == uiIndex)
				{
					InsertAfter(m_pCurPos, v) ;
					return true ;
				}

				i++ ;

			} while(GetNext()) ;


			Reset() ;
			return false ;
		}

		bool PopBack(T& v)
		{
			if(Last(v))
			{
				DeleteNode(m_pCurPos) ;
				return true ;
			}

			return false ;
		}

		bool PopFront(T& v)
		{
			if(First(v))
			{
				DeleteNode(m_pCurPos) ;
				return true ;
			}

			return false ;
		}

		bool DeleteByIndex(unsigned uiIndex)
		{
			unsigned i =  0 ;
			Reset() ;

			while(GetNext())
			{
				if(i == uiIndex)
				{
					DeleteNode(m_pCurPos) ;
					return true ;
				}
				i++ ;
			}

			Reset() ;
			return false ;
		}

		bool DeleteByValue(const T& v)
		{
			Node* pNode = Find(v) ;

			if(pNode)
			{
				DeleteNode(pNode) ;
				return true ;
			}

			return false ;
		}

		Node* Find(const T& v)
		{
			Reset() ;

			while(GetNext())
			{
				if(m_pCurPos->GetValue() == v)
					return m_pCurPos ;
			}

			Reset() ;
			return NULL ;
		}

		bool FindByValue(const T& v)
		{
			return (Find(v) != NULL) ;
		}

		void SortedInsert(const T& v, bool bAsc)
		{
			Reset() ;

			while(GetNext())
			{
				if(bAsc)
				{
					if(!(v < m_pCurPos->GetValue()))
					{
						InsertBefore(m_pCurPos, v) ;
						return ;
					}
				}
				else
				{
					if(!(m_pCurPos->GetValue() < v))
					{
						InsertBefore(m_pCurPos, v) ;
						return ;
					}
				}
			}

			PushBack(v) ;
		}
} ;

#endif
