#ifndef __PCIE_HOST_PCD_H__
#define __PCIE_HOST_PCD_H__

/* platform specific variables */
/* 40 lans total, 4 lan per controller */
#define SG2044_PCIE_MAX_ROOT        (10)
#define PCIE_RANGES_MEM32_FLAG      (0x02000000)
#define PCIE_RANGES_PMEM32_FLAG     (0x42000000)
#define PCIE_RANGES_MEM64_FLAG      (0x03000000)
#define PCIE_RANGES_PMEM64_FLAG     (0x43000000)
#define PCIE_RANGES_IO_FLAG         (0x01000000)

#pragma pack(push, 1)
// typedef struct {

//   UINT64   Pmem32CpuAddr;
//   UINT64   Pmem32PciAddr;
//   UINT64   Pmem32Size;

//   UINT64   Mem32CpuAddr;
//   UINT64   Mem32PciAddr;
//   UINT64   Mem32Size;

//   UINT64   Pmem64CpuAddr;
//   UINT64   Pmem64PciAddr;
//   UINT64   Pmem64Size;

//   UINT64   Mem64CpuAddr;
//   UINT64   Mem64PciAddr;
//   UINT64   Mem64Size;

//   UINT64   IoCpuAddr;
//   UINT64   IoPciAddr;
//   UINT64   IoSize;

// }PCIE_RANGES;

typedef struct {
  UINT64   CpuAddr;
  UINT64   PciAddr;
  UINT64   RangeSize;
}PCIE_RANGES;

typedef struct {
  UINT64 DbiBase;
  UINT64 DbiSize;
  UINT64 CtrBase;
  UINT64 CtrSize;
  UINT64 AtuBase;
  UINT64 AtuSize;
  UINT64 CfgBase;
  UINT64 CfgSize;
}PCIE_REG;

typedef struct {
  BOOLEAN Mem32Support;
  BOOLEAN Pmem32Support;
  BOOLEAN Mem64Support;
  BOOLEAN Pmem64Support;
  BOOLEAN IoSupport;
}PCIE_SUPPORT_FLAG;

typedef struct {
  UINT64 RootBusBase;
  UINT64 RootBusLimit;
  UINT64 RootBusTranslation;
}PCIE_BUS_CONFIG;

typedef struct {
  UINT8  NumOfControllers;
  UINT8  PcieSupportFlag[SG2044_PCIE_MAX_ROOT][sizeof(PCIE_SUPPORT_FLAG)];
  UINT8  PcieDomain[SG2044_PCIE_MAX_ROOT][sizeof(UINT32)];
  UINT8  RootBusConfig[SG2044_PCIE_MAX_ROOT][sizeof(PCIE_BUS_CONFIG)];
  UINT8  PcieReg[SG2044_PCIE_MAX_ROOT][sizeof(PCIE_REG)];
  UINT8  PciePmem32Ranges[SG2044_PCIE_MAX_ROOT][sizeof(PCIE_RANGES)];
  UINT8  PcieMem32Ranges[SG2044_PCIE_MAX_ROOT][sizeof(PCIE_RANGES)];
  UINT8  PciePmem64Ranges[SG2044_PCIE_MAX_ROOT][sizeof(PCIE_RANGES)];
  UINT8  PcieMem64Ranges[SG2044_PCIE_MAX_ROOT][sizeof(PCIE_RANGES)];
  UINT8  PcieIoRanges[SG2044_PCIE_MAX_ROOT][sizeof(PCIE_RANGES)];
}PCIE_HOST_BRIDGE_TABLE;
#pragma pack(pop)

#endif
