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
#ifndef _DISK_CACHE_H_
#define _DISK_CACHE_H_

#include <BTree.h>
#include <List.h>
#include <PIT.h>
#include <StringUtil.h>
#include <MemPool.h>

#define DiskCache_SUCCESS 0
#define DiskCache_FAILURE 1

struct DriveInfo ;
typedef struct DriveInfo DriveInfo ;

class DiskCacheKey : public BTreeKey
{
	private:
		unsigned m_uiSectorID ;

	public:
		DiskCacheKey() : m_uiSectorID(0) { }
		DiskCacheKey(const unsigned uiSectorID) : m_uiSectorID(uiSectorID) { }

		virtual bool operator<(const BTreeKey& rKey) const
		{
			const DiskCacheKey& rhs = static_cast<const DiskCacheKey&>(rKey) ;

			return m_uiSectorID < rhs.GetSectorID() ;
		}

		unsigned GetSectorID() const { return m_uiSectorID ; }
		void SetSectorID(const unsigned& uiSectorID) { m_uiSectorID = uiSectorID ; }
} ;

class DiskCacheValue : public BTreeValue
{
	private:
		static const unsigned SEC_SIZE = 512 ;
		unsigned m_uiLastAccess ;
		unsigned m_uiHitCount ;
		byte m_bSectorBuffer[ SEC_SIZE ] ;

	public:
		DiskCacheValue() : m_uiLastAccess(PIT_GetClockCount()), m_uiHitCount(1)
   		{ 
		}

		byte* GetSectorBuffer() { return m_bSectorBuffer ; }

		inline unsigned GetHitCount() const { return m_uiHitCount ; }
		inline unsigned GetLastAccess() const { return m_uiLastAccess ; }

		void Read(byte* pDest)
		{
			memcpy(pDest, m_bSectorBuffer, SEC_SIZE) ;
			m_uiHitCount++ ;
			m_uiLastAccess = PIT_GetClockCount() ;
		}

		void Write(const byte* pSrc)
		{
			memcpy(m_bSectorBuffer, pSrc, SEC_SIZE) ;
			m_uiHitCount++ ;
			m_uiLastAccess = PIT_GetClockCount() ;
		}
} ;

class LFUSectorManager : public BTree::InOrderVisitor
{
	private:
		class CacheRankNode	
		{
			public:
				unsigned m_uiSectorID ;
				double m_dRank ;

				inline bool operator<(const CacheRankNode& rhs) const 
				{
					return m_dRank < rhs.m_dRank ;
				}
		} ;

		List<CacheRankNode> m_mReleaseList ;

		DriveInfo* m_pDriveInfo ;
		unsigned m_bReleaseListBuilt ; 
		unsigned m_uiMaxRelListSize ;
		unsigned m_uiBuildBreak ;
		bool m_bAbort ;

		unsigned m_uiCurrent ;
		unsigned m_uiBuildCount ;

	private:
		inline bool IsCacheFull() ;

	public:
		LFUSectorManager(DriveInfo* pDriveInfo) ;

		void Run() ;
		bool ReplaceCache(unsigned uiSectorID, byte* bDataBuffer) ;
		void operator()(const BTreeKey& rKey, BTreeValue* pValue) ;

		bool Abort() const { return m_bAbort ; }
} ;

class DestroyDiskCacheKeyValue ;

struct DiskCache
{
	public:
		class SecKeyCacheValue
		{
			public:
				SecKeyCacheValue() { } 
				SecKeyCacheValue(unsigned uiSectorID, byte* pBuffer) : m_uiSectorID(uiSectorID), m_pSectorBuffer(pBuffer) { }

				inline bool operator==(const SecKeyCacheValue& rhs)
				{
					return m_uiSectorID == rhs.m_uiSectorID ;
				}

				inline SecKeyCacheValue& operator=(const SecKeyCacheValue& rhs)
				{
					m_uiSectorID = rhs.m_uiSectorID ;
					m_pSectorBuffer = rhs.m_pSectorBuffer ;

					return *this ;
				}

				unsigned m_uiSectorID ;
				byte* m_pSectorBuffer ;
		} ;

	public:
		BTree* m_pTree ;
		DestroyDiskCacheKeyValue* m_pDestroyKeyValue ;

		MemPool<DiskCacheKey>* m_pCacheKeyMemPool ;
		MemPool<DiskCacheValue>* m_pCacheValueMemPool ;

		List<SecKeyCacheValue>* m_pDirtyCacheList ;
		LFUSectorManager* m_pLFUSectorManager ;

		DriveInfo* pDriveInfo ;
		int iMaxCacheSectors ;
		int bStopReleaseCacheTask ;
};

class DestroyDiskCacheKeyValue : public BTree::DestroyKeyValue
{
	private:
		DiskCache* m_pCache ;

	public:
		DestroyDiskCacheKeyValue(DiskCache* pCache) : m_pCache(pCache) { }

		void DestroyKey(BTreeKey* pKey)
		{
			m_pCache->m_pCacheKeyMemPool->Release(static_cast<DiskCacheKey*>(pKey)) ;
		}

		void DestroyValue(BTreeValue* pValue)
		{
			m_pCache->m_pCacheValueMemPool->Release(static_cast<DiskCacheValue*>(pValue)) ;
		}
} ;

void DiskCache_Setup(DriveInfo* pDriveInfo) ;
byte DiskCache_Read(DriveInfo* pDriveInfo, unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer) ;
byte DiskCache_Write(DriveInfo* pDriveInfo, unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer) ;
void DiskCache_StopReleaseCacheTask(DriveInfo* pDriveInfo) ;
void DiskCache_StartReleaseCacheTask(DriveInfo* pDriveInfo) ;
byte DiskCache_FlushDirtyCacheSectors(DriveInfo* pDriveInfo, int iCount = -1) ; // Negative iCount => All Dirty Sectors

#endif 
