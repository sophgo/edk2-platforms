/** @file
  PCIe slot information library implementation.

  Provides device-path-based lookup of per-platform BOARD_SLOT
  entries.  Consumers use PcieSlotInfoByDomain / PcieSlotInfoByPath.

  The device-path parsing logic is lifted from BMC
  PhysicalSlotLocation.c and shared via this library.

  Copyright (c) 2023-2026, SOPHGO Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcieSlotInfoLib.h>

/*
  Device-path node walking, done locally so this library stays
  dependency-free (BaseMemoryLib only) and usable from PEI -- the
  MdePkg DevicePathLib instances do not support PEIM.
*/
#define DP_NODE_TYPE(Node)     ((Node)->Type & 0x7FU)
#define DP_NODE_SUBTYPE(Node)  ((Node)->SubType)
#define DP_NODE_LENGTH(Node)   ((UINTN)((Node)->Length[0] | ((UINTN)(Node)->Length[1] << 8)))
#define DP_NODE_IS_END(Node)   \
  ((DP_NODE_TYPE (Node) == END_DEVICE_PATH_TYPE) && \
   (DP_NODE_SUBTYPE (Node) == END_ENTIRE_DEVICE_PATH_SUBTYPE))
#define DP_NODE_NEXT(Node)     \
  ((CONST EFI_DEVICE_PATH_PROTOCOL *)((CONST UINT8 *)(Node) + DP_NODE_LENGTH (Node)))

/* ------- Device-path helpers (lifted from BMC PhysicalSlotLocation.c) ------ */

STATIC
BOOLEAN
IsPcieRootNode (
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
ParseDevicePath (
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

  while (!DP_NODE_IS_END (Node)) {
    if ((DP_NODE_TYPE (Node) == ACPI_DEVICE_PATH) &&
        (DP_NODE_SUBTYPE (Node) == ACPI_DP))
    {
      if (IsPcieRootNode ((CONST ACPI_HID_DEVICE_PATH *)Node,
                          &Topo->RootUid)) {
        Topo->FoundRoot = TRUE;
        AfterRoot       = TRUE;
        Topo->PciCount  = 0;
      }
    } else if (AfterRoot &&
               (DP_NODE_TYPE (Node) == HARDWARE_DEVICE_PATH) &&
               (DP_NODE_SUBTYPE (Node) == HW_PCI_DP))
    {
      if (Topo->PciCount < PCIE_SLOT_MAX_PCI_HOPS) {
        Topo->PciDevices[Topo->PciCount] =
          ((CONST PCI_DEVICE_PATH *)Node)->Device;
        Topo->PciCount++;
      }
    }

    Node = DP_NODE_NEXT (Node);
  }
}

/* ------- Public API (wrappers call through to STATIC helpers) ------- */

BOOLEAN
EFIAPI
PcieSlotIsPcieRootNode (
  IN CONST ACPI_HID_DEVICE_PATH  *Acpi,
  OUT UINT32                     *RootUid
  )
{
  return IsPcieRootNode (Acpi, RootUid);
}

VOID
EFIAPI
PcieSlotParseDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT PCIE_SLOT_TOPOLOGY        *Topo
  )
{
  ParseDevicePath (DevicePath, Topo);
}

/* ------- Slot lookup ------- */

/*
  PcieSlotInfoGetBoardTable() is provided by a separate library
  instance selected via the platform DSC (the Null instance, or a
  board table such as SD3-10's SlotTable.c).
*/

CONST BOARD_SLOT *
EFIAPI
PcieSlotInfoByDomain (
  IN UINT32  Domain
  )
{
  CONST BOARD_SLOT  *Table;
  UINTN              Count;
  UINTN              i;

  Table = PcieSlotInfoGetBoardTable (&Count);
  if (Table == NULL) {
    return NULL;
  }

  /*
    Only RC-direct entries (PathLen == 1: root port hop only) are
    considered: a domain whose slots all hang off a downstream switch
    (e.g. SRA3-40 domain 6) has no slot on the root port itself.
  */
  for (i = 0; i < Count; i++) {
    if ((Table[i].Domain == Domain) && (Table[i].PathLen == 1)) {
      return &Table[i];
    }
  }

  return NULL;
}

CONST BOARD_SLOT *
EFIAPI
PcieSlotInfoByPath (
  IN UINT32  Domain,
  IN UINT8   *DevPath,
  IN UINTN   PathLen
  )
{
  CONST BOARD_SLOT  *Table;
  UINTN              Count;
  UINTN              i;

  Table = PcieSlotInfoGetBoardTable (&Count);
  if (Table == NULL) {
    return NULL;
  }

  for (i = 0; i < Count; i++) {
    if ((Table[i].Domain == Domain) &&
        (Table[i].PathLen == PathLen) &&
        (CompareMem (Table[i].DevPath, DevPath, PathLen) == 0)) {
      return &Table[i];
    }
  }

  return NULL;
}
