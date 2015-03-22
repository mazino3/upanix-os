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
#ifndef _KERNEL_SERVICE_H_
#define _KERNEL_SERVICE_H_

#include <Global.h>
#include <Atomic.h>
#include <Queue.h>
#include <ProcessManager.h>

class KernelService
{
	private:
		class Request
		{
			private:
				int m_iRequestProcessID ;

			public:
				Request() : m_iRequestProcessID(ProcessManager_GetCurProcId()) { }
				virtual ~Request() { }
				virtual void Process() = 0 ;

				inline int GetRequestProcessID() { return m_iRequestProcessID ; }	
		} ;

	private:
		Queue<Request*> m_qRequest ;
		Mutex m_mutexQRequest ;

		List<int> m_lServerList ;
		Mutex m_mutexServer ;


	public:
		KernelService() { }
		~KernelService() { }

		int Spawn() ;
		bool Stop(int iServerProcessID) ;

		// RequestFactory
		bool RequestDLLAlloCopy(int iDLLEntryIndex, unsigned uiAllocPageCnt, unsigned uiNoOfPages) ;
		unsigned RequestFlatAddress(unsigned uiAddress) ;
		bool RequestPageFault(unsigned uiFaultyAddress) ;
		int RequestProcessExec(const char* szFile, int iNoOfArgs, const char** szArgs) ;

	private:
		static void Server(KernelService* pService, int i) ;

		void AddRequest(Request* pRequest) ;
		Request* GetRequest() ;

	private:
		class DLLAllocCopy : public Request
		{
			private:
				int m_iProcessDLLEntryIndex ;
				unsigned m_uiAllocatedPagesCount ;
				unsigned m_uiNoOfPagesForDLL ;
				bool m_bStatus ;

			public:
				DLLAllocCopy(int iDLLEntryIndex, unsigned uiAllocPageCnt, unsigned uiNoOfPages) ;
				void Process() ;
				inline bool GetStatus() { return m_bStatus ; }
		} ;

		class FlatAddress : public Request
		{
			private:
				unsigned m_uiAddress ;
				unsigned m_uiFlatAddress ;

			public:
				FlatAddress(unsigned uiVirtualAddress) ;
				void Process() ;
				inline unsigned GetFlatAddress() { return m_uiFlatAddress ; }
		} ;

		class PageFault : public Request
		{
			private:
				unsigned m_uiFaultyAddress ;
				bool m_bStatus ;

			public:
				PageFault(unsigned uiFaultyAddress) ;
				void Process() ;
				inline bool GetStatus() { return m_bStatus ; }
		} ;

		class ProcessExec : public Request
		{
			private:
				int m_iNoOfArgs ;
				char* m_szFile ;
				char** m_szArgs ;
				int m_iNewProcId ;

			public:
				ProcessExec(int iNoOfArgs, const char* szFile, const char** szArgs) ;
				~ProcessExec() ;
				void Process() ;
				inline int GetNewProcId() { return m_iNewProcId ; }
		} ;
} ;

#endif
