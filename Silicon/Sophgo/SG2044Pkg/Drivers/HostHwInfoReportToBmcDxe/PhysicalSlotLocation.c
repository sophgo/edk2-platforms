/** @file
  Map UEFI device paths to BMC physical_slot (platform-specific tables).

  Copyright (c) 2026 SOPHGO Technologies Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <HostHwInfoReportToBmcOem.h>
#include "HwInventoryInternal.h"

#include <Protocol/DevicePath.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>

#define PCIE_SLOT_MAX_PCI_HOPS  16

typedef struct {
  BOOLEAN  FoundRoot;
  UINT32   RootUid;
  UINT8    PciDevices[PCIE_SLOT_MAX_PCI_HOPS];
  UINTN    PciCount;
} PCIE_SLOT_TOPOLOGY;

//
// Switch root complex on SRA3-40: C2C1 Wrapper1, PCIe domain 6
// (RootUid == PCIe domain; see PciPlatformLib).
//
#define SRA3_40_SWITCH_ROOT_COMPLEX_UID  6
#define SRA3_40_SWITCH_PCI_DEV           0x04
#define SRA3_40_SLOT_PCI_INDEX           4

STATIC
BOOLEAN
PcieSlotIsPcieRootNode (
  IN CONST ACPI_HID_DEVICE_PATH  *Acpi,
  OUT UINT32                     *RootUid
  )
{
  if ((Acpi->HID & PNP_EISA_ID_MASK) != PNP_EISA_ID_CONST) {
    return FALSE;
  }

  if (EISA_ID_TO_NUM (Acpi->HID) != 0x0A08) {
    return FALSE;
  }

  if (RootUid != NULL) {
    *RootUid = Acpi->UID;
  }

  return TRUE;
}

STATIC
VOID
PcieSlotParseDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT PCIE_SLOT_TOPOLOGY        *Topo
  )
{
  CONST EFI_DEVICE_PATH_PROTOCOL  *Node;
  BOOLEAN                          AfterRoot;

  ZeroMem (Topo, sizeof (*Topo));
  if (DevicePath == NULL) {
    return;
  }

  AfterRoot = FALSE;
  Node      = DevicePath;

  while (!IsDevicePathEnd (Node)) {
    if ((DevicePathType (Node) == ACPI_DEVICE_PATH) &&
        (DevicePathSubType (Node) == ACPI_DP))
    {
      if (PcieSlotIsPcieRootNode ((CONST ACPI_HID_DEVICE_PATH *)Node, &Topo->RootUid)) {
        Topo->FoundRoot = TRUE;
        AfterRoot       = TRUE;
        Topo->PciCount  = 0;
      }
    } else if (AfterRoot &&
               (DevicePathType (Node) == HARDWARE_DEVICE_PATH) &&
               (DevicePathSubType (Node) == HW_PCI_DP))
    {
      if (Topo->PciCount < PCIE_SLOT_MAX_PCI_HOPS) {
        Topo->PciDevices[Topo->PciCount] = ((CONST PCI_DEVICE_PATH *)Node)->Device;
        Topo->PciCount++;
      }
    }

    Node = NextDevicePathNode (Node);
  }
}

//
// SRA3-40 (platform_type = 0)
//

STATIC
UINT8
Sra340MapPcieRootUid (
  IN UINT32  RootUid
  )
{
  //
  // RootUid is the ACPI device-path UID, which equals the PCIe domain number
  // (see PciPlatformLib). On SRA3-40 the enabled controllers are domains
  // 0/2/4/6/8; domain 6 (C2C1 W1) is the switch root handled separately.
  //
  switch (RootUid) {
    case 0:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE1;
    case 2:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE2;
    case 4:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE3;
    case 8:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE4;
    default:
      return BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN;
  }
}

STATIC
UINT8
Sra340MapPcieRoot3Path (
  IN CONST PCIE_SLOT_TOPOLOGY  *Topo
  )
{
  BOOLEAN  SwitchBranch;
  UINT8    SlotDevice;

  if ((Topo == NULL) || (Topo->PciCount <= SRA3_40_SLOT_PCI_INDEX)) {
    return BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN;
  }

  SwitchBranch = (Topo->PciCount > 2) &&
                 (Topo->PciDevices[2] == SRA3_40_SWITCH_PCI_DEV);
  SlotDevice   = Topo->PciDevices[SRA3_40_SLOT_PCI_INDEX];

  if (SwitchBranch) {
    switch (SlotDevice) {
      case 0x06:
        return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_ONBOARD_NIC_PORT0;
      case 0x07:
        return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_ONBOARD_NIC_PORT1;
      case 0x08:
        return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE6;
      default:
        return BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN;
    }
  }

  switch (SlotDevice) {
    case 0x08:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_MB_M2;
    case 0x0C:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_BACKPLANE_M2;
    case 0x18:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE5;
    default:
      return BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN;
  }
}

STATIC
UINT8
Sra340ResolvePhysicalSlot (
  IN CONST PCIE_SLOT_TOPOLOGY  *Topo
  )
{
  if ((Topo == NULL) || !Topo->FoundRoot) {
    return BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN;
  }

  if (Topo->RootUid == SRA3_40_SWITCH_ROOT_COMPLEX_UID) {
    return Sra340MapPcieRoot3Path (Topo);
  }

  return Sra340MapPcieRootUid (Topo->RootUid);
}

//
// SRA3-40-8 (platform_type = 1)
//

//
// Switch root complex on SRA3-40-8: PCIe domain 6
// (RootUid == PCIe domain; see PciPlatformLib).
//
#define SRA3_40_8_PCIE_ROOT_SWITCH_UID   6
#define SRA3_40_8_SWITCH_BRANCH_PCI_DEV  0x04
#define SRA3_40_8_SLOT_PCI_INDEX         4

STATIC
UINT8
Sra3408MapPcieRootUid (
  IN UINT32  RootUid
  )
{
  //
  // RootUid is the ACPI device-path UID, which equals the PCIe domain number
  // (see PciPlatformLib). On SRA3-40-8 the enabled controllers are domains
  // 0/1/2/3/4/5/6/8/9; domain 6 is the switch root handled separately.
  //
  switch (RootUid) {
    case 0:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE1;
    case 1:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE2;
    case 2:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE3;
    case 3:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE4;
    case 4:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE5;
    case 5:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE6;
    case 8:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE7;
    case 9:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE8;
    default:
      return BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN;
  }
}

STATIC
UINT8
Sra3408MapPcieRoot6Path (
  IN CONST PCIE_SLOT_TOPOLOGY  *Topo
  )
{
  BOOLEAN  SwitchBranch;
  UINT8    SlotDevice;

  if ((Topo == NULL) || (Topo->PciCount <= SRA3_40_8_SLOT_PCI_INDEX)) {
    return BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN;
  }

  SwitchBranch = (Topo->PciCount > 2) &&
                 (Topo->PciDevices[2] == SRA3_40_8_SWITCH_BRANCH_PCI_DEV);
  SlotDevice   = Topo->PciDevices[SRA3_40_8_SLOT_PCI_INDEX];

  if (SwitchBranch) {
    switch (SlotDevice) {
      case 0x06:
        return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_ONBOARD_NIC_PORT0;
      case 0x07:
        return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_ONBOARD_NIC_PORT1;
      case 0x08:
        return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_BACKPLANE_M2_3;
      case 0x0C:
        return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_BACKPLANE_M2_2;
      default:
        return BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN;
    }
  }

  switch (SlotDevice) {
    case 0x08:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_MB_M2;
    case 0x0C:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_BACKPLANE_M2_1;
    case 0x18:
      return BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE9;
    default:
      return BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN;
  }
}

STATIC
UINT8
Sra3408ResolvePhysicalSlot (
  IN CONST PCIE_SLOT_TOPOLOGY  *Topo
  )
{
  if ((Topo == NULL) || !Topo->FoundRoot) {
    return BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN;
  }

  if (Topo->RootUid == SRA3_40_8_PCIE_ROOT_SWITCH_UID) {
    return Sra3408MapPcieRoot6Path (Topo);
  }

  return Sra3408MapPcieRootUid (Topo->RootUid);
}

VOID
HostHwInfoReportToBmcResolvePhysicalSlot (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath OPTIONAL,
  OUT UINT8                     *PhysicalSlot
  )
{
  PCIE_SLOT_TOPOLOGY  Topo;
  UINT8               PlatformType;
  UINT8               Slot;

  if (PhysicalSlot == NULL) {
    return;
  }

  *PhysicalSlot = BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN;

  if (DevicePath == NULL) {
    return;
  }

  PlatformType = HostHwInfoReportToBmcGetPlatformType ();
  PcieSlotParseDevicePath (DevicePath, &Topo);

  switch (PlatformType) {
    case BMC_HW_INFO_PLATFORM_TYPE_SRA3_40:
      Slot = Sra340ResolvePhysicalSlot (&Topo);
      break;
    case BMC_HW_INFO_PLATFORM_TYPE_SRA3_40_8:
      Slot = Sra3408ResolvePhysicalSlot (&Topo);
      break;
    default:
      return;
  }

  *PhysicalSlot = Slot;

  DEBUG ((
    DEBUG_INFO,
    "HostHwInfoReportToBmc: platform_type=%u physical_slot=%u "
    "(PcieRoot(0x%x) pci_hops=%u)\n",
    PlatformType,
    Slot,
    Topo.RootUid,
    (UINT32)Topo.PciCount
    ));
}
