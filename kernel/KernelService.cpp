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

void KernelService::DLLAllocCopy::Execute() {
  try {
    Process &pas = ProcessManager::Instance().GetAddressSpace(GetRequestProcessID()).value();
    pas.MapDLLPagesToProcess(m_iProcessDLLEntryIndex, m_uiNoOfPagesForDLL, m_uiAllocatedPagesCount);
  } catch(upan::exception& e) {
    e.Print();
    m_bStatus = false;
    return;
  }
	m_bStatus = true ;
}

KernelService::FlatAddress::FlatAddress(unsigned uiVirtualAddress) : m_uiAddress(uiVirtualAddress)
{ 
}

void KernelService::FlatAddress::Execute()
{
	m_uiFlatAddress = MemManager::Instance().GetFlatAddress(m_uiAddress) ;
}

KernelService::PageFault::PageFault(unsigned uiFaultyAddress) : m_uiFaultyAddress(uiFaultyAddress)
{
}

void KernelService::PageFault::Execute() {
  m_bStatus = MemManager::Instance().AllocatePage(GetRequestProcessID(), m_uiFaultyAddress) == Success;
}

KernelService::ProcessExec::ProcessExec(int iNoOfArgs, const char* szFile, const char** szArgs)
   	: m_iNoOfArgs(iNoOfArgs), _szFile(szFile), m_szArgs(NULL)
{
	m_szArgs = new char*[iNoOfArgs] ;
	for(int i = 0; i < iNoOfArgs; i++)
	{
		m_szArgs[i] = new char[strlen(szArgs[i]) + 1] ;
		strcpy(m_szArgs[i], szArgs[i]) ;
	}
}

KernelService::ProcessExec::~ProcessExec()
{
	for(int i = 0; i < m_iNoOfArgs; i++)
		delete[] m_szArgs[i] ;
	delete[] m_szArgs ;
}

void KernelService::ProcessExec::Execute()
{
	int iOldDDriveID ;
	FileSystem::PresentWorkingDirectory mOldPWD ;
	ProcessManager::Instance().CopyDiskDrive(GetRequestProcessID(), iOldDDriveID, mOldPWD) ;

	byte bStatus = ProcessManager::Instance().Create(_szFile.c_str(), GetRequestProcessID(), true, &m_iNewProcId, DERIVE_FROM_PARENT, m_iNoOfArgs, m_szArgs) ;
	if(bStatus != ProcessManager_SUCCESS)
		m_iNewProcId = -1 ;

	int iPID = ProcessManager::Instance().GetCurProcId() ;
	ProcessManager::Instance().GetAddressSpace( iPID ).value().setDriveID(iOldDDriveID);
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(mOldPWD), MemUtil_GetDS(),
                    (unsigned)&(ProcessManager::Instance().GetAddressSpace( iPID ).value().processPWD()),
                    sizeof(FileSystem::PresentWorkingDirectory));
}

void KernelService::ThreadExec::Execute() {
  if(!ProcessManager::Instance().CreateThreadTask(GetRequestProcessID(), _entryAddress, _arg, _threadID)) {
    _threadID = -1;
  }
}

bool KernelService::RequestDLLAlloCopy(int iDLLEntryIndex, unsigned uiAllocPageCnt, unsigned uiNoOfPages)
{
	KernelService::DLLAllocCopy* pRequest = new KernelService::DLLAllocCopy(iDLLEntryIndex, uiAllocPageCnt, uiNoOfPages) ;
	AddRequest(pRequest) ;

	ProcessManager::Instance().WaitOnKernelService() ;

	bool bStatus = pRequest->GetStatus() ;

	delete pRequest ;

	return bStatus ;
}

unsigned KernelService::RequestFlatAddress(unsigned uiVirtualAddress)
{
	KernelService::FlatAddress* pRequest = new KernelService::FlatAddress(uiVirtualAddress) ;
	AddRequest(pRequest) ;

	ProcessManager::Instance().WaitOnKernelService() ;

	unsigned uiFlatAddress = pRequest->GetFlatAddress() ;

	delete pRequest ;

	return uiFlatAddress ;
}

bool KernelService::RequestPageFault(unsigned uiFaultyAddress)
{
	KernelService::PageFault* pRequest = new KernelService::PageFault(uiFaultyAddress) ;

	AddRequest(pRequest) ;
	ProcessManager::Instance().WaitOnKernelService() ;

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

		strcpy(szFullProcPath, szProcessPath) ;
		strcat(szFullProcPath, szFile) ;
	}
	else
	{
		strcpy(szFullProcPath, szFile) ;
	}
	
	KernelService::ProcessExec* pRequest = new KernelService::ProcessExec(iNoOfArgs, szFullProcPath, szArgs) ;

	AddRequest(pRequest) ;
	ProcessManager::Instance().WaitOnKernelService() ;

	int iNewProcId = pRequest->GetNewProcId() ;

	delete pRequest ;

	return iNewProcId ;
}

int KernelService::RequestThreadExec(uint32_t entryAddresss, void* arg) {
  KernelService::ThreadExec* pRequest = new KernelService::ThreadExec(entryAddresss, arg);

  AddRequest(pRequest) ;
  ProcessManager::Instance().WaitOnKernelService() ;

  int threadID = pRequest->GetThreadID();
  delete pRequest;
  return threadID;
}

void KernelService::AddRequest(Request* pRequest)
{
  MutexGuard g(m_mutexQRequest);
	m_qRequest.push_back(pRequest) ;
}

KernelService::Request* KernelService::GetRequest()
{
  MutexGuard g(m_mutexQRequest);

	Request* pRequest = NULL;
  if(!m_qRequest.empty())
  {
    pRequest = m_qRequest.front();
    m_qRequest.pop_front();
  }

	return pRequest ;
}

void KernelService::Server(KernelService* pService)
{
	while(true)
	{
		Request* pRequest = pService->GetRequest() ;
		if(!pRequest)
		{
			ProcessManager::Instance().Sleep(10) ;
			continue ;
		}

		Process* pPAS = &ProcessManager::Instance().GetCurrentPAS();
		auto ksProcessGroupID = pPAS->processGroup();
		pPAS->setProcessGroup(ProcessManager::Instance().GetAddressSpace( pRequest->GetRequestProcessID() ).value().processGroup());
    pRequest->Execute() ;
		pPAS->setProcessGroup(ksProcessGroupID);
    
		ProcessManager::Instance().WakeUpFromKSWait(pRequest->GetRequestProcessID()) ;
	}
}

int KernelService::Spawn()
{
  MutexGuard g(m_mutexServer);

	static const char* szKS = "kers-" ;
	static int iID = 0 ;

  upan::string szName(szKS);
  szName += upan::string::to_string(iID);
	iID++ ;

	int pid ;
	if(ProcessManager::Instance().CreateKernelProcess(szName, (unsigned) &(KernelService::Server),
                                                    ProcessManager::GetCurrentProcessID(),
                                                    false, (unsigned) this, NULL, &pid) !=
			ProcessManager_SUCCESS)
	{
		printf("\n Failed to create Kernel Service Process %s", szName.c_str()) ;
		return -1 ;
	}

	m_lServerList.push_back(pid) ;

	return pid ;
}

bool KernelService::Stop(int iServerProcessID)
{
  MutexGuard g(m_mutexServer);

	if(!m_lServerList.erase(iServerProcessID))
	{
		printf("\n Invalid KernelService ProcessID %d", iServerProcessID) ;
		return false ;
	}

	ProcessManager::Instance().Kill(iServerProcessID) ;

	return true;
}

