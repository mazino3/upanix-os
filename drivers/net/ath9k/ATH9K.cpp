#include <PCIBusHandler.h>
#include <MemManager.h>
#include <ATH9K.h>

ATH9K::ATH9K(PCIEntry& pciEntry) : NetworkDevice(pciEntry)
{
//  struct ath_softc *sc;
//  struct ieee80211_hw *hw;
//  char hw_name[64];

  uint16_t usCommand;
  pciEntry.ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
  printf("\n CurVal of PCI_COMMAND: %x", usCommand);
  pciEntry.WritePCIConfig(PCI_COMMAND, 2, usCommand | PCI_COMMAND_MMIO);

  /*
   * The default setting of latency timer yields poor results,
   * set it to the value used by other systems. It may be worth
   * tweaking this setting more.
   */
  pciEntry.WritePCIConfig(PCI_LATENCY_TIMER, 1, 0xa8);

  pciEntry.ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
  printf("\n CurVal of PCI_COMMAND: %x", usCommand);
  pciEntry.WritePCIConfig(PCI_COMMAND, 2, usCommand | PCI_COMMAND_MASTER);

  /*
   * Disable the RETRY_TIMEOUT register (0x41) to keep
   * PCI Tx retries from interfering with C3 CPU state.
   */
  uint32_t val;
  pciEntry.ReadPCIConfig(0x40, 4, &val);
  if ((val & 0x0000ff00) != 0)
    pciEntry.WritePCIConfig(0x40, 4, val & 0xffff00ff);

  unsigned uiIOAddr = pciEntry.BusEntity.NonBridge.uiBaseAddress0;
  printf("\n PCI BaseAddr: %x", uiIOAddr);

  uiIOAddr = uiIOAddr & PCI_ADDRESS_MEMORY_32_MASK;
  unsigned ioSize = pciEntry.GetPCIMemSize(0);
  printf(", Raw MMIO BaseAddr: %x, IOSize: %d", uiIOAddr, ioSize);

  const unsigned availableMemMapSize = NET_ATH9K_MMIO_BASE_ADDR_END - NET_ATH9K_MMIO_BASE_ADDR;
  if(ioSize > availableMemMapSize)
    throw upan::exception(XLOC, "ATH9K IO Size is %x > greater than available size %x !", ioSize, availableMemMapSize);

  unsigned pagesToMap = ioSize / PAGE_SIZE;
  if(ioSize % PAGE_SIZE)
    ++pagesToMap;
  unsigned uiPDEAddress = MEM_PDBR ;
//  const unsigned uiMappedIOAddr = KERNEL_VIRTUAL_ADDRESS(NET_ATH9K_MMIO_BASE_ADDR + (uiIOAddr % PAGE_SIZE));
  uint32_t memMapAddress = NET_ATH9K_MMIO_BASE_ADDR;
  printf("\n Total pages to Map: %d", pagesToMap);
  for(unsigned i = 0; i < pagesToMap; ++i)
  {
    unsigned uiPDEIndex = ((memMapAddress >> 22) & 0x3FF) ;
    unsigned uiPTEIndex = ((memMapAddress >> 12) & 0x3FF) ;
    unsigned uiPTEAddress = (((unsigned*)(KERNEL_VIRTUAL_ADDRESS(uiPDEAddress)))[uiPDEIndex]) & 0xFFFFF000 ;
    // This page is a Read Only area for user process. 0x5 => 101 => User Domain, Read Only, Present Bit
    ((unsigned*)(KERNEL_VIRTUAL_ADDRESS(uiPTEAddress)))[uiPTEIndex] = (uiIOAddr & 0xFFFFF000) | 0x5 ;
    if(MemManager::Instance().MarkPageAsAllocated(uiIOAddr / PAGE_SIZE) != Success)
    {
    }

    memMapAddress += PAGE_SIZE;
    uiIOAddr += PAGE_SIZE;
  }

  Mem_FlushTLB();

  printf("\n Bus: %d, Dev: %d, Func: %d", pciEntry.uiBusNumber, pciEntry.uiDeviceNumber, pciEntry.uiFunction);
  printf("\n Vendor: %x, Device: %x, Rev: %x", pciEntry.usVendorID, pciEntry.usDeviceID, pciEntry.bRevisionID);

//  ath9k_fill_chanctx_ops();
//  hw = ieee80211_alloc_hw(sizeof(struct ath_softc), &ath9k_ops);
//  if (!hw) {
//    dev_err(&pdev->dev, "No memory for ieee80211_hw\n");
//    return -ENOMEM;
//  }

//  SET_IEEE80211_DEV(hw, &pdev->dev);
//  pci_set_drvdata(pdev, hw);

//  sc = hw->priv;
//  sc->hw = hw;
//  sc->dev = &pdev->dev;
//  sc->mem = pcim_iomap_table(pdev)[0];
//  sc->driver_data = id->driver_data;

//  ret = request_irq(pdev->irq, ath_isr, IRQF_SHARED, "ath9k", sc);
//  if (ret) {
//    dev_err(&pdev->dev, "request_irq failed\n");
//    goto err_irq;
//  }

//  sc->irq = pdev->irq;

//  ret = ath9k_init_device(id->device, sc, &ath_pci_bus_ops);
//  if (ret) {
//    dev_err(&pdev->dev, "Failed to initialize device\n");
//    goto err_init;
//  }

//  ath9k_hw_name(sc->sc_ah, hw_name, sizeof(hw_name));
//  wiphy_info(hw->wiphy, "%s mem=0x%lx, irq=%d\n",
//       hw_name, (unsigned long)sc->mem, pdev->irq);

//  return 0;

//err_init:
//  free_irq(sc->irq, sc);
//err_irq:
//  ieee80211_free_hw(hw);
//  return ret;
}

void ATH9K::Tx() { }
int ATH9K::Start() { return -1; }
void ATH9K::Stop() { }
int ATH9K::AddInterface() { return -1; }
int ATH9K::ChangeInterface() { return -1; }
void ATH9K::RemoveInterface() { }
int ATH9K::Configure() { return -1; }
void ATH9K::ConfigureFilter() { }
int ATH9K::STAState() { return -1; }
void ATH9K::STANotify() { }
void ATH9K::ConfigureTx() { }
void ATH9K::BSSInfoChanged() { }
int ATH9K::SetKey() { return -1; }
uint64_t ATH9K::GetTSF() { return 0; }
void ATH9K::SetTFS() { }
void ATH9K::ResetTSF() { }
int ATH9K::AMPDUAction() { return -1; }
int ATH9K::GetSurvey() { return -1; }
void ATH9K::SetCoverageClass() { }
void ATH9K::Flush() { }
bool ATH9K::TxFramesPending() { return true; }
int ATH9K::TxLastBeacon() { return -1; }
int ATH9K::GetStats() { return -1; }
int ATH9K::GetAntenna() { return -1; }
void ATH9K::ReleaseBufferedFrames() { }
void ATH9K::SWScanStart() { }
void ATH9K::SWScanComplete() { }
void ATH9K::WakeTxQueue() { }
