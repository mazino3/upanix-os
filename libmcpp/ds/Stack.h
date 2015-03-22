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
#ifndef _MCPP_STACK_H_
#define _MCPP_STACK_H_

template <typename T>
class Stack
{
	private:
		const int MAX_ELEMENTS ;
		int m_iTop ;
		T* m_tArray ;

	public:
		Stack(int iSize) : MAX_ELEMENTS(iSize), m_iTop(0)
		{
			m_tArray = new T[MAX_ELEMENTS] ;
		}

		~Stack()
		{
			delete[] m_tArray ;
		}

		inline bool IsFull() const { return m_iTop == MAX_ELEMENTS ; }
		inline bool IsEmpty() const { return m_iTop == 0 ; }
		inline int Size() const { return m_iTop ; }
		inline int MaxElements() const { return MAX_ELEMENTS ; }

		bool Push(T a) 
		{
			// Stack full
			if(m_iTop == MAX_ELEMENTS)
				return false ;

			m_tArray[ m_iTop ] = a ;
			++m_iTop ;

			return true ;
		}

		T Pop()
		{
			//Stack Empty
			if(m_iTop == 0)
				return false ;

			--m_iTop ;
			return m_tArray[ m_iTop ] ;
		}
} ;

#endif
