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
# include <BufferedReader.h>
# include <DMM.h>
# include <FileOperations.h>
# include <ProcFileManager.h>
# include <MemUtil.h>
# include <Display.h>

/* This is used in case UnBuffered Data comes after Buffered Data. If the unbuffered data offset is beyond file size
file op will return error which causes buffered reader to return error... which shouldn't happen because the request
is not completely beyond file size. So, to avoid this, request to file op is sent with offset - OVERFLOW_ADJUST bytes
to ensure some non zero byte request is there within file size limit*/
#define OVERFLOW_ADJUST 2

BufferedReader::BufferedReader(const char* szFileName, unsigned uiOffSet, unsigned uiBufferSize) : m_bObjectState(false)
{
	m_uiOffSet = uiOffSet ;

	if(FileOperations_Open(&m_iFD, szFileName, O_RDONLY) != FileOperations_SUCCESS)
	{
		printf("\n Failed to open File: %s", szFileName) ;
		return ;
	}

	if(FileOperations_Seek(m_iFD, uiOffSet, SEEK_SET) != FileOperations_SUCCESS)
	{
		FileOperations_Close(m_iFD) ;
		printf("\n Seek failed. ") ;
		return ;
	}

	m_szBuffer = new char[uiBufferSize] ;
	if(m_szBuffer == NULL)
	{
		FileOperations_Close(m_iFD) ;
		printf("\n Failed to alloc memory for Buffer - in BufferedReader\n") ;
		return ;
	}

	if(FileOperations_Read(m_iFD, m_szBuffer, uiBufferSize, &m_uiBufferSize) != FileOperations_SUCCESS) 
	{
		FileOperations_Close(m_iFD) ;
		printf("\n Failed to Read from file: %s", szFileName) ;
		delete []m_szBuffer ;
	}

	m_bObjectState = true ;
}

BufferedReader::~BufferedReader()
{
	if(!m_bObjectState)
		return ;

	delete []m_szBuffer ;

	FileOperations_Close(m_iFD) ;
}

bool BufferedReader::Seek(unsigned uiOffSet)
{
	return (FileOperations_Seek(m_iFD, uiOffSet, SEEK_SET) == FileOperations_SUCCESS) ;
}

bool BufferedReader::Read(char* szBuffer, int iLen, unsigned* pReadLen)
{
	if(!m_bObjectState)
	{
		printf("\n BufferedReader initialization has failed!") ;
		return false ;
	}

	unsigned uiCurrentOffset ;
	RETURN_X_IF_NOT(FileOperations_GetOffset(m_iFD, &uiCurrentOffset), FileOperations_SUCCESS, false) ;

	if(uiCurrentOffset >= m_uiOffSet && (uiCurrentOffset + iLen) <= (m_uiOffSet + m_uiBufferSize))
	{
		// Case 1: Data Completely Buffered
		// Buffer:-		<----------------------->
		// Request:-		<--------->
		
		memcpy(szBuffer, m_szBuffer + (uiCurrentOffset - m_uiOffSet), iLen) ;
		*pReadLen = iLen ;
	}
	else if((uiCurrentOffset + iLen) < m_uiOffSet || uiCurrentOffset > (m_uiOffSet + m_uiBufferSize))
	{
		// Case 2: Data Completely UnBuffered
		// Buffer:-						<----------------------->
		// Request:-	<--------->
		// Request:-												<--------->

		RETURN_X_IF_NOT(FileOperations_Read(m_iFD, szBuffer, iLen, pReadLen), FileOperations_SUCCESS, false) ;
	}
	else
	{
		// Case 3: Partially Buffered
		// Case 3a: 
		// Buffer:-				<----------------------->
		// Request:-							<--------------->

		if(
		(uiCurrentOffset >= m_uiOffSet && uiCurrentOffset <= (m_uiOffSet + m_uiBufferSize - OVERFLOW_ADJUST))
			&&
		((uiCurrentOffset + iLen) > (m_uiOffSet + m_uiBufferSize))
		)
		{
			unsigned uiBufLen = m_uiBufferSize - (uiCurrentOffset - m_uiOffSet) - OVERFLOW_ADJUST ;

			memcpy(szBuffer, m_szBuffer + (uiCurrentOffset - m_uiOffSet), uiBufLen) ;

			RETURN_X_IF_NOT(FileOperations_Seek(m_iFD, uiCurrentOffset + uiBufLen, SEEK_SET), FileOperations_SUCCESS, false) ;
			RETURN_X_IF_NOT(FileOperations_Read(m_iFD, szBuffer + uiBufLen, iLen - uiBufLen, pReadLen), FileOperations_SUCCESS, false) ;

			*pReadLen = *pReadLen + uiBufLen ;
		}
		// Case 3b: 
		// Buffer:-				<----------------------->
		// Request:-	<--------------->
		else if(
		((uiCurrentOffset + iLen) >= m_uiOffSet && (uiCurrentOffset + iLen) <= (m_uiOffSet + m_uiBufferSize))
			&&
		(uiCurrentOffset < m_uiOffSet)
		)
		{
			unsigned uiFileLen = (m_uiOffSet - uiCurrentOffset) ;
			RETURN_X_IF_NOT(FileOperations_Read(m_iFD, szBuffer, uiFileLen, pReadLen), FileOperations_SUCCESS, false) ;

			memcpy(szBuffer + uiFileLen, m_szBuffer, (iLen - uiFileLen)) ;	

			*pReadLen = *pReadLen + (iLen - uiFileLen) ;
		}
		// Case 3c: 
		// Buffer:-				<----------------------->
		// Request:-	<---------------------------------------->
		else if(((uiCurrentOffset + iLen) > (m_uiOffSet + m_uiBufferSize) && uiCurrentOffset < m_uiOffSet))
		{
			unsigned uiReadLen ;
			unsigned uiFileLenLeft = (m_uiOffSet - uiCurrentOffset) ;
			RETURN_X_IF_NOT(FileOperations_Read(m_iFD, szBuffer, uiFileLenLeft, &uiReadLen), FileOperations_SUCCESS, false) ;

			unsigned uiBufLen = m_uiBufferSize - OVERFLOW_ADJUST ;
			memcpy(szBuffer + uiFileLenLeft, m_szBuffer, uiBufLen) ;

			//unsigned uiFileLenRight = (m_uiOffSet + m_uiBufferSize) - (uiCurrentOffset + iLen) + OVERFLOW_ADJUST ;
			uiReadLen += uiBufLen ;
			unsigned uiFileLenRight = iLen - uiReadLen ;
			RETURN_X_IF_NOT(FileOperations_Seek(m_iFD, uiCurrentOffset + uiReadLen, SEEK_SET), FileOperations_SUCCESS, false) ;
			RETURN_X_IF_NOT(FileOperations_Read(m_iFD, szBuffer + uiReadLen, uiFileLenRight, pReadLen), FileOperations_SUCCESS, false) ;

			*pReadLen = *pReadLen + uiReadLen ;
		}
		else
		{
			printf("\n Bug in BufferedReader impl") ;
			return false ;
		}

	}

	return true ;
}

