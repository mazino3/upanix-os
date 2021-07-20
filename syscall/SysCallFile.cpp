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
# include <SysCall.h>
# include <SysCallFile.h>
# include <DeviceDrive.h>
# include <try.h>

byte SysCallFile_IsPresent(unsigned uiSysCallID)
{
	return (uiSysCallID > SYS_CALL_FILE_START && uiSysCallID < SYS_CALL_FILE_END) ;
}

__volatile__ bool bDoAddrTranslation = true ;

void SysCallFile_Handle(
__volatile__ int* piRetVal,
__volatile__ unsigned uiSysCallID,
__volatile__ bool bDoAddrTranslation,
__volatile__ unsigned uiP1,
__volatile__ unsigned uiP2, 
__volatile__ unsigned uiP3, 
__volatile__ unsigned uiP4, 
__volatile__ unsigned uiP5, 
__volatile__ unsigned uiP6, 
__volatile__ unsigned uiP7, 
__volatile__ unsigned uiP8, 
__volatile__ unsigned uiP9)
{
	
	switch(uiSysCallID)
	{
		case SYS_CALL_CHANGE_DIR : //Change Directory
			//P1 => Directory Path
			{
				char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;

				*piRetVal = 0 ;
        try
        {
          FileOperations_ChangeDir(szPathAddress);
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
					*piRetVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_PWD : //Get PWD
			//P1 => Return Dir Name Pointer
			{
				char** szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char**, uiP1) ;
        *piRetVal = 0;
        try
        {
          Directory_PresentWorkingDirectory( &ProcessManager::Instance().GetCurrentPAS(), szPathAddress);
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1;
        }
			}
			break ;

		case SYS_CALL_CWD : //Get CWD
			//P1 => Return Dir Name Pointer
			//P2 => Buf Length
			{
				char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;
				*piRetVal = 0;

        try
        {
          FileOperations_GetCWD(szPathAddress, uiP2);
        }
        catch(upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1;
        }
			}
			break ;

		case SYS_CALL_MKDIR : //Create Dir / File
		case SYS_CALL_FILE_CREATE:
			//P1 => Dir Path
			//P2 => Dir Attr
			{
				char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;
				unsigned short usType = (uiSysCallID == SYS_CALL_MKDIR) ? ATTR_TYPE_DIRECTORY : ATTR_TYPE_FILE ; 
				*piRetVal = 0 ;

        try
        {
          FileOperations_SyncPWD();
          FileOperations_Create(szPathAddress, usType, (unsigned short)(uiP2));
        }
        catch(const upan::exception& ex)
				{
          ex.Print();
					*piRetVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_RMDIR : //Delete Dir / File
			//P1 => Dir Path
			{
				char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;
				*piRetVal = 0 ;
        try
        {
          FileOperations_SyncPWD();
          FileOperations_Delete(szPathAddress);
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_GET_DIR_LIST :
			// P1 => Dir Path
			// P2 => Ret Dir Content List Address
			// P3 => Ret Dir Content List Size Address
			{
				char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;
				FileSystem::Node** pRetDirContentList = KERNEL_ADDR(bDoAddrTranslation, FileSystem::Node**, uiP2) ;
				int* pRetDirContentListSize = KERNEL_ADDR(bDoAddrTranslation, int*, uiP3) ;
				*piRetVal = 0 ;

        try
        {
          FileOperations_SyncPWD();
          FileOperations_GetDirectoryContent(szPathAddress, pRetDirContentList, pRetDirContentListSize);
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1 ;
        }
			}
			break ;
		
		case SYS_CALL_FILE_OPEN:
			// P1 => File Name / Path
			// P2 => Mode
			{
				const char* szFileNameAddr = KERNEL_ADDR(bDoAddrTranslation, const char*, uiP1) ;
				byte mode = uiP2 ;

				*piRetVal = 0 ;

        try
        {
          FileOperations_SyncPWD();
          *piRetVal = FileOperations_Open(szFileNameAddr, mode).id();
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_FILE_CLOSE:
			// P1 => File Desc
			{
				*piRetVal = 0 ;
				if(FileOperations_Close((int)uiP1) != FileOperations_SUCCESS)
					*piRetVal = -1 ;
			}
			break ;

		case SYS_CALL_FILE_READ:
			// P1 => File Desc
			// P2 => Read buffer address
			// P3 => Byte to read 
			// Return Value = Bytes Read
			{
				char* szBufferAddr = KERNEL_ADDR(bDoAddrTranslation, char*, uiP2) ;

				*piRetVal = 0 ;
        try
        {
          auto& file = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)uiP1);
          *piRetVal = file.read(szBufferAddr, (int)uiP3);
        }
        catch(...)
        {
          *piRetVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_FILE_WRITE:
			// P1 => File Desc
			// P2 => Write buffer address
			// P3 => Byte to write 
			{
				const char* szBufferAddr = (bDoAddrTranslation) ? KERNEL_ADDR(bDoAddrTranslation, const char*, uiP2) : (const char*)uiP2 ;

				*piRetVal = 0 ;
        try
        {
          auto& file = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)uiP1);
          *piRetVal =  file.write(szBufferAddr, (int)uiP3);
        }
        catch(const upan::exception& ex)
				{
          ex.Print();
					*piRetVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_SEEK:
			// P1 => File Desc
			// P2 => Seek Type
			// P3 => Offset
			{
				*piRetVal = 0 ;
        try
        {
          auto& file = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)uiP1);
          file.seek((int)uiP3, (int)uiP2);
        }
        catch(upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1;
        }
      }
			break ;

		case SYS_CALL_FILE_TELL:
			// P1 => File Desc
			// P2 => Seek Type
			// P3 => Offset
			{
        try
        {
          *piRetVal = FileOperations_GetOffset((int)uiP1);
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1;
        }
			}
			break ;

		case SYS_CALL_FILE_MODE:
			// P1 => File Desc
			// P2 => Seek Type
			// P3 => Offset
			{
        try
        {
          *piRetVal = FileOperations_GetFileOpenMode((int)uiP1);
        }
        catch(const upan::exception& ex)
				{
					*piRetVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_STAT:
			// P1 => File Name
			// P2 => FileStat
			{
				*piRetVal = 0 ;
				const char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, const char*, uiP1) ;
        try
        {
          FileSystem_FileStat* pFileStat = KERNEL_ADDR(bDoAddrTranslation, FileSystem_FileStat*, uiP2);
          *pFileStat = FileOperations_GetStat(szPathAddress, FROM_FILE);
        }
        catch(const upan::exception& ex)
				{
          ex.Print();
					*piRetVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_STAT_FD:
			// P1 => File Name
			// P2 => FileStat
			{
				*piRetVal = 0 ;
				int iFD = uiP1 ;
        try
        {
          FileSystem_FileStat* pFileStat = KERNEL_ADDR(bDoAddrTranslation, FileSystem_FileStat*, uiP2) ;
          *pFileStat = FileOperations_GetStatFD(iFD);
        }
        catch(const upan::exception& ex)
				{
          ex.Print();
					*piRetVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_ACCESS:
			// P1 => File Name
			// P2 => Mode
			{
				*piRetVal = 0 ;
				const char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, const char*, uiP1) ;
        if(!FileOperations_FileAccess(szPathAddress, FROM_FILE, (int)uiP2))
				{
					*piRetVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_DUP2:
			// P1 => Old FD
			// P2 => New FD
			{
				*piRetVal = 0 ;
				try {
          FileOperations_Dup2(uiP1, uiP2);
				} catch(upan::exception& e) {
				  e.Print();
          *piRetVal = -1 ;
				}
			}
			break ;
	}
}
