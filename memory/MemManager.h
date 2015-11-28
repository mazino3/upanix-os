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
#ifndef _MEM_MANAGER_H_
#define _MEM_MANAGER_H_

#include <Global.h>
#include <MemConstants.h>
#include <ProcessConstants.h>
#include <ReturnHandler.h>

#define KERNEL_PROCESS_PDE_ID	1022

extern "C"
{
	extern unsigned MEM_PDBR ;
	void Mem_EnablePaging() ;
	void Mem_FlushTLB() ;
	void Mem_FlushTLBPage(unsigned uiPageNumber) ;
}

class MemManager
{
	private:
		MemManager();
	public:
		static MemManager& Instance()
		{
			static MemManager instance;
			return instance;
		}
		Result MarkPageAsAllocated(unsigned uiPageNumber) ;
		Result AllocatePhysicalPage(unsigned* uiPageNumber) ;
		void DeAllocatePhysicalPage(const unsigned uiPageNumber) ;
		Result AllocatePage(int iProcessID, unsigned uiFaultyAddress) ;
		Result DeAllocatePage(const unsigned uiAddress) ;
		void DisplayNoOfFreePages() ;
		unsigned GetFlatAddress(unsigned uiVirtualAddress) ;
		int GetFreeKernelProcessStackBlockID() ;
		void FreeKernelProcessStackBlock(int id) ;
		void DisplayNoOfAllocPages() ;

		static void InitPage(unsigned uiPage) ;
		static void PageFaultHandlerTaskGate() ;

		inline unsigned GetKernelHeapStartAddr() { return m_uiKernelHeapStartAddress ; }
		inline unsigned* GetKernelProcessStackPTEBase() { return m_uipKernelProcessStackPTEBase ; }
		inline unsigned GetRamSize() { return RAM_SIZE; }

		static inline unsigned GetProcessSizeInPages(unsigned uiSizeInBytes)
		{
			return ((uiSizeInBytes - 1) / PAGE_SIZE) + 1 ;
		}

		static inline unsigned GetPTESizeInPages(unsigned uiSizeInPages)
		{
			return ((uiSizeInPages - 1) / PAGE_TABLE_ENTRIES) + 1 ;
		}

		inline unsigned* GetKernelAUTAddress()
		{
			return &m_uiKernelAUTAddress ;
		}

	private:
		bool BuildRawPageMap() ;
		bool BuildPageTable() ;

	private:
		unsigned m_uiNoOfPages ;
		unsigned m_uiNoOfResvPages ;
		unsigned m_uiKernelHeapSize ;
		unsigned m_uiKernelHeapStartAddress ;

		unsigned* m_uiPageMap ;
		unsigned m_uiPageMapSize ;

		unsigned m_uiResvSize ;

		unsigned* m_uiPDEBase ;
		unsigned* m_uiPTEBase ;
		unsigned* m_uipKernelProcessStackPTEBase ;
		unsigned m_uiKernelAUTAddress ;

		int m_iNoOfKernelProcessStackBlocks ;
		bool m_bAllocationMapForKernelProcessStackBlock[PAGE_TABLE_ENTRIES / PROCESS_KERNEL_STACK_PAGES] ;

		const unsigned RAM_SIZE ;
};

#endif
