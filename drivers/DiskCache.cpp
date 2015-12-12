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

LFUSectorManager::LFUSectorManager(DiskDrive* pDiskDrive) : 
	m_pDiskDrive(pDiskDrive), 
	m_bReleaseListBuilt(false),
	m_uiMaxRelListSize(0.1 * m_pDiskDrive->mCache.iMaxCacheSectors),
	m_uiBuildBreak(0.05 * m_pDiskDrive->mCache.iMaxCacheSectors),
	m_bAbort(false)
{
}

bool LFUSectorManager::IsCacheFull()
{
	return (m_pDiskDrive->mCache.m_pTree->GetTotalElements() >= m_pDiskDrive->mCache.iMaxCacheSectors) ;
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

	m_pDiskDrive->mCache.m_pTree->InOrderTraverse(*this) ;

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
    auto l = m_pDiskDrive->mCache.m_pDirtyCacheList;
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

	if(!(m_pDiskDrive->mCache.m_pTree->Delete(DiskCacheKey(node.m_uiSectorID))))
	{
		printf("\n Failed to delete ranked sector: %u from the Cache BTree", node.m_uiSectorID) ;
		ProcessManager::Instance().Sleep(10000); 
		return false ;
	}

	DiskCacheKey* pKey = m_pDiskDrive->mCache.CreateKey(uiSectorID) ;
	DiskCacheValue* pVal = m_pDiskDrive->mCache.CreateValue(bDataBuffer) ;

	if(!m_pDiskDrive->mCache.m_pTree->Insert(pKey, pVal))
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
  auto l = m_pDiskDrive->mCache.m_pDirtyCacheList;
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

static void DiskCache_TaskFlushCache(DiskDrive* pDiskDrive, unsigned uiParam2)
{
	do
	{
	//	if(pDiskDrive->Mounted())
    pDiskDrive->FlushDirtyCacheSectors(10) ;
		ProcessManager::Instance().Sleep(200) ;
	} while(!pDiskDrive->mCache.bStopReleaseCacheTask) ;

	ProcessManager_EXIT() ;
}

static void DiskCache_TaskReleaseCache(DiskDrive* pDiskDrive, unsigned uiParam2)
{
	do
	{
		if(pDiskDrive->Mounted())
			pDiskDrive->mCache.m_pLFUSectorManager->Run() ;

		ProcessManager::Instance().Sleep(50) ;
	} while(!pDiskDrive->mCache.bStopReleaseCacheTask) ;

	ProcessManager_EXIT() ;
}

/***************************************************************************************************************************************/

void DiskCache_Setup(DiskDrive& diskDrive)
{
  diskDrive.EnableDiskCache(true);
	diskDrive.mCache.pDiskDrive = &diskDrive ;

	diskDrive.mCache.iMaxCacheSectors = 16384 ;
	diskDrive.mCache.m_pDestroyKeyValue = new DestroyDiskCacheKeyValue(&(diskDrive.mCache)) ;

	diskDrive.mCache.m_pTree = new BTree(diskDrive.mCache.iMaxCacheSectors) ;
	diskDrive.mCache.m_pTree->SetDestoryKeyValueCallBack(diskDrive.mCache.m_pDestroyKeyValue) ;

	diskDrive.mCache.m_pCacheKeyMemPool = MemPool<DiskCacheKey>::CreateMemPool(diskDrive.mCache.iMaxCacheSectors) ;
	diskDrive.mCache.m_pCacheValueMemPool = MemPool<DiskCacheValue>::CreateMemPool(diskDrive.mCache.iMaxCacheSectors) ;

	if(diskDrive.mCache.m_pCacheValueMemPool == NULL || diskDrive.mCache.m_pCacheKeyMemPool == NULL)
	{
		printf("\n DiskCache Setup Failed due to MemPool creation failure") ;
		return ;
	}

	//pDiskDrive->mCache.m_pTree->SetMaxElements(pDiskDrive->mCache.iMaxCacheSectors) ;
	diskDrive.mCache.m_pDirtyCacheList = new upan::list<DiskCache::SecKeyCacheValue>() ;
	diskDrive.mCache.m_pLFUSectorManager = new LFUSectorManager(&diskDrive) ;
}

void DiskCache_StartReleaseCacheTask(DiskDrive& diskDrive)
{
	diskDrive.mCache.bStopReleaseCacheTask = false;

	int pid;
	char szDCFName[64] = "dcf-";
	String_CanCat(szDCFName, diskDrive.DriveName().c_str());
	ProcessManager::Instance().CreateKernelImage((unsigned)&DiskCache_TaskFlushCache, ProcessManager::Instance().GetCurProcId(), false, (unsigned)&diskDrive, 0, &pid, szDCFName);

	char szDCRName[64] = "dcr-";
	String_CanCat(szDCRName, diskDrive.DriveName().c_str());
	ProcessManager::Instance().CreateKernelImage((unsigned)&DiskCache_TaskReleaseCache, ProcessManager::Instance().GetCurProcId(), false, (unsigned)&diskDrive, 0, &pid, szDCRName);
}

void DiskCache_StopReleaseCacheTask(DiskDrive* pDiskDrive)
{
	pDiskDrive->mCache.bStopReleaseCacheTask = true ;
}

