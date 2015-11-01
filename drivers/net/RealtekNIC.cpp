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
#include <RealtekNIC.h>
#include <Display.h>
#include <stdio.h>

// PCI Space Registers
#define REG_MAC			0		/* Ethernet hardware address. 	*/
#define REG_MAR			8		/* Multicast filter. 	*/
#define REG_TX_STATUS	0x10	/* Transmit status (Four 32bit registers). 	*/
#define REG_TX_ADDR		0x20	/* Tx descriptors (also four 32bit). 	*/
#define REG_RX_BUF		0x30
#define REG_COMMAND		0x37
#define REG_RX_BUFPTR	0x38
#define REG_RX_BUFADDR 	0x3A
#define REG_INT_MASK 	0x3C
#define REG_INT_STATUS 	0x3E
#define REG_TX_CONFIG 	0x40
#define REG_RX_CONFIG 	0x44
#define REG_TIMER		0x48	/* A general-purpose counter. 	*/
#define REG_RX_MISSED	0x4C	/* 24 bits valid write clears. 	*/
#define REG_CFG9346 	0x50
#define REG_CONFIG0 	0x51
#define REG_CONFIG1 	0x52
#define REG_FLASH 		0x54
#define REG_MEDIA_STAT	0x58
#define REG_CONFIG3 	0x59
#define REG_CONFIG4 	0x5A	/* absent on RTL-8139A 	*/
#define REG_HLTCLK 		0x5B
#define REG_MULTI_INTR 	0x5C
#define REG_TX_SUMMARY 	0x60
#define REG_MODE_CTRL 	0x62
#define REG__MODE_STAT 	0x64
#define REG_NWAY_ADVERT 0x66
#define REG_NWAY_LPAR 	0x68
#define REG_NWAY_EXPAN 	0x6A
#define REG_FIFOTMS 	0x70	/* FIFO Control and test. 	*/
#define REG_CSCR 		0x74	/* Chip Status and Configuration Register. 	*/
#define REG_PARA78 		0x78
#define REG_PARA7C 		0x7C	/* Magic transceiver parameter register. 	*/
#define REG_CONFIG5 	0XD8	/* absent on RTL-8139A 	*/

//Command Register Bits
#define CMD_RX_BUFEMPTY	0x01
#define CMD_TX_ENB		0x04
#define CMD_RX_ENB		0x08
#define CMD_RESET		0x10

// Interrupt status register bits
#define RX_OK		0x01
#define RX_ERR		0x02
#define TX_OK		0x04
#define TX_ERR		0x08
#define RX_OVERFLOW	0x10
#define RX_UNDERRUN	0x20
#define RX_FIFOOVER	0x40
#define PCS_TIMEOUT	0x4000
#define PCI_ERR		0x8000
#define RX_ACKBITS	(RX_FIFOOVER | RX_OVERFLOW | RX_OK)

// TX Status bits
#define TX_HOSTOWNS	 	0x2000
#define TX_UNDERRUN	 	0x4000
#define TX_STATOK	 	0x8000
#define TX_OUTOFWINDOW	0x20000000
#define TX_ABORTED		0x40000000
#define TX_CARRIERLOST	0x80000000

// RX Status bits
#define RX_STATUSOK		0x0001
#define RX_BADALIGN		0x0002
#define RX_CRCERR		0x0004
#define RX_TOOLONG		0x0008
#define RX_RUN			0x0010
#define RX_BADSYMBOL	0x0020
#define RX_BROADCAST	0x2000
#define RX_PHYSICAL		0x4000
#define RX_MULTICAST	0x8000

// RX Config register bits
#define RX_ACCEPTERR		0x20
#define RX_ACCEPTRUNT		0x10
#define RX_ACCEPTBROADCAST	0x08
#define RX_ACCEPTMULTICAST	0x04
#define RX_ACCEPTMYPHYS		0x02
#define RX_ACCEPTALLPHYS	0x01

// TX Config register bits
#define TX_IFG1			0x2000000	/* Interframe Gap Time */
#define TX_IFG0			0x1000000	/* Enabling these bits violates IEEE 802.3 */
#define TX_LOOPBACK		0x60000		/* enable loopback test mode */
#define TX_CRC			0x10000		/* DISABLE appending CRC to end of Tx_ packets */
#define TX_CLEARABT		1			/* Clear abort (WO) */
#define TX_DMASHIFT		8			/* DMA burst value (0-7) is shifted this many bits */
#define TX_RETRYSHIFT	4			/* TXRR value (0-15) is shifted this many bits */
#define TX_VERSIONMASK	0x7C800000	/* mask out version bits 30-26 23 */

// RX Config register bits
/* rx fifo threshold */
#define RX_CFGFIFOSHIFT		13
#define RX_CFGFIFONONE		0xE000
/* Max DMA burst */
#define RX_CFGDMA_SHIFT		8
#define RX_CFGDMA_ULIMITED	0x700
/* rx ring buffer length */
#define RX_CFG_RCV8K		0
#define RX_CFG_RCV16K		0x800
#define RX_CFG_RCV32K		0x1000
#define RX_CFG_RCV64K		0x1800 
/* Disable packet wrap at end of Rx buffer */
#define RX_NOWRAP			0x80

// Config1 register bits
#define CFG1_PM_ENABLE	0x01
#define CFG1_VPD_ENABLE	0x02
#define CFG1_PIO		0x04
#define CFG1_MMIO		0x08
#define LWAKE			0x10	/* not on 8139 8139A */
#define CFG1_DRVR_LOAD	0x20
#define CFG1_LED0		0x40
#define CFG1_LED1		0x80
#define SLEEP			2 		/* only on 8139 8139A */
#define PWRDN			1 		/* only on 8139 8139A */

// Config3 register bits
#define CFG3_FBTBEN		0x1		/* 1 = Fast Back to Back */
#define CFG3_FUNCREGEN	0x2 	/* 1 = enable CardBus Function registers */
#define CFG3_CLKRUN_EN	0x4 	/* 1 = enable CLKRUN */
#define CFG3_CARDB_EN	0x8 	/* 1 = enable CardBus registers */
#define CFG3_LINKUP		0x10 	/* 1 = wake up on link up */
#define CFG3_MAGIC		0x20 	/* 1 = wake up on Magic Packet (tm) */
#define CFG3_PARM_EN	0x40 	/* 0 = software can set twister parameters */
#define CFG3_GNTSEL		0x80 	/* 1 = delay 1 clock from PCI GNT signal */

// Config4 register bits
#define CFG4_LWPTN		0x4 /* not on 8139, 8139A */

// Config5 register bits
#define CFG5_PME_STS	0x1		/* 1 = PCI reset resets PME_Status */
#define CFG5_LANWAKE	0x2 	/* 1 = enable LANWake signal */
#define CFG5_LDPS		0x4 	/* 0 = save power when link is down */
#define CFG5_FIFOADRPTR	0x8 	/* Realtek internal SRAM testing */
#define CFG5_UWF		0x10 	/* 1 = accept unicast wakeup frame */
#define CFG5_MWF		0x20 	/* 1 = accept multicast wakeup frame */
#define CFG5_BWF		0x40 	/* 1 = accept broadcast wakeup frame */

/* Twister tuning parameters from RealTek.
   Completely undocumented, but required to tune bad links on some boards. */
#define CSCR_LINKOKBIT		0x0400
#define CSCR_LINKCHANGEBIT	0x0800
#define CSCR_LINKSTATUSBITS	0x0F000
#define CSCR_LINKDOWNOFFCMD	0x003C0
#define CSCR_LINKDOWNCMD	0x0F3C0

#define CFG9346_LOCK	0x00
#define CFG9346_UNLOCK	0xC0

const IRQ* RealtekNIC::m_pIRQ ;

RealtekNIC::RealtekNIC(const PCIEntry* pPCIEntry) : NIC(pPCIEntry)
{
	m_bSetup = Setup() ;
}

bool RealtekNIC::Setup()
{
	printf("\n Vendor: %x, Device: %x, Revision: %x"
			"\n SubSysVendor: %x, SubSysDevice: %x"
			"\n Interrupt: %d",
			m_pPCIEntry->usVendorID, m_pPCIEntry->usDeviceID, m_pPCIEntry->bRevisionID,
			m_pPCIEntry->BusEntity.NonBridge.usSubsystemVendorID,
			m_pPCIEntry->BusEntity.NonBridge.usSubsystemDeviceID,
			m_pPCIEntry->BusEntity.NonBridge.bInterruptLine) ;

	// TODO: PCI Enable Device	
	
	// only PIO mode is supported
	m_uiIOPort = m_pPCIEntry->BusEntity.NonBridge.uiBaseAddress0 ;
	if(!(m_uiIOPort & PCI_IO_ADDRESS_SPACE))
	{
		printf("\n PCI Address space is not PIO. Only PIO mode is supported currently!") ;
		return false ;
	}
	m_uiIOPort &= PCI_ADDRESS_IO_MASK ;
	m_bInterrupt = m_pPCIEntry->BusEntity.NonBridge.bInterruptLine ;

	PCIBusHandler_EnableBusMaster(m_pPCIEntry) ;

	// Bring old chips out of low-power mode
	SendByte(REG_HLTCLK, 'R') ;

	// Check hardware
	unsigned uiVersion = ReceiveDWord(REG_TX_CONFIG) ;
	if(uiVersion == 0xFFFFFFFF)
	{
		printf("\n NIC Chip not responding. Failed to setup device") ;
		return false ;
	}
	
	// Identify NIC chip on board 
	printf("\n Chip version: %x", uiVersion >> 24) ;
	printf("\n Currently only 8139 chip is supported. Defaulting to 8139") ;

	// TODO: Power Management setup (LWAKE etc...) 
	if(!Reset())
	{
		printf("\n Reset failed") ;
		return false ;
	}

	// should it really be read out of EEPROM when it can be 
	// done away with a simple read from PCI space!!
	printf("\n MAC Address = ") ;
	m_macAddress.m_iLen = 6 ;
	for(int i = 0; i < m_macAddress.m_iLen; i++)
	{
		m_macAddress.m_address[ i ] = ReceiveByte(i) ;
		printf("%.2x:", m_macAddress.m_address[ i ]) ;
	}

	return true ;
}

bool RealtekNIC::Reset()
{
	SendByte(REG_COMMAND, 0x10) ;
	int iTimeOut = 0xFFFFF ;

	do
	{
		if(!(ReceiveByte(REG_COMMAND) & 0x10))
			break ;
		iTimeOut-- ;
	} while(iTimeOut > 0) ;

	return iTimeOut > 0 ;
}

void RealtekNIC::Handler()
{
}

bool RealtekNIC::Open()
{
	m_pIRQ = PIC::Instance().RegisterIRQ(m_bInterrupt, (unsigned)&RealtekNIC::Handler) ;

//	tp->tx_bufs = pci_alloc_consistent(tp->pci_dev, TX_BUF_TOT_LEN,
//					   &tp->tx_bufs_dma);
//	tp->rx_ring = pci_alloc_consistent(tp->pci_dev, RX_BUF_TOT_LEN,
//					   &tp->rx_ring_dma);
//	if (tp->tx_bufs == NULL || tp->rx_ring == NULL) {
//		free_irq(dev->irq, dev);
//
//		if (tp->tx_bufs)
//			pci_free_consistent(tp->pci_dev, TX_BUF_TOT_LEN,
//					    tp->tx_bufs, tp->tx_bufs_dma);
//		if (tp->rx_ring)
//			pci_free_consistent(tp->pci_dev, RX_BUF_TOT_LEN,
//					    tp->rx_ring, tp->rx_ring_dma);
//
//		return -ENOMEM;
//
//	}

//	tp->mii.full_duplex = tp->mii.force_media;
//	tp->tx_flag = (TX_FIFO_THRESH << 11) & 0x003f0000;
//
//	rtl8139_init_ring (dev);
//	rtl8139_hw_start (dev);
//
//	if (netif_msg_ifup(tp))
//		printk(KERN_DEBUG "%s: rtl8139_open() ioaddr %#lx IRQ %d"
//			" GP Pins %2.2x %s-duplex.\n",
//			dev->name, pci_resource_start (tp->pci_dev, 1),
//			dev->irq, RTL_R8 (MediaStatus),
//			tp->mii.full_duplex ? "full" : "half");
//
//	rtl8139_start_thread(dev);

	return true ;
}

void RealtekNIC::InitRing()
{
//	struct rtl8139_private *tp = dev->priv;
//	int i;
//
//	tp->cur_rx = 0;
//	tp->cur_tx = 0;
//	tp->dirty_tx = 0;
//
//	for (i = 0; i < NUM_TX_DESC; i++)
//		tp->tx_buf[i] = &tp->tx_bufs[i * TX_BUF_SIZE];
}

//static void rtl8139_hw_start (struct net_device *dev)
//{
//	struct rtl8139_private *tp = dev->priv;
//	void *ioaddr = tp->mmio_addr;
//	u32 i;
//	u8 tmp;
//
//	/* Bring old chips out of low-power mode. */
//	if (rtl_chip_info[tp->chipset].flags & HasHltClk)
//		RTL_W8 (HltClk, 'R');
//
//	rtl8139_chip_reset (ioaddr);
//
//	/* unlock Config[01234] and BMCR register writes */
//	RTL_W8_F (Cfg9346, Cfg9346_Unlock);
//	/* Restore our idea of the MAC address. */
//	RTL_W32_F (MAC0 + 0, cpu_to_le32 (*(u32 *) (dev->dev_addr + 0)));
//	RTL_W32_F (MAC0 + 4, cpu_to_le32 (*(u32 *) (dev->dev_addr + 4)));
//
//	/* Must enable Tx/Rx before setting transfer thresholds! */
//	RTL_W8 (ChipCmd, CmdRxEnb | CmdTxEnb);
//
//	tp->rx_config = rtl8139_rx_config | AcceptBroadcast | AcceptMyPhys;
//	RTL_W32 (RxConfig, tp->rx_config);
//
//	/* Check this value: the documentation for IFG contradicts ifself. */
//	RTL_W32 (TxConfig, rtl8139_tx_config);
//
//	tp->cur_rx = 0;
//
//	rtl_check_media (dev, 1);
//
//	if (tp->chipset >= CH_8139B) {
//		/* Disable magic packet scanning, which is enabled
//		 * when PM is enabled in Config1.  It can be reenabled
//		 * via ETHTOOL_SWOL if desired.  */
//		RTL_W8 (Config3, RTL_R8 (Config3) & ~Cfg3_Magic);
//	}
//
//	DPRINTK("init buffer addresses\n");
//
//	/* Lock Config[01234] and BMCR register writes */
//	RTL_W8 (Cfg9346, Cfg9346_Lock);
//
//	/* init Rx ring buffer DMA address */
//	RTL_W32_F (RxBuf, tp->rx_ring_dma);
//
//	/* init Tx buffer DMA addresses */
//	for (i = 0; i < NUM_TX_DESC; i++)
//		RTL_W32_F (TxAddr0 + (i * 4), tp->tx_bufs_dma + (tp->tx_buf[i] - tp->tx_bufs));
//
//	RTL_W32 (RxMissed, 0);
//
//	rtl8139_set_rx_mode (dev);
//
//	/* no early-rx interrupts */
//	RTL_W16 (MultiIntr, RTL_R16 (MultiIntr) & MultiIntrClear);
//
//	/* make sure RxTx has started */
//	tmp = RTL_R8 (ChipCmd);
//	if ((!(tmp & CmdRxEnb)) || (!(tmp & CmdTxEnb)))
//		RTL_W8 (ChipCmd, CmdRxEnb | CmdTxEnb);
//
//	/* Enable all known interrupts by setting the interrupt mask. */
//	RTL_W16 (IntrMask, rtl8139_intr_mask);
//
//	netif_start_queue (dev);
//}

