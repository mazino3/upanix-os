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
# include <ConsoleCommands.h>
# include <CommandLineParser.h>

#include <Display.h>
#include <BuiltInKeyboardDriver.h>
#include <StringUtil.h>
#include <Floppy.h>
#include <MemUtil.h>
#include <ProcessManager.h>
#include <FileSystem.h>
#include <Directory.h>
#include <MemManager.h>
#include <DMM.h>
#include <ATADrive.h>
#include <ATADeviceController.h>
#include <PartitionManager.h>
#include <ProcFileManager.h>
#include <FileOperations.h>
#include <UserManager.h>
#include <GenericUtil.h>
#include <SessionManager.h>
#include <KeyboardHandler.h>
#include <DeviceDrive.h>
#include <RTC.h>
#include <MultiBoot.h>
#include <SystemUtil.h>
#include <MountManager.h>
#include <UHCIManager.h>
#include <EHCIManager.h>
#include <XHCIManager.h>
#include <BTree.h>
#include <TestSuite.h>
#include <video.h>
#include <MouseDriver.h>
#include <exception.h>
#include <stdio.h>
#include <PCSound.h>
#include <Apic.h>
#include <typeinfo.h>

/**** Command Fucntion Declarations  *****/
static void ConsoleCommands_ChangeDrive() ;
static void ConsoleCommands_ShowDrive() ;
static void ConsoleCommands_MountDrive() ;
static void ConsoleCommands_UnMountDrive() ;
static void ConsoleCommands_FormatDrive() ;

static void ConsoleCommands_ClearScreen() ;

static void ConsoleCommands_CreateDirectory() ;
static void ConsoleCommands_RemoveFile() ;
static void ConsoleCommands_ListDirContent() ;
static void ConsoleCommands_ReadFileContent() ;
static void ConsoleCommands_ChangeDirectory() ;
static void ConsoleCommands_PresentWorkingDir() ;
static void ConsoleCommands_CopyFile() ;

static void ConsoleCommands_InitUser() ;
static void ConsoleCommands_ListUser() ;
static void ConsoleCommands_AddUser() ;
static void ConsoleCommands_DeleteUser() ;

static void ConsoleCommands_OpenSession() ;

static void ConsoleCommands_ShowPartitionTable() ;
static void ConsoleCommands_ClearPartitionTable() ;
static void ConsoleCommands_SetSysIdForPartition() ;
static void ConsoleCommands_CreatePrimaryPartition() ;
static void ConsoleCommands_CreateExtendedPartitionEntry() ;
static void ConsoleCommands_CreateExtendedPartition() ;
static void ConsoleCommands_DeletePrimaryPartition() ;
static void ConsoleCommands_DeleteExtendedPartition() ;

static void ConsoleCommands_LoadExe() ;
static void ConsoleCommands_WaitPID() ;
static void ConsoleCommands_Clone() ;
static void ConsoleCommands_Exit() ;
static void ConsoleCommands_Reboot() ;
static void ConsoleCommands_Date() ;
static void ConsoleCommands_MultiBootHeader() ;
static void ConsoleCommands_ListCommands() ;
static void ConsoleCommands_ListProcess() ;
static void ConsoleCommands_ChangeRootDrive() ;
static void ConsoleCommands_Echo() ;
static void ConsoleCommands_Export() ;
static void ConsoleCommands_ProbeUHCIUSB() ;
static void ConsoleCommands_PerformECHIHandoff() ;
static void ConsoleCommands_ProbeEHCIUSB() ;
static void ConsoleCommands_ProbeXHCIUSB() ;
static void ConsoleCommands_SetXHCIEventMode();
static void ConsoleCommands_ShowRawDiskList() ;
static void ConsoleCommands_InitFloppyController() ;
static void ConsoleCommands_InitATAController() ;
static void ConsoleCommands_InitMountManager() ;
static void ConsoleCommands_Test() ;
static void ConsoleCommands_Testv() ;
static void ConsoleCommands_TestNet() ;
static void ConsoleCommands_TestCPP() ;
static void ConsoleCommands_Beep();
static void ConsoleCommands_Sleep();

/*****************************************/

int ConsoleCommands_NoOfInterCommands ;

static const ConsoleCommand ConsoleCommands_CommandList[] = {
	{ "chd",		&ConsoleCommands_ChangeDrive },
	{ "shd",		&ConsoleCommands_ShowDrive },
	{ "mount",		&ConsoleCommands_MountDrive },
	{ "umount",		&ConsoleCommands_UnMountDrive },
	{ "format",		&ConsoleCommands_FormatDrive },

	{ "clear",		&ConsoleCommands_ClearScreen },

	{ "mkdir",		&ConsoleCommands_CreateDirectory },
	{ "rm",			&ConsoleCommands_RemoveFile },
	{ "ls",			&ConsoleCommands_ListDirContent },
	{ "read",		&ConsoleCommands_ReadFileContent },
	{ "cd",			&ConsoleCommands_ChangeDirectory },
	{ "pwd",		&ConsoleCommands_PresentWorkingDir },
	{ "cp",			&ConsoleCommands_CopyFile },

	{ "init_usr",	&ConsoleCommands_InitUser },
	{ "list_usr",	&ConsoleCommands_ListUser },
	{ "add_usr",	&ConsoleCommands_AddUser },
	{ "del_usr",	&ConsoleCommands_DeleteUser },

	{ "session",	&ConsoleCommands_OpenSession },

	{ "show_pt",	&ConsoleCommands_ShowPartitionTable },
	{ "clear_pt",	&ConsoleCommands_ClearPartitionTable },
	{ "setsid_pt",	&ConsoleCommands_SetSysIdForPartition },
	{ "create_ppt",	&ConsoleCommands_CreatePrimaryPartition },
	{ "create_xpt",	&ConsoleCommands_CreateExtendedPartitionEntry },
	{ "create_ept",	&ConsoleCommands_CreateExtendedPartition },
	{ "delete_ppt",	&ConsoleCommands_DeletePrimaryPartition },
	{ "delete_ept",	&ConsoleCommands_DeleteExtendedPartition },

	{ "load",		&ConsoleCommands_LoadExe },
	{ "wait",		&ConsoleCommands_WaitPID },
	{ "clone",		&ConsoleCommands_Clone },
	{ "exit",		&ConsoleCommands_Exit },
	{ "reboot",		&ConsoleCommands_Reboot },
	{ "date",		&ConsoleCommands_Date },
	{ "mtbt",		&ConsoleCommands_MultiBootHeader },

	{ "help",		&ConsoleCommands_ListCommands },
	{ "ps",			&ConsoleCommands_ListProcess },
	{ "crd",		&ConsoleCommands_ChangeRootDrive },
	{ "echo",		&ConsoleCommands_Echo },
	{ "export",		&ConsoleCommands_Export },
	{ "usbprobe",	&ConsoleCommands_ProbeUHCIUSB },
	{ "ehcihoff",	&ConsoleCommands_PerformECHIHandoff },
	{ "eusbprobe",	&ConsoleCommands_ProbeEHCIUSB },
	{ "xusbprobe",	&ConsoleCommands_ProbeXHCIUSB },
  { "xhciemode", &ConsoleCommands_SetXHCIEventMode },
	{ "showdisk",	&ConsoleCommands_ShowRawDiskList },
	{ "initfdc",	&ConsoleCommands_InitFloppyController },
	{ "initata",	&ConsoleCommands_InitATAController },
	{ "initmntmgr",	&ConsoleCommands_InitMountManager },
	{ "test",		&ConsoleCommands_Test },
	{ "testv",		&ConsoleCommands_Testv },
	{ "testn",		&ConsoleCommands_TestNet },
	{ "testcpp", &ConsoleCommands_TestCPP },
	{ "beep", &ConsoleCommands_Beep },
	{ "sleep", &ConsoleCommands_Sleep },
	{ "\0",			NULL }
} ;

void ConsoleCommands_Init()
{
	for(ConsoleCommands_NoOfInterCommands = 0; 
		ConsoleCommands_CommandList[ConsoleCommands_NoOfInterCommands].szCommand[0] != '\0';
		ConsoleCommands_NoOfInterCommands++) ;
}

void ConsoleCommands_ExecuteInternalCommand(const char* szCommand)
{
	for(int i = 0; i < ConsoleCommands_NoOfInterCommands; i++)
	{
		if(strcmp(szCommand, ConsoleCommands_CommandList[i].szCommand) == 0)
		{
      try
      {
  			ConsoleCommands_CommandList[i].cmdFunc() ;
      }
      catch(const upan::exception& ex)
      {
        ex.Print();
      }
      return;
		}
	}
}

void ConsoleCommands_ChangeDrive()
{
  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByDriveName(CommandLineParser::Instance().GetParameterAt(0), false).goodValueOrThrow(XLOC);

	ProcessManager::Instance().GetCurrentPAS().iDriveID = pDiskDrive->Id();

  MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pDiskDrive->_fileSystem.FSpwd), MemUtil_GetDS(),
                     (unsigned)&ProcessManager::Instance().GetCurrentPAS().processPWD, sizeof(FileSystem::PresentWorkingDirectory)) ;
}

void ConsoleCommands_ShowDrive()
{
	DiskDriveManager::Instance().DisplayList() ;
}

void ConsoleCommands_MountDrive()
{
  DiskDriveManager::Instance().GetByDriveName(CommandLineParser::Instance().GetParameterAt(0), false).goodValueOrThrow(XLOC)->Mount();
  printf("\nDrive Mounted");
}

void ConsoleCommands_UnMountDrive()
{
  DiskDriveManager::Instance().GetByDriveName(CommandLineParser::Instance().GetParameterAt(0), false).goodValueOrThrow(XLOC)->UnMount();
  printf("\nDrive UnMounted");
}

void ConsoleCommands_FormatDrive()
{
  DiskDriveManager::Instance().FormatDrive(CommandLineParser::Instance().GetParameterAt(0));
}

void ConsoleCommands_ClearScreen()
{
	KC::MDisplay().ClearScreen() ;
}

void ConsoleCommands_CreateDirectory()
{
  FileOperations_Create((char*)(CommandLineParser::Instance().GetParameterAt(0)), ATTR_TYPE_DIRECTORY, ATTR_DIR_DEFAULT);
  printf("\n DIR Created\n");
}

void ConsoleCommands_RemoveFile()
{
  FileOperations_Delete((char*)(CommandLineParser::Instance().GetParameterAt(0)));
  printf("\n DIR Deleted\n");
}

void ConsoleCommands_ListDirContent()
{
	FileSystem::Node* pDirList ;

	int iListSize = 0 ;
	const char* szListDirName = "." ;

  if(CommandLineParser::Instance().GetNoOfParameters())
    szListDirName = CommandLineParser::Instance().GetParameterAt(0) ;

  FileOperations_GetDirectoryContent(szListDirName, &pDirList, &iListSize);
  int i ;
	for(i = 0; i < iListSize; i++)
	{
		if(!(i % 3))
			KC::MDisplay().NextLine() ;
    printf("%-20s", pDirList[i].Name()) ;
	}

	DMM_DeAllocateForKernel((unsigned)pDirList) ;
}

void ConsoleCommands_ReadFileContent()
{
	char bDataBuffer[513] ;
	
  const char* szFileName = CommandLineParser::Instance().GetParameterAt(0) ;

  const int fd = FileOperations_Open(szFileName, O_RDONLY);

	KC::MDisplay().Character('\n', ' ') ;
	while(true)
	{
    int n = FileOperations_Read(fd, bDataBuffer, 512);

		bDataBuffer[n] = '\0' ;

    if(n < 512)
		{
			KC::MDisplay().Message(bDataBuffer, Display::WHITE_ON_BLACK()) ;
			break ;
		}
		
		KC::MDisplay().Message(bDataBuffer, Display::WHITE_ON_BLACK()) ;
	}

	if(FileOperations_Close(fd) != FileOperations_SUCCESS)
	{	
		KC::MDisplay().Message("\n File Close Failed", Display::WHITE_ON_BLACK()) ;
		return ;
	}
}

void ConsoleCommands_ChangeDirectory()
{
  FileOperations_ChangeDir(CommandLineParser::Instance().GetParameterAt(0));
}

void ConsoleCommands_PresentWorkingDir()
{
	char* szPWD ;
	Directory_PresentWorkingDirectory(
	&ProcessManager::Instance().GetCurrentPAS(),
	&szPWD) ;
	KC::MDisplay().Message("\n", ' ') ;
	KC::MDisplay().Message(szPWD, Display::WHITE_ON_BLACK()) ;
	DMM_DeAllocateForKernel((unsigned)szPWD) ;
}

void ConsoleCommands_CopyFile()
{
	const int iBufSize = 512 ;
	char bDataBuffer[iBufSize] ;

  const char* szFileName = CommandLineParser::Instance().GetParameterAt(0) ;
  const int fd = FileOperations_Open(szFileName, O_RDONLY);

  const char* szDestFile = CommandLineParser::Instance().GetParameterAt(1) ;

  FileOperations_Create(szDestFile, ATTR_TYPE_FILE, ATTR_FILE_DEFAULT);

  const int fd1 = FileOperations_Open(szDestFile, O_RDWR);

  printf("\n Progress = ");
	int cr = KC::MDisplay().GetCurrentCursorPosition();
	int i = 0 ;
  const FileSystem_FileStat& fStat = FileOperations_GetStatFD(fd);
	unsigned fsize = fStat.st_size ;
	if(fsize == 0)
	{
		KC::MDisplay().Message("\n F Size = 0 !!!", ' ') ;
		fsize = 1 ;
		//return ;
	}	

	while(true)
	{
    int n = FileOperations_Read(fd, bDataBuffer, iBufSize);
		int w ;

    if(n < 512)
		{
			if(n > 0)
        FileOperations_Write(fd1, bDataBuffer, n, &w);
			KC::MDisplay().ShowProgress("", cr, 100) ;
			break ;
		}
		
    FileOperations_Write(fd1, bDataBuffer, 512, &w);

		i++ ;
		KC::MDisplay().ShowProgress("", cr, (i * iBufSize * 100) / fsize) ;
	}

	if(FileOperations_Close(fd) != FileOperations_SUCCESS)
	{	
		KC::MDisplay().Message("\n File Close Failed", Display::WHITE_ON_BLACK()) ;
		return ;
	}

	if(FileOperations_Close(fd1) != FileOperations_SUCCESS)
	{	
		KC::MDisplay().Message("\n File1 Close Failed", Display::WHITE_ON_BLACK()) ;
		return ;
	}
}

void ConsoleCommands_InitUser()
{
  UserManager::Instance();
}

void ConsoleCommands_ListUser()
{
  const auto& users = UserManager::Instance().GetUserList();
	for(auto i : users)
    printf("\n %s", i.first.c_str());
}

void ConsoleCommands_AddUser()
{
	char szUserName[MAX_USER_LENGTH + 1] ;
	char szPassword[MAX_USER_LENGTH + 1] ;
	char szHomeDirPath[MAX_HOME_DIR_LEN + 1] ;

  printf("\n User Name: ");
	GenericUtil_ReadInput(szUserName, MAX_USER_LENGTH, true) ;

  printf("\n Password: ");
	GenericUtil_ReadInput(szPassword, MAX_USER_LENGTH, false) ;

  printf("\n Home dir path: ");
	GenericUtil_ReadInput(szHomeDirPath, MAX_HOME_DIR_LEN, true) ;

	if(UserManager::Instance().Create(szUserName, szPassword, szHomeDirPath, NORMAL_USER))
    printf("\n User created!");
}

void ConsoleCommands_DeleteUser()
{
	char szUserName[MAX_USER_LENGTH + 1] ;

  printf("\n User Name: ");
	GenericUtil_ReadInput(szUserName, MAX_USER_LENGTH, true) ;

  if(UserManager::Instance().Delete(szUserName))
    printf("\n User deleted!");
}

void ConsoleCommands_OpenSession()
{
	int pid ;
	ProcessManager::Instance().CreateKernelImage((unsigned)&SessionManager_StartSession, NO_PROCESS_ID, true, NULL, NULL, &pid, "session") ;
	ProcessManager::Instance().WaitOnChild(pid) ;
}

RawDiskDrive* ConsoleCommands_CheckDiskParam()
{
  if(CommandLineParser::Instance().GetNoOfParameters() < 1)
	{
		printf("\n Disk name parameter missing") ;
		return NULL ;
	}

  RawDiskDrive* pDisk = DiskDriveManager::Instance().GetRawDiskByName(CommandLineParser::Instance().GetParameterAt(0)) ;

	if(!pDisk)
	{
    printf("\n '%s' no such disk", CommandLineParser::Instance().GetParameterAt(0)) ;
		return NULL ;
	}

	return pDisk ;
}

void ConsoleCommands_ShowPartitionTable()
{
	RawDiskDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	printf("\n Total Sectors = %d", pDisk->SizeInSectors());
	PartitionTable partitionTable(*pDisk);
  partitionTable.VerbosePrint();
}

void ConsoleCommands_ClearPartitionTable()
{
	RawDiskDrive* pDisk = ConsoleCommands_CheckDiskParam() ;
	if(!pDisk)
		return;
	PartitionTable partitionTable(*pDisk);
  partitionTable.ClearPartitionTable();
}

void ConsoleCommands_SetSysIdForPartition()
{
	RawDiskDrive* pDisk = ConsoleCommands_CheckDiskParam() ;
	if(!pDisk)
		return ;
  pDisk->UpdateSystemIndicator(63, 0x83) ;
}

void ConsoleCommands_CreatePrimaryPartition()
{
	RawDiskDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	PartitionTable partitionTable(*pDisk);

	char szSizeInSectors[11] ;

	KC::MDisplay().Message("\n Size in Sector = ", Display::WHITE_ON_BLACK()) ;
	GenericUtil_ReadInput(szSizeInSectors, 10, true) ;

  int sizeInSectors = atoi(szSizeInSectors);
  if(sizeInSectors <= 0)
	{
    printf("\n Invalid sector size: %d", sizeInSectors);
		return ;
	}

  partitionTable.CreatePrimaryPartitionEntry(sizeInSectors, MBRPartitionInfo::NORMAL);
  printf("\n Partition Created!");
}

void ConsoleCommands_CreateExtendedPartitionEntry()
{
	RawDiskDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	PartitionTable partitionTable(*pDisk);
	if(partitionTable.IsEmpty())
	{
    printf("\n No Primary Partitions") ;
		return ;
	}

	char szSizeInSectors[11] ;

	KC::MDisplay().Message("\n Size in Sector = ", Display::WHITE_ON_BLACK()) ;
	GenericUtil_ReadInput(szSizeInSectors, 10, true) ;

  int sizeInSectors = atoi(szSizeInSectors);
  if(sizeInSectors <= 0)
	{
    printf("\n Invalid sector size %d", sizeInSectors);
		return ;
	}
  partitionTable.CreatePrimaryPartitionEntry(sizeInSectors, MBRPartitionInfo::EXTENEDED);
  printf("\n Partition Created!");
}

void ConsoleCommands_CreateExtendedPartition()
{
	RawDiskDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	PartitionTable partitionTable(*pDisk);
	if(partitionTable.IsEmpty())
	{
		KC::MDisplay().Message("\n No Primary Partitions", ' ') ;
		return ;
	}

	char szSizeInSectors[11] ;
	KC::MDisplay().Message("\n Size in Sector = ", Display::WHITE_ON_BLACK()) ;
	GenericUtil_ReadInput(szSizeInSectors, 10, true) ;

  int sizeInSectors = atoi(szSizeInSectors);
  if(sizeInSectors <= 0)
	{
    printf("\n Invalid sector size: %d", sizeInSectors);
		return ;
	}

  partitionTable.CreateExtPartitionEntry(sizeInSectors) ;
	printf("\n Partition Created");
}

void ConsoleCommands_DeletePrimaryPartition()
{
	RawDiskDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	PartitionTable partitionTable(*pDisk);
	if(partitionTable.IsEmpty())
	{
		KC::MDisplay().Message("\n Disk Not Partitioned", ' ') ;
		return ;
	}

	partitionTable.DeletePrimaryPartition() ;
  printf("\n Partition Deleted");
}

void ConsoleCommands_DeleteExtendedPartition()
{
	RawDiskDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	PartitionTable partitionTable(*pDisk);
	if(partitionTable.IsEmpty())
	{
		KC::MDisplay().Message("\n Disk Not Partitioned", ' ') ;
		return ;
	}

	partitionTable.DeleteExtPartition() ;
  printf("\n Partition Deleted");
}

void ConsoleCommands_LoadExe()
{
	byte bStatus ;
	int iChildProcessID ;
	char a1[14], a2[40] ;
	strcpy(a1, "100") ;
	strcpy(a2, "200") ;
	char* argv[2] ;
	argv[0] = (char*)&a1 ;
	argv[1] = (char*)&a2 ;

  if((bStatus = ProcessManager::Instance().Create(CommandLineParser::Instance().GetParameterAt(0), ProcessManager::GetCurrentProcessID(), true, &iChildProcessID, DERIVE_FROM_PARENT, 2, argv)) != ProcessManager_SUCCESS)
	{
		KC::MDisplay().Address("\n Load User Process Failed: ", bStatus) ;
		KC::MDisplay().Character('\n', Display::WHITE_ON_BLACK()) ;
	}
	else
	{
		ProcessManager::Instance().WaitOnChild(iChildProcessID) ;
	}
}

void ConsoleCommands_WaitPID()
{
  int pid = atoi(CommandLineParser::Instance().GetParameterAt(0));
  if(pid <= 0)
    printf("\n Invalid PID : %d", pid);
  else
    ProcessManager::Instance().WaitOnChild(pid);
}

void ConsoleCommands_Exit()
{
	ProcessManager_Exit() ;
}

void ConsoleCommands_Clone()
{
	int pid ;

	extern void Console_StartUpanixConsole() ;
	
	ProcessManager::Instance().CreateKernelImage((unsigned)&Console_StartUpanixConsole, ProcessManager::GetCurrentProcessID(), true, NULL, NULL, &pid, "console_1") ;
	
	ProcessManager::Instance().WaitOnChild(pid) ;
}

void ConsoleCommands_Reboot()
{
	SystemUtil_Reboot() ;
}

void ConsoleCommands_Date()
{
	RTCDateTime rtcDateTime ;
	RTC::GetDateTime(rtcDateTime) ;
	
	printf("\n %d/%d/%d - %d:%d:%d", rtcDateTime.bDayOfMonth, rtcDateTime.bMonth, 
		rtcDateTime.bCentury * 100 + rtcDateTime.bYear, rtcDateTime.bHour, rtcDateTime.bMinute, 
		rtcDateTime.bSecond) ;
}

void ConsoleCommands_MultiBootHeader()
{
  MultiBoot::Instance().Print();
  if(typeid(IrqManager::Instance()) == typeid(Apic))
    printf("\n Using APIC...");
}

void ConsoleCommands_ListCommands()
{
	for(int i = 0; i < ConsoleCommands_NoOfInterCommands; i++)
	{
		if(!(i % 3))
			KC::MDisplay().NextLine() ;
		printf("%-20s", ConsoleCommands_CommandList[i].szCommand) ;
	}
}

void ConsoleCommands_ListProcess()
{
	unsigned uiSize;
	PS* pPS = ProcessManager::Instance().GetProcList(uiSize);
	
	unsigned i ;
	for(i = 0; i < uiSize; i++)
	{
		printf("\n %-7d%-5d%-5d%-5d%-5d%-10s", pPS[i].pid, pPS[i].iParentProcessID, pPS[i].iProcessGroupID, pPS[i].status, pPS[i].iUserID,
					pPS[i].pname) ;
	}
	KC::MDisplay().NextLine() ;

	ProcessManager::Instance().FreeProcListMem(pPS, uiSize) ;
}

void ConsoleCommands_ChangeRootDrive()
{
  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByDriveName(CommandLineParser::Instance().GetParameterAt(0), false).goodValueOrThrow(XLOC);
	MountManager_SetRootDrive(pDiskDrive) ;
}

void ConsoleCommands_Echo()
{
  if(CommandLineParser::Instance().GetNoOfParameters())
    printf("\n%s", CommandLineParser::Instance().GetParameterAt(0)) ;
}

void ConsoleCommands_Export()
{
    if(CommandLineParser::Instance().GetNoOfParameters() < 1)
        return ;

    const char* szExp = CommandLineParser::Instance().GetParameterAt(0) ;
    const char* var = szExp ;
    char* val = strchr(szExp, '=') ;

    if(val == NULL)
    {
      printf("\n Incorrect syntax");
      return ;
    }

    val[0] = '\0' ;
    val = val + 1 ;

    if(setenv(var, val) < 0)
    {
      printf("\n Failed to set env variable\n") ;
      return ;
    }
}

void ConsoleCommands_ProbeUHCIUSB()
{
	RTCDateTime rtcStartTime, rtcStopTime ;
	RTC::GetDateTime(rtcStartTime) ;
  UHCIManager::Instance().Probe();
	RTC::GetDateTime(rtcStopTime) ;

	printf("\n %d/%d/%d - %d:%d:%d", rtcStartTime.bDayOfMonth, rtcStartTime.bMonth, 
		rtcStartTime.bCentury * 100 + rtcStartTime.bYear, rtcStartTime.bHour, rtcStartTime.bMinute, 
		rtcStartTime.bSecond) ;
	
	printf("\n %d/%d/%d - %d:%d:%d", rtcStopTime.bDayOfMonth, rtcStopTime.bMonth, 
		rtcStopTime.bCentury * 100 + rtcStopTime.bYear, rtcStopTime.bHour, rtcStopTime.bMinute, 
		rtcStopTime.bSecond) ;
}

void ConsoleCommands_PerformECHIHandoff()
{
	EHCIManager::Instance().RouteToCompanionController() ;
}

void ConsoleCommands_ProbeEHCIUSB()
{
	EHCIManager::Instance().ProbeDevice() ;
}

void ConsoleCommands_ProbeXHCIUSB()
{
	XHCIManager::Instance().ProbeDevice() ;
}

void ConsoleCommands_SetXHCIEventMode()
{
  if(CommandLineParser::Instance().GetNoOfParameters() < 1)
      return ;

  upan::string mode(CommandLineParser::Instance().GetParameterAt(0));
  if(mode == "poll")
    XHCIManager::Instance().SetEventMode(XHCIManager::Poll);
  else if(mode == "int")
    XHCIManager::Instance().SetEventMode(XHCIManager::Interrupt);
  else
    printf("\n Invalid EventMode");
}

void ConsoleCommands_ShowRawDiskList()
{
  unsigned uiCount = CommandLineParser::Instance().GetNoOfParameters() ;
	RawDiskDrive* pDisk = NULL ;

	printf("\n%-15s %-18s %-10s %-15s %-10s", "Name", "Type", "Sec-Size", "Tot-Sectors", "Size (MB)") ;
	printf("\n-----------------------------------------------------------------------") ;

	class LamdaDisplay
	{
		public:
			void operator()(const RawDiskDrive* pParamDisk)
			{
				static const char szTypes[2][32] = { "ATA Hard Disk", "USB SCSI Disk" } ;

				printf("\n%-15s %-18s %-10d %-15d %-10d", pParamDisk->Name().c_str(), 
          szTypes[pParamDisk->Type() - ATA_HARD_DISK], 
					pParamDisk->SectorSize(), 
          pParamDisk->SizeInSectors(), 
          pParamDisk->SectorSize() * pParamDisk->SizeInSectors() / (1024 * 1024));
			}

	} lamdaDisplay ;

	if(uiCount)
	{
		for(unsigned i = 0; i < uiCount; i++)
		{
      const char* szName = CommandLineParser::Instance().GetParameterAt(i) ;
			pDisk = DiskDriveManager::Instance().GetRawDiskByName(szName) ;
			lamdaDisplay(pDisk) ;
		}
	}
	else
	{
    for(auto pDisk : DiskDriveManager::Instance().RawDiskDriveList())
			lamdaDisplay(pDisk) ;
	}
}

void ConsoleCommands_InitFloppyController()
{
	if(!Floppy_GetInitStatus())
		Floppy_Initialize() ;
	else
		printf("\n Floppy Controller already initialized") ;
}

void ConsoleCommands_InitATAController()
{
  ATADeviceController_Initialize() ;
}

void ConsoleCommands_InitMountManager()
{
	if(!MountManager_GetInitStatus())
		MountManager_Initialize() ;
	else
		printf("\n MountManager already initialized") ;
}

void ConsoleCommands_Test()
{
	TestSuite t ;
  t.Run() ;
}

extern void TestMTerm() ;
extern void DiskCache_ShowTotalDiskReads() ;

class DisplayCache : public BTree::InOrderVisitor
{
	public:
		DisplayCache(DiskDrive* pDiskDrive) : m_pDiskDrive(pDiskDrive), m_iCount(0), m_bAbort(false) { }

		void operator()(const BTreeKey& rKey, BTreeValue* pValue) const
		{
			if(!pValue)
				return ;

			const DiskCacheKey& key = static_cast<const DiskCacheKey&>(rKey) ;
		//	DiskCacheValue* pCacheValue = static_cast<DiskCacheValue*>(pValue) ;

		//	printf(", %d ", key.GetSectorID()) ;
			if(key.GetSectorID() % 20344)
				m_iCount++ ;
		}

		bool Abort() const { return m_bAbort ; }

	private:
		DiskDrive* m_pDiskDrive ;
		mutable int m_iCount ;
		mutable bool m_bAbort ;
} ;

extern unsigned DMM_uiTotalKernelAllocation ;

class RankInOrderVisitor : public BTree::InOrderVisitor
{
	private:
		unsigned m_uiSectorID ;
		double m_dRank ;
		const unsigned m_uiCurrent ;

	public:
		RankInOrderVisitor() : m_uiSectorID(0), m_dRank(0), m_uiCurrent(PIT_GetClockCount()) { }

		void operator()(const BTreeKey& rKey, BTreeValue* pValue) 
		{
			const DiskCacheKey& key = static_cast<const DiskCacheKey&>(rKey) ;
			const DiskCacheValue* value = static_cast<const DiskCacheValue*>(pValue) ;

			unsigned uiTimeDiff = m_uiCurrent - value->GetLastAccess() ;
			double dRank = 	(double)uiTimeDiff / (double)value->GetHitCount() ;
			if(dRank > m_dRank)
			{
				m_uiSectorID = key.GetSectorID() ;
				m_dRank = dRank ;
			}
		}

		inline unsigned GetSectorID() { return m_uiSectorID ; }
		inline double GetRank() { return m_dRank ; }

		bool Abort() const { return false ; }
} ;

typedef struct
{
	int len ;
	unsigned cnt ;
	unsigned time ;
	unsigned begin ;
} ReadStat ;

ReadStat read_stat[256] ;
void _UpdateReadStat(unsigned len, bool bFirst)
{
	unsigned uiTime = PIT_GetClockCount() ;
	static bool bInit = false ;
	if(!bInit)
	{
		bInit = true ;
		for(int i = 0; i < 256; i++)
		{
			read_stat[i].len = -1 ;
			read_stat[i].cnt = 0 ;
			read_stat[i].time = 0 ;
			read_stat[i].begin = 0 ;
		}
	}

	if(!bFirst)
	{
		for(int i = 0; i < 256; i++)
		{
			if((unsigned)read_stat[i].len == len)
			{
				read_stat[i].time += (uiTime - read_stat[i].begin) ;
				break ;
			}
		}
		return ;
	}

	int free = -1 ;
	bool bUpdated = false ;
	for(int i = 0; i < 256; i++)
	{
		if((unsigned)read_stat[i].len == len)
		{
			read_stat[i].cnt++ ;
			read_stat[i].begin = uiTime ;
			bUpdated = true ;
			break ;
		}

		if(free == -1 && read_stat[i].len == -1)
			free = i ;
	}

	if(!bUpdated)
	{
		if(free >= 0 && free < 256)
		{
			read_stat[free].len = len ;
	 		read_stat[free].cnt	= 1 ;
			read_stat[free].begin = uiTime ;
			read_stat[free].time = 0 ;
		}
	}
}

void _DisplayReadStat()
{
	for(int i = 0 ; i < 256; i++)
	{
		if(read_stat[i].len != -1)
			printf("\n Len: %u, ReadCount: %u, TickCount: %u", read_stat[i].len, read_stat[i].cnt, read_stat[i].time) ;
	}
}

void ConsoleCommands_Testv()
{
	//_DisplayReadStat() ;
	//printf("\n RAM SIZE: %u", MemManager::Instance().GetRamSize()) ;
	//VM86_Test() ;
	KC::MMouseDriver() ;
	//KC::MDisplay().SetMouseCursorPos(KC::MDisplay().GetMouseCursorPos() + 70) ;
}

void ConsoleCommands_TestNet()
{
	IrqManager::Instance().DisplayIRQList() ;
	//KC::MNetworkManager() ;
}

class Global
{
	public:
		Global()
		{
			printf("GLOBAL CONSTR\n");
		}
		~Global()
		{
			printf("GLOBAL DESTR\n");
			while(1);
		};
};

static Global GLOBAL;

//#include <vector>
void ConsoleCommands_TestCPP()
{
//	std::vector<int> x;
//	std::vector<char> y;
//	for(int i = 0; i < 10; i++)
//	{
//		x.push_back(i);
//		y.push_back(40 + i);
//	}
//
//	for(int i = 0; i < 10; i++)
//	{
//		printf("\n%d -> %c, ", x[i], y[i]);
//	}
//	printf("\n");
Global x;
}

void ConsoleCommands_Beep()
{
  PCSound::Instance().Beep();
}

void ConsoleCommands_Sleep()
{
  uint32_t t = atoi(CommandLineParser::Instance().GetParameterAt(0));
  ProcessManager::Instance().Sleep(t * 1000);
}
