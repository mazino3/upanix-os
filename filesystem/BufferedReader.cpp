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
# include <uniq_ptr.h>

/* This is used in case UnBuffered Data comes after Buffered Data. If the unbuffered data offset is beyond file size
file op will return error which causes buffered reader to return error... which shouldn't happen because the request
is not completely beyond file size. So, to avoid this, request to file op is sent with offset - OVERFLOW_ADJUST bytes
to ensure some non zero byte request is there within file size limit*/
#define OVERFLOW_ADJUST 2

BufferedReader::BufferedReader(const char* szFileName, unsigned uiOffSet, unsigned uiBufferSize) : m_uiOffSet(uiOffSet), m_szBuffer(nullptr)
{
	if(FileOperations_Open(&m_iFD, szFileName, O_RDONLY) != FileOperations_SUCCESS)
    throw upan::exception(XLOC, "Failed to open File: %s", szFileName);

	if(FileOperations_Seek(m_iFD, uiOffSet, SEEK_SET) != FileOperations_SUCCESS)
  {
    FileOperations_Close(m_iFD);
    throw upan::exception(XLOC, "Seek failed");
	}

  upan::uniq_ptr<char[]> buffer(new char[uiBufferSize]);

  if(FileOperations_Read(m_iFD, buffer.get(), uiBufferSize, &m_uiBufferSize) != FileOperations_SUCCESS)
	{
    FileOperations_Close(m_iFD);
    throw upan::exception(XLOC, "Failed to Read from file: %s", szFileName);
	}

  m_szBuffer = buffer.release();
}

BufferedReader::~BufferedReader()
{
  delete[] m_szBuffer;
	FileOperations_Close(m_iFD) ;
}

void BufferedReader::Seek(unsigned uiOffSet)
{
  if(FileOperations_Seek(m_iFD, uiOffSet, SEEK_SET) != FileOperations_SUCCESS)
    throw upan::exception(XLOC, "Buffered Reader: Failed to seek");
}

uint32_t BufferedReader::Read(char* szBuffer, int iLen)
{
  uint32_t n;
  if(!DoRead(szBuffer, iLen, &n))
    throw upan::exception(XLOC, "Buffered Readeer: Failed to read %d bytes", iLen);
  return n;
}

bool BufferedReader::DoRead(char* szBuffer, int iLen, unsigned* pReadLen)
{
  unsigned uiCurrentOffset;
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

