/** @file
  Host hardware inventory: DEBUG dump and OEM IPMI report to BMC at ReadyToBoot.

  Copyright (c) 2026 SOPHGO Technologies Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include "HwInventoryInternal.h"

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

EFI_HANDLE  mHwInventoryImageHandle;

STATIC
VOID
EFIAPI
HostHwInfoReportToBmcOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gBS->CloseEvent (Event);
  DEBUG ((
    DEBUG_INFO,
    "HostHwInfoReportToBmcDxe: NVMe/NIC inventory (dump + BMC report)\n"
    ));
  DumpNvmeInventory ();
  DumpNicInventory ();
}

EFI_STATUS
EFIAPI
HostHwInfoReportToBmcDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Ev;

  mHwInventoryImageHandle = ImageHandle;
  Status                  = EfiCreateEventReadyToBootEx (
                              TPL_CALLBACK,
                              HostHwInfoReportToBmcOnReadyToBoot,
                              NULL,
                              &Ev
                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "HostHwInfoReportToBmcDxe: EfiCreateEventReadyToBootEx %r\n",
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}
