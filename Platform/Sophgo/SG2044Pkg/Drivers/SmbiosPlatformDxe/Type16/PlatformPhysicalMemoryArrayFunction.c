/** @file

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2024. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include "SmbiosPlatformDxe.h"

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformPhysicalMemoryArray) {
  EFI_STATUS           Status;
  SMBIOS_TABLE_TYPE16  *InputData;

  InputData = (SMBIOS_TABLE_TYPE16 *)RecordData;

  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)InputData, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    InputData++;
  }

  return Status;
}