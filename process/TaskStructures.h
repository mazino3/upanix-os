#ifndef TASKSTRUCTURES_H
#define TASKSTRUCTURES_H


typedef struct
{
  unsigned short	backlink ;
  unsigned short	FILLER1 ;

  unsigned		ESP0 ;
  unsigned short	SS0 ;
  unsigned short	FILLER2 ;

  unsigned		ESP1 ;
  unsigned short	SS1 ;
  unsigned short	FILLER3 ;

  unsigned		ESP2 ;
  unsigned short	SS2 ;
  unsigned short	FILLER4 ;

  unsigned		CR3_PDBR ;
  unsigned		EIP ;
  unsigned		EFLAGS ;

  unsigned		EAX ;
  unsigned		ECX ;
  unsigned		EDX ;
  unsigned		EBX ;
  unsigned		ESP ;
  unsigned		EBP ;
  unsigned		ESI ;
  unsigned		EDI ;

  unsigned short	ES ;
  unsigned short	FILLER5 ;

  unsigned short	CS ;
  unsigned short	FILLER6 ;

  unsigned short	SS ;
  unsigned short	FILLER7 ;

  unsigned short	DS ;
  unsigned short	FILLER8 ;

  unsigned short	FS ;
  unsigned short	FILLER9 ;

  unsigned short	GS ;
  unsigned short	FILLER10 ;

  unsigned short	LDT ;
  unsigned short	FILLER11 ;

  unsigned short	DEBUG_T_BIT ;
  unsigned short	IO_MAP_BASE ;
} PACKED TaskState ;

typedef struct
{
  unsigned short	usLimit15_0;
  unsigned short	usBase15_0 ;
  byte			bBase23_16 ;
  byte			bType ;
  byte			bLimit19_16_Flags ;
  byte			bBase31_24 ;
} PACKED Descriptor ;

typedef struct
{
  unsigned short int lowerOffset ;
  unsigned short int selector ;
  byte parameterCount ;
  byte options ; /*	constant:5;	 5 bits
           dpl:2;		 2 bits
           present:1;	 1 bit		*/
  unsigned short int upperOffset ;
} PACKED GateDescriptor ;

typedef struct
{
  Descriptor NullDesc ;
  Descriptor LinearDesc ;
  Descriptor CodeDesc ;
  Descriptor DataDesc ;
  Descriptor StackDesc ;
  Descriptor CallGateStackDesc ;
} PACKED ProcessLDT ;

#endif // TASKSTRUCTURES_H
