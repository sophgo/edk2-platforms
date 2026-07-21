/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2024. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>

#include "SmbiosPlatformDxe.h"

#define MAX_INSTALLEDSIZE_1k 0x7FFF
#define MAX_INSTALLEDSIZE_64K 0x1FFC00
#define INSTALLEDSIZE_GRANULARITY_BIT 15
#define INSTALLEDSIZE2_GRANULARITY_BIT 31

EFI_STATUS
UpdateCacheSize(
  IN CHAR16          *UnicodeStr,
  SMBIOS_TABLE_TYPE7  *InputData
  )
{
  UINT64     Value;
  UINT32     Bytes;
  UINT32     Bytes32;

  if (!StrCmp(UnicodeStr, L"L1 Cache")) {
    Value = FixedPcdGet64(PcdCpuCount) * (FixedPcdGet64(PcdCpuL1ICacheSizeBytes) + FixedPcdGet64(PcdCpuL1DCacheSizeBytes));
  } else if (!StrCmp(UnicodeStr, L"L2 Cache")) {
    Value = (FixedPcdGet64(PcdCpuCount) / 4) * FixedPcdGet64(PcdCpuL2CacheSizeBytes);
  } else if (!StrCmp(UnicodeStr, L"L3 Cache (SLC)")) {
    Value = FixedPcdGet64(PcdCpuL3CacheSizeBytes);
  } else {
    return EFI_NOT_FOUND;
  }

  Bytes = Value / 1024;
  Bytes32 = Bytes / 64;

  if (Bytes <= MAX_INSTALLEDSIZE_1k) {
    InputData->InstalledSize.Size = (UINT16)Bytes;
    InputData->InstalledSize.Granularity64K = 0;
    InputData->MaximumCacheSize.Size = (UINT16)Bytes;
    InputData->MaximumCacheSize.Granularity64K = 0;
    InputData->MaximumCacheSize2.Size = Bytes;
    InputData->MaximumCacheSize2.Granularity64K = 0;
    InputData->InstalledSize2.Size = Bytes;
    InputData->InstalledSize2.Granularity64K = 0;
  } else if (Bytes < MAX_INSTALLEDSIZE_64K) {
    InputData->InstalledSize.Size = (UINT16)Bytes32;
    InputData->InstalledSize.Granularity64K = 1;
    InputData->MaximumCacheSize.Size = (UINT16)Bytes32;
    InputData->MaximumCacheSize.Granularity64K = 1;
    InputData->MaximumCacheSize2.Size = Bytes32;
    InputData->MaximumCacheSize2.Granularity64K = 1;
    InputData->InstalledSize2.Size = Bytes32;
    InputData->InstalledSize2.Granularity64K = 1;
  } else {
    InputData->InstalledSize.Size = 0x7FFF;
    InputData->InstalledSize.Granularity64K = 1;
    InputData->MaximumCacheSize.Size = 0x7FFF;
    InputData->MaximumCacheSize.Granularity64K = 1;
    InputData->InstalledSize2.Size = Bytes32;
    InputData->InstalledSize2.Granularity64K = 1;
    InputData->MaximumCacheSize2.Size = Bytes32;
    InputData->MaximumCacheSize2.Granularity64K = 1;
  }
  return EFI_SUCCESS;
}

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformCache) {
  EFI_STATUS          Status;
  STR_TOKEN_INFO      *InputStrToken;
  SMBIOS_TABLE_TYPE7  *Type7Record;
  SMBIOS_TABLE_TYPE7  *InputData;
  CHAR16              *UnicodeStr;
  InputData     = (SMBIOS_TABLE_TYPE7 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;

  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
    Status = SmbiosPlatformDxeSaveHiiDefaultString (InputStrToken);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    UnicodeStr = HiiGetString(mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[0], NULL);

    Status = UpdateCacheSize(UnicodeStr, InputData);

    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type7Record,
      (VOID *)&InputData,
      sizeof (SMBIOS_TABLE_TYPE7),
      InputStrToken
    );
    if (Type7Record == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type7Record, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (Type7Record);
      return Status;
    }

    FreePool (Type7Record);
    Status = SmbiosPlatformDxeRestoreHiiDefaultString (InputStrToken);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    InputData++;
    InputStrToken++;
  }

  return EFI_SUCCESS;
}
