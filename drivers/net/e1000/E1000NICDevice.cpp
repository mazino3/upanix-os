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

#include <Global.h>
#include <exception.h>
#include <Bit.h>
#include <AsmUtil.h>
#include <DMM.h>
#include <PCIBusHandler.h>
#include <NetworkDevice.h>
#include <MemManager.h>
#include <ProcessManager.h>
#include <E1000NICDevice.h>

static IRQ* E1000_IRQ = nullptr;

static void irq_handler() {
	unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	AsmUtil_SET_KERNEL_DATA_SEGMENTS

	printf("\n NIC E1000 IRQ \n");
	//ProcessManager_SignalInterruptOccured(*E1000_IRQ) ;

	IrqManager::Instance().SendEOI(*E1000_IRQ) ;
	
	AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
	AsmUtil_RESTORE_GPR(GPRStack) ;
	
	asm("leave") ;
	asm("IRET") ;
}

E1000NICDevice::E1000NICDevice(const PCIEntry& pciEntry) : NetworkDevice(pciEntry) {
  unsigned ioAddr = pciEntry.BusEntity.NonBridge.uiBaseAddress0;
	printf("\n PCI BaseAddr: %x", ioAddr);

	ioAddr &= PCI_ADDRESS_MEMORY_32_MASK;
	const unsigned ioSize = pciEntry.GetPCIMemSize(0);
  const unsigned availableMemMapSize = NET_E1000_MMIO_BASE_ADDR_END - NET_E1000_MMIO_BASE_ADDR;

	printf(", Raw MMIO BaseAddr: %x, IOSize: %d, AvailableIOSize: %d", ioAddr, ioSize, availableMemMapSize);
  
	if(ioSize > availableMemMapSize)
    throw upan::exception(XLOC, "E1000 IO Size is %x > greater than available size %x !", ioSize, availableMemMapSize);

  unsigned pagesToMap = ioSize / PAGE_SIZE;
  if(ioSize % PAGE_SIZE)
    ++pagesToMap;

  unsigned uiPDEAddress = MEM_PDBR ;
  unsigned memMapBaseAddress = NET_E1000_MMIO_BASE_ADDR;
  printf("\n Total pages to Map: %d", pagesToMap);
  for(unsigned i = 0; i < pagesToMap; ++i)
  {
  	unsigned uiPDEIndex = ((memMapBaseAddress >> 22) & 0x3FF) ;
	  unsigned uiPTEIndex = ((memMapBaseAddress >> 12) & 0x3FF) ;
	  unsigned uiPTEAddress = (((unsigned*)(KERNEL_VIRTUAL_ADDRESS(uiPDEAddress)))[uiPDEIndex]) & 0xFFFFF000 ;
    // This page is a Read Only area for user process. 0x5 => 101 => User Domain, Read Only, Present Bit
    ((unsigned*)(KERNEL_VIRTUAL_ADDRESS(uiPTEAddress)))[uiPTEIndex] = (ioAddr & 0xFFFFF000) | 0x5 ;
    if(MemManager::Instance().MarkPageAsAllocated(ioAddr / PAGE_SIZE) != Success)
    {
    }

    memMapBaseAddress += PAGE_SIZE;
    ioAddr += PAGE_SIZE;
  }

	Mem_FlushTLB();

  _memIOBase = KERNEL_VIRTUAL_ADDRESS(NET_E1000_MMIO_BASE_ADDR + (ioAddr % PAGE_SIZE));

    /* Enable busmaster */
  unsigned short usCommand ;
  pciEntry.ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
  pciEntry.WritePCIConfig(PCI_COMMAND, 2, usCommand | PCI_COMMAND_IO | PCI_COMMAND_MASTER) ;
  printf("\n Enabled PCI bus master for NIC");

  E1000_IRQ = IrqManager::Instance().RegisterIRQ(pciEntry.BusEntity.NonBridge.bInterruptLine, irq_handler);
  IrqManager::Instance().EnableIRQ(*E1000_IRQ);

  const RegEEPROM regEEPROM(_memIOBase);
  regEEPROM.print();

  RegIntControl regIntControl(_memIOBase);
  regIntControl.disable();

  RegControl regControl(_memIOBase);
  RegRXDescriptor regRx(_memIOBase);
  RegTXDescriptor regTx(_memIOBase);
  
  printf("\n E1000 NIC initialization done, enabling interrupts");
  regIntControl.enable();
  printf("\n E1000 NIC interrupt enabled");
  ProcessManager::Instance().Sleep(100);
  //volatile uint32_t* rstat = (volatile uint32_t*)(_memIOBase + 0x8);
  //printf("\n NIC Status: %x", *rstat);
}

void E1000NICDevice::NotifyEvent() {
    
}

constexpr volatile uint32_t* REG(const uint32_t base, const uint32_t offset) {
  return reinterpret_cast<volatile uint32_t*>(base + offset);
}

E1000NICDevice::RegEEPROM::RegEEPROM(const uint32_t memIOBase) : _eeprom(REG(memIOBase, REG_EEPROM)) {
  for(int i = 0; i < 3; ++i) {
    uint32_t word = readEEPROM(i);
    _macAddress.push_back(word & 0xFF);
    _macAddress.push_back(word >> 8);
  }
}

void E1000NICDevice::RegEEPROM::print() const {
  printf ("\n MAC ADDRESS: ");
  for(const auto& b : _macAddress) {
    printf("%x ", b);
  }
}

uint32_t E1000NICDevice::RegEEPROM::readEEPROM(const int wordPos) {
  *_eeprom = 0x001 | (wordPos << 8);
  ProcessManager::Instance().Sleep(10);
  uint32_t val = *_eeprom;
  if (val & 0x10) {
    val >>= 16;
    return val & 0xFFFF;
  }
  throw upan::exception(XLOC, "E1000 NIC EEPROM register is not supported!");
}

E1000NICDevice::RegIntControl::RegIntControl(const uint32_t memIOBase) :
  _icr(REG(memIOBase, REG_ICR)),
  _itr(REG(memIOBase, REG_ITR)),
  _ics(REG(memIOBase, REG_ICS)),
  _ims(REG(memIOBase, REG_IMS)),
  _imc(REG(memIOBase, REG_IMC)) {
    x = memIOBase;
}

void E1000NICDevice::RegIntControl::disable() {
  *_imc = 0xFFFFFFFF;
  UNUSED volatile uint32_t x = *_icr;
  *_itr = (uint32_t)0;
}

void E1000NICDevice::RegIntControl::enable() {
  *_rdtr = 0;
  *_radv = 0;
  *_rsrpd = 0;

  //*_ims = 0x1FFFF;
  *_ims = 0x1F6DC;
  *_ims = 0xFF & ~4;

  *((volatile uint32_t*)(x + 0xC0));
}

E1000NICDevice::RegControl::RegControl(const uint32_t memIOBase) : 
  _pba(REG(memIOBase, REG_PBA)),
  _txcw(REG(memIOBase, REG_TXCW)),
  _ctrl(REG(memIOBase, REG_CTRL)),
  _mta(REG(memIOBase, REG_MTA)) {
  // PBA: set rx buffer size. tx buffer is calculated as 64 - rx buffer
  // default RX buffer size = 48 KB
  *_pba = 48;

  // TXCW: set ANE, TxConfigWord (Half/Full duplex, Next Page Request)
  *_txcw = 0x80008060;

  // CTRL: clear LRST, set SLU and ASDE, clear PHY_RST, VME, and ILOS
  uint32_t ctrlVal = *_ctrl;
  ctrlVal = Bit::Set(ctrlVal, (1 << 3), false); // clear LRST
  ctrlVal = Bit::Set(ctrlVal, (1 << 5), true); // set ASDE
  ctrlVal = Bit::Set(ctrlVal, (1 << 6), true); // set SLU
  ctrlVal = Bit::Set(ctrlVal, (1 << 7), true); // clear ILOS
  ctrlVal = Bit::Set(ctrlVal, (1 << 30), true); // clear VME
  ctrlVal = Bit::Set(ctrlVal, (1 << 31), true); // clear PHY_RST
  *_ctrl = ctrlVal;

  // MTA reset
  for(int i = 0; i < 0x80; ++i) {
    _mta[i] = 0;
  }
}

E1000NICDevice::RXDescriptor::RXDescriptor() {
  addr = KERNEL_REAL_ADDRESS(DMM_AllocateForKernel(8 KB, 16));
  length = 0;
  checksum = 0;
  status = 0;
  errors = 0;
  special = 0;
}

E1000NICDevice::RegRXDescriptor::RegRXDescriptor(const uint32_t memIOBase) :
  _alow(REG(memIOBase, REG_RDBAL)),
  _ahigh(REG(memIOBase, REG_RDBAH)),
  _len(REG(memIOBase, REG_RDLEN)),
  _head(REG(memIOBase, REG_RDH)),
  _tail(REG(memIOBase, REG_RDT)) {
  _rxDescriptors = new ((void*)DMM_AllocateForKernel(sizeof(RXDescriptor) * NUM_OF_DESC, 16))RXDescriptor[NUM_OF_DESC];
  *_alow = KERNEL_REAL_ADDRESS(_rxDescriptors);
  *_ahigh = 0;
  *_len = NUM_OF_DESC * 16;
  *_head = 0;
  *_tail = NUM_OF_DESC - 1;

  //RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC  | RCTL_BSIZE_2048
  //0110 0000 0010 1000 0000 0001 1110
  *_rxctrl = 0x602801E;
  // Receiver Enable, Store Bad Packets, Broadcast Accept Mode, Strip Ethernet CRC from incoming packet
  //*_rxctrl = 0x04008006;
  
}

E1000NICDevice::TXDescriptor::TXDescriptor() {
  addr = 0;
  length = 0;
  cso = 0;
  cmd = 0;
  status = TSTA_DD;
  css = 0;
  special = 0;
}

E1000NICDevice::RegTXDescriptor::RegTXDescriptor(const uint32_t memIOBase) :
  _alow(REG(memIOBase, REG_TDBAL)),
  _ahigh(REG(memIOBase, REG_TDBAH)),
  _len(REG(memIOBase, REG_TDLEN)),
  _head(REG(memIOBase, REG_TDH)),
  _tail(REG(memIOBase, REG_TDT)) {
  _txDescriptors = new ((void*)DMM_AllocateForKernel(sizeof(TXDescriptor) * NUM_OF_DESC, 16))TXDescriptor[NUM_OF_DESC];
  *_alow = KERNEL_REAL_ADDRESS(_txDescriptors);
  *_ahigh = 0;
  *_len = NUM_OF_DESC * 16;
  *_head = 0;
  *_tail = 0;

  // Enabled, Pad Short Packets, 15 retries, 64-byte COLD, Re-transmit on Late Collision
  *_txctrl = 0x010400FA; // for E1000
   //*_txctrl = 0x3003F0FA; // 0b0110000000000111111000011111010 --> for E1000e

  // IPGT 10, IPGR1 8, IPGR2 6
  *_tipg = 0x0060200A;
}