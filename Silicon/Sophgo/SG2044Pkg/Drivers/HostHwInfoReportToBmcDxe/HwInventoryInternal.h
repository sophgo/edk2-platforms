/** @file
  Internal declarations for merged HW inventory dump + BMC report DXE.

  Copyright (c) 2026 SOPHGO Technologies Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HW_INVENTORY_INTERNAL_H_
#define HW_INVENTORY_INTERNAL_H_

#include <Uefi.h>

#include <Protocol/DevicePath.h>

extern EFI_HANDLE  mHwInventoryImageHandle;

VOID
HostHwInfoReportToBmcResolvePhysicalSlot (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath OPTIONAL,
  OUT UINT8                     *PhysicalSlot
  );

VOID
DumpNvmeInventory (
  VOID
  );

VOID
DumpNicInventory (
  VOID
  );

#endif
