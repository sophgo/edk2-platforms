/** @file
  Library to query per-platform PCIe slot descriptions.

  Consumers (PEI SLTCAP programming, DXE SMBIOS Type 9, ACPI _SUN)
  retrieve a CONST BOARD_SLOT by matching a PCI device-path against
  a per-variant board slot table.

  Copyright (c) 2023-2026, SOPHGO Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __PCIE_SLOT_INFO_LIB_H__
#define __PCIE_SLOT_INFO_LIB_H__

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/DevicePath.h>

/* Maximum PCI hops after the root bridge in a device path. */
#define PCIE_SLOT_MAX_PCI_HOPS  16

/*
  SlotNumber sentinel meaning "no physical slot" (e.g. a controller
  whose lanes go nowhere, or an onboard device with no slot identity).
  0 is a VALID slot number (SD3-12/SRM3-C0 use it for PCIE0), so the
  sentinel must not be 0.
*/
#define PCIE_SLOT_NUMBER_NONE  0xFFFF

/**
  Parsed PCIe topology extracted from a UEFI device path.

  RootUid is the ACPI _UID of the root bridge, which equals the PCIe
  domain number (== ACPI _SEG).  PciDevices[] holds the Device numbers
  of ALL PCI_DEVICE_PATH nodes after the root bridge, starting with
  the root port itself (always Device 0 on SG2044, matching the
  "Segment/Dev.Fn/..." notation of the board mapping document).
  The Function sub-field is ignored entirely: all SG2044 board
  slot/device paths use Function 0 (single-function ports and
  devices), so DevPath entries carry Device numbers only and Function
  must always be 0 in board tables.

  For a root-complex-direct slot the path is exactly one hop -- the
  root port (PciCount == 1, PciDevices[0] == 0).
**/
typedef struct {
  BOOLEAN  FoundRoot;
  UINT32   RootUid;                             /* PCIe domain */
  UINT8    PciDevices[PCIE_SLOT_MAX_PCI_HOPS];  /* Device numbers after root */
  UINTN    PciCount;
} PCIE_SLOT_TOPOLOGY;

/**
  Board-level slot descriptor.

  Each entry describes one physical connector (standard PCIe, M.2, OCP,
  MCIO, etc.) on the board.  The Domain + DevPath serve as the lookup
  key; the remaining fields are slot attributes consumed by config-space
  PSN, Type 9, and _SUN.

  DevPath always starts with the root port hop (Device 0), so a
  root-complex-direct slot has PathLen == 1; slots behind an onboard
  switch have PathLen > 1.

  Type 9 fields (SlotLength / SlotHeight / SlotInformation /
  Characteristics1 / Characteristics2 / SlotPitch) are explicit members,
  filled per slot from the board mapping wiki -- no runtime derivation.
**/
typedef struct {
  UINT32         Domain;
  UINT8          DevPath[PCIE_SLOT_MAX_PCI_HOPS]; /* Device numbers only, Fn always 0 */
  UINTN          PathLen;
  UINT16                    SlotNumber;    /* chassis-unique slot number; 0 is valid,
                                              PCIE_SLOT_NUMBER_NONE = no physical slot */
  MISC_SLOT_TYPE            SlotType;      /* e.g. SlotTypePCIExpressGen5X8 */
  CONST CHAR8               *Designation;  /* silk-screen label, e.g. "SLOT1" */
  MISC_SLOT_DATA_BUS_WIDTH  SlotDataBusWidth; /* Slot Data Bus Width (0x06, ENUM), e.g. SlotDataBusWidth8X */
  UINT8                     DataBusWidth;     /* Data Bus Width (0x11, Varies), raw lane count, e.g. 8 for x8 */
  MISC_SLOT_DATA_BUS_WIDTH  PhysicalWidth; /* physical slot width (Type 9 extended) */
  MISC_SLOT_LENGTH          SlotLength;    /* Type 9 SlotLength: Long/Short/Unknown */
  MISC_SLOT_HEIGHT          SlotHeight;    /* Type 9 extended SlotHeight: Full/LowProfile/None */
  UINT8                     SlotInformation; /* Type 9 extended Gen (3/4/5; 0 for M.2/MCIO) */
  UINT8                     Characteristics1; /* Type 9 Char1; all slots 0x04 (3.3V) */
  UINT8                     Characteristics2; /* Type 9 Char2; all slots 0x04 (SMBus) */
  UINT16                    SlotPitch;     /* Type 9 extended SlotPitch; 0 = unknown */
} BOARD_SLOT;

/**
  Determine whether an ACPI_DP device-path node represents a PCIe root
  bridge (HID == PNP0A08).  If RootUid is non-NULL it receives the
  _UID which equals the PCIe domain / ACPI _SEG.

  @param[in]  Acpi     Pointer to an ACPI_HID_DEVICE_PATH node.
  @param[out] RootUid  Receives the _UID (domain), optional.

  @retval TRUE   This node is a PCIe root bridge.
  @retval FALSE  Not a root bridge.
**/
BOOLEAN
EFIAPI
PcieSlotIsPcieRootNode (
  IN  CONST ACPI_HID_DEVICE_PATH  *Acpi,
  OUT UINT32                     *RootUid
  );

/**
  Parse a UEFI device path and extract the PCIe topology (domain +
  downstream PCI device numbers).  Bus numbers are NOT used.

  @param[in]  DevicePath  PCI device path to parse.
  @param[out] Topo        Receives the parsed topology.
**/
VOID
EFIAPI
PcieSlotParseDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT PCIE_SLOT_TOPOLOGY        *Topo
  );

/**
  Look up a board slot by PCIe domain (for RC-direct slots with no
  downstream switch hops).  This is the PEI-safe entry point.

  @param[in] Domain  PCIe domain number.

  @return Pointer to the slot entry, or NULL if not found.
**/
CONST BOARD_SLOT *
EFIAPI
PcieSlotInfoByDomain (
  IN UINT32  Domain
  );

/**
  Look up a board slot by domain and downstream PCI-device path.

  Domain, DevPath[] and PathLen must match the table entry exactly.
  To find the slot a device is plugged into, pass the path of the
  slot's downstream port (i.e. strip the device's own trailing
  PCI_DEVICE_PATH node).  Use GetLocation() from the matched device
  handle to obtain the runtime SBDF for the port.

  @param[in] Domain   PCIe domain number.
  @param[in] DevPath  Array of PCI device numbers after the root bridge.
  @param[in] PathLen  Number of entries in DevPath.

  @return Pointer to the slot entry, or NULL if not found.
**/
CONST BOARD_SLOT *
EFIAPI
PcieSlotInfoByPath (
  IN UINT32  Domain,
  IN UINT8   *DevPath,
  IN UINTN   PathLen
  );

/**
  Return the platform's board slot table.

  Each platform provides a single STATIC CONST array of BOARD_SLOT;
  the number of entries is returned via Count (no terminator entry
  is used).

  The implementation is selected by the platform DSC's PcieSlotInfoLib
  mapping: a board table instance (e.g. SD3-10's SlotTable.c) or the
  Null instance, which returns NULL.

  @param[out] Count  Receives the number of slot entries (optional).

  @return Pointer to the slot array, or NULL if not available.
**/
CONST BOARD_SLOT *
EFIAPI
PcieSlotInfoGetBoardTable (
  OUT UINTN  *Count OPTIONAL
  );

#endif /* __PCIE_SLOT_INFO_LIB_H__ */
