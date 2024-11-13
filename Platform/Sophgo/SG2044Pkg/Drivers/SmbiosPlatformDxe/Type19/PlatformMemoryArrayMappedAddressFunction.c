/** @file

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "SmbiosPlatformDxe.h"

/**
  This function adds SMBIOS Table (Type 19) records.

  @param  RecordData                 Pointer to SMBIOS Table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS Table was successfully added.
  @retval Other                      Failed to update the SMBIOS Table.

**/
SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformMemoryArrayMappedAddress) {
  EFI_STATUS           Status;
  STR_TOKEN_INFO       *InputStrToken;
  SMBIOS_TABLE_TYPE19  *InputData;
  SMBIOS_TABLE_TYPE19  *Type19Record;

  InputData     = (SMBIOS_TABLE_TYPE19 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;
  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type19Record,
      (VOID *)&InputData,
      sizeof (SMBIOS_TABLE_TYPE19),
      InputStrToken
      );
    if (Type19Record == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type19Record, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (Type19Record);
      return Status;
    }

    FreePool (Type19Record);

    InputData++;
    InputStrToken++;
  }

  return Status;
}
