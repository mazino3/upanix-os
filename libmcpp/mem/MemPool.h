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
#ifndef _MCPP_MEM_POOL_H_
#define _MCPP_MEM_POOL_H_

#include <stddef.h>
#include <stdio.h>
#include <Alloc.h>
#include <Stack.h>
#include <exception.h>
#include <DMM.h>

template <typename T>
class MemPool
{
	private:
		const unsigned MAX_ELEMENTS ;
		const unsigned CHUNK_SIZE ;
		const unsigned MAX_CHUNKS ;
		
		Stack<T*> m_mStackMemPool ;

		unsigned m_uiNoOfChunks ;

		unsigned* m_pAllocAddressArray ;

	private:
		MemPool(unsigned uiSize, unsigned uiChunkSize) : MAX_ELEMENTS(uiSize), CHUNK_SIZE(uiChunkSize), MAX_CHUNKS(MAX_ELEMENTS/CHUNK_SIZE), m_mStackMemPool(uiSize)
		{
			m_uiNoOfChunks = 0 ;
			m_pAllocAddressArray = new unsigned[ MAX_CHUNKS ] ;
		}

		bool AllocateChunk()
		{
			if(m_uiNoOfChunks == MAX_CHUNKS)
				return false ;

			int iTotalSize = CHUNK_SIZE * sizeof(T) ;
			unsigned uiAddress = DMM_AllocateForKernel(iTotalSize) ;

			for(unsigned i = 0; i < CHUNK_SIZE; i++)
			{
				T* pVal = reinterpret_cast<T*>(uiAddress + i * sizeof(T)) ;

				if(!m_mStackMemPool.Push(pVal))
				{
					printf("\n MemPool AllocateChunk Failed!!\n") ;
					DMM_DeAllocateForKernel(uiAddress) ;
					return false ;
				}
			}

			m_pAllocAddressArray[ m_uiNoOfChunks ] = uiAddress ;
			m_uiNoOfChunks++ ;
			return true;
		}

		bool IsMemFromPool(T* pAddress)
		{
			unsigned uiAddress = (unsigned)pAddress ;
			
			for(unsigned i = 0; i < m_uiNoOfChunks; i++)
			{
				unsigned uiStart = m_pAllocAddressArray[ i ] ;
				unsigned uiEnd = uiStart + CHUNK_SIZE * sizeof(T) ;
				if(uiAddress >= uiStart && uiAddress < uiEnd)
				{
					if(((uiAddress - uiStart) % sizeof(T)) == 0)
						return true ;
				}
			}

			return false ;
		}

	public:
		// Factory
		static MemPool<T>& CreateMemPool(unsigned uiSize, unsigned uiChunkSize = 1024)
		{
			if((uiSize % uiChunkSize) != 0)
        throw upan::exception(XLOC, "\n MemPool Creation Failure. MemPool Size: %u is not a multiple of Chunk Size: %u", uiSize, uiChunkSize);
			return *new MemPool<T>(uiSize, uiChunkSize) ;
		}

		~MemPool()
		{
			for(unsigned i = 0; i < m_uiNoOfChunks; i++)
				DMM_DeAllocateForKernel(m_pAllocAddressArray[ i ]) ;
			delete[] m_pAllocAddressArray ;
		}

		T* Create()
		{
			T* buf;
			//allocate from pool
			if(m_mStackMemPool.Pop(buf))
				return new (buf)T();

			//if pool is empty then populate pool
			AllocateChunk();

			//allocate from pool
			if(m_mStackMemPool.Pop(buf))
				return new (buf)T();

			//if pool AllocateChunk failed then allocate directly from heap
			return new T();
		}

		bool Release(T* pAddress)
		{
			if(IsMemFromPool(pAddress))
			{
				if(m_mStackMemPool.IsFull())
				{
					printf("\n MemPool is Full! which can happen only if the Pool Memory is double deallocated") ;
					return false ;
				}

				m_mStackMemPool.Push(pAddress) ;
				return true ;
			}

			delete pAddress ;
			return true ;
		}
} ;

#endif
