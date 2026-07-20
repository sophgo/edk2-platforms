#ifndef __PCIE_HOST_PCD_H__
#define __PCIE_HOST_PCD_H__

/* platform specific variables */
/* 40 lans total, 4 lan per controller */
#define SG2044_PCIE_MAX_ROOT        (10)

/* One PHY/wrapper per controller pair: c2c0-w0/w1, c2c1-w0/w1, cxp-w0. */
#define SG2044_PCIE_MAX_WRAPPER     (5)

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
// Per-controller PHY/link bring-up parameters, used by the PEI bring-up
// module (PcieBringupPei) to train the link. C2cId/WrapperId/PhyId locate the
// controller in the SoC's C2C/wrapper/PHY hierarchy (from which the FSBL
// register-base formulas are derived); the rest describe this controller's
// link. The SerDes lane grouping (x8 vs x4+x4) is NOT here -- it is a
// per-wrapper property shared by the wrapper's two controllers, see PCIE_PHY.
//
typedef struct {
  UINT8   C2cId;       // C2C subsystem: 0, 1, or 4 (CXP)
  UINT8   WrapperId;   // wrapper within the subsystem: 0 or 1
  UINT8   PhyId;       // PHY within the wrapper: 0 or 1
  UINT8   GenSpeed;    // target link generation (e.g. 5)
  UINT8   LaneCount;   // lanes for this controller: 8 or 4 (must agree with
                       // the owning wrapper's SerdesMode)
  UINT16  PerstGpio;   // PERST GPIO pin (flat SoC pin number, per FSBL perst_gpio[])
}PCIE_CTRL_INIT;

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
  PCIE_CTRL_INIT     CtrlInit;       // PEI per-controller link bring-up params
}PCIE_CONTROLLER;

//
// Per-wrapper PHY configuration. A wrapper's two controllers either form one
// x8 link or split into two x4 links; SerdesMode is that mux setting and is
// the ONLY bring-up parameter shared between the two controllers. Programmed
// once per wrapper (see FSBL pcie_config_ss_mode, guarded by phy_id == 0).
//
typedef struct {
  UINT8   C2cId;       // C2C subsystem: 0, 1, or 4 (CXP)
  UINT8   WrapperId;   // wrapper within the subsystem: 0 or 1
  UINT8   SerdesMode;  // 0 = x8 (PCIE_SERDES_MODE_X8), 1 = x4+x4 (X4_X4)
}PCIE_PHY;

typedef struct {
  UINT8            NumOfControllers;
  PCIE_CONTROLLER  Controller[SG2044_PCIE_MAX_ROOT];
  UINT8            NumOfPhys;
  PCIE_PHY         Phy[SG2044_PCIE_MAX_WRAPPER];
}PCIE_HOST_BRIDGE_TABLE;
#pragma pack(pop)

#endif
