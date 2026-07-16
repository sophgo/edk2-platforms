#ifndef __PCIE_HOST_PCD_H__
#define __PCIE_HOST_PCD_H__

/* platform specific variables */
/* 40 lans total, 4 lan per controller */
#define SG2044_PCIE_MAX_ROOT        (10)

#pragma pack(push, 1)

//
// One outbound MMIO/IO window: CPU-side base, PCI-side base and size.
// PciAddr == CpuAddr means an identity-mapped window (Translation == 0).
//
typedef struct {
  UINT64   CpuAddr;
  UINT64   PciAddr;
  UINT64   RangeSize;
}PCIE_RANGES;

//
// DesignWare controller register apertures (DBI / control / iATU / config).
//
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

//
// Per-window enable flags. A window with its flag set to FALSE is reported to
// EDK2 as NO_MAPPING and skipped when programming the outbound iATU, so each
// window (including 64bit non-prefetchable) is purely software configurable
// from the PCD data below.
//
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

//
// All configuration for a single PCIe controller, grouped together so the DSC
// can assign it with named scalar fields (e.g. Controller[2].Reg.DbiBase) and
// the C consumers can read typed members directly without byte-wise CopyMem.
//
typedef struct {
  UINT32             Domain;         // PCIe domain / ACPI _SEG (was PcieDomain)
  PCIE_SUPPORT_FLAG  Flag;           // per-window enable flags
  PCIE_BUS_CONFIG    Bus;            // root bus base/limit/translation
  PCIE_REG           Reg;            // DesignWare register apertures
  PCIE_RANGES        Pmem32;         // 32bit prefetchable window
  PCIE_RANGES        Mem32;          // 32bit non-prefetchable window
  PCIE_RANGES        Pmem64;         // 64bit prefetchable window
  PCIE_RANGES        Mem64;          // 64bit non-prefetchable window
  PCIE_RANGES        Io;             // IO window
  UINT32             Space32Start;   // slave-map 32bit CPU region start
  UINT32             Space32End;     // slave-map 32bit CPU region end
  UINT64             Space64Start;   // slave-map 64bit CPU region start
  UINT64             Space64End;     // slave-map 64bit CPU region end
}PCIE_CONTROLLER;

typedef struct {
  UINT8            NumOfControllers;
  PCIE_CONTROLLER  Controller[SG2044_PCIE_MAX_ROOT];
}PCIE_HOST_BRIDGE_TABLE;
#pragma pack(pop)

#endif
