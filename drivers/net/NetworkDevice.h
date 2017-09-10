#pragma once

class PCIEntry;

class NetworkDevice
{
public:
  static NetworkDevice* Probe(PCIEntry& pciEntry);
};
