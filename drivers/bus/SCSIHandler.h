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
#ifndef _SCSIHANDLER_H_
#define _SCSIHANDLER_H_

#include <Global.h>
#include <string.h>

#define SCSIHandler_SUCCESS 0
#define SCSIHandler_FAILURE 1

#define MAX_SCSI_TAB_ENTRIES 256
#define MAX_SCSI_DEV_TYPES 14

#define SCSI_CMD_SIZE 16
#define SCSI_SENSE_SIZE 64

// Tranfser Directions
#define SCSI_DATA_WRITE 1
#define SCSI_DATA_READ 2
#define SCSI_DATA_NONE 3

/* SCSI command sets */
#define SCSI_UNKNOWN	0
#define SCSI_1			1
#define SCSI_1_CCS		2
#define SCSI_2			3
#define SCSI_3			4

/* SCSI opcodes */
#define SCSI_TEST_UNIT_READY	0x00
#define SCSI_REZERO_UNIT		0x01
#define SCSI_REQUEST_SENSE		0x03
#define SCSI_FORMAT_UNIT		0x04
#define SCSI_READ_BLOCK_LIMITS	0x05
#define SCSI_REASSIGN_BLOCKS	0x07
#define SCSI_READ_6				0x08
#define SCSI_WRITE_6			0x0A
#define SCSI_SEEK_6				0x0B
#define SCSI_READ_REVERSE		0x0F
#define SCSI_WRITE_FILEMARKS	0x10
#define SCSI_SPACE				0x11
#define SCSI_INQUIRY			0x12
#define SCSI_RECOVER_BUFFERED_DATA	0x14
#define SCSI_MODE_SELECT		0x15
#define SCSI_RESERVE			0x16
#define SCSI_RELEASE			0x17
#define SCSI_COPY				0x18
#define SCSI_ERASE				0x19
#define SCSI_MODE_SENSE			0x1A
#define SCSI_START_STOP			0x1B
#define SCSI_RECEIVE_DIAGNOSTIC	0x1C
#define SCSI_SEND_DIAGNOSTIC	0x1D
#define SCSI_ALLOW_MEDIUM_REMOVAL	0x1E

#define SCSI_SET_WINDOW			0x24
#define SCSI_READ_CAPACITY		0x25
#define SCSI_READ_10			0x28
#define SCSI_WRITE_10			0x2A
#define SCSI_SEEK_10			0x2B
#define SCSI_WRITE_VERIFY		0x2E
#define SCSI_VERIFY				0x2F
#define SCSI_SEARCH_HIGH		0x30
#define SCSI_SEARCH_EQUAL		0x31
#define SCSI_SEARCH_LOW			0x32
#define SCSI_SET_LIMITS			0x33
#define SCSI_PRE_FETCH			0x34
#define SCSI_READ_POSITION		0x34
#define SCSI_SYNCHRONIZE_CACHE	0x35
#define SCSI_LOCK_UNLOCK_CACHE	0x36
#define SCSI_READ_DEFECT_DATA	0x37
#define SCSI_MEDIUM_SCAN		0x38
#define SCSI_COMPARE			0x39
#define SCSI_COPY_VERIFY		0x3A
#define SCSI_WRITE_BUFFER		0x3B
#define SCSI_READ_BUFFER		0x3C
#define SCSI_UPDATE_BLOCK		0x3D
#define SCSI_READ_LONG			0x3E
#define SCSI_WRITE_LONG			0x3F
#define SCSI_CHANGE_DEFINITION	0x40
#define SCSI_WRITE_SAME			0x41
#define SCSI_READ_TOC			0x43
#define SCSI_LOG_SELECT			0x4C
#define SCSI_LOG_SENSE			0x4D
#define SCSI_MODE_SELECT_10		0x55
#define SCSI_RESERVE_10			0x56
#define SCSI_RELEASE_10			0x57
#define SCSI_MODE_SENSE_10		0x5A
#define SCSI_PERSISTENT_RESERVE_IN	0x5E
#define SCSI_PERSISTENT_RESERVE_OUT	0x5F
#define SCSI_MOVE_MEDIUM		0xA5
#define SCSI_READ_12			0xA8
#define SCSI_WRITE_12			0xAA
#define SCSI_WRITE_VERIFY_12	0xAE
#define SCSI_SEARCH_HIGH_12		0xB0
#define SCSI_SEARCH_EQUAL_12	0xB1
#define SCSI_SEARCH_LOW_12		0xB2
#define SCSI_SEND_VOLUME_TAG	0xB6
#define SCSI_READ_ELEMENT_STATUS	0xB8
#define SCSI_READ_CD			0xBE
#define SCSI_WRITE_LONG_2		0xEA

/* Sense keys */
#define SCSI_NO_SENSE			0x00
#define SCSI_RECOVERED_ERROR	0x01
#define SCSI_NOT_READY			0x02
#define SCSI_MEDIUM_ERROR		0x03
#define SCSI_HARDWARE_ERROR		0x04
#define SCSI_ILLEGAL_REQUEST	0x05
#define SCSI_UNIT_ATTENTION		0x06
#define SCSI_DATA_PROTECT		0x07
#define SCSI_BLANK_CHECK		0x08
#define SCSI_VENDOR_SPECIFIC	0x09
#define SCSI_COPY_ABORTED		0x0A
#define SCSI_ABORTED_COMMAND	0x0B
#define SCSI_VOLUME_OVERFLOW	0x0D
#define SCSI_MISCOMPARE			0x0E

/* Additional sense codes */
#define SCSI_NO_ASC_DATA			0x00
#define SCSI_LOGICAL_UNIT_NOT_READY	0x04
	#define SCSI_NOT_REPORTABLE		0x00
	#define SCSI_BECOMING_READY		0x01
	#define SCSI_MUST_INITIALIZE	0x02
	#define SCSI_MANUAL_INTERVENTION	0x03
	#define SCSI_FORMAT_IN_PROGRESS		0x04
	#define SCSI_REBUILD_IN_PROGRESS	0x05
	#define SCSI_RECALC_IN_PROGRESS		0x06
	#define SCSI_OP_IN_PROGRESS			0x07
	#define SCSI_LONG_WRITE_IN_PROGRESS	0x08
	#define SCSI_SELF_TEST_IN_PROGRESS	0x09
	#define SCSI_ASSYM_ACCESS_STATE_TRANS	0x0A
	#define SCSI_TARGET_PORT_STANDBY		0x0B
	#define SCSI_TARGET_PORT_UNAVAILABLE	0x0C
	#define SCSI_AUX_MEM_UNAVAILABLE		0x10
	#define SCSI_NOTIFY_REQUIRED			0x11
#define SCSI_NOT_RESPONDING			0x05
#define SCSI_MEDIUM					0x3A
	#define SCSI_MEDIUM_NOT_PRESENT	0x00
	#define SCSI_MEDIUM_TRAY_CLOSED	0x01
	#define SCSI_MEDIUM_TRAY_OPEN	0x02
	#define SCSI_MEDIUM_LOADABLE	0x03

/* Device types */
#define SCSI_TYPE_DISK		0x00
#define SCSI_TYPE_TAPE		0x01
#define SCSI_TYPE_PRINTER	0x02
#define SCSI_TYPE_PROCESSOR	0x03
#define SCSI_TYPE_WORM		0x04
#define SCSI_TYPE_ROM		0x05
#define SCSI_TYPE_SCANNER	0x06
#define SCSI_TYPE_MOD		0x07
#define SCSI_TYPE_MEDIUM_CHANGER	0x08
#define SCSI_TYPE_COMM		0x09
#define SCSI_TYPE_ENCLOSURE	0x0D
#define SCSI_TYPE_NO_LUN	0x7F

/* Status codes */
#define SCSI_GOOD				0x00
#define SCSI_CHECK_CONDITION	0x01
#define SCSI_CONDITION_GOOD		0x02
#define SCSI_BUSY				0x04
#define SCSI_INTERMEDIATE_GOOD	0x08
#define SCSI_INTERMEDIATE_C_GOOD	0x0A
#define SCSI_RESERVATION_CONFLICT	0x0C
#define SCSI_COMMAND_TERMINATED		0x11
#define SCSI_QUEUE_FULL			0x14
#define SCSI_STATUS_MASK		0x3E

/* Message codes */
#define SCSI_COMMAND_COMPLETE	0x00
#define SCSI_EXTENDED_MESSAGE	0x01
	#define SCSI_EXTENDED_MODIFY_DATA_POINTER	0x00
	#define SCSI_EXTENDED_SDTR					0x01
	#define SCSI_EXTENDED_EXTENDED_IDENTIFY		0x02
	#define SCSI_EXTENDED_WDTR					0x03
#define SCSI_SAVE_POINTERS		0x02
#define SCSI_RESTORE_POINTERS	0x03
#define SCSI_DISCONNECT			0x04
#define SCSI_INITIATOR_ERROR	0x05
#define SCSI_ABORT				0x06
#define SCSI_MESSAGE_REJECT		0x07
#define SCSI_NOP				0x08
#define SCSI_MSG_PARITY_ERROR	0x09
#define SCSI_LINKED_CMD_COMPLETE	0x0A
#define SCSI_LINKED_FLG_CMD_COMPLETE	0x0B
#define SCSI_BUS_DEVICE_RESET	0x0C
#define SCSI_INITIATE_RECOVERY	0x0F
#define SCSI_RELEASE_RECOVERY	0x10
#define SCSI_SIMPLE_QUEUE_TAG	0x20
#define SCSI_HEAD_OF_QUEUE_TAG	0x21
#define SCSI_ORDERED_QUEUE_TAG	0x22

/* Host codes */
#define SCSI_OK			0x00
#define SCSI_ERROR		0x01

#define SCSI_NEED_RETRY	0x2001
#define SCSI_SUCCESS	0x2002
#define SCSI_FAILED		0x2003
#define SCSI_QUEUED		0x2004
#define SCSI_SOFT_ERRO	0x2005
#define SCSI_ADD_TO_MLQUEUE	0x2006

/* Status macros */
#define SCSI_STATUS( result )	( ( ( result ) >> 1 ) & 0x1F )
#define SCSI_STATUS_MESSAGE( result )	( ( ( result ) >> 8 ) & 0xFF )
#define SCSI_STATUS_HOST( result )	( ( ( result ) >> 16 ) & 0xFF )
#define SCSI_STATUS_DRIVER( result )	( ( ( result ) >> 24 ) & 0xFF )

typedef enum {
	SENSE_OK,
	SENSE_RETRY,
	SENSE_FATAL
} SCSIHandler_eCheckSense ;

typedef struct SCSICommand SCSICommand ;

struct SCSIHost ;
typedef struct SCSIHost SCSIHost ;

class SCSIHost
{
  public:
  virtual ~SCSIHost() {}
	virtual upan::string GetName() = 0;
  virtual bool QueueCommand(SCSICommand*) = 0;
};

typedef struct
{
	SCSIHost* pHost ;
	int iOpenCount ;

	int iChannel ;
	int iDevice ;
	int iLun ;

	int iDeviceTypeIndex ;
	const char* pszDeviceType ;

	upan::string szName;
	upan::string szVendor;
	upan::string szModel;
	upan::string szRevision;

	unsigned uiSectorSize ;
	unsigned uiSectors ;
	unsigned long ulSize ;

	int iSCSILevel ;
	bool bRemovable ;
} SCSIDevice ;

/* Sense data */
typedef struct
{
	byte ErrorCode:7;
	byte Valid:1;

	byte SegmentNumber;

	byte SenseKey:4;
	byte Reserved2:1;
	byte Ili:1;
	byte Reserved1:2;

	byte aInformation[ 4 ];
	byte AddSenseLen ;
	byte aCommandInfo[ 4 ];
	byte Asc;
	byte Ascq;
	byte Fruc;
	
	byte Sks[ 3 ];
	byte Asb[ 46 ];
} PACKED SCSISense ;

struct SCSICommand
{
	SCSIHost* pHost ;

	int iChannel ;
	int iDevice ;
	int iLun ;

	int iDirection ;

	byte bCommand[ SCSI_CMD_SIZE ] ;
	int iCmdLen ;

	byte* pRequestBuffer ;
	int iRequestLen ;
	int iResult ;

    union
	{
		byte bSense[ SCSI_SENSE_SIZE ];
		SCSISense Sense ;
    } u;
} ;

void SCSIHandler_Initialize() ;
byte SCSIHandler_Start(SCSIDevice* pDevice) ;
byte SCSIHandler_Eject(SCSIDevice* pDevice) ;
unsigned SCSIHandler_GetTransferLen(SCSICommand* pCommand) ;
int SCSIHandler_CheckSense(SCSISense* pSense) ;
byte SCSIHandler_RequestSense(SCSIDevice* pDevice, SCSISense* pSense) ;
SCSIDevice* SCSIHandler_ScanDevice(SCSIHost *pHost, int iChannel, int iDevice, int iLun) ;

byte SCSIHandler_GenericOpen(SCSIDevice* pDevice) ;
byte SCSIHandler_GenericClose(SCSIDevice* pDevice) ;
byte SCSIHandler_GenericRead(SCSIDevice* pDevice, unsigned uiStartSector, unsigned uiNumOfSectors, byte* pDataBuffer) ;
byte SCSIHandler_GenericWrite(SCSIDevice* pDevice, unsigned uiStartSector, unsigned uiNumOfSectors, byte* pDataBuffer) ;

#endif
