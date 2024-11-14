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

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformCache) {
  EFI_STATUS          Status;
  STR_TOKEN_INFO      *InputStrToken;
  SMBIOS_TABLE_TYPE7  *Type7Record;
  SMBIOS_TABLE_TYPE7  *InputData;
  CHAR16              *UnicodeStr;
  CHAR8                value[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  CHAR8		      *End;
  UINT64	       Uint;
  InputData     = (SMBIOS_TABLE_TYPE7 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;

  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
    Status = SmbiosPlatformDxeSaveHiiDefaultString (InputStrToken);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    UnicodeStr = HiiGetString(mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[0], NULL);
    if (!StrCmp(UnicodeStr, L"L1 Instruction Cache")) {
      if (IniGetValueBySectionAndName ("CPU", "l1-i-cache-size", value) == 0) {
        Status = AsciiStrDecimalToUint64S (value, &End, &Uint);
        if (RETURN_ERROR (Status)) {
          return RETURN_UNSUPPORTED;
        }
        InputData->MaximumCacheSize = Uint;
        InputData->InstalledSize = Uint;
        InputData->MaximumCacheSize2 = Uint;
        InputData->InstalledSize2 = Uint;
      }
    }

    if (!StrCmp(UnicodeStr, L"L1 Data Cache")) {
      if (IniGetValueBySectionAndName ("CPU", "l1-d-cache-size", value) == 0) {
        Status = AsciiStrDecimalToUint64S (value, &End, &Uint);
        if (RETURN_ERROR (Status)) {
          return RETURN_UNSUPPORTED;
        }
        InputData->MaximumCacheSize = Uint;
        InputData->InstalledSize = Uint;
        InputData->MaximumCacheSize2 = Uint;
        InputData->InstalledSize2 = Uint;
      }
    }

    if (!StrCmp(UnicodeStr, L"L2 Cache")) {
      if (IniGetValueBySectionAndName ("CPU", "l2-cache-size", value) == 0) {
        Status = AsciiStrDecimalToUint64S (value, &End, &Uint);
        if (RETURN_ERROR (Status)) {
          return RETURN_UNSUPPORTED;
        }
        InputData->MaximumCacheSize = Uint;
        InputData->InstalledSize = Uint;
        InputData->MaximumCacheSize2 = Uint;
        InputData->InstalledSize2 = Uint;
      }
    }

    if (!StrCmp(UnicodeStr, L"L3 Cache (SLC)")) {
      if (IniGetValueBySectionAndName ("CPU", "l3-cache-size", value) == 0) {
        Status = AsciiStrDecimalToUint64S (value, &End, &Uint);
        if (RETURN_ERROR (Status)) {
          return RETURN_UNSUPPORTED;
        }
        InputData->MaximumCacheSize = Uint;
        InputData->InstalledSize = Uint;
        InputData->MaximumCacheSize2 = Uint;
        InputData->InstalledSize2 = Uint;
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
