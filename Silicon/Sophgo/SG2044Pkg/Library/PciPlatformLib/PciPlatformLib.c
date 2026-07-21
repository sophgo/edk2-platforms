/** @file

  Copyright (c) 2024, SOPHGO INC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Include/PciPlatformLib.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/FdtClient.h>
#include <Protocol/Cpu.h>
#include <IndustryStandard/Pci22.h>
#include <Include/PcieHostPcd.h>

#define NO_MAPPING ((UINT64)~0ULL)

/* designware controller specific variables */
#define DW_PCIE_ATU_LOWER_BASE      0x0008
#define DW_PCIE_ATU_UPPER_BASE      0x000c
#define DW_PCIE_ATU_LOWER_TARGET    0x0014
#define DW_PCIE_ATU_UPPER_TARGET    0x0018
#define DW_PCIE_ATU_LOWER_LIMIT     0x0010
#define DW_PCIE_ATU_UPPER_LIMIT     0x0020
#define DW_PCIE_ATU_REGION_CTRL1    0x0000
#define DW_PCIE_ATU_REGION_CTRL2    0x0004

#define DW_PCIE_PORT_DEBUG1                     0x072c
#define DW_PCIE_PORT_DEBUG1_LINK_UP             BIT4
#define DW_PCIE_PORT_DEBUG1_LINK_IN_TRAINING    BIT29

#define DW_PCIE_ATU_INCREASE_REGION_SIZE        BIT13
#define DW_PCIE_ATU_ENABLE                      BIT31

#define DW_PCIE_ATU_TYPE_MEM                    0x0
#define DW_PCIE_ATU_TYPE_IO                     0x2
#define DW_PCIE_ATU_TYPE_CFG0                   0x4
#define DW_PCIE_ATU_TYPE_CFG1                   0x5

#define DW_PCIE_ATU_REG_SIZE                    0x200
#define DW_PCIE_ATU_OUTBOUND_REG_OFFSET         0
#define DW_PCIE_ATU_INBOUND_REG_OFFSET          0x100

#define DW_PCIE_MISC_CONTROL_1                  0x8bc
#define DW_PCIE_DBI_RO_WR_EN                    BIT0

//PCIE CTRL REG
#define PCIE_CTRL_REG_OFFSET                                0x0c00
#define PCIE_CTRL_SFT_RST_SIG_REG                           0x050
#define PCIE_CTRL_REMAPPING_EN_REG                          0x060
//
// REMAPPING_EN bit0 = hni_to_pcie_up4g_en, bit1 = hni_to_pcie_dw4g_en. SERVER
// (CCN) mode needs both set so the HNI slave map actually forwards CPU MMIO to
// PCIe. Mirrors FSBL pcie_config_slv_mapping() (val |= 0x3).
//
#define PCIE_CTRL_REMAP_EN_HNI_TO_PCIE_MASK                 0x3
#define PCIE_CTRL_HNI_UP_START_ADDR_REG                     0x064
#define PCIE_CTRL_HNI_UP_END_ADDR_REG                       0x068
#define PCIE_CTRL_HNI_DW_ADDR_REG                           0x06c
#define PCIE_CTRL_SN_UP_START_ADDR_REG                      0x070
#define PCIE_CTRL_SN_UP_END_ADDR_REG                        0x074
#define PCIE_CTRL_SN_DW_ADDR_REG                            0x078
#define PCIE_CTRL_AXI_MSI_GEN_CTRL_REG                      0x07c
#define PCIE_CTRL_AXI_MSI_GEN_LOWER_ADDR_REG                0x088
#define PCIE_CTRL_AXI_MSI_GEN_UPPER_ADDR_REG                0x08c
#define PCIE_CTRL_AXI_MSI_GEN_USER_DATA_REG                 0x090
#define PCIE_CTRL_AXI_MSI_GEN_MASK_IRQ_REG                  0x094
#define PCIE_CTRL_IRQ_EN_REG                                0x0a0


typedef struct {
  UINTN   DbiBase;
  UINTN   DbiSize;
  UINTN   CtrBase;
  UINTN   CtrSize;
  UINTN   AtuBase;
  UINTN   AtuSize;
  UINTN   CfgBase;
  UINTN   CfgSize;
} DW_PCIE;


typedef struct {
  UINT32   StartAddr32Bit;
  UINT32   EndAddr32Bit;
  UINT64   StartAddr64Bit;
  UINT64   EndAddr64Bit;
} SLAVE_MAP_ADDR_PCIE;

/* edk2 related */
#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

#define GET_SEGMENT(Address)    (((Address) >> 32) & 0xFFFF)
#define GET_BUS(Address)        (((Address) >> 20) & 0xFF)
#define GET_DEVICE(Address)     (((Address) >> 15) & 0x1F)
#define GET_FUNCTION(Address)   (((Address) >> 12) & 0x07)
#define GET_OFFSET(Address)     ((Address) & 0xFFF)

/* SG2044_PCIE_MAX_ROOT comes from Include/PcieHostPcd.h */

typedef struct {
  PCI_ROOT_BRIDGE                   PciRoot[SG2044_PCIE_MAX_ROOT];
  DW_PCIE                           DwPcie[SG2044_PCIE_MAX_ROOT];
  EFI_PCI_ROOT_BRIDGE_DEVICE_PATH   PciDevicePath[SG2044_PCIE_MAX_ROOT];
  SLAVE_MAP_ADDR_PCIE               SlaveMapAddrPcie[SG2044_PCIE_MAX_ROOT];
  UINTN                             Count;
} SG2044_PCIE_ROOT;

STATIC SG2044_PCIE_ROOT mSG2044PciRoot;

STATIC
VOID
DwPcieOutboundAtuWrite32 (
    IN  DW_PCIE *Pcie,
    IN  UINT32 Index,
    IN  UINT32 Offset,
    IN  UINT32 Value
    )
{
  MmioWrite32 (Pcie->AtuBase + DW_PCIE_ATU_OUTBOUND_REG_OFFSET + Index * DW_PCIE_ATU_REG_SIZE + Offset, Value);
}

STATIC
UINT32
DwPcieOutboundAtuRead32 (
    IN  DW_PCIE *Pcie,
    IN  UINT32 Index,
    IN  UINT32 Offset
    )
{
  return MmioRead32 (Pcie->AtuBase + DW_PCIE_ATU_OUTBOUND_REG_OFFSET + Index * DW_PCIE_ATU_REG_SIZE + Offset);
}

STATIC
VOID
DwPcieInboundAtuWrite32 (
    IN  DW_PCIE *Pcie,
    IN  UINT32 Index,
    IN  UINT32 Offset,
    IN  UINT32 Value
    )
{
  MmioWrite32 (Pcie->AtuBase + DW_PCIE_ATU_INBOUND_REG_OFFSET + Index * DW_PCIE_ATU_REG_SIZE + Offset, Value);
}

STATIC
UINT32
DwPcieInboundAtuRead32 (
    IN  DW_PCIE *Pcie,
    IN  UINT32 Index,
    IN  UINT32 Offset
    )
{
  return MmioRead32 (Pcie->AtuBase + DW_PCIE_ATU_INBOUND_REG_OFFSET + Index * DW_PCIE_ATU_REG_SIZE + Offset);
}

STATIC
VOID
DwPcieDbiWrite32 (
    IN  DW_PCIE *Pcie,
    IN  UINT32 Offset,
    IN  UINT32 Value
    )
{
  MmioWrite32 (Pcie->DbiBase + Offset, Value);
}

STATIC
VOID
DwPcieDbiWrite16 (
    IN  DW_PCIE *Pcie,
    IN  UINT32  Offset,
    IN  UINT16  Value
    )
{
  MmioWrite16 (Pcie->DbiBase + Offset, Value);
}

STATIC
VOID
DwPcieDbiWrite8 (
    IN  DW_PCIE *Pcie,
    IN  UINT32  Offset,
    IN  UINT8   Value
    )
{
  MmioWrite8 (Pcie->DbiBase + Offset, Value);
}

STATIC
UINT32
DwPcieDbiRead32 (
    IN  DW_PCIE *Pcie,
    IN  UINT32 Offset
    )
{
  return MmioRead32 (Pcie->DbiBase + Offset);
}

STATIC
VOID
DwPcieCtrWrite32 (
    IN  DW_PCIE *Pcie,
    IN  UINT32 Offset,
    IN  UINT32 Value
    )
{
  MmioWrite32 (Pcie->CtrBase + Offset, Value);
}

STATIC
UINT32
DwPcieCtrRead32 (
    IN  DW_PCIE *Pcie,
    IN  UINT32 Offset
    )
{
  return MmioRead32 (Pcie->CtrBase + Offset);
}

STATIC
UINT32
Lower32Bits (
    IN  UINT64 n
    )
{
  return (n & 0xffffffff);
}

STATIC
UINT32
Upper32Bits (
    IN  UINT64 n
    )
{
  return ((n >> 32) & 0xffffffff);
}

STATIC
BOOLEAN
DwPcieLinkUp (
    IN  DW_PCIE *Pcie
    )
{
	UINT32  Value;

	Value = DwPcieDbiRead32 (Pcie, DW_PCIE_PORT_DEBUG1);

	return ((Value & DW_PCIE_PORT_DEBUG1_LINK_UP) &&
      !(Value & DW_PCIE_PORT_DEBUG1_LINK_IN_TRAINING));
}

STATIC
UINT64
DwPcieAtuPciAddr (
    IN  UINT32 Bus,
    IN  UINT32 Dev,
    IN  UINT32 Func
    )
{
  return  ((Bus & (PCI_MAX_BUS)) << 24) |
    ((Dev & (PCI_MAX_DEVICE)) << 19) |
    ((Func & (PCI_MAX_FUNC)) << 16);
}

RETURN_STATUS
DwPcieSetAtuOutbound (
    IN  DW_PCIE *Pcie,
    IN  UINT32 Index,
    IN  UINT32 Type,
    IN  UINT64 CpuAddr,
    IN  UINT64 PciAddr,
    IN  UINT64 Size
    )
{
  UINT32 LoopCount = 0;
  UINT64 LimitAddr = 0;

  LimitAddr = CpuAddr + Size - 1;

  /* cpu domain address */
  DwPcieOutboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_LOWER_BASE,
      Lower32Bits (CpuAddr));
  DwPcieOutboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_UPPER_BASE,
      Upper32Bits (CpuAddr));

  /* pci domain address */
  DwPcieOutboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_LOWER_TARGET,
      Lower32Bits (PciAddr));
  DwPcieOutboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_UPPER_TARGET,
      Upper32Bits (PciAddr));

  /* cpu domain limit address */
  DwPcieOutboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_LOWER_LIMIT,
      Lower32Bits (LimitAddr));
  DwPcieOutboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_UPPER_LIMIT,
      Upper32Bits (LimitAddr));

  DwPcieOutboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_REGION_CTRL1,
      Type | DW_PCIE_ATU_INCREASE_REGION_SIZE);

  DwPcieOutboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_REGION_CTRL2,
      DW_PCIE_ATU_ENABLE);

  DEBUG ((DEBUG_VERBOSE, "ATU%d: [CPU: %016lx - %016lx] --> [PCI: %016lx - %016lx]\n",
        Index, CpuAddr, LimitAddr, PciAddr, PciAddr + Size - 1));

  /*
   * Make sure ATU enable takes effect before any subsequent config
   * and I/O accesses.
   */
  for (LoopCount = 0; LoopCount < 10000; ++LoopCount) {
    if (DwPcieOutboundAtuRead32 (Pcie, Index, DW_PCIE_ATU_REGION_CTRL2)
        & DW_PCIE_ATU_ENABLE) {
      return EFI_SUCCESS;
    }
  }

  DEBUG ((DEBUG_ERROR, "Outbound iATU is not being enabled\n"));

  return EFI_DEVICE_ERROR;
}

RETURN_STATUS
DwPcieSetAtuInbound (
    IN  DW_PCIE *Pcie,
    IN  UINT32 Index,
    IN  UINT32 Type,
    IN  UINT64 CpuAddr,
    IN  UINT64 PciAddr,
    IN  UINT64 Size
    )
{
  UINT32 LoopCount = 0;
  UINT64 LimitAddr = 0;

  LimitAddr = PciAddr + Size - 1;

  /* pci domain address */
  DwPcieInboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_LOWER_BASE,
      Lower32Bits (PciAddr));
  DwPcieInboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_UPPER_BASE,
      Upper32Bits (PciAddr));

  /* cpu domain address */
  DwPcieInboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_LOWER_TARGET,
      Lower32Bits (CpuAddr));
  DwPcieInboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_UPPER_TARGET,
      Upper32Bits (CpuAddr));

  /* pci domain limit address */
  DwPcieInboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_LOWER_LIMIT,
      Lower32Bits (LimitAddr));
  DwPcieInboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_UPPER_LIMIT,
      Upper32Bits (LimitAddr));

  DwPcieInboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_REGION_CTRL1,
      Type | DW_PCIE_ATU_INCREASE_REGION_SIZE);

  DwPcieInboundAtuWrite32 (Pcie, Index, DW_PCIE_ATU_REGION_CTRL2,
      DW_PCIE_ATU_ENABLE);

  DEBUG ((DEBUG_VERBOSE, "ATU%d: [CPU: %016lx - %016lx] <-- [PCI: %016lx - %016lx]\n",
        Index, CpuAddr, CpuAddr + Size - 1, PciAddr, LimitAddr));

  /*
   * Make sure ATU enable takes effect before any subsequent config
   * and I/O accesses.
   */
  for (LoopCount = 0; LoopCount < 10000; ++LoopCount) {
    if (DwPcieInboundAtuRead32 (Pcie, Index, DW_PCIE_ATU_REGION_CTRL2)
        & DW_PCIE_ATU_ENABLE) {
      return EFI_SUCCESS;
    }
  }

  DEBUG ((DEBUG_ERROR, "Inbound iATU is not being enabled\n"));

  return EFI_DEVICE_ERROR;
}

VOID
DwPcieEnableMaster (
    IN  PCI_ROOT_BRIDGE   *PciRoot,
    IN  DW_PCIE           *Pcie
    )
{
  UINT32  Value;

  /* enable write permission for read-only registers */
  Value = DwPcieDbiRead32(Pcie, DW_PCIE_MISC_CONTROL_1);
  Value |= DW_PCIE_DBI_RO_WR_EN;
  DwPcieDbiWrite32(Pcie, DW_PCIE_MISC_CONTROL_1, Value);

  /* init bus numbers, edk2 enumerate should set these parameters to reasonable value */
  DwPcieDbiWrite8(Pcie, PCI_BRIDGE_PRIMARY_BUS_REGISTER_OFFSET, 0);
  DwPcieDbiWrite8(Pcie, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET, 0);
  DwPcieDbiWrite8(Pcie, PCI_BRIDGE_SUBORDINATE_BUS_REGISTER_OFFSET, 0);
  DwPcieDbiWrite8(Pcie, PCI_BRIDGE_SECONDARY_LATENCY_TIMER_OFFSET, 0);

  /* reversion */
  DwPcieDbiWrite8(Pcie, PCI_REVISION_ID_OFFSET, 0);

  /* class code */
  /* program interface, bridge program interface always 0 */
  DwPcieDbiWrite8(Pcie, PCI_CLASSCODE_OFFSET, PCI_IF_BRIDGE_P2P);
  /* sub class code */
  DwPcieDbiWrite8(Pcie, PCI_CLASSCODE_OFFSET + 1, PCI_CLASS_BRIDGE_P2P);
  /* base class code */
  DwPcieDbiWrite8(Pcie, PCI_CLASSCODE_OFFSET + 2, PCI_CLASS_BRIDGE);

  /* edk2 enumerate will enable corresponding bits */
  DwPcieDbiWrite16 (Pcie, PCI_COMMAND_OFFSET, 0);

  /* disable write permission for read-only registers */
  Value = DwPcieDbiRead32(Pcie, DW_PCIE_MISC_CONTROL_1);
  Value &= ~DW_PCIE_DBI_RO_WR_EN;
  DwPcieDbiWrite32(Pcie, DW_PCIE_MISC_CONTROL_1, Value);
}

VOID
DwPcieSetSlaveMap (
    IN  DW_PCIE           *Pcie,
    IN  SLAVE_MAP_ADDR_PCIE *SlaveMapAddrPcie
    )
{
  UINT32  Value;

  //64 bit start address
  DwPcieCtrWrite32 (Pcie, PCIE_CTRL_REG_OFFSET + PCIE_CTRL_HNI_UP_START_ADDR_REG, (UINT32)((SlaveMapAddrPcie->StartAddr64Bit >> 16) & 0xFFFFFFFF));
  DwPcieCtrWrite32 (Pcie, PCIE_CTRL_REG_OFFSET + PCIE_CTRL_HNI_UP_END_ADDR_REG, (UINT32)((SlaveMapAddrPcie->EndAddr64Bit >> 16) & 0xFFFFFFFF));

  //32 bit end address
  DwPcieCtrWrite32 (Pcie, PCIE_CTRL_REG_OFFSET + PCIE_CTRL_HNI_DW_ADDR_REG, (UINT32)((((SlaveMapAddrPcie->EndAddr32Bit >> 16) & 0xFFFF) << 16)
        | ((SlaveMapAddrPcie->StartAddr32Bit >> 16) & 0xFFFF)));

  //
  // Enable HNI-to-PCIe up-4G and dw-4G address remapping (bit0|bit1). FSBL's
  // pcie_config_slv_mapping() used to set this in SERVER mode; now that RC
  // bring-up has moved out of FSBL, the RC slave map is only live once EDK2
  // sets these enable bits. Without them the address window above is programmed
  // but inert, so device BAR MMIO reads back all-ones. RMW to preserve the
  // other remap-enable bits.
  //
  Value  = DwPcieCtrRead32 (Pcie, PCIE_CTRL_REG_OFFSET + PCIE_CTRL_REMAPPING_EN_REG);
  Value |= PCIE_CTRL_REMAP_EN_HNI_TO_PCIE_MASK;
  DwPcieCtrWrite32 (Pcie, PCIE_CTRL_REG_OFFSET + PCIE_CTRL_REMAPPING_EN_REG, Value);

  DEBUG ((DEBUG_INFO, "Set Rc Ctr Reg [0x%lx], Slave Map: 64bit [0x%lx - 0x%lx], 32bit [0x%lx - 0x%lx]\n",
        Pcie->CtrBase + PCIE_CTRL_REG_OFFSET,
        SlaveMapAddrPcie->StartAddr64Bit, SlaveMapAddrPcie->EndAddr64Bit,
        SlaveMapAddrPcie->StartAddr32Bit, SlaveMapAddrPcie->EndAddr32Bit));

}

/**
  Assert that an enabled window's CPU-side range lies within one of the
  controller's two slave-map regions.

  On SG2044 each controller has exactly two CPU-side slave-map regions: one
  32bit region [Space32Start, Space32End] and one 64bit region [Space64Start,
  Space64End] (see DwPcieSetSlaveMap). The RC only forwards CPU addresses that
  fall inside one of these two regions, so every enabled outbound window's CPU
  range must be contained in one of them. Which region a given window uses is
  not fixed by its "32/64bit" name and varies per board (e.g. on the -LB
  variants controller 0's config and Io sit in the 32bit region, elsewhere Io
  sits in the 64bit region), so accept either. A window contained in neither is
  always a misconfigured PCD (e.g. a mistyped Space or range bound); catch it
  early on DEBUG builds. The whole check compiles out of RELEASE.

  @param  Name         Window name for the diagnostic.
  @param  CpuAddr      Window CPU-side base.
  @param  Size         Window size (0 / disabled windows are skipped by callers).
  @param  Space32Start 32bit slave-map region start.
  @param  Space32End   32bit slave-map region end.
  @param  Space64Start 64bit slave-map region start.
  @param  Space64End   64bit slave-map region end.
**/
STATIC
VOID
AssertWindowInSlaveMap (
    IN  CONST CHAR8  *Name,
    IN  UINT64       CpuAddr,
    IN  UINT64       Size,
    IN  UINT64       Space32Start,
    IN  UINT64       Space32End,
    IN  UINT64       Space64Start,
    IN  UINT64       Space64End
    )
{
  UINT64   CpuLimit;
  BOOLEAN  In32;
  BOOLEAN  In64;

  if (Size == 0) {
    return;
  }

  CpuLimit = CpuAddr + Size - 1;
  In32 = (CpuAddr >= Space32Start) && (CpuLimit <= Space32End);
  In64 = (CpuAddr >= Space64Start) && (CpuLimit <= Space64End);

  if (!In32 && !In64) {
    DEBUG ((DEBUG_ERROR,
        "PCIe %a window CPU range [%016lx - %016lx] outside both slave-map "
        "regions 32bit [%016lx - %016lx] / 64bit [%016lx - %016lx]\n",
        Name, CpuAddr, CpuLimit,
        Space32Start, Space32End, Space64Start, Space64End));
    ASSERT (FALSE);
  }
}

UINT32
InitPlatformFromPcd (
    OUT   SG2044_PCIE_ROOT *SG2044PciRoot
    )
{
  UINT32                            Index;
  PCIE_HOST_BRIDGE_TABLE            *PcieRcConfig;
  PCI_ROOT_BRIDGE                   *PciRoot;
  DW_PCIE                           *DwPcie;
  SLAVE_MAP_ADDR_PCIE               *SlaveMapAddrPcie;
  PCIE_CONTROLLER                   *Ctrl;

  SetMem (SG2044PciRoot, sizeof (SG2044_PCIE_ROOT), 0);

  PcieRcConfig = (PCIE_HOST_BRIDGE_TABLE *)PcdGetPtr (PcdPcieHostBridgeTable);

  if (PcieRcConfig->NumOfControllers > SG2044_PCIE_MAX_ROOT) {
    DEBUG ((DEBUG_ERROR, "Too many PCIe controllers, only %d supported\n", SG2044_PCIE_MAX_ROOT));
    return 0;
  }

  for (Index = 0; Index < PcieRcConfig->NumOfControllers; Index++) {
    PciRoot = &SG2044PciRoot->PciRoot[Index];
    DwPcie = &SG2044PciRoot->DwPcie[Index];
    SlaveMapAddrPcie = &SG2044PciRoot->SlaveMapAddrPcie[Index];
    Ctrl = &PcieRcConfig->Controller[Index];

    SlaveMapAddrPcie->StartAddr32Bit = Ctrl->Space32Start;
    SlaveMapAddrPcie->EndAddr32Bit   = Ctrl->Space32End;
    SlaveMapAddrPcie->StartAddr64Bit = Ctrl->Space64Start;
    SlaveMapAddrPcie->EndAddr64Bit   = Ctrl->Space64End;

    //Init DwPcie parameters
    DwPcie->DbiBase = Ctrl->Reg.DbiBase;
    DwPcie->DbiSize = Ctrl->Reg.DbiSize;
    DwPcie->CtrBase = Ctrl->Reg.CtrBase;
    DwPcie->CtrSize = Ctrl->Reg.CtrSize;
    DwPcie->AtuBase = Ctrl->Reg.AtuBase;
    DwPcie->AtuSize = Ctrl->Reg.AtuSize;
    DwPcie->CfgBase = Ctrl->Reg.CfgBase;
    DwPcie->CfgSize = Ctrl->Reg.CfgSize;

    //Init PciRoot parameters
    PciRoot->Supports                  = 0;
    PciRoot->Attributes                = 0;
    PciRoot->DmaAbove4G                = TRUE;
    PciRoot->NoExtendedConfigSpace     = FALSE;
    PciRoot->ResourceAssigned          = FALSE;
    PciRoot->AllocationAttributes      = EFI_PCI_HOST_BRIDGE_MEM64_DECODE;
    //
    // Report the real PCIe domain number (as consumed by the OS via ACPI
    // MCFG/DSDT _SEG) rather than the controller loop index, so the UEFI PCI
    // segment matches what Linux enumerates. The domain comes from the same
    // Domain field that AcpiPlatformDxe uses to build the ACPI tables.
    // Index remains the packed controller index used for all the DwPcie[] and
    // range arrays; only the reported Segment carries the real domain.
    //
    PciRoot->Segment                   = Ctrl->Domain;

    //slave mapping
    PciRoot->Bus.Base                  = Ctrl->Bus.RootBusBase;
    PciRoot->Bus.Limit                 = Ctrl->Bus.RootBusLimit;
    PciRoot->Bus.Translation           = Ctrl->Bus.RootBusTranslation;
    if (Ctrl->Flag.Pmem32Support) {
      PciRoot->PMem.Base                 = Ctrl->Pmem32.PciAddr;
      PciRoot->PMem.Limit                = Ctrl->Pmem32.PciAddr + Ctrl->Pmem32.RangeSize - 1;
      PciRoot->PMem.Translation          = Ctrl->Pmem32.PciAddr - Ctrl->Pmem32.CpuAddr;
      AssertWindowInSlaveMap ("PMem32", Ctrl->Pmem32.CpuAddr, Ctrl->Pmem32.RangeSize,
          Ctrl->Space32Start, Ctrl->Space32End, Ctrl->Space64Start, Ctrl->Space64End);
      DEBUG ((DEBUG_VERBOSE,
          "Pmem32PciRange                        [%016lx - %016lx]\n"
          "Pmem32CpuRange                        [%016lx - %016lx]\n",
          Ctrl->Pmem32.PciAddr, Ctrl->Pmem32.PciAddr + Ctrl->Pmem32.RangeSize - 1,
          Ctrl->Pmem32.CpuAddr, Ctrl->Pmem32.CpuAddr + Ctrl->Pmem32.RangeSize - 1));
    } else {
      PciRoot->PMem.Base                 = NO_MAPPING;
      PciRoot->PMem.Limit                = 0;
    }
    if (Ctrl->Flag.Mem32Support) {
      PciRoot->Mem.Base                  = Ctrl->Mem32.PciAddr;
      PciRoot->Mem.Limit                 = Ctrl->Mem32.PciAddr + Ctrl->Mem32.RangeSize - 1;
      PciRoot->Mem.Translation           = Ctrl->Mem32.PciAddr - Ctrl->Mem32.CpuAddr;
      AssertWindowInSlaveMap ("Mem32", Ctrl->Mem32.CpuAddr, Ctrl->Mem32.RangeSize,
          Ctrl->Space32Start, Ctrl->Space32End, Ctrl->Space64Start, Ctrl->Space64End);
      DEBUG ((DEBUG_VERBOSE,
          "Mem32PciRange                         [%016lx - %016lx]\n"
          "Mem32CpuRange                         [%016lx - %016lx]\n",
          Ctrl->Mem32.PciAddr, Ctrl->Mem32.PciAddr + Ctrl->Mem32.RangeSize - 1,
          Ctrl->Mem32.CpuAddr, Ctrl->Mem32.CpuAddr + Ctrl->Mem32.RangeSize - 1));
    } else {
      PciRoot->Mem.Base                  = NO_MAPPING;
      PciRoot->Mem.Limit                 = 0;
    }
    if (Ctrl->Flag.Pmem64Support) {
      PciRoot->PMemAbove4G.Base          = Ctrl->Pmem64.PciAddr;
      PciRoot->PMemAbove4G.Limit         = Ctrl->Pmem64.PciAddr + Ctrl->Pmem64.RangeSize - 1;
      PciRoot->PMemAbove4G.Translation   = Ctrl->Pmem64.PciAddr - Ctrl->Pmem64.CpuAddr;
      AssertWindowInSlaveMap ("PMem64", Ctrl->Pmem64.CpuAddr, Ctrl->Pmem64.RangeSize,
          Ctrl->Space32Start, Ctrl->Space32End, Ctrl->Space64Start, Ctrl->Space64End);
      DEBUG ((DEBUG_VERBOSE,
          "Pmem64PciRange                         [%016lx - %016lx]\n"
          "Pmem64CpuRange                         [%016lx - %016lx]\n",
          Ctrl->Pmem64.PciAddr, Ctrl->Pmem64.PciAddr + Ctrl->Pmem64.RangeSize - 1,
          Ctrl->Pmem64.CpuAddr, Ctrl->Pmem64.CpuAddr + Ctrl->Pmem64.RangeSize - 1));
    } else {
      PciRoot->PMemAbove4G.Base          = NO_MAPPING;
      PciRoot->PMemAbove4G.Limit         = 0;
    }
    if (Ctrl->Flag.Mem64Support) {
      PciRoot->MemAbove4G.Base           = Ctrl->Mem64.PciAddr;
      PciRoot->MemAbove4G.Limit          = Ctrl->Mem64.PciAddr + Ctrl->Mem64.RangeSize - 1;
      PciRoot->MemAbove4G.Translation    = Ctrl->Mem64.PciAddr - Ctrl->Mem64.CpuAddr;
      AssertWindowInSlaveMap ("Mem64", Ctrl->Mem64.CpuAddr, Ctrl->Mem64.RangeSize,
          Ctrl->Space32Start, Ctrl->Space32End, Ctrl->Space64Start, Ctrl->Space64End);
      DEBUG ((DEBUG_VERBOSE,
          "Mem64PciRange                         [%016lx - %016lx]\n"
          "Mem64CpuRange                         [%016lx - %016lx]\n",
          Ctrl->Mem64.PciAddr, Ctrl->Mem64.PciAddr + Ctrl->Mem64.RangeSize - 1,
          Ctrl->Mem64.CpuAddr, Ctrl->Mem64.CpuAddr + Ctrl->Mem64.RangeSize - 1));
    } else {
      PciRoot->MemAbove4G.Base           = NO_MAPPING;
      PciRoot->MemAbove4G.Limit          = 0;
    }
    if (Ctrl->Flag.IoSupport) {
      PciRoot->Io.Base                   = Ctrl->Io.PciAddr;
      PciRoot->Io.Limit                  = Ctrl->Io.PciAddr + Ctrl->Io.RangeSize - 1;
      PciRoot->Io.Translation            = Ctrl->Io.PciAddr - Ctrl->Io.CpuAddr;
      AssertWindowInSlaveMap ("Io", Ctrl->Io.CpuAddr, Ctrl->Io.RangeSize,
          Ctrl->Space32Start, Ctrl->Space32End, Ctrl->Space64Start, Ctrl->Space64End);
      DEBUG ((DEBUG_VERBOSE,
          "IoPciRange                         [%016lx - %016lx]\n"
          "IoCpuRange                         [%016lx - %016lx]\n",
          Ctrl->Io.PciAddr, Ctrl->Io.PciAddr + Ctrl->Io.RangeSize - 1,
          Ctrl->Io.CpuAddr, Ctrl->Io.CpuAddr + Ctrl->Io.RangeSize - 1));
    } else {
      PciRoot->Io.Base                   = NO_MAPPING;
      PciRoot->Io.Limit                  = 0;
    }
  }
  mSG2044PciRoot.Count = PcieRcConfig->NumOfControllers;
  return mSG2044PciRoot.Count;
}


/**
  Program one outbound iATU window, or skip it if the window is disabled.

  A disabled window is reported by InitPlatformFromPcd as Base == NO_MAPPING
  (see the per-window Flag handling there). Programming an iATU for such a
  window would compute a bogus CpuAddr/Size, so simply skip it. This keeps the
  set of enabled windows (including 64bit non-prefetchable) purely driven by the
  per-controller PCD flags.

  @param  DwPcie   The DesignWare controller.
  @param  Index    The outbound iATU region index (fixed per window role).
  @param  Type     The iATU type (DW_PCIE_ATU_TYPE_MEM / _IO).
  @param  Window   The PCI_ROOT_BRIDGE aperture describing this window.
**/
STATIC
VOID
SetupOutboundWindow (
    IN  DW_PCIE               *DwPcie,
    IN  UINT32                Index,
    IN  UINT32                Type,
    IN  PCI_ROOT_BRIDGE_APERTURE *Window
    )
{
  if (Window->Base == NO_MAPPING) {
    DEBUG ((DEBUG_VERBOSE, "ATU%d: window disabled, skipped\n", Index));
    return;
  }

  DwPcieSetAtuOutbound (
      DwPcie,
      Index,
      Type,
      Window->Base - Window->Translation,
      Window->Base,
      Window->Limit + 1 - Window->Base
      );
}

VOID
SetupPciRoot (
    IN  PCI_ROOT_BRIDGE   *PciRoot,
    IN  DW_PCIE           *DwPcie,
    IN  UINT64            SystemMemoryStart,
    IN  UINT64            SystemMemorySize
    )
{
  SetupOutboundWindow (DwPcie, 1, DW_PCIE_ATU_TYPE_IO,  &PciRoot->Io);
  SetupOutboundWindow (DwPcie, 2, DW_PCIE_ATU_TYPE_MEM, &PciRoot->PMem);
  SetupOutboundWindow (DwPcie, 3, DW_PCIE_ATU_TYPE_MEM, &PciRoot->Mem);
  SetupOutboundWindow (DwPcie, 4, DW_PCIE_ATU_TYPE_MEM, &PciRoot->PMemAbove4G);
  SetupOutboundWindow (DwPcie, 5, DW_PCIE_ATU_TYPE_MEM, &PciRoot->MemAbove4G);

  DwPcieSetAtuInbound (
      DwPcie,
      0,
      DW_PCIE_ATU_TYPE_MEM,
      SystemMemoryStart,
      SystemMemoryStart,
      SystemMemorySize
      );

  DwPcieEnableMaster (PciRoot, DwPcie);
}

STATIC
EFI_STATUS
SetPciMemoryAttribute (
    IN  PCI_ROOT_BRIDGE   *PciRoot,
    IN  DW_PCIE           *DwPcie
    )
{
  EFI_CPU_ARCH_PROTOCOL *Cpu;
  EFI_STATUS            Status;

  Status = gBS->LocateProtocol (
      &gEfiCpuArchProtocolGuid,
      NULL,
      (VOID **)&Cpu
      );

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot locate CPU arch service\n"));
  }

  Status = Cpu->SetMemoryAttributes (
      Cpu,
      DwPcie->DbiBase,
      DwPcie->DbiSize,
      EFI_MEMORY_UC
      );

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot add designware PCIe DBI space %016lx - %016lx\n",
          DwPcie->DbiBase, DwPcie->DbiBase + DwPcie->DbiSize));
    return EFI_INVALID_PARAMETER;
  }

  Status = Cpu->SetMemoryAttributes (
      Cpu,
      DwPcie->CtrBase,
      DwPcie->CtrSize,
      EFI_MEMORY_UC
      );

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot add designware PCIe CTR space %016lx - %016lx\n",
          DwPcie->CtrBase, DwPcie->CtrBase + DwPcie->CtrSize));
    return EFI_INVALID_PARAMETER;
  }

  Status = Cpu->SetMemoryAttributes (
      Cpu,
      DwPcie->AtuBase,
      DwPcie->AtuSize,
      EFI_MEMORY_UC
      );

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot add designware PCIe ATU space %016lx - %016lx\n",
          DwPcie->AtuBase, DwPcie->AtuBase + DwPcie->AtuSize));
    return EFI_INVALID_PARAMETER;
  }

  Status = Cpu->SetMemoryAttributes (
      Cpu,
      DwPcie->CfgBase,
      DwPcie->CfgSize,
      EFI_MEMORY_UC
      );

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot add designware PCIe CFG space %016lx - %016lx\n",
          DwPcie->CfgBase, DwPcie->CfgBase + DwPcie->CfgSize));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

RETURN_STATUS
EFIAPI
PciPlatformInit (
    VOID
    )
{
  UINT32                            PciRootIter;
  UINT32                            PciRootCount;
  UINT64                            SystemMemoryStart;
  UINT64                            SystemMemoryEnd;
  UINT64                            SystemMemorySize;
  UINTN                             NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR   *MemorySpaceMap;
  EFI_STATUS                        Status;

  DEBUG ((DEBUG_INFO, "SG2044 PCIe Init\n"));

  PciRootCount = InitPlatformFromPcd (&mSG2044PciRoot);

  /* get the start of system memory and the end of system memory */
  SystemMemoryStart = MAX_ADDRESS;
  SystemMemoryEnd = 0;

  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot get system memory map\n"));
    return EFI_INVALID_PARAMETER;
  }

  for (UINT32 Index = 0; Index < NumberOfDescriptors; ++Index) {
    DEBUG ((DEBUG_VERBOSE, "%d: [%016lx - %016lx] Cap: %016lx, Attr: %016lx, Type: %d\n",
          Index,
          MemorySpaceMap[Index].BaseAddress,
          MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length,
          MemorySpaceMap[Index].Capabilities,
          MemorySpaceMap[Index].Attributes,
          MemorySpaceMap[Index].GcdMemoryType));

    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeSystemMemory) {
      if (MemorySpaceMap[Index].BaseAddress < SystemMemoryStart)
        SystemMemoryStart = MemorySpaceMap[Index].BaseAddress;

      if (MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length > SystemMemoryEnd)
        SystemMemoryEnd = MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length;
    }
  }

  ASSERT (SystemMemoryEnd > SystemMemoryStart);

  SystemMemorySize = SystemMemoryEnd - SystemMemoryStart;

  DEBUG ((DEBUG_VERBOSE, "System Memory: [%016lx - %016lx]\n", SystemMemoryStart, SystemMemoryEnd));

  for (PciRootIter = 0; PciRootIter < PciRootCount; ++PciRootIter) {

    SetPciMemoryAttribute (&mSG2044PciRoot.PciRoot[PciRootIter], &mSG2044PciRoot.DwPcie[PciRootIter]);

    //pcie slave map (from fsbl)
    DwPcieSetSlaveMap (&mSG2044PciRoot.DwPcie[PciRootIter], &mSG2044PciRoot.SlaveMapAddrPcie[PciRootIter]);

    SetupPciRoot (&mSG2044PciRoot.PciRoot[PciRootIter], &mSG2044PciRoot.DwPcie[PciRootIter],
        SystemMemoryStart, SystemMemorySize);

    mSG2044PciRoot.PciRoot[PciRootIter].DevicePath               =
      (EFI_DEVICE_PATH_PROTOCOL *)&mSG2044PciRoot.PciDevicePath[PciRootIter];

    STATIC CONST EFI_PCI_ROOT_BRIDGE_DEVICE_PATH EfiPciRootBridgeDevicePathTemplate = {
      {
        {
          ACPI_DEVICE_PATH,
          ACPI_DP,
          {
            (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)),
            (UINT8)((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)
          }
        },
        EISA_PNP_ID (0x0A08), // PCIe
        0,
      }, {
        END_DEVICE_PATH_TYPE,
        END_ENTIRE_DEVICE_PATH_SUBTYPE,
        {
          END_DEVICE_PATH_LENGTH,
          0
        }
      }
    };

    CopyMem (mSG2044PciRoot.PciRoot[PciRootIter].DevicePath, &EfiPciRootBridgeDevicePathTemplate,
        sizeof (EFI_PCI_ROOT_BRIDGE_DEVICE_PATH));

    //
    // Match the ACPI device-path UID to the PCIe domain (== DSDT _UID/_SEG),
    // not the controller loop index, so the device path lines up with the OS
    // view. PciRoot[].Segment already holds the domain (see InitPlatformFromPcd).
    //
    mSG2044PciRoot.PciDevicePath[PciRootIter].AcpiDevicePath.UID =
      mSG2044PciRoot.PciRoot[PciRootIter].Segment;
  }

  return EFI_SUCCESS;
}

PCI_ROOT_BRIDGE *
EFIAPI
PciPlatformGetRoot (
    OUT UINTN  *Count
    )
{
  *Count = mSG2044PciRoot.Count;
  return mSG2044PciRoot.PciRoot;
}

/**
  Translate a PCIe segment (domain) number encoded in a config-space address
  into the packed controller index used by the DwPcie[]/PciRoot[] arrays.

  PciRoot[].Segment carries the real domain (which may be sparse, e.g. 0,2,4,6,8),
  while the internal arrays are packed by controller order (0..Count-1). This
  keeps config-space access working after the reported segment was aligned to
  the OS/ACPI domain numbering.

  @param  Segment   The PCIe domain number from the config-space address.

  @return The controller index, or MAX_UINT32 if no controller owns the domain.
**/
STATIC
UINT32
PciSegmentToControllerIndex (
  IN  UINT32  Segment
  )
{
  UINT32  Index;

  for (Index = 0; Index < mSG2044PciRoot.Count; Index++) {
    if (mSG2044PciRoot.PciRoot[Index].Segment == Segment) {
      return Index;
    }
  }

  return MAX_UINT32;
}

UINT32
EFIAPI
PciSegmentRead (
  IN UINT64                         Address,
  IN UINT32                         Width
  )
{
  UINT32    Segment;
  UINT32    Index;
  UINT32    Bus;
  UINT32    Device;
  UINT32    Function;
  UINT32    Offset;

  UINT32    Value;
  DW_PCIE   *DwPcie;
  UINT64    PciAddr;
  UINT32    Type;
  UINTN     CfgBase;

  Segment = GET_SEGMENT (Address);
  Bus = GET_BUS (Address);
  Device = GET_DEVICE (Address);
  Function = GET_FUNCTION (Address);
  Offset = GET_OFFSET (Address);

  Index = PciSegmentToControllerIndex (Segment);
  if (Index == MAX_UINT32) {
    DEBUG ((DEBUG_ERROR, "Invalid PCIe segment %d\n", Segment));
    return 0xffffffff;
  }

  /* Find PCIe controller */
  DwPcie = &mSG2044PciRoot.DwPcie[Index];

  if (Bus == mSG2044PciRoot.PciRoot[Index].Bus.Base) {
    /* host root complex */
    if (Device != 0)
      return 0xffffffff;

    CfgBase = DwPcie->DbiBase;
  } else {
    /* devices other than root complex, including pcie switches */
    if (!DwPcieLinkUp (DwPcie))
      return 0xffffffff;

    PciAddr = DwPcieAtuPciAddr (Bus, Device, Function);

    /* devices direct linked with root complex */
    if (Bus == mSG2044PciRoot.PciRoot[Index].Bus.Base + 1) {
      if (Device != 0)
        return 0xffffffff;

      Type = DW_PCIE_ATU_TYPE_CFG0;
    } else {
      Type = DW_PCIE_ATU_TYPE_CFG1;
    }

    DwPcieSetAtuOutbound (DwPcie, 0, Type, DwPcie->CfgBase, PciAddr, DwPcie->CfgSize);

    CfgBase = DwPcie->CfgBase;
  }

  switch (Width) {
    case 8:
      Value = MmioRead8 (CfgBase + Offset);
      break;
    case 16:
      Value = MmioRead16 (CfgBase + Offset);
      break;
    case 32:
      Value = MmioRead32 (CfgBase + Offset);
      break;
    default:
      DEBUG ((DEBUG_ERROR, "Not supported width for reading\n"));
      Value = 0xffffffff;
      break;
  }

  DEBUG ((DEBUG_VERBOSE, "R%d: %04x:%02x:%02x.%1x - %04x 0x%08x\n",
      Width, Segment, Bus, Device, Function, Offset, Value));

  return Value;
}

UINT32
EFIAPI
PciSegmentWrite (
  IN UINT64                         Address,
  IN UINT32                         Value,
  IN UINT32                         Width
  )
{
  UINT32    Segment;
  UINT32    Index;
  UINT32    Bus;
  UINT32    Device;
  UINT32    Function;
  UINT32    Offset;

  DW_PCIE   *DwPcie;
  UINT64    PciAddr;
  UINT32    Type;
  UINTN     CfgBase;

  Segment = GET_SEGMENT (Address);
  Bus = GET_BUS (Address);
  Device = GET_DEVICE (Address);
  Function = GET_FUNCTION (Address);
  Offset = GET_OFFSET (Address);

  Index = PciSegmentToControllerIndex (Segment);
  if (Index == MAX_UINT32) {
    DEBUG ((DEBUG_ERROR, "Invalid PCIe segment %d\n", Segment));
    return Value;
  }

  /* Find PCIe controller */
  DwPcie = &mSG2044PciRoot.DwPcie[Index];

  if (Bus == mSG2044PciRoot.PciRoot[Index].Bus.Base) {
    /* host root complex */
    CfgBase = DwPcie->DbiBase;
  } else {
    /* devices other than root complex, including pcie switches */
    if (!DwPcieLinkUp (DwPcie))
      return Value;

    PciAddr = DwPcieAtuPciAddr (Bus, Device, Function);

    if (Bus == mSG2044PciRoot.PciRoot[Index].Bus.Base + 1)
      Type = DW_PCIE_ATU_TYPE_CFG0;
    else
      Type = DW_PCIE_ATU_TYPE_CFG1;

    DwPcieSetAtuOutbound (DwPcie, 0, Type, DwPcie->CfgBase, PciAddr, DwPcie->CfgSize);

    CfgBase = DwPcie->CfgBase;
  }

  switch (Width) {
    case 8:
      MmioWrite8 (CfgBase + Offset, Value);
      break;
    case 16:
      MmioWrite16 (CfgBase + Offset, Value);
      break;
    case 32:
      MmioWrite32 (CfgBase + Offset, Value);
      break;
    default:
      DEBUG ((DEBUG_ERROR, "Not supported width for writing\n"));
      break;
  }

  DEBUG ((DEBUG_VERBOSE, "W%d: %04x:%02x:%02x.%1x - %04x 0x%08x\n",
      Width, Segment, Bus, Device, Function, Offset, Value));

  return Value;
}

