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
# include <DisplayManager.h>
# include <PortCom.h>
# include <MemConstants.h>
# include <ProcessGroupManager.h>
# include <ProcessManager.h>
# include <DMM.h>
# include <Display.h>
# include <KernelComponents.h>

#define VIDEO_BUFFER_ADDRESS			0xB8000
#define VIDEOMEM						(DisplayManager::GetDisplayMemAddress(ProcessManager_iCurrentProcessID))

DisplayBuffer::DisplayBuffer() : m_mCursor(0, 0), m_uiDisplayMemRawAddress(VIDEO_BUFFER_ADDRESS)
{
}

void DisplayBuffer::Initialize(unsigned uiDisplayMemAddr)
{
	m_mCursor.SetCurPos(0) ;
	m_mCursor.SetCurBytePos(0) ;
	m_uiDisplayMemRawAddress = uiDisplayMemAddr ;

	char* pDisplayMem = (char*)(uiDisplayMemAddr - GLOBAL_DATA_SEGMENT_BASE) ;
	for(int i = 0; i < PAGE_SIZE; i += 2)
	{
		pDisplayMem[i] = ' ' ;
		pDisplayMem[i+1] = Display::WHITE_ON_BLACK() ;
	}
}

DisplayBuffer& DisplayManager::KernelDisplayBuffer()
{
	static DisplayBuffer mKernelDisplayBuffer ;
	return mKernelDisplayBuffer ;
}

void DisplayManager::Initialize()
{
	// This function will be executed atmost once
	static bool bDone = false ;

	if(!bDone)
	{
		bDone = true ;

		InitCursor() ;
		GetMouseCursor() ;
		//Just calling to make sure DisplayBuffer is initialized
		KernelDisplayBuffer().Initialize(VIDEO_BUFFER_ADDRESS) ;

		UpdateCursorPosition(0, true) ;

		KC::MDisplay().LoadMessage("Video Initialization", Success) ;
	}
	else
	{
		KC::MDisplay().Message("\n DisplayManager is already initialized!") ;		
	}
}

void DisplayManager::InitializeDisplayBuffer(DisplayBuffer& rDisplayBuffer, unsigned uiDisplayMemAddr)
{
	rDisplayBuffer.Initialize(uiDisplayMemAddr) ;
}

void DisplayManager::InitCursor()
{
	// Get Cursor Size Start
    PortCom_SendByte(CRT_INDEX_REG, 0x0A) ;
    byte bCurStart = PortCom_ReceiveByte(CRT_DATA_REG) ;
	// Get Cursor Size End
    PortCom_SendByte(CRT_INDEX_REG, 0x0A) ;
    byte bCurEnd = PortCom_ReceiveByte(CRT_DATA_REG) ;

    bCurStart = (bCurStart & 0xC0 ) | (CURSOR_HIEGHT - 3) ;
    bCurEnd = (bCurEnd & 0xE0 ) | (CURSOR_HIEGHT - 2) ;

	// Set Cursor Size Start 
    PortCom_SendByte(CRT_INDEX_REG, 0x0A) ;
    PortCom_SendByte(CRT_DATA_REG, bCurStart) ;
	// Set Cursor Size End
    PortCom_SendByte(CRT_INDEX_REG, 0x0B) ;
    PortCom_SendByte(CRT_DATA_REG, bCurEnd) ;
}

DisplayBuffer& DisplayManager::GetDisplayBuffer(int pid)
{
	if(IS_KERNEL())
	{
		return KernelDisplayBuffer() ;
	}
	
	ProcessAddressSpace* pAddrSpace = &ProcessManager_processAddressSpace[pid] ;
	ProcessGroup* pGroup = &ProcessGroupManager_AddressSpace[pAddrSpace->iProcessGroupID] ;
	return pGroup->videoBuffer ;
}

DisplayBuffer& DisplayManager::GetDisplayBuffer()
{
	return GetDisplayBuffer(ProcessManager_iCurrentProcessID) ;
}

byte* DisplayManager::GetDisplayMemAddress(int pid)
{
	unsigned uiDisplayMemRawAddress = DisplayManager::GetDisplayBuffer(pid).GetDisplayMemAddr() ;

	if(IS_KERNEL())
		return (byte*)(uiDisplayMemRawAddress - GLOBAL_DATA_SEGMENT_BASE) ;

	if(IS_KERNEL_PROCESS(pid))
		return (byte*)(uiDisplayMemRawAddress - GLOBAL_DATA_SEGMENT_BASE) ;

	return (byte*)(PROCESS_VIDEO_BUFFER - GLOBAL_DATA_SEGMENT_BASE) ;
}


void DisplayManager::UpdateCursorPosition(int iCursorPos, bool bUpdateCursorOnScreen)
{
	DisplayBuffer& mDisplayBuffer = GetDisplayBuffer() ;
	
	mDisplayBuffer.GetCursor().SetCurPos(iCursorPos) ;
	mDisplayBuffer.GetCursor().SetCurBytePos(iCursorPos * 2) ;

	if(bUpdateCursorOnScreen && ( IS_KERNEL() || IS_FG_PROCESS_GROUP() ))
	{
		int x = mDisplayBuffer.GetCursor().GetCurPos() % DisplayManager::NO_COLUMNS ;
		int y = mDisplayBuffer.GetCursor().GetCurPos() / DisplayManager::NO_COLUMNS ;

		Goto(x, y) ;
	}
}

void DisplayManager::Goto(int x, int y)
{
	int iPos = x + y * DisplayManager::NO_COLUMNS ;

    PortCom_SendByte(CRT_INDEX_REG, 15) ;
    PortCom_SendByte(CRT_DATA_REG, iPos & 0xFF) ;

    PortCom_SendByte(CRT_INDEX_REG, 14) ;
    PortCom_SendByte(CRT_DATA_REG, (iPos >> 8) & 0xFF) ;
}

int DisplayManager::GetCurrentCursorPosition()
{
	return GetDisplayBuffer().GetCursor().GetCurPos() ;
}

int DisplayManager::GetCurrentDisplayBytePosition()
{
	return GetDisplayBuffer().GetCursor().GetCurBytePos() ;
}

void DisplayManager::SetCurrentCursorPosition(int iPos)
{
	GetDisplayBuffer().GetCursor().SetCurPos(iPos) ;
}

void DisplayManager::SetCurrentDisplayBytePosition(int iPos)
{
	GetDisplayBuffer().GetCursor().SetCurBytePos(iPos) ;
}

void DisplayManager::PutChar(int iPos, byte ch)
{
	VIDEOMEM[iPos] = ch ;

	if(!IS_KERNEL() && IS_FG_PROCESS_GROUP())
	{
		ch = CheckMouseCursor(iPos, ch) ;
		((char*)(VIDEO_BUFFER_ADDRESS - GLOBAL_DATA_SEGMENT_BASE))[iPos] = ch ;
	}
}

byte DisplayManager::GetChar(int iPos)
{
	return VIDEOMEM[iPos] ;
}

// Called for Non Kernel Processes only
void DisplayManager::SetupPageTableForDisplayBuffer(int iProcessGroupID, unsigned uiPDEAddress)
{
	unsigned uiDisplayMemRawAddress = ProcessGroupManager_AddressSpace[iProcessGroupID].videoBuffer.GetDisplayMemAddr() ;

	unsigned uiPDEIndex = ((PROCESS_VIDEO_BUFFER >> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((PROCESS_VIDEO_BUFFER >> 12) & 0x3FF) ;

	unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;

	((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = (uiDisplayMemRawAddress & 0xFFFFF000) | 0x3 ;
	Mem_FlushTLB();
}

/**********************************************************/
void DisplayManager::RefreshScreen()
{
	const ProcessGroup* pFGGroup = ProcessGroupManager_GetFGProcessGroup() ;
	const DisplayBuffer& mDisplayBuffer = pFGGroup->videoBuffer ;

	char* pDestDisplayBuf = (char*)(VIDEO_BUFFER_ADDRESS - GLOBAL_DATA_SEGMENT_BASE) ;
	char* pSrcDisplayBuf = (char*)(mDisplayBuffer.GetDisplayMemAddr() - GLOBAL_DATA_SEGMENT_BASE) ;

	unsigned noOfDisplayBytes = DisplayManager::NO_ROWS * DisplayManager::NO_COLUMNS * DisplayManager::NO_BYTES_PER_CHARACTER ;
	unsigned i ;

	for(i = 0; i < noOfDisplayBytes; i++)
	{
		pSrcDisplayBuf[i] = CheckMouseCursor(i, pSrcDisplayBuf[i]) ;
		pDestDisplayBuf[i] = pSrcDisplayBuf[i] ;
	}

	int x = mDisplayBuffer.GetCursor().GetCurPos() % DisplayManager::NO_COLUMNS ;
	int y = mDisplayBuffer.GetCursor().GetCurPos() / DisplayManager::NO_COLUMNS ;

	Goto(x, y) ;
}

MouseCursor& DisplayManager::GetMouseCursor()
{
	static MouseCursor mMouseCursor(80 * 3 + 40) ;
	return mMouseCursor ;
}

byte DisplayManager::CheckMouseCursor(int iPos, byte ch)
{
	if(iPos % 2)
	{
		int curPos = iPos / 2 ;
		if(GetMouseCursor().GetCurPos() == curPos)
		{
			GetMouseCursor().SetOrigAttr(ch) ;
			ch = GetMouseCursor().GetCursorAttr() ;
		}
	}

	return ch ;
}

MouseCursor::MouseCursor(const int& ccp) : m_iCurrentCursorPosition(ccp)
{
	char* vBuf = (char*)(VIDEO_BUFFER_ADDRESS - GLOBAL_DATA_SEGMENT_BASE) ;
	int iPos = ccp * 2 + 1 ;

	m_bCursorAttr = 'A' ;
	m_bOrigAttr = vBuf[ iPos ] ;
	vBuf[ iPos ] = m_bCursorAttr ;
}

void MouseCursor::SetCurPos(int iPos)
{
	int iNewPos = iPos * 2 + 1 ;
	int iCurPos = m_iCurrentCursorPosition * 2 + 1 ;
	char* vBuf = (char*)(VIDEO_BUFFER_ADDRESS - GLOBAL_DATA_SEGMENT_BASE) ;

	vBuf[ iCurPos ] = m_bOrigAttr ;
	m_bOrigAttr = vBuf[ iNewPos ] ;
	vBuf[ iNewPos ] = m_bCursorAttr ;

   	m_iCurrentCursorPosition = iPos ;
}

#undef VIDOMEM
