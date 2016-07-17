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
#include <MemManager.h>
#include <Display.h>
#include <ProcessManager.h>
#include <DMM.h>
#include <ProcessAllocator.h>
#include <KernelService.h>
#include <MultiBoot.h>
#include <MemUtil.h>
#include <AsmUtil.h>
#include <IDT.h>
#include <Atomic.h>
#include <exception.h>
#include <GraphicsVideo.h>

extern "C" { 
	unsigned MEM_PDBR ;
}

void MemManager::PageFaultHandlerTaskGate()
{
//	__asm__ __volatile__("pusha") ;
//	__asm__ __volatile__("pushf") ;
//	AsmUtil_LOAD_KERNEL_SEGS_ON_STACK() ;

	__volatile__ unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	
	__volatile__ unsigned short usDS = MemUtil_GetDS() ; 
	__volatile__ unsigned short usES = MemUtil_GetES() ; 
	__volatile__ unsigned short usFS = MemUtil_GetFS() ; 
	__volatile__ unsigned short usGS = MemUtil_GetGS() ;

	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("popw %ds") ; 
	__asm__ __volatile__("popw %fs") ; 
	__asm__ __volatile__("popw %gs") ; 

	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("popw %es") ; 

//	__volatile__ unsigned MM_errCode ;
//	__volatile__ unsigned CS ;
//	__volatile__ unsigned IP ;
//
	__volatile__ unsigned uiFaultyAddress ;
	__asm__ __volatile__("mov %%cr2, %0" : "=r"(uiFaultyAddress) : ) ;

//	KC::MDisplay().Address("\n P F A = ", uiFaultyAddress) ;

	if(!KC::MKernelService().RequestPageFault(uiFaultyAddress))
	{
//	__asm__ __volatile__("leave") ;
//	__asm__ __volatile__("popl %0" : "=m"(MM_errCode) : ) ;
//	__asm__ __volatile__("popl %0" : "=m"(IP) : ) ;
//	__asm__ __volatile__("popl %0" : "=m"(CS) : ) ;
//
//	KC::MDisplay().Address("\nMM_errCode = ", MM_errCode) ;
//	KC::MDisplay().Address("\nIP = ", IP) ;
//	KC::MDisplay().Address("\nCS = ", CS) ;

		ProcessManager_EXIT() ;
	}

	__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usDS) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usES) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%fs" :: "m"(usFS) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%gs" :: "m"(usGS) ) ;

	AsmUtil_RESTORE_GPR(GPRStack) ;

//	AsmUtil_UNLOAD_KERNEL_SEGS_ON_STACK() ;
//	__asm__ __volatile__("popf") ;
//	__asm__ __volatile__("popa") ;

	__asm__ __volatile__("leave") ;
//	__asm__ __volatile__("popl %%ss:%0" : "=m"(MM_errCode) : ) ;
//	TODO: Make this POP more meaningful.
//	__asm__ __volatile__("popl %ecx") ;
	__asm__ __volatile__("addl $0x4, %esp");
	
	__asm__ __volatile__("iret") ;
}

MemManager::MemManager() : 
	m_uiKernelAUTAddress(NULL),
	RAM_SIZE(MultiBoot::Instance().GetRamSize())
{
	if(BuildRawPageMap())
	{
		if(BuildPageTable())
		{
      MemMapGraphicsLFB(MEM_PDBR);
			Mem_EnablePaging() ;
	
			KC::MDisplay().LoadMessage("Memory Manager Initialization", Success) ;
			printf("\n\tRAM SIZE = %d", RAM_SIZE) ;
			printf("\n\tNo. of Pages = %d", m_uiNoOfPages) ;
			printf("\n\tNo. of Resv Pages = %d", m_uiNoOfResvPages) ;
			return;
		}
	}

	KC::MDisplay().Message("\n *********** KERNEL PANIC ************ \n", '$') ;
	while(1) ;
}

void MemManager::MemMapGraphicsLFB(unsigned uiPDEAddress)
{
  if(!GraphicsVideo::Instance())
    return;
  unsigned noOfPages = (GraphicsVideo::Instance()->LFBSize() / PAGE_SIZE) + 1;
  unsigned availablePages = MEM_GRAPHICS_VIDEO_MAP_SIZE / PAGE_SIZE;
  if(noOfPages > availablePages)
  {
    printf("\n Insufficient graphics video buffer. Required pages: %u", noOfPages);
    while(1);
  }
  unsigned lfbaddress = GraphicsVideo::Instance()->FlatLFBAddress();
  unsigned mapAddress = MEM_GRAPHICS_VIDEO_MAP_START;
  for(unsigned i = 0; i < noOfPages; ++i)
  {
    unsigned addr = lfbaddress + PAGE_SIZE * i;
    unsigned uiPDEIndex = ((mapAddress >> 22) & 0x3FF);
    unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000;
    unsigned uiPTEIndex = ((mapAddress >> 12) & 0x3FF);
    // This page is a Read Only area for user process. 0x5 => 101 => User Domain, Read Only, Present Bit
    ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = (addr & 0xFFFFF000) | 0x5;
    //No need to mark page as allocated as that would be already done while building PTE for reserved area
    MarkPageAsAllocated(addr / PAGE_SIZE);
    mapAddress += PAGE_SIZE;
  }
  GraphicsVideo::Instance()->MappedLFBAddress(MEM_GRAPHICS_VIDEO_MAP_START);
}

void MemManager::InitPage(unsigned uiPage)
{
	uiPage = uiPage * PAGE_SIZE;
  memset((void*)(uiPage - GLOBAL_DATA_SEGMENT_BASE), 0, PAGE_SIZE);
}

bool MemManager::BuildRawPageMap()
{
	m_uiPageMap = (unsigned*)MEM_PAGE_MAP_START ;
	m_uiPageMapSize = (((RAM_SIZE / PAGE_SIZE) / 8) / 4) ;
	m_uiResvSize = (((MEM_RESV_SIZE / PAGE_SIZE) / 8) / 4) ;
	m_uiKernelHeapSize = (MEM_KERNEL_HEAP_SIZE / PAGE_SIZE) / 8 / 4 ;
	m_uiKernelHeapStartAddress = MEM_RESV_SIZE ;

	if((m_uiPageMapSize * 4) > (MEM_PAGE_MAP_END - MEM_PAGE_MAP_START))
	{
		KC::MDisplay().Message("\n Mem Page Map Size InSufficient\n", 'A') ;
		return false ;
	}

	unsigned i ;
	for(i = 0; i < m_uiPageMapSize; i++)
		m_uiPageMap[i] &= 0x0 ;

	for(i = 0; i < m_uiResvSize + m_uiKernelHeapSize; i++)
		m_uiPageMap[i] |= 0xFFFFFFFF ;
 
	return true ;
}	

bool MemManager::BuildPageTable()
{
	// Expecting a complete 4 GB Page Table setup
	// i.e, PTE of size 4 MB and PDE of size 4 KB
	MEM_PDBR = MEM_PDE_START + GLOBAL_DATA_SEGMENT_BASE ;

	unsigned uiNoOfPDEEntries ;
	unsigned uiPDE ;
	unsigned uiPTE ;
	unsigned uiPageAddress ;
	unsigned uiPageTableAddress ;
	
	m_uiNoOfPages = RAM_SIZE / PAGE_SIZE ;
	uiNoOfPDEEntries = m_uiNoOfPages / PAGE_TABLE_ENTRIES ;
	m_uiNoOfResvPages = MEM_RESV_SIZE / PAGE_SIZE ;

	m_uiPDEBase = (unsigned*)MEM_PDE_START ; 
	m_uiPTEBase = (unsigned*)MEM_PTE_START ;
	
	if((uiNoOfPDEEntries * 4) > (MEM_PDE_END - MEM_PDE_START))
	{
		KC::MDisplay().Message("\n PDE Size InSufficient\n", 'A') ;
		return false ;
	}

	if((m_uiNoOfPages * 4) > (MEM_PTE_END - MEM_PTE_START))
	{
		KC::MDisplay().Message("\n PTE Size InSufficient\n", 'A') ;
		return false ;
	}

	memset((void*)MEM_PDE_START, 0, (MEM_PDE_END-MEM_PDE_START));
	memset((void*)MEM_PTE_START, 0, (MEM_PTE_END-MEM_PTE_START));
	
	uiPageTableAddress = MEM_PTE_START + GLOBAL_DATA_SEGMENT_BASE ;
	
	//Map all PTEs into PDE. Pages in PTE will be mapped based 
	//on RAM SIZE.
	for(uiPDE = 0; uiPDE < PAGE_TABLE_ENTRIES; uiPDE++)
	{
		m_uiPDEBase[uiPDE] = (uiPageTableAddress & 0xFFFFF000) | 0x3 ;
		uiPageTableAddress += PAGE_TABLE_SIZE ;
	}

	uiPageAddress = 0 ;
	for(uiPTE = 0; uiPTE < m_uiNoOfResvPages; uiPTE++)
	{
		m_uiPTEBase[uiPTE] = (uiPageAddress & 0xFFFFF000) | 0x3 ;
		uiPageAddress += PAGE_SIZE ;
	}

	for(;uiPTE < m_uiNoOfPages; uiPTE++)
	{
		m_uiPTEBase[uiPTE] = (uiPageAddress & 0xFFFFF000) | 0x3 ;
		uiPageAddress += PAGE_SIZE ;
	}

	/***** Initialize Kernel Processes Stack Pages Table Entries *****/
	m_uipKernelProcessStackPTEBase = (unsigned*)(MEM_PTE_START + KERNEL_PROCESS_PDE_ID * PAGE_TABLE_SIZE);
	m_iNoOfKernelProcessStackBlocks = PAGE_TABLE_ENTRIES / PROCESS_KERNEL_STACK_PAGES ;
	
	for(int i = 0; i < m_iNoOfKernelProcessStackBlocks; i++)
		m_bAllocationMapForKernelProcessStackBlock[i] = false ;

	return true ;
}

int MemManager::GetFreeKernelProcessStackBlockID()
{
  ProcessSwitchLock pLock;
	for(int i = 0; i < m_iNoOfKernelProcessStackBlocks; i++)
	{
		if(m_bAllocationMapForKernelProcessStackBlock[i] == false)
		{
			m_bAllocationMapForKernelProcessStackBlock[i] = true ;
			return i;
		}
	}
	return -1 ;
}

void MemManager::FreeKernelProcessStackBlock(int id)
{
  ProcessSwitchLock pLock;
	if(id < 0 || id >= m_iNoOfKernelProcessStackBlocks)
		return ;
	m_bAllocationMapForKernelProcessStackBlock[id] = false ;
}

Result MemManager::MarkPageAsAllocated(unsigned uiPageNumber)
{
  ProcessSwitchLock pLock;
	unsigned uiPageMapIndex = uiPageNumber / 32 ;
  //Sometimes MEM IO addresses can fall beyond actual ram size (?) - no need to mark those pages as allocated then
  if(uiPageMapIndex >= m_uiPageMapSize)
    return Success;
	unsigned uiPageBitIndex = uiPageNumber % 32 ;
	unsigned uiCurVal = (m_uiPageMap[ uiPageMapIndex ] >> uiPageBitIndex) & 0x1 ;
	if(uiCurVal)
	{
		printf("\n Error: Page: %u is already marked as allocated", uiPageNumber) ;
		return Failure;
	}
	m_uiPageMap[ uiPageMapIndex ] |= (0x1 << uiPageBitIndex) ;
	return Success ;
}

unsigned MemManager::AllocatePhysicalPage()
{	
	ProcessSwitchLock lock;

	unsigned uiPageMapPosition ;
	unsigned uiPageOffset ;
	unsigned uiPageMapEntry ;
	
	for(uiPageMapPosition = m_uiResvSize; uiPageMapPosition < m_uiPageMapSize; uiPageMapPosition++)
	{
		if((m_uiPageMap[uiPageMapPosition] & 0xFFFFFFFF) != 0xFFFFFFFF)
		{
			uiPageMapEntry = m_uiPageMap[uiPageMapPosition] ;
			for(uiPageOffset = 0; uiPageOffset < 32; uiPageOffset++)
			{
				if((uiPageMapEntry & 0x1) == 0x0)
				{
					m_uiPageMap[uiPageMapPosition] |= (0x1	<< uiPageOffset) ;
          return (uiPageMapPosition * 4 * 8) + uiPageOffset;
				}
				uiPageMapEntry >>= 1 ;
			}
		}
	}
  throw upan::exception(XLOC, "Out of memory pages!");
}

void MemManager::DeAllocatePhysicalPage(const unsigned uiPageNumber)
{
  ProcessSwitchLock pLock;

	unsigned uiPageMapPosition ;
	unsigned uiPageOffset ;

	uiPageOffset = uiPageNumber % (8 * 4) ;
	uiPageMapPosition = uiPageNumber / (8 * 4) ;
	
	m_uiPageMap[uiPageMapPosition] = m_uiPageMap[uiPageMapPosition] & ~(0x1 << uiPageOffset) ;
}

//uint32_t MemManager::AllocatePhysicalPage(const uint32_t noOfPages)
//{	
//	ProcessSwitchLock lock;
//  if(noOfPages == 0)
//    throw upan::exception(XLOC, "NoOfPages to allocate must be > 0");
//  struct { 
//    uint32_t index;
//    uint32_t offset;
//  } startPage, endPage;
//  auto finishAllocation = [this, &startPage, &endPage]() {
//      for(uint32_t mapIndex = startPage.index; mapIndex <= endPage.index; ++mapIndex)
//      {
//        const uint32_t sOffset = mapIndex == startPage.index ? startPage.offset : 0;
//        const uint32_t eOffset = mapIndex == endPage.index ? endPage.offset : 32;
//        for(uint32_t offset = sOffset; offset <= eOffset; ++offset)
//          m_uiPageMap[mapIndex] |= (1 << offset);
//      }
//  };
//  uint32_t count = noOfPages;
//	for(uint32_t uiPageMapPosition = m_uiResvSize; uiPageMapPosition < m_uiPageMapSize; uiPageMapPosition++)
//	{
//	  uint32_t uiPageMapEntry = m_uiPageMap[uiPageMapPosition];
//		if((uiPageMapEntry & 0xFFFFFFFF) != 0xFFFFFFFF)
//		{
//			for(uint32_t uiPageOffset = 0; uiPageOffset < 32; uiPageOffset++)
//			{
//				if((uiPageMapEntry & 0x1) == 0x0)
//				{
//          if(count == noOfPages)
//          {
//            startPage.index = uiPageMapPosition;
//            startPage.offset = uiPageOffset;
//          }
//          --count;
//          if(count == 0)
//          {
//            endPage.index = uiPageMapPosition;
//            endPage.offset = uiPageOffset;
//            finishAllocation();
//            return (startPage.index * 4 * 8) + startPage.offset;
//          }
//				}
//        else
//          count = noOfPages;
//				uiPageMapEntry >>= 1;
//			}
//		}
//	}
//  throw upan::exception(XLOC, "Out of memory pages!");
//}
//
//void MemManager::DeAllocatePhysicalPage(uint32_t pageNo, const uint32_t noOfPages)
//{
//  ProcessSwitchLock pLock;
//  for(uint32_t count = 0; count < noOfPages; ++count, ++pageNo)
//  {
//    const uint32_t index = pageNo / (8 * 4);
//    const uint32_t offset = pageNo % (8 * 4);
//    m_uiPageMap[index] = m_uiPageMap[index] & ~(0x1 << offset) ;
//  }
//}

extern __volatile__ int SYS_CALL_ID;
extern __volatile__ int KERNEL_DMM_ON;

Result MemManager::AllocatePage(int iProcessID, unsigned uiFaultyAddress)
{
	unsigned uiFreePageNo, uiVirtualPageNo ;
	unsigned uiPDEAddress, uiPTEAddress, uiPTEFreePage ;

	//KC::MDisplay().Address("\n Addr: ", uiFaultyAddress) ; 

	uiVirtualPageNo = uiFaultyAddress / PAGE_SIZE ;

	if(ProcessManager::Instance().GetAddressSpace(iProcessID).bIsKernelProcess == true)
	{
		printf("\n Page Fault in Kernel! COMMON MAN... FIX THIS !!!") ;
		printf("\n Page Fault Address/Page: %u / %u", uiFaultyAddress, uiVirtualPageNo);
		while(1);
		m_uiPTEBase[uiVirtualPageNo] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x3 ;
	}
	else
	{
		//For User Process a Page Fault can occur only while accessing the HEAP AREA which starts at Virtual Address
		//2 GB = 0x80000000
		
		/* Not accessing Heap and Not the startUp Address access (20MB) in proc_init */
		if((uiFaultyAddress < PROCESS_HEAP_START_ADDRESS && uiFaultyAddress != PROCESS_INIT_DOCK_ADDRESS) || 
				(uiFaultyAddress >= PROCESS_HEAP_START_ADDRESS && !ProcessManager::Instance().IsDMMOn(iProcessID)))
		{
			KC::MDisplay().Number("\n SYS CALL ID = ", SYS_CALL_ID) ;
			KC::MDisplay().Address("\n Segmentation Fault @ Address: ", uiFaultyAddress) ;
			KC::MDisplay().Number("\n PID = ", iProcessID) ;
			KC::MDisplay().Number("\n DMM Flag = ", ProcessManager::Instance().IsDMMOn(iProcessID));
			return Failure;
		}

		uiPDEAddress = ProcessManager::Instance().GetAddressSpace(iProcessID).taskState.CR3_PDBR ;

		uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[ ((uiFaultyAddress >> 22) & 0x3FF) ]) ;

		if((uiPTEAddress & 0x1) == 0x0)
		{
			uiPTEFreePage = AllocatePhysicalPage();

			((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[ ((uiFaultyAddress >> 22) & 0x3FF)] = 
				((uiPTEFreePage * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;

			uiPTEAddress = uiPTEFreePage * PAGE_SIZE ;
			InitPage(uiPTEFreePage) ;

		}
		else
		{
			uiPTEAddress = uiPTEAddress & 0xFFFFF000 ;
		}

		unsigned uiPageAdress = ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[((uiFaultyAddress >> 12) & 0x3FF)];

		// TODO: Fix This Properly
		if((uiPageAdress & 0x01) == 0x00)
		{
			uiFreePageNo = AllocatePhysicalPage();

			((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[ ((uiFaultyAddress >> 12) & 0x3FF) ] = 
					((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;

			InitPage(uiFreePageNo) ;
		}
		else
		{
			/* Crash the Process..... With SegFault Or OutOfMemeory Error*/
			KC::MDisplay().Address("\n Segmentation/Permission Fault @ Address: ", uiFaultyAddress) ;
			return Failure;
		}
	}

	//Mem_FlushTLB();
//	KC::MDisplay().Address("\n Alloc Done: ", uiFaultyAddress) ;
	return Success;
}

Result MemManager::DeAllocatePage(const unsigned uiAddress)
{
	unsigned uiFreePageNo, uiVirtualPageNo ;

	uiVirtualPageNo = uiAddress / PAGE_SIZE ;

	if((m_uiPTEBase[uiVirtualPageNo] & 0x1) == 0x0)
		return DupDealloc;
	
	uiFreePageNo = (m_uiPTEBase[uiVirtualPageNo] & 0xFFFFF000) / PAGE_SIZE ;
	m_uiPTEBase[uiVirtualPageNo] &= 0x2 ;
	DeAllocatePhysicalPage(uiFreePageNo) ;

//	KC::MDisplay().Address("\n DeAllocate Virtual: Address = ", uiAddress) ;
//	KC::MDisplay().Address(" : Page = ", uiVirtualPageNo) ;
//	KC::MDisplay().Address("\n DeAllocated Real Page = ", uiFreePageNo) ;

	return Success;
}

//void MemManager::PageFaultHandlerTask()
//{
//	/*	What is This...... ;) while(1) in a Exception Handler... 
//		This Exception Handler is Called Via Task Gate in IDT... So After the First Call
//		the EIP will be pointing to the Next Instruction of IRET..... U Got it,,, right
//		Yes.... No one there to update the TSS of this Task Gate.... Hence a tweak which works... */
//	
//	while(1) 
//	{
//		SPECIAL_TASK = true ;
//
//		__volatile__ unsigned uiFaultyAddress ;
//		__asm__ __volatile__("mov %%cr2, %0" : "=r"(uiFaultyAddress) : ) ;
//		
//		if(AllocatePage(ProcessManager_iCurrentProcessID, uiFaultyAddress) == Failure)
//		{
//			//TODO:
//			ProcessManager::Instance().GetCurrentPAS().status = TERMINATE ;
//			__asm__ __volatile__("HLT") ;
//		}
//		SPECIAL_TASK = false ;
//		__asm__ __volatile__("IRET") ;
//	}
//}

void MemManager::DisplayNoOfFreePages()
{	
	unsigned uiPageMapPosition ;
	unsigned uiPageOffset ;
	unsigned uiPageMapEntry ;
	unsigned uiFreePageCount = 0 ;
	
	for(uiPageMapPosition = m_uiResvSize; uiPageMapPosition < m_uiPageMapSize; uiPageMapPosition++)
	{
		if((m_uiPageMap[uiPageMapPosition] & 0xFFFFFFFF) != 0xFFFFFFFF)
		{
			uiPageMapEntry = m_uiPageMap[uiPageMapPosition] ;
			for(uiPageOffset = 0; uiPageOffset < 32; uiPageOffset++)
			{
				if((uiPageMapEntry & 0x1) == 0x0)
					uiFreePageCount++ ;
				uiPageMapEntry >>= 1 ;
			}
		}
	}
	
	KC::MDisplay().Number("\n Free Page Count = ", uiFreePageCount) ;
	KC::MDisplay().Message("\n", Display::WHITE_ON_BLACK());
}

unsigned MemManager::GetFlatAddress(unsigned uiVirtualAddress)
{
	unsigned uiPDEAddress = ProcessManager::Instance().GetCurrentPAS().taskState.CR3_PDBR ;

	unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[((uiVirtualAddress >> 22) & 0x3FF)]) ;

	if((uiPTEAddress & 0x1) == 0x0)
		return NULL ;

	uiPTEAddress = uiPTEAddress & 0xFFFFF000 ;
		
	unsigned uiPageAddress = ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[((uiVirtualAddress >> 12) & 0x3FF)] ;

	if((uiPageAddress & 0x1) == 0x0)
		return NULL ;	

	uiPageAddress = uiPageAddress & 0xFFFFF000 ;

	return uiPageAddress + (uiVirtualAddress & 0xFFF) ;
}

void MemManager::DisplayNoOfAllocPages()
{	
	unsigned uiPageMapPosition ;
	unsigned uiPageOffset ;
	unsigned uiPageMapEntry ;
	unsigned uiAllocPageCount = 0 ;
	
	KC::MDisplay().Message("\n\n", ' ');
	for(uiPageMapPosition = m_uiResvSize + m_uiKernelHeapSize; uiPageMapPosition < m_uiPageMapSize; uiPageMapPosition++)
	{
		//if((m_uiPageMap[uiPageMapPosition] & 0xFFFFFFFF) != 0xFFFFFFFF)
		{
			uiPageMapEntry = m_uiPageMap[uiPageMapPosition] ;
			for(uiPageOffset = 0; uiPageOffset < 32; uiPageOffset++)
			{
				if((uiPageMapEntry & 0x1) == 0x1)
				{
					uiAllocPageCount++ ;
					KC::MDisplay().Number(",", (uiPageMapPosition * 4 * 8) + uiPageOffset);
				}
				uiPageMapEntry >>= 1 ;
			}
		}
	}
	
	KC::MDisplay().Number("\n Alloc Page Count = ", uiAllocPageCount) ;
	KC::MDisplay().Message("\n", Display::WHITE_ON_BLACK());
}

