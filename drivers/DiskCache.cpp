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

static byte DiskCache_RawWrite(DriveInfo* pDriveInfo, unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer) ;
static DiskCacheKey* DiskCache_CreateKey(DiskCache* pCache, unsigned uiSectorID) ;
static DiskCacheValue* DiskCache_CreateValue(DiskCache* pCache, const byte* pSrc) ;

LFUSectorManager::LFUSectorManager(DriveInfo* pDriveInfo) : 
	m_pDriveInfo(pDriveInfo), 
	m_bReleaseListBuilt(false),
	m_uiMaxRelListSize(0.1 * m_pDriveInfo->mCache.iMaxCacheSectors),
	m_uiBuildBreak(0.05 * m_pDriveInfo->mCache.iMaxCacheSectors),
	m_bAbort(false)
{
}

bool LFUSectorManager::IsCacheFull()
{
	return (m_pDriveInfo->mCache.m_pTree->GetTotalElements() >= m_pDriveInfo->mCache.iMaxCacheSectors) ;
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

	m_pDriveInfo->mCache.m_pTree->InOrderTraverse(*this) ;

	if(m_bAbort)
		return ;

	if(m_mReleaseList.Size() == m_uiMaxRelListSize)
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

	if(m_mReleaseList.Size() == 0)
	{
		m_bReleaseListBuilt = false ;
		//printf("\n Release List is empty, not rebuilt yet!") ;
		return true ;
	}

	bool bFound = false ;
	CacheRankNode node ;
	while(m_mReleaseList.Size() > 0)
	{
		if(!m_mReleaseList.PopFront(node))
		{
			printf("\n Failed to remove an element from Release List!!") ;
			return false ;
		}

		// Skip dirty sectors from replacing
		if(m_pDriveInfo->mCache.m_pDirtyCacheList->FindByValue(DiskCache::SecKeyCacheValue(node.m_uiSectorID, NULL)))
			continue ;

		bFound = true ;
		break ;
	}

	if(!bFound)
	{
		printf(" All lfu ranked nodes were marked dirty. Need to rebuild LFU list\n") ;
		m_bReleaseListBuilt = false ;
		return true ;
	}

	if(!(m_pDriveInfo->mCache.m_pTree->Delete(DiskCacheKey(node.m_uiSectorID))))
	{
		printf("\n Failed to delete ranked sector: %u from the Cache BTree", node.m_uiSectorID) ;
		ProcessManager::Instance().Sleep(10000); 
		return false ;
	}

	DiskCacheKey* pKey = DiskCache_CreateKey(&(m_pDriveInfo->mCache), uiSectorID) ;
	DiskCacheValue* pVal = DiskCache_CreateValue(&(m_pDriveInfo->mCache), bDataBuffer) ;

	if(!m_pDriveInfo->mCache.m_pTree->Insert(pKey, pVal))
	{
		printf("\n Disk Cache Insert failed!!! %s:%d", __FILE__, __LINE__) ;
		return false ;
	}

	if(m_mReleaseList.Size() == 0)
		m_bReleaseListBuilt = false ;

	return true ;
}

void LFUSectorManager::operator()(const BTreeKey& rKey, BTreeValue* pValue)
{
	const DiskCacheKey& key = static_cast<const DiskCacheKey&>(rKey) ;
	const DiskCacheValue* value = static_cast<const DiskCacheValue*>(pValue) ;

	// Skip dirty sectors from lfu ranking
	if(m_pDriveInfo->mCache.m_pDirtyCacheList->FindByValue(DiskCache::SecKeyCacheValue(key.GetSectorID(), NULL)))
		return ;

	CacheRankNode node ;
	unsigned uiTimeDiff = m_uiCurrent - value->GetLastAccess() ;
	double dRank = 	(double)uiTimeDiff / (double)value->GetHitCount() ;

	if(m_mReleaseList.Size() == m_uiMaxRelListSize)
	{
		if(!m_mReleaseList.Last(node))
		{
			printf("\n Unexpected error in ReleaseCacheList Build") ;
			m_bAbort = true ;
			return ;
		}

		if(dRank <= node.m_dRank)
			return ;

		m_mReleaseList.PopBack(node) ;
	}
	
	node.m_uiSectorID = key.GetSectorID() ;
	node.m_dRank = dRank ;
	m_mReleaseList.SortedInsert(node, true) ;
}

static void DiskCache_InsertToDirtyList(DiskCache* pCache, const DiskCache::SecKeyCacheValue& v)
{
	if(!pCache->m_pDirtyCacheList->FindByValue(v))
		pCache->m_pDirtyCacheList->PushBack(v) ;
}

static DiskCacheKey* DiskCache_CreateKey(DiskCache* pCache, unsigned uiSectorID)
{
	DiskCacheKey* pKey = pCache->m_pCacheKeyMemPool->Create() ;
	pKey->SetSectorID(uiSectorID) ;
	return pKey ;
}

static DiskCacheValue* DiskCache_CreateValue(DiskCache* pCache, const byte* pSrc)
{
	DiskCacheValue* pValue = pCache->m_pCacheValueMemPool->Create() ;
	pValue->Write(pSrc) ;
	return pValue ;
}

bool DiskCache_FlushSector(DriveInfo* pDriveInfo, const unsigned& uiSectorID, const byte* pBuffer)
{
	if(!pBuffer)
		return false ;

	if(DiskCache_RawWrite(pDriveInfo, uiSectorID, 1, (byte*)pBuffer) != DiskCache_SUCCESS)
		return false ;

	return true ;
}

static unsigned uiTotalFloppyDiskReads = 0;
static unsigned uiTotalATADiskReads = 0;
static unsigned uiTotalUSBDiskReads = 0;

void DiskCache_ShowTotalDiskReads()
{
	printf("\n Total Floppy Disk Reads: %u", uiTotalFloppyDiskReads) ;
	printf("\n Total ATA Disk Reads: %u", uiTotalATADiskReads) ;
	printf("\n Total USB Disk Reads: %u", uiTotalUSBDiskReads) ;
}

static byte DiskCache_RawRead(DriveInfo* pDriveInfo, unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
	byte bStatus ;
	switch(pDriveInfo->drive.deviceType)
	{
	case DEV_FLOPPY:
		uiTotalFloppyDiskReads++ ;
		bStatus = Floppy_Read(&pDriveInfo->drive, uiStartSector, uiStartSector + uiNoOfSectors, bDataBuffer) ;
		break ;

	case DEV_ATA_IDE:
		uiTotalATADiskReads++ ;
		bStatus = ATADrive_Read((ATAPort*)pDriveInfo->pDevice, uiStartSector, bDataBuffer, uiNoOfSectors) ;
		break ;

	case DEV_SCSI_USB_DISK:
		uiTotalUSBDiskReads++ ;
		bStatus = SCSIHandler_GenericRead((SCSIDevice*)pDriveInfo->pDevice, uiStartSector, uiNoOfSectors, bDataBuffer) ;
		break ;

	default:
		bStatus = DeviceDrive_ERR_UNKNOWN_DEVICE_TYPE ;
	}

	return bStatus ;
}

static byte DiskCache_RawWrite(DriveInfo* pDriveInfo, unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
	byte bStatus ;
	switch(pDriveInfo->drive.deviceType)
	{
	case DEV_FLOPPY:
		bStatus = Floppy_Write(&pDriveInfo->drive, uiStartSector, uiStartSector + uiNoOfSectors, bDataBuffer) ;
		break ;

	case DEV_ATA_IDE:
		bStatus = ATADrive_Write((ATAPort*)pDriveInfo->pDevice, uiStartSector, bDataBuffer, uiNoOfSectors) ;
		break ;

	case DEV_SCSI_USB_DISK:
		bStatus = SCSIHandler_GenericWrite((SCSIDevice*)pDriveInfo->pDevice, uiStartSector, uiNoOfSectors, bDataBuffer) ;
		break ;

	default:
		bStatus = DeviceDrive_ERR_UNKNOWN_DEVICE_TYPE ;
	}

	return bStatus ;
}

static void DiskCache_TaskFlushCache(DriveInfo* pDriveInfo, unsigned uiParam2)
{
	do
	{
	//	if(pDriveInfo->drive.bMounted)
			DiskCache_FlushDirtyCacheSectors(pDriveInfo, 10) ;

		ProcessManager::Instance().Sleep(200) ;
	} while(!pDriveInfo->mCache.bStopReleaseCacheTask) ;

	ProcessManager_EXIT() ;
}

static void DiskCache_TaskReleaseCache(DriveInfo* pDriveInfo, unsigned uiParam2)
{
	do
	{
		if(pDriveInfo->drive.bMounted)
			pDriveInfo->mCache.m_pLFUSectorManager->Run() ;

		ProcessManager::Instance().Sleep(50) ;
	} while(!pDriveInfo->mCache.bStopReleaseCacheTask) ;

	ProcessManager_EXIT() ;
}

/***************************************************************************************************************************************/

void DiskCache_Setup(DriveInfo* pDriveInfo)
{
	pDriveInfo->bEnableDiskCache = true ;
	pDriveInfo->mCache.pDriveInfo = pDriveInfo ;

	pDriveInfo->mCache.iMaxCacheSectors = 16384 ;
	pDriveInfo->mCache.m_pDestroyKeyValue = new DestroyDiskCacheKeyValue(&(pDriveInfo->mCache)) ;

	pDriveInfo->mCache.m_pTree = new BTree(pDriveInfo->mCache.iMaxCacheSectors) ;
	pDriveInfo->mCache.m_pTree->SetDestoryKeyValueCallBack(pDriveInfo->mCache.m_pDestroyKeyValue) ;

	pDriveInfo->mCache.m_pCacheKeyMemPool = MemPool<DiskCacheKey>::CreateMemPool(pDriveInfo->mCache.iMaxCacheSectors) ;
	pDriveInfo->mCache.m_pCacheValueMemPool = MemPool<DiskCacheValue>::CreateMemPool(pDriveInfo->mCache.iMaxCacheSectors) ;

	if(pDriveInfo->mCache.m_pCacheValueMemPool == NULL || pDriveInfo->mCache.m_pCacheKeyMemPool == NULL)
	{
		printf("\n DiskCache Setup Failed due to MemPool creation failure") ;
		return ;
	}

	//pDriveInfo->mCache.m_pTree->SetMaxElements(pDriveInfo->mCache.iMaxCacheSectors) ;
	pDriveInfo->mCache.m_pDirtyCacheList = new List<DiskCache::SecKeyCacheValue>() ;
	pDriveInfo->mCache.m_pLFUSectorManager = new LFUSectorManager(pDriveInfo) ;
}

byte DiskCache_Read(DriveInfo* pDriveInfo, unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
	uiStartSector += pDriveInfo->drive.uiLBAStartSector ;

	if(!pDriveInfo->bEnableDiskCache)
	{
		return DiskCache_RawRead(pDriveInfo, uiStartSector, uiNoOfSectors, bDataBuffer) ;
	}

	DiskCacheValue* pCacheValue[ uiNoOfSectors ] ;
	DiskCache* pCache = &(pDriveInfo->mCache) ;

	unsigned uiEndSector = uiStartSector + uiNoOfSectors ;
	unsigned uiSectorIndex ;
	unsigned uiFirstBreak, uiLastBreak ;
	uiFirstBreak = uiLastBreak = uiEndSector ;

	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		unsigned uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ] = static_cast<DiskCacheValue*>(pCache->m_pTree->Find(DiskCacheKey(uiSectorIndex))) ;
		if(!pCacheValue[ uiIndex ])
		{
			if(uiFirstBreak == uiEndSector)
				uiFirstBreak = uiSectorIndex ;

			uiLastBreak = uiSectorIndex ;
		}
	}

	__volatile__ unsigned uiIndex ;
	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiFirstBreak; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Read(bDataBuffer + (uiIndex * 512)) ;
	}

	for(uiSectorIndex = uiLastBreak + 1; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Read(bDataBuffer + (uiIndex * 512)) ;
	}

	if(uiFirstBreak < uiEndSector)
	{
		uiIndex = uiFirstBreak - uiStartSector ;

		byte bStatus ;
		RETURN_IF_NOT(bStatus, DiskCache_RawRead(pDriveInfo, uiFirstBreak, uiLastBreak - uiFirstBreak + 1, (bDataBuffer + uiIndex * 512)), DiskCache_SUCCESS) ;

		for(uiSectorIndex = uiFirstBreak; uiSectorIndex <= uiLastBreak; uiSectorIndex++)
		{
			uiIndex = uiSectorIndex - uiStartSector ;

			if(!pCacheValue[ uiIndex ])
			{
				if(pCache->m_pTree->GetTotalElements() >= pCache->iMaxCacheSectors)
				{
					if(!pCache->m_pLFUSectorManager->ReplaceCache(uiSectorIndex, bDataBuffer + (uiIndex * 512)))
					{
						printf("\n Cache Error: Disabling Disk Cache") ;
						pDriveInfo->bEnableDiskCache = false ;
						return DiskCache_SUCCESS ;
					}

					continue ;
				}

				DiskCacheKey* pKey = DiskCache_CreateKey(pCache, uiSectorIndex) ;
				DiskCacheValue* pVal = DiskCache_CreateValue(pCache, bDataBuffer + (uiIndex * 512)) ;

				if(!pCache->m_pTree->Insert(pKey, pVal))
				{
					printf("\n Disk Cache failed Insert for SectorID: %d", uiSectorIndex) ;
					printf("\n Disabling Disk Cache!!! %s:%d", __FILE__, __LINE__) ;
					pDriveInfo->bEnableDiskCache = false ;
					return DiskCache_SUCCESS ;
				}
			}
			else
			{
				pCacheValue[ uiIndex ]->Read(bDataBuffer + (uiIndex * 512)) ;
			}
		}
	}
	
	return DiskCache_SUCCESS ;
}

byte DiskCache_Write(DriveInfo* pDriveInfo, unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
	uiStartSector += pDriveInfo->drive.uiLBAStartSector ;
	
	if(!pDriveInfo->bEnableDiskCache)
	{
		return DiskCache_RawWrite(pDriveInfo, uiStartSector, uiNoOfSectors, bDataBuffer) ;
	}

	DiskCacheValue* pCacheValue[ uiNoOfSectors ] ;
	DiskCache* pCache = &(pDriveInfo->mCache) ;

	unsigned uiEndSector = uiStartSector + uiNoOfSectors ;
	unsigned uiSectorIndex ;
	unsigned uiFirstBreak, uiLastBreak ;
	uiFirstBreak = uiLastBreak = uiEndSector ;

	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		unsigned uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ] = static_cast<DiskCacheValue*>(pCache->m_pTree->Find(DiskCacheKey(uiSectorIndex))) ;

		if(!pCacheValue[ uiIndex ])
		{
			if(uiFirstBreak == uiEndSector)
				uiFirstBreak = uiSectorIndex ;

			uiLastBreak = uiSectorIndex ;
		}
	}

	__volatile__ unsigned uiIndex ;
	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiFirstBreak; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Write(bDataBuffer + (uiIndex * 512)) ;
		DiskCache_InsertToDirtyList(pCache, DiskCache::SecKeyCacheValue(uiSectorIndex, pCacheValue[ uiIndex ]->GetSectorBuffer())) ;
	}

	for(uiSectorIndex = uiLastBreak + 1; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Write(bDataBuffer + (uiIndex * 512)) ;
		DiskCache_InsertToDirtyList(pCache, DiskCache::SecKeyCacheValue(uiSectorIndex, pCacheValue[ uiIndex ]->GetSectorBuffer())) ;
	}

	if(uiFirstBreak < uiEndSector)
	{
		uiIndex = uiFirstBreak - uiStartSector ;

		//byte bStatus ;
		//RETURN_IF_NOT(bStatus, DiskCache_RawWrite(pDriveInfo, uiFirstBreak, uiLastBreak - uiFirstBreak + 1, (bDataBuffer + uiIndex * 512)), DiskCache_SUCCESS) ;

		for(uiSectorIndex = uiFirstBreak; uiSectorIndex <= uiLastBreak; uiSectorIndex++)
		{
			uiIndex = uiSectorIndex - uiStartSector ;

			if(!pCacheValue[ uiIndex ])
			{
				if(pCache->m_pTree->GetTotalElements() >= pCache->iMaxCacheSectors)
				{
					byte bStatus ;
					RETURN_IF_NOT(bStatus, 
							DiskCache_RawWrite(pDriveInfo, uiSectorIndex, 1, (bDataBuffer + uiIndex * 512)), 
							DiskCache_SUCCESS) ;

					if(!pCache->m_pLFUSectorManager->ReplaceCache(uiSectorIndex, bDataBuffer + (uiIndex * 512)))
					{
						printf("\n Cache Error: Disabling Disk Cache") ;
						pDriveInfo->bEnableDiskCache = false ;
						return DiskCache_RawWrite(pDriveInfo, uiFirstBreak, uiLastBreak - uiFirstBreak + 1, (bDataBuffer + uiIndex * 512)) ;
					}
					continue ;
				}
			
				DiskCacheKey* pKey = DiskCache_CreateKey(pCache, uiSectorIndex) ;
				DiskCacheValue* pVal = DiskCache_CreateValue(pCache, bDataBuffer + (uiIndex * 512)) ;

				if(!pCache->m_pTree->Insert(pKey, pVal))
				{
					printf("\n Disk Cache failed Insert. Disabling Disk Cache!!! %s:%d", __FILE__, __LINE__) ;
					pDriveInfo->bEnableDiskCache = false ;
					return DiskCache_SUCCESS ;
				}

				DiskCache_InsertToDirtyList(pCache, DiskCache::SecKeyCacheValue(uiSectorIndex, pVal->GetSectorBuffer())) ;
			}
			else
			{
				pCacheValue[ uiIndex ]->Write(bDataBuffer + (uiIndex * 512)) ;
				DiskCache_InsertToDirtyList(pCache, DiskCache::SecKeyCacheValue(uiSectorIndex, pCacheValue[ uiIndex ]->GetSectorBuffer())) ;
			}
		}
	}
	
	return DiskCache_SUCCESS ;
}

byte DiskCache_FlushDirtyCacheSectors(DriveInfo* pDriveInfo, int iCount)
{
	if(!pDriveInfo->bEnableDiskCache)
		return DiskCache_SUCCESS ;

	pDriveInfo->mDriveMutex.Lock() ;

	DiskCache::SecKeyCacheValue v ;
	DiskCache* pCache = &(pDriveInfo->mCache) ;

	if(pCache->m_pDirtyCacheList->Size())
	{
		while(iCount != 0)
		{
			if(pCache->m_pDirtyCacheList->PopFront(v))
			{
				if(!DiskCache_FlushSector(pDriveInfo, v.m_uiSectorID, v.m_pSectorBuffer))
				{
					printf("\n Flushing Sector %u to Drive %s failed !!", v.m_uiSectorID, pDriveInfo->drive.driveName) ;
					pDriveInfo->mDriveMutex.UnLock() ;
					return DiskCache_FAILURE ;
				}
			}
			else
				break ;

			--iCount ;
		}
	}

	pDriveInfo->mDriveMutex.UnLock() ;

	return DiskCache_SUCCESS ;
}

void DiskCache_StartReleaseCacheTask(DriveInfo* pDriveInfo)
{
	pDriveInfo->mCache.bStopReleaseCacheTask = false ;

	int pid ;
	char szDCFName[64] = "dcf-" ;
	String_CanCat(szDCFName, pDriveInfo->drive.driveName) ;
	ProcessManager::Instance().CreateKernelImage((unsigned)&DiskCache_TaskFlushCache, ProcessManager::Instance().GetCurProcId(), false, (unsigned)pDriveInfo, 0, &pid, szDCFName) ;

	char szDCRName[64] = "dcr-" ;
	String_CanCat(szDCRName, pDriveInfo->drive.driveName) ;
	ProcessManager::Instance().CreateKernelImage((unsigned)&DiskCache_TaskReleaseCache, ProcessManager::Instance().GetCurProcId(), false, (unsigned)pDriveInfo, 0, &pid, szDCRName) ;
}

void DiskCache_StopReleaseCacheTask(DriveInfo* pDriveInfo)
{
	pDriveInfo->mCache.bStopReleaseCacheTask = true ;
}

