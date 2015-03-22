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
# include <stdio.h>
# include <MOSMain.h>
# include <MemUtil.h>

# include <FileSystem.h>
# include <ProcessManager.h>
# include <SysCall.h>
# include <MemManager.h>
# include <PCIBusHandler.h>
# include <DMM.h>
# include <Directory.h>
# include <Floppy.h>
# include <MemUtil.h>
# include <String.h>
# include <FSCommand.h>
# include <KernelService.h>
# include <ATADeviceController.h>
# include <ProcFileManager.h>
# include <SessionManager.h>
# include <UserManager.h>
# include <PartitionManager.h>
# include <RTC.h>
# include <MultiBoot.h>
# include <MountManager.h>
# include <DisplayManager.h>
# include <UHCIController.h>
# include <EHCIController.h>

/**** Global Variable declaration/definition *****/
byte KERNEL_MODE ;
byte SPECIAL_TASK ;
int debug_point ;
/***********************************************/

static int errno ;
static int MOSMain_KernelPID ;

extern void Global_Object_Init() ;

void MOSMain_KernelProcess()
{
	int pid ;

	//MountManager_MountDrives() ;
	
	MOSMain_KernelPID = ProcessManager_iCurrentProcessID ;

	KC::MKernelService().Spawn() ;
	KC::MKernelService().Spawn() ;

	ProcessManager_CreateKernelImage((unsigned)&Console_StartMOSConsole, ProcessManager_iCurrentProcessID, true, NULL, NULL, &pid, "console") ;
//	ProcessManager_CreateKernelImage((unsigned)&FloatProcess, ProcessManager_iCurrentProcessID, false, NULL, NULL, &pid, "float") ;
//	ProcessManager_CreateKernelImage((unsigned)&FloatProcess, ProcessManager_iCurrentProcessID, false, NULL, NULL, &pid, "float1") ;
//	ProcessManager_CreateKernelImage((unsigned)&SessionManager_StartSession, NO_PROCESS_ID, true, NULL, NULL, &pid, "sesman") ;
//	SessionManager_SetSessionIDMap(SessionManager_KeyToSessionIDMap(Keyboard_F1), pid) ;

	ProcessManager_WaitOnChild(pid) ;
	ProcessManager_EXIT() ;
}

void Initialize()
{
	Global_Object_Init() ;

	debug_point = 0 ;

	KERNEL_MODE = true ;
	SPECIAL_TASK = false ;

	MultiBoot_Initialize() ;

	DisplayManager::Initialize() ;
	KC::MDisplay().Message("\n****    Welcome To MOS The Mother Operating System   ****\n", Display::Attribute(' ')) ;

	// RAII
	KC::MMemManager() ;

	IDT::Initialize() ;
	PIC::Initialize() ;
	DMA_Initialize() ;
	PIT_Initialize() ;

	ProcessManager_Initialize() ;
	SysCall_Initialize() ;

	KC::MKernelService() ;

	PIC::EnableInterrupt(PIC::TIMER_IRQ) ;

/* Start - Peripheral Device Initialization */
//TODO: An Abstract Bus Handler which should internally take care of different
//types of bus like ISA, PCI etc... 
	PCIBusHandler_Initialize() ;

	DeviceDrive_Initialize() ;

	Keyboard_Initialize() ;

	//Floppy_Initialize() ;
	//ATADeviceController_Initialize() ;

	//MountManager_Initialize() ;

/*End - Peripheral Device Initialization */

	RTC::Initialize() ;
	//while(1) ;
	
	//USB
	USBController_Initialize() ;
	EHCIController_Initialize() ;
	UHCIController_Initialize() ;
	USBMassBulkStorageDisk_Initialize() ;

	SessionManager_Initialize() ;

	Console_Initialize() ;
}

Mutex& MOSMain_GetDMMMutex()
{
	static Mutex mDMMMutex ;
	return mDMMMutex ;
}

byte* GetArea()
{
	static byte area[0x1000] ;
	return area ;
}

extern void VM86_Test() ;
void MOSMain()
{
	byte* bios = (byte*)(0 - GLOBAL_DATA_SEGMENT_BASE) ;

	for(int i = 0; i < 0x500; i++) 
		GetArea()[i] = bios[i] ;

	Initialize() ;

	int pid ;
	ProcessManager_CreateKernelImage((unsigned)&MOSMain_KernelProcess, NO_PROCESS_ID, true, NULL, NULL, &pid, "kerparent") ;
//	ProcessManager_CreateKernelImage((unsigned)&Console_StartMOSConsole, NO_PROCESS_ID, true, NULL, NULL, &pid) ;

	ProcessManager_StartScheduler() ;
	while(1) ;
}

bool MOSMain_IsKernelDebugOn()
{
	const char* szVal = getenv("MOS_KDEBUG") ;
	if(szVal != NULL)
		if(String_Compare(szVal, "1") == 0)
			return true ;

	return false ;
}

bool MOSMain_isCoProcPresent()
{
	if(CO_PROC_FPU_TYPE == NO_CO_PROC)
		return false ;
	return true ;
}

int MOSMain_KernelProcessID()
{
	return MOSMain_KernelPID ;
}

