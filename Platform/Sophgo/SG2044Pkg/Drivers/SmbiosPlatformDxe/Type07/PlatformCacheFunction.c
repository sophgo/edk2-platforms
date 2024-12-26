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

#include "SmbiosPlatformDxe.h"
EFI_STATUS
UpdateCacheSize(
  IN CHAR16          *UnicodeStr,
  IN CHAR8           *IniField,
  SMBIOS_TABLE_TYPE7  *InputData
  )
{
  EFI_STATUS Status;
  CHAR8      value[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  CHAR8      *End;
  UINT64     Uint;
  UINT16     InstalledSizeValue;
  UINT64     Bytes;
  UINT32     Bytes32;

  if (IniGetValueBySectionAndName("CPU", IniField, value) == 0) {
    Status = AsciiStrDecimalToUint64S(value, &End, &Uint);
    if (RETURN_ERROR(Status)) {
      return RETURN_UNSUPPORTED;
    }

    Bytes = Uint / 1024;
    Bytes32 = (Uint > MAX_UINT32) ? MAX_UINT32 : (UINT32)Uint;
    if (Bytes <= 0x7FFF) {
      InstalledSizeValue = (UINT16)Bytes;
    } else {
      UINT64 Increments = Bytes / 64;
      if (Increments > 0x7FFF) {
        Increments = 0x7FFF;
      }
      InstalledSizeValue = (UINT16)(0x8000 | (UINT16)Increments);
    }

    InputData->MaximumCacheSize  = InstalledSizeValue;
    InputData->InstalledSize     = InstalledSizeValue;
    InputData->MaximumCacheSize2 = Bytes32;
    InputData->InstalledSize2    = Bytes32;

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
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
    if (!StrCmp(UnicodeStr, L"L1 Instruction Cache")) {
      Status = UpdateCacheSize(UnicodeStr, "l1-i-cache-size", InputData);
      if (Status == RETURN_UNSUPPORTED) {
        return Status;
      }
    }

    if (!StrCmp(UnicodeStr, L"L1 Data Cache")) {
      Status = UpdateCacheSize(UnicodeStr, "l1-d-cache-size", InputData);
      if (Status == RETURN_UNSUPPORTED) {
        return Status;
      }
    }

    if (!StrCmp(UnicodeStr, L"L2 Cache")) {
      Status = UpdateCacheSize(UnicodeStr, "l2-cache-size", InputData);
      if (Status == RETURN_UNSUPPORTED) {
        return Status;
      }
    }

    if (!StrCmp(UnicodeStr, L"L3 Cache (SLC)")) {
      Status = UpdateCacheSize(UnicodeStr, "l3-cache-size", InputData);
      if (Status == RETURN_UNSUPPORTED) {
        return Status;
      }
    }

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
