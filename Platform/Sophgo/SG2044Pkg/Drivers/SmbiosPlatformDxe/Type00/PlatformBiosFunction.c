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

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformBios) {
  EFI_STATUS          Status;
  STR_TOKEN_INFO      *InputStrToken;
  SMBIOS_TABLE_TYPE0  *Type0Record;
  SMBIOS_TABLE_TYPE0  *InputData;
  CHAR16               UnicodeStr[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  CHAR16               UnicodeStrTmp[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  CHAR8                value[SMBIOS_UNICODE_STRING_MAX_LENGTH];

  InputData     = (SMBIOS_TABLE_TYPE0 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;

  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
    Status = SmbiosPlatformDxeSaveHiiDefaultString (InputStrToken);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (IniGetValueBySectionAndName ("BIOS Information", "vendor", value) == 0) {
      AsciiStrToUnicodeStrS (value, UnicodeStr, SMBIOS_UNICODE_STRING_MAX_LENGTH);
      HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[0], UnicodeStr, NULL);
    }

    if (IniGetValueBySectionAndName ("BIOS Information", "version", value) == 0) {
      AsciiStrToUnicodeStrS (value, UnicodeStr, SMBIOS_UNICODE_STRING_MAX_LENGTH);
      HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[1], UnicodeStr, NULL);
    }

    if (IniGetValueBySectionAndName ("Date Time", "date", value) == 0) {
      AsciiStrToUnicodeStrS (value, UnicodeStr, SMBIOS_UNICODE_STRING_MAX_LENGTH);

      if (IniGetValueBySectionAndName ("Date Time", "time", value) == 0) {
        AsciiStrToUnicodeStrS (value, UnicodeStrTmp, SMBIOS_UNICODE_STRING_MAX_LENGTH);
      }
      StrCatS(UnicodeStr, sizeof (UnicodeStr), L" ");
      StrCatS(UnicodeStr, sizeof (UnicodeStr), UnicodeStrTmp);
      HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[2], UnicodeStr, NULL);
    }

    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type0Record,
      (VOID *)&InputData,
      sizeof (SMBIOS_TABLE_TYPE0),
      InputStrToken
      );
    if (Type0Record == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type0Record, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (Type0Record);
      return Status;
    }

    FreePool (Type0Record);
    Status = SmbiosPlatformDxeRestoreHiiDefaultString (InputStrToken);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    InputData++;
    InputStrToken++;
  }

  return EFI_SUCCESS;
}
