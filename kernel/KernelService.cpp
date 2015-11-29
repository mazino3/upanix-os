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
# include <KernelService.h>
# include <DMM.h>
# include <Display.h>
# include <AsmUtil.h>
# include <DynamicLinkLoader.h>
# include <DLLLoader.h>
# include <ProcessAllocator.h>
# include <UserManager.h>
# include <GenericUtil.h>
# include <ProcessEnv.h>
# include <MemManager.h>

KernelService::DLLAllocCopy::DLLAllocCopy(int iDLLEntryIndex, unsigned uiAllocPageCnt, unsigned uiNoOfPages) :
	m_iProcessDLLEntryIndex(iDLLEntryIndex),
	m_uiAllocatedPagesCount(uiAllocPageCnt),
	m_uiNoOfPagesForDLL(uiNoOfPages)
{
}

void KernelService::DLLAllocCopy::Process()
{
	ProcessAddressSpace* pPAS = &ProcessManager::Instance().GetAddressSpace( GetRequestProcessID() ) ;

	ProcessSharedObjectList* pPSOList = (ProcessSharedObjectList*)DLLLoader_GetProcessDLLPageAdrressForKernel(pPAS) ;
	ProcessSharedObjectList* pPSO = &pPSOList[ m_iProcessDLLEntryIndex ] ;

	if(ProcessAllocator_AllocatePagesForDLL(m_uiNoOfPagesForDLL, pPSO) != ProcessAllocator_SUCCESS)
	{
		//TODO
		// Crash the Process..... With SegFault Or OutOfMemeory Error
		KC::MDisplay().Message("\n Out Of Memory4\n", Display::WHITE_ON_BLACK()) ;
		__asm__ __volatile__("HLT") ;
	}

	if(DLLLoader_MapDLLPagesToProcess(pPAS, pPSO, m_uiAllocatedPagesCount) != DLLLoader_SUCCESS)
	{
		m_bStatus = false ;
		return ;
	}

	m_bStatus = true ;
}

KernelService::FlatAddress::FlatAddress(unsigned uiVirtualAddress) : m_uiAddress(uiVirtualAddress)
{ 
}

void KernelService::FlatAddress::Process()
{
	m_uiFlatAddress = MemManager::Instance().GetFlatAddress(m_uiAddress) ;
}

KernelService::PageFault::PageFault(unsigned uiFaultyAddress) : m_uiFaultyAddress(uiFaultyAddress)
{
}

void KernelService::PageFault::Process()
{
	if(MemManager::Instance().AllocatePage(GetRequestProcessID(), m_uiFaultyAddress) == Failure)
	{
		//TODO:
		ProcessManager_Kill(ProcessManager::GetCurrentProcessID()) ;
		m_bStatus = false ;
		return ;
	}

	m_bStatus = true ;
}

KernelService::ProcessExec::ProcessExec(int iNoOfArgs, const char* szFile, const char** szArgs)
   	: m_iNoOfArgs(iNoOfArgs), _szFile(szFile), m_szArgs(NULL)
{
	m_szArgs = new char*[iNoOfArgs] ;
	for(int i = 0; i < iNoOfArgs; i++)
	{
		m_szArgs[i] = new char[String_Length(szArgs[i]) + 1] ;
		String_Copy(m_szArgs[i], szArgs[i]) ;
	}
}

KernelService::ProcessExec::~ProcessExec()
{
	for(int i = 0; i < m_iNoOfArgs; i++)
		delete[] m_szArgs[i] ;
	delete[] m_szArgs ;
}

void KernelService::ProcessExec::Process()
{
	int iOldDDriveID ;
	FileSystem_PresentWorkingDirectory mOldPWD ;
	ProcessManager_CopyDriveInfo(GetRequestProcessID(), iOldDDriveID, mOldPWD) ;

	byte bStatus = ProcessManager_Create(_szFile.Value(), GetRequestProcessID(), true, &m_iNewProcId, DERIVE_FROM_PARENT, m_iNoOfArgs, m_szArgs) ;
	if(bStatus != ProcessManager_SUCCESS)
		m_iNewProcId = -1 ;

	int iPID = ProcessManager_GetCurProcId() ;
	ProcessManager::Instance().GetAddressSpace( iPID ).iDriveID = iOldDDriveID ;
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(mOldPWD), MemUtil_GetDS(), (unsigned)&(ProcessManager::Instance().GetAddressSpace( iPID ).processPWD),
				sizeof(FileSystem_PresentWorkingDirectory));
}

bool KernelService::RequestDLLAlloCopy(int iDLLEntryIndex, unsigned uiAllocPageCnt, unsigned uiNoOfPages)
{
	KernelService::DLLAllocCopy* pRequest = new KernelService::DLLAllocCopy(iDLLEntryIndex, uiAllocPageCnt, uiNoOfPages) ;
	AddRequest(pRequest) ;

	ProcessManager_WaitOnKernelService() ;

	bool bStatus = pRequest->GetStatus() ;

	delete pRequest ;

	return bStatus ;
}

unsigned KernelService::RequestFlatAddress(unsigned uiVirtualAddress)
{
	KernelService::FlatAddress* pRequest = new KernelService::FlatAddress(uiVirtualAddress) ;
	AddRequest(pRequest) ;

	ProcessManager_WaitOnKernelService() ;

	unsigned uiFlatAddress = pRequest->GetFlatAddress() ;

	delete pRequest ;

	return uiFlatAddress ;
}

bool KernelService::RequestPageFault(unsigned uiFaultyAddress)
{
	KernelService::PageFault* pRequest = new KernelService::PageFault(uiFaultyAddress) ;

	AddRequest(pRequest) ;
	ProcessManager_WaitOnKernelService() ;

	bool bStatus = pRequest->GetStatus() ;
	delete pRequest ;
	return bStatus ;
}

int KernelService::RequestProcessExec(const char* szFile, int iNoOfArgs, const char** szArgs)
{
	char szProcessPath[128] ;
	char szFullProcPath[128] ;
	
	if(String_Chr(szFile, '/') < 0)
	{
		if(!GenericUtil_GetFullFilePathFromEnv(PATH_ENV, BIN_PATH, szFile, szProcessPath))
		{
			return -2 ;
		}

		String_Copy(szFullProcPath, szProcessPath) ;
		String_CanCat(szFullProcPath, szFile) ;
	}
	else
	{
		String_Copy(szFullProcPath, szFile) ;
	}
	
	KernelService::ProcessExec* pRequest = new KernelService::ProcessExec(iNoOfArgs, szFullProcPath, szArgs) ;

	AddRequest(pRequest) ;
	ProcessManager_WaitOnKernelService() ;

	int iNewProcId = pRequest->GetNewProcId() ;

	delete pRequest ;

	return iNewProcId ;
}

void KernelService::AddRequest(Request* pRequest)
{
	m_mutexQRequest.Lock() ;

	m_qRequest.PushBack(pRequest) ;

	m_mutexQRequest.UnLock() ;
}

KernelService::Request* KernelService::GetRequest()
{
	m_mutexQRequest.Lock() ;

	Request* pRequest ;
	if(!m_qRequest.PopFront(pRequest))
		pRequest = NULL ;

	m_mutexQRequest.UnLock() ;

	return pRequest ;
}

void KernelService::Server(KernelService* pService)
{
	while(true)
	{
		Request* pRequest = pService->GetRequest() ;

		if(!pRequest)
		{
			ProcessManager_Sleep(10) ;
			continue ;
		}

		ProcessAddressSpace* pPAS = &ProcessManager::Instance().GetCurrentPAS();
		int iKSProcessGroupID = pPAS->iProcessGroupID ;
		pPAS->iProcessGroupID = ProcessManager::Instance().GetAddressSpace( pRequest->GetRequestProcessID() ).iProcessGroupID ;
		pRequest->Process() ;
		pPAS->iProcessGroupID = iKSProcessGroupID ;

		ProcessManager_WakeUpFromKSWait(pRequest->GetRequestProcessID()) ;
	}
}

int KernelService::Spawn()
{
	m_mutexServer.Lock() ;

	static const char* szKS = "kers-" ;
	static int iID = 0 ;

  String szName(szKS);
  szName += ToString(iID);
	iID++ ;

	int pid ;
	if(ProcessManager_CreateKernelImage((unsigned)&(KernelService::Server), ProcessManager::GetCurrentProcessID(), false, (unsigned)this, NULL, &pid, szName.Value()) !=
			ProcessManager_SUCCESS)
	{
		m_mutexServer.UnLock() ;
		printf("\n Failed to create Kernel Service Process %s", szName.Value()) ;
		return -1 ;
	}

	m_lServerList.PushBack(pid) ;

	m_mutexServer.UnLock() ;

	return pid ;
}

bool KernelService::Stop(int iServerProcessID)
{
	m_mutexServer.Lock() ;

	if(!m_lServerList.DeleteByValue(iServerProcessID))
	{
		printf("\n Invalid KernelService ProcessID %d", iServerProcessID) ;
		m_mutexServer.UnLock() ;
		return false ;
	}

	ProcessManager_Kill(iServerProcessID) ;

	m_mutexServer.UnLock() ;
	return true;
}

