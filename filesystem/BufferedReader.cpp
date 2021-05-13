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
# include <FileDescriptorTable.h>
# include <MemUtil.h>
# include <Display.h>
# include <uniq_ptr.h>

/* This is used in case UnBuffered Data comes after Buffered Data. If the unbuffered data offset is beyond file size
file op will return error which causes buffered reader to return error... which shouldn't happen because the request
is not completely beyond file size. So, to avoid this, request to file op is sent with offset - OVERFLOW_ADJUST bytes
to ensure some non zero byte request is there within file size limit*/
#define OVERFLOW_ADJUST 2

BufferedReader::BufferedReader(const upan::string& szFileName, unsigned uiOffSet, unsigned uiBufferSize) : m_uiOffSet(uiOffSet), m_szBuffer(nullptr)
{
  m_iFD = FileOperations_Open(szFileName.c_str(), O_RDONLY);

  try
  {
    FileOperations_Seek(m_iFD, uiOffSet, SEEK_SET);
    upan::uniq_ptr<char[]> buffer(new char[uiBufferSize]);
    m_uiBufferSize = FileOperations_Read(m_iFD, buffer.get(), uiBufferSize);
    m_szBuffer = buffer.release();
  }
  catch(...)
	{
    FileOperations_Close(m_iFD);
    throw;
	}
}

BufferedReader::~BufferedReader()
{
  delete[] m_szBuffer;
	FileOperations_Close(m_iFD) ;
}

void BufferedReader::Seek(unsigned uiOffSet)
{
  FileOperations_Seek(m_iFD, uiOffSet, SEEK_SET);
}

int BufferedReader::Read(char* szBuffer, int iLen)
{
  return DoRead(szBuffer, iLen);
}

int BufferedReader::DoRead(char* szBuffer, int iLen)
{
  unsigned uiCurrentOffset = FileOperations_GetOffset(m_iFD);

	if(uiCurrentOffset >= m_uiOffSet && (uiCurrentOffset + iLen) <= (m_uiOffSet + m_uiBufferSize))
	{
		// Case 1: Data Completely Buffered
		// Buffer:-		<----------------------->
		// Request:-		<--------->
		
		memcpy(szBuffer, m_szBuffer + (uiCurrentOffset - m_uiOffSet), iLen) ;
    return iLen ;
	}
	else if((uiCurrentOffset + iLen) < m_uiOffSet || uiCurrentOffset > (m_uiOffSet + m_uiBufferSize))
	{
		// Case 2: Data Completely UnBuffered
		// Buffer:-						<----------------------->
		// Request:-	<--------->
		// Request:-												<--------->

    return FileOperations_Read(m_iFD, szBuffer, iLen);
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

      FileOperations_Seek(m_iFD, uiCurrentOffset + uiBufLen, SEEK_SET);
      return FileOperations_Read(m_iFD, szBuffer + uiBufLen, iLen - uiBufLen) + uiBufLen;
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
      int readLen = FileOperations_Read(m_iFD, szBuffer, uiFileLen);

			memcpy(szBuffer + uiFileLen, m_szBuffer, (iLen - uiFileLen)) ;	

      return readLen + (iLen - uiFileLen) ;
		}
		// Case 3c: 
		// Buffer:-				<----------------------->
		// Request:-	<---------------------------------------->
		else if(((uiCurrentOffset + iLen) > (m_uiOffSet + m_uiBufferSize) && uiCurrentOffset < m_uiOffSet))
		{
      unsigned uiFileLenLeft = (m_uiOffSet - uiCurrentOffset) ;
      int readLen = FileOperations_Read(m_iFD, szBuffer, uiFileLenLeft);

			unsigned uiBufLen = m_uiBufferSize - OVERFLOW_ADJUST ;
			memcpy(szBuffer + uiFileLenLeft, m_szBuffer, uiBufLen) ;

			//unsigned uiFileLenRight = (m_uiOffSet + m_uiBufferSize) - (uiCurrentOffset + iLen) + OVERFLOW_ADJUST ;
      readLen += uiBufLen ;
      unsigned uiFileLenRight = iLen - readLen ;
      FileOperations_Seek(m_iFD, uiCurrentOffset + readLen, SEEK_SET);
      return FileOperations_Read(m_iFD, szBuffer + readLen, uiFileLenRight) + readLen;
		}
		else
		{
      throw upan::exception(XLOC, "Bug in BufferedReader impl");
		}
	}
}

