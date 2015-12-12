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
# include <DiskCache.h>
# include <Floppy.h>
# include <ATADrive.h>
# include <ATADeviceController.h>
# include <DMM.h>
# include <SCSIHandler.h>
# include <stdio.h>
# include <MemUtil.h>
# include <ProcessManager.h>
# include <Display.h>

LFUSectorManager::LFUSectorManager(DiskCache& mCache) :
  _mCache(mCache),
	m_bReleaseListBuilt(false),
	m_uiMaxRelListSize(0.1 * _mCache.iMaxCacheSectors),
	m_uiBuildBreak(0.05 * _mCache.iMaxCacheSectors),
	m_bAbort(false)
{
}

bool LFUSectorManager::IsCacheFull()
{
	return (_mCache.m_pTree->GetTotalElements() >= _mCache.iMaxCacheSectors) ;
}

void LFUSectorManager::Run()
{
	// this is accessing the tree after cache is full there is no need to lock/unlock the drive mutex
	// because the tree is not altered. And this can be altered only when release cache list is built
	// but if release cache list is built it will not access the tree till release cache list is empty
	if(!IsCacheFull())
		return ;

	if(m_bReleaseListBuilt)
		return ;

	m_uiCurrent = PIT_GetClockCount() ;
	m_uiBuildCount = 0 ;

	_mCache.m_pTree->InOrderTraverse(*this) ;

	if(m_bAbort)
		return ;

	if(m_mReleaseList.size() == m_uiMaxRelListSize)
		m_bReleaseListBuilt = true ;

	if(m_bReleaseListBuilt)
		printf("\n Release List is Built") ;
}

bool LFUSectorManager::ReplaceCache(unsigned uiSectorID, byte* bDataBuffer)
{
	if(!m_bReleaseListBuilt)
	{
		//printf("\n Release List is not built yet!") ;
		return true ;
	}

	if(m_mReleaseList.empty())
	{
		m_bReleaseListBuilt = false ;
		//printf("\n Release List is empty, not rebuilt yet!") ;
		return true ;
	}

	bool bFound = false ;
	CacheRankNode node ;
	while(!m_mReleaseList.empty())
	{
    const CacheRankNode& node = m_mReleaseList.front();
    m_mReleaseList.pop_front();
		// Skip dirty sectors from replacing
    DiskCache::SecKeyCacheValue skey(node.m_uiSectorID, NULL);
    auto l = _mCache.m_pDirtyCacheList;
    if(upan::find(l->begin(), l->end(), skey) != l->end())
			continue;
		bFound = true;
		break ;
	}

	if(!bFound)
	{
		printf(" All lfu ranked nodes were marked dirty. Need to rebuild LFU list\n") ;
		m_bReleaseListBuilt = false ;
		return true ;
	}

	if(!(_mCache.m_pTree->Delete(DiskCacheKey(node.m_uiSectorID))))
	{
		printf("\n Failed to delete ranked sector: %u from the Cache BTree", node.m_uiSectorID) ;
		ProcessManager::Instance().Sleep(10000); 
		return false ;
	}

	DiskCacheKey* pKey = _mCache.CreateKey(uiSectorID) ;
	DiskCacheValue* pVal = _mCache.CreateValue(bDataBuffer) ;

	if(!_mCache.m_pTree->Insert(pKey, pVal))
	{
		printf("\n Disk Cache Insert failed!!! %s:%d", __FILE__, __LINE__) ;
		return false ;
	}

	if(m_mReleaseList.empty())
		m_bReleaseListBuilt = false ;

	return true ;
}

void LFUSectorManager::operator()(const BTreeKey& rKey, BTreeValue* pValue)
{
	const DiskCacheKey& key = static_cast<const DiskCacheKey&>(rKey) ;
	const DiskCacheValue* value = static_cast<const DiskCacheValue*>(pValue) ;

	// Skip dirty sectors from lfu ranking
  DiskCache::SecKeyCacheValue skey(key.GetSectorID(), NULL);
  auto l = _mCache.m_pDirtyCacheList;
  if(upan::find(l->begin(), l->end(), skey) != l->end())
		return ;

	unsigned uiTimeDiff = m_uiCurrent - value->GetLastAccess() ;
	double dRank = 	(double)uiTimeDiff / (double)value->GetHitCount() ;
  CacheRankNode node;
	if(m_mReleaseList.size() == m_uiMaxRelListSize)
	{
    node = m_mReleaseList.back();
		if(dRank <= node.m_dRank)
			return;
		m_mReleaseList.pop_back();
	}
	node.m_uiSectorID = key.GetSectorID() ;
	node.m_dRank = dRank ;
	m_mReleaseList.sorted_insert_asc(node);
}

DiskCache::DiskCache(DiskDrive& diskDrive) :
  pDiskDrive(&diskDrive),
  iMaxCacheSectors(16384)
{
	m_pCacheKeyMemPool = MemPool<DiskCacheKey>::CreateMemPool(iMaxCacheSectors) ;
	m_pCacheValueMemPool = MemPool<DiskCacheValue>::CreateMemPool(iMaxCacheSectors) ;

	if(m_pCacheValueMemPool == NULL || m_pCacheKeyMemPool == NULL)
	{
		printf("\n DiskCache Setup Failed due to MemPool creation failure") ;
		return;
	}

	m_pDestroyKeyValue = new DestroyDiskCacheKeyValue(this);

	m_pTree = new BTree(iMaxCacheSectors) ;
	m_pTree->SetDestoryKeyValueCallBack(m_pDestroyKeyValue) ;

	m_pDirtyCacheList = new upan::list<DiskCache::SecKeyCacheValue>() ;
	m_pLFUSectorManager = new LFUSectorManager(*this);
}

void DiskCache::InsertToDirtyList(const DiskCache::SecKeyCacheValue& v)
{
  if(upan::find(m_pDirtyCacheList->begin(), m_pDirtyCacheList->end(), v) == m_pDirtyCacheList->end())
    m_pDirtyCacheList->push_back(v);
}

DiskCacheKey* DiskCache::CreateKey(unsigned uiSectorID)
{
	DiskCacheKey* pKey = m_pCacheKeyMemPool->Create() ;
	pKey->SetSectorID(uiSectorID) ;
	return pKey ;
}

DiskCacheValue* DiskCache::CreateValue(const byte* pSrc)
{
	DiskCacheValue* pValue = m_pCacheValueMemPool->Create() ;
	pValue->Write(pSrc) ;
	return pValue ;
}
