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

typedef struct {
  UINTN   DbiBase;
  UINTN   DbiSize;
  UINTN   AtuBase;
  UINTN   AtuSize;
  UINTN   CfgBase;
  UINTN   CfgSize;
} DW_PCIE;

/* edk2 related */
#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

#define GET_SEGMENT(Address)    (((Address) >> 32) & 0xFFFF)
#define GET_BUS(Address)        (((Address) >> 20) & 0x7F)
#define GET_DEVICE(Address)     (((Address) >> 15) & 0x1F)
#define GET_FUNCTION(Address)   (((Address) >> 12) & 0x07)
#define GET_OFFSET(Address)     ((Address) & 0xFFF)

/* platform specific variables */
/* 40 lans total, 4 lan per controller */
#define SG2044_PCIE_MAX_ROOT  (10)

typedef struct {
  PCI_ROOT_BRIDGE                   PciRoot[SG2044_PCIE_MAX_ROOT];
  DW_PCIE                           DwPcie[SG2044_PCIE_MAX_ROOT];
  EFI_PCI_ROOT_BRIDGE_DEVICE_PATH   PciDevicePath[SG2044_PCIE_MAX_ROOT];
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
UINT16
DwPcieDbiRead16 (
    IN  DW_PCIE *Pcie,
    IN  UINT32 Offset
    )
{
  return MmioRead16 (Pcie->DbiBase + Offset);
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

  /* program bus numbers */
  DwPcieDbiWrite8(Pcie, PCI_BRIDGE_PRIMARY_BUS_REGISTER_OFFSET, 0);
  DwPcieDbiWrite8(Pcie, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET, 1);
  DwPcieDbiWrite8(Pcie, PCI_BRIDGE_SUBORDINATE_BUS_REGISTER_OFFSET, PCI_MAX_BUS);

  /* class code */
  /* program interface, bridge program interface always 0 */
  DwPcieDbiWrite8(Pcie, PCI_CLASSCODE_OFFSET, 0);
  /* sub class code */
  DwPcieDbiWrite8(Pcie, PCI_CLASSCODE_OFFSET + 1, PCI_CLASS_BRIDGE_P2P);
  /* base class code */
  DwPcieDbiWrite8(Pcie, PCI_CLASSCODE_OFFSET + 2, PCI_CLASS_BRIDGE);

  Value = DwPcieDbiRead16 (Pcie, PCI_COMMAND_OFFSET);
  Value |=
    EFI_PCI_COMMAND_IO_SPACE |
    EFI_PCI_COMMAND_MEMORY_SPACE |
    EFI_PCI_COMMAND_BUS_MASTER |
    EFI_PCI_COMMAND_SERR;
  DwPcieDbiWrite16 (Pcie, PCI_COMMAND_OFFSET, Value);

  /* disable write permission for read-only registers */
  Value = DwPcieDbiRead32(Pcie, DW_PCIE_MISC_CONTROL_1);
  Value &= ~DW_PCIE_DBI_RO_WR_EN;
  DwPcieDbiWrite32(Pcie, DW_PCIE_MISC_CONTROL_1, Value);
}

typedef struct {
  UINT32    Flag;
  UINT64    PciAddr;
  UINT64    CpuAddr;
  UINT64    Size;
} FDT_PCI_RANGE;

#define FDT_PCI_PARENT_ADDRESS_CELLS  2
#define FDT_PCI_PARENT_SIZE_CELLS     2
#define FDT_PCI_ADDRESS_CELLS         3
#define FDT_PCI_SIZE_CELLS            2
#define FDT_PCI_RANGE_SIZE            \
  ((FDT_PCI_PARENT_ADDRESS_CELLS + FDT_PCI_ADDRESS_CELLS + FDT_PCI_SIZE_CELLS) * 4)

#define FDT_PCI_MEM_TYPE_SHIFT  (24)
#define FDT_PCI_MEM_TYPE_MASK   (0x03 << FDT_PCI_MEM_TYPE_SHIFT)
#define FDT_PCI_MEM_TYPE_IO     (1 << FDT_PCI_MEM_TYPE_SHIFT)
#define FDT_PCI_MEM_TYPE_MEM32  (2 << FDT_PCI_MEM_TYPE_SHIFT)
#define FDT_PCI_MEM_TYPE_MEM64  (3 << FDT_PCI_MEM_TYPE_SHIFT)

#define FDT_PCI_MEM_PREFETCH_SHIFT    (30)
#define FDT_PCI_MEM_PREFETCH_MASK     (1 << FDT_PCI_MEM_PREFETCH_SHIFT)
#define FDT_PCI_MEM_PREFETCH          (1 << FDT_PCI_MEM_PREFETCH_SHIFT)

STATIC
VOID
InitSlaveMappingFromFdt (
    IN  FDT_CLIENT_PROTOCOL *FdtClient,
    OUT PCI_ROOT_BRIDGE     *PciRoot,
    IN  INT32               Node
    )
{
  CONST VOID                *Prop;
  UINT32                    PropSize;
  EFI_STATUS                Status;
  FDT_PCI_RANGE             Range[5];
  UINT32                    RangeIndex;
  PCI_ROOT_BRIDGE_APERTURE  *Aperture;

  /* parse bus range */
  Status = FdtClient->GetNodeProperty (FdtClient, Node, "bus-range", &Prop, &PropSize);
  if (Status != EFI_SUCCESS)
    DEBUG ((DEBUG_WARN, "Cannot found ranges from dt, assume 0-255\n"));

  /* bus number always 0 for root port */
  PciRoot->Bus.Base         = 0;
  PciRoot->Bus.Limit        = 255;
  PciRoot->Bus.Translation  = 0;

  Status = FdtClient->GetNodeProperty (FdtClient, Node, "ranges", &Prop, &PropSize);

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Cannot found ranges from dt\n"));
    return;
  }

  if (PropSize > ARRAY_SIZE (Range) * FDT_PCI_RANGE_SIZE) {
    DEBUG ((DEBUG_WARN, "Too many range in dt, maybe a wrong config\n"));
    DEBUG ((DEBUG_WARN, "Only range[0] - range[%d] effect on\n", ARRAY_SIZE (Range)));
    PropSize = sizeof (Range);
  }

  /* get flag */
  for (RangeIndex = 0; RangeIndex < ARRAY_SIZE (Range); ++RangeIndex, Prop += FDT_PCI_RANGE_SIZE) {
    Range[RangeIndex].Flag = SwapBytes32 (*(UINT32 *)Prop);
    /* platform must support unaligned access */
    Range[RangeIndex].PciAddr =
      SwapBytes64 (*(UINT64 *)(Prop + 4));
    Range[RangeIndex].CpuAddr =
      SwapBytes64 (*(UINT64 *)(Prop + FDT_PCI_ADDRESS_CELLS * 4));
    Range[RangeIndex].Size =
      SwapBytes64 (*(UINT64 *)(Prop +  (FDT_PCI_ADDRESS_CELLS + FDT_PCI_PARENT_ADDRESS_CELLS) * 4));
  }

  for (RangeIndex = 0; RangeIndex < ARRAY_SIZE (Range); ++RangeIndex) {
    switch (Range[RangeIndex].Flag & (FDT_PCI_MEM_TYPE_MASK | FDT_PCI_MEM_PREFETCH_MASK)) {
      case FDT_PCI_MEM_TYPE_IO:
        Aperture = &PciRoot->Io;
        break;
      case FDT_PCI_MEM_TYPE_MEM32:
        Aperture = &PciRoot->Mem;
        break;
      case FDT_PCI_MEM_TYPE_MEM32 | FDT_PCI_MEM_PREFETCH:
        Aperture = &PciRoot->PMem;
        break;
      case FDT_PCI_MEM_TYPE_MEM64:
        Aperture = &PciRoot->MemAbove4G;
        break;
      case FDT_PCI_MEM_TYPE_MEM64 | FDT_PCI_MEM_PREFETCH:
        Aperture = &PciRoot->PMemAbove4G;
        break;
      default:
        DEBUG ((DEBUG_ERROR, "Undefined PCI memory type\n"));
        continue;
    }
    Aperture->Base           = Range[RangeIndex].PciAddr;
    Aperture->Limit          = Range[RangeIndex].PciAddr + Range[RangeIndex].Size - 1;
    Aperture->Translation    = Range[RangeIndex].PciAddr - Range[RangeIndex].CpuAddr;
  }
}

STATIC
RETURN_STATUS
FdtGetResourceByName (
    IN  FDT_CLIENT_PROTOCOL *FdtClient,
    IN  INT32               Node,
    IN  CONST CHAR8         *ResourceName,
    OUT UINTN               *Base,
    OUT UINTN               *Size
    )
{
  CONST CHAR8  *NameList;
  UINT32      NameIndex;
  UINT32      NameListSize;
  UINT32      ResourceIndex;
  CONST VOID  *ResourceProp;
  UINT32      ResourcePropSize;
  UINT32      ResourceOffset;
  UINT32      ResourcePropElementSize;
  EFI_STATUS  Status;

  Status = FdtClient->GetNodeProperty(FdtClient, Node, "reg-names",
      (CONST VOID **)&NameList, &NameListSize);

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "No reg-names property\n"));
    return EFI_NOT_FOUND;
  }

  for (NameIndex = 0, ResourceIndex = 0; NameIndex < NameListSize; ++NameIndex, ++ResourceIndex) {
    if (AsciiStrCmp(NameList + NameIndex, ResourceName) != 0) {
      /* to next string */
      for (; NameIndex < NameListSize; ++NameIndex) {
        if (NameList[NameIndex] == 0)
          break;
      }
    } else {
      break;
    }
  }

  /* not found */
  if (NameIndex >= NameListSize) {
    DEBUG ((DEBUG_ERROR, "Resource %a not found\n", ResourceName));
    return EFI_NOT_FOUND;
  }

  /* found */
  Status = FdtClient->GetNodeProperty(FdtClient, Node, "reg", &ResourceProp, &ResourcePropSize);

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "No reg property\n"));
    return EFI_NOT_FOUND;
  }

  ResourcePropElementSize = (FDT_PCI_PARENT_ADDRESS_CELLS + FDT_PCI_PARENT_SIZE_CELLS) * 4;
  ResourceOffset = ResourcePropElementSize * ResourceIndex;

  if (ResourceOffset + ResourcePropElementSize > ResourcePropSize) {
    DEBUG ((DEBUG_ERROR, "Not enough reg properties\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  if (Base != NULL)
    *Base = SwapBytes64(*(UINT64 *)(ResourceProp + ResourceOffset));

  if (Size != NULL)
    *Size = SwapBytes64(*(UINT64 *)(ResourceProp + ResourceOffset + FDT_PCI_PARENT_ADDRESS_CELLS * 4));

  return EFI_SUCCESS;
}

UINT32
InitPlatformFromFdt (
    OUT   SG2044_PCIE_ROOT *SG2044PciRoot
    )
{
  RETURN_STATUS       FindNodeStatus;
  RETURN_STATUS       Status;
  FDT_CLIENT_PROTOCOL *FdtClient;
  INT32               Node;
  CONST VOID          *Prop;
  UINT32              PropSize;
  CONST CHAR8         *Compatible = "sophgo,sg2044-pcie-host";
  UINT32              Segment;
  PCI_ROOT_BRIDGE     *PciRoot;
  DW_PCIE             *DwPcie;

  SetMem (SG2044PciRoot, sizeof (SG2044_PCIE_ROOT), 0);

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL, (VOID **)&FdtClient);

  if (Status) {
    DEBUG ((DEBUG_ERROR, "No FDT client service found\n"));
    DEBUG ((DEBUG_ERROR, "Cannot init PCIe controllers\n"));
    return 0;
  }

  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, Compatible, &Node), Segment = 0;
      FindNodeStatus == EFI_SUCCESS;
      FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, Compatible, Node, &Node), ++Segment) {

    /* Setup registers by name */
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "reg", &Prop, &PropSize);

    if (Status != EFI_SUCCESS) {
      DEBUG ((DEBUG_ERROR, "Cannot find reg property\n"));
      continue;
    }

    if (PropSize < sizeof (DwPcie->DbiBase) + sizeof (DwPcie->AtuBase) + sizeof (DwPcie->CfgBase)) {
      DEBUG ((DEBUG_ERROR, "Not enough reg properties, should have dbi, atu and config registers\n"));
      continue;
    }

    PciRoot = &SG2044PciRoot->PciRoot[Segment];
    DwPcie = &SG2044PciRoot->DwPcie[Segment];

    PciRoot->Segment = Segment;

    FdtGetResourceByName (FdtClient, Node, "dbi", &DwPcie->DbiBase, &DwPcie->DbiSize);
    FdtGetResourceByName (FdtClient, Node, "atu", &DwPcie->AtuBase, &DwPcie->AtuSize);
    FdtGetResourceByName (FdtClient, Node, "config", &DwPcie->CfgBase, &DwPcie->CfgSize);

    DEBUG ((DEBUG_INFO, "PCIe%d:\n"
          "DBI    [%016lx - %016lx]\n"
          "ATU    [%016lx - %016lx]\n"
          "Config [%016lx - %016lx]\n",
        Segment,
        DwPcie->DbiBase, DwPcie->DbiBase + DwPcie->DbiSize,
        DwPcie->AtuBase, DwPcie->AtuBase + DwPcie->AtuSize,
        DwPcie->CfgBase, DwPcie->CfgBase + DwPcie->CfgSize));

    PciRoot->Supports                  = 0;
    PciRoot->Attributes                = 0;
    PciRoot->DmaAbove4G                = TRUE;
    PciRoot->NoExtendedConfigSpace     = FALSE;
    PciRoot->ResourceAssigned          = FALSE;
    PciRoot->AllocationAttributes      = EFI_PCI_HOST_BRIDGE_MEM64_DECODE;

    InitSlaveMappingFromFdt (FdtClient, PciRoot, Node);
  }

  mSG2044PciRoot.Count = Segment;

  return mSG2044PciRoot.Count;
}

VOID
SetupPciRoot (
    IN  PCI_ROOT_BRIDGE   *PciRoot,
    IN  DW_PCIE           *DwPcie,
    IN  UINT64            SystemMemoryStart,
    IN  UINT64            SystemMemorySize
    )
{
  DwPcieSetAtuOutbound (
      DwPcie,
      1,
      DW_PCIE_ATU_TYPE_IO,
      PciRoot->Io.Base - PciRoot->Io.Translation,
      PciRoot->Io.Base,
      PciRoot->Io.Limit + 1 - PciRoot->Io.Base
      );

  DwPcieSetAtuOutbound (
      DwPcie,
      2,
      DW_PCIE_ATU_TYPE_MEM,
      PciRoot->PMem.Base - PciRoot->PMem.Translation,
      PciRoot->PMem.Base,
      PciRoot->PMem.Limit + 1 - PciRoot->PMem.Base
      );

  DwPcieSetAtuOutbound (
      DwPcie,
      3,
      DW_PCIE_ATU_TYPE_MEM,
      PciRoot->Mem.Base - PciRoot->Mem.Translation,
      PciRoot->Mem.Base,
      PciRoot->Mem.Limit + 1 - PciRoot->Mem.Base
      );

  DwPcieSetAtuOutbound (
      DwPcie,
      4,
      DW_PCIE_ATU_TYPE_MEM,
      PciRoot->PMemAbove4G.Base - PciRoot->PMemAbove4G.Translation,
      PciRoot->PMemAbove4G.Base,
      PciRoot->PMemAbove4G.Limit + 1 - PciRoot->PMemAbove4G.Base
      );

  DwPcieSetAtuOutbound (
      DwPcie,
      5,
      DW_PCIE_ATU_TYPE_MEM,
      PciRoot->MemAbove4G.Base - PciRoot->MemAbove4G.Translation,
      PciRoot->MemAbove4G.Base,
      PciRoot->MemAbove4G.Limit + 1 - PciRoot->MemAbove4G.Base
      );

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

  PciRootCount = InitPlatformFromFdt (&mSG2044PciRoot);

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

    mSG2044PciRoot.PciDevicePath[PciRootIter].AcpiDevicePath.UID = PciRootIter;
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

UINT32
EFIAPI
PciSegmentRead (
  IN UINT64                         Address,
  IN UINT32                         Width
  )
{
  UINT32    Segment;
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

  if (Segment > mSG2044PciRoot.Count) {
    DEBUG ((DEBUG_ERROR, "Invalid PCIe segment %d\n", Segment));
    return 0xffffffff;
  }

  /* Find PCIe controller */
  DwPcie = &mSG2044PciRoot.DwPcie[Segment];

  if (Bus == mSG2044PciRoot.PciRoot[Segment].Bus.Base) {
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
    if (Bus == mSG2044PciRoot.PciRoot[Segment].Bus.Base + 1) {
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
  Function = GET_DEVICE (Address);
  Offset = GET_OFFSET (Address);

  if (Segment > mSG2044PciRoot.Count) {
    DEBUG ((DEBUG_ERROR, "Invalid PCIe segment %d\n", Segment));
    return Value;
  }

  /* Find PCIe controller */
  DwPcie = &mSG2044PciRoot.DwPcie[Segment];

  if (Bus == mSG2044PciRoot.PciRoot[Segment].Bus.Base) {
    /* host root complex */
    CfgBase = DwPcie->DbiBase;
  } else {
    /* devices other than root complex, including pcie switches */
    if (!DwPcieLinkUp (DwPcie))
      return Value;

    PciAddr = DwPcieAtuPciAddr (Bus, Device, Function);

    if (Bus == mSG2044PciRoot.PciRoot[Segment].Bus.Base + 1)
      Type = DW_PCIE_ATU_TYPE_CFG0;
    else
      Type = DW_PCIE_ATU_TYPE_CFG1;

    DwPcieSetAtuOutbound (DwPcie, 0, DW_PCIE_ATU_TYPE_CFG0, DwPcie->CfgBase, PciAddr, DwPcie->CfgSize);

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

