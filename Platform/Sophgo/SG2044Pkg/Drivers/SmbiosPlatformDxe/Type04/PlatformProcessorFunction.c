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

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformProcessor) {
  EFI_STATUS          Status;
  STR_TOKEN_INFO      *InputStrToken;
  SMBIOS_TABLE_TYPE4  *Type4Record;
  SMBIOS_TABLE_TYPE4  *InputData;
  CHAR16               UnicodeStr[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  CHAR8                value[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  CHAR8		      *End;
  UINTN		       Freq;


  InputData     = (SMBIOS_TABLE_TYPE4 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;

  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
    Status = SmbiosPlatformDxeSaveHiiDefaultString (InputStrToken);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (IniGetValueBySectionAndName ("CPU", "type", value) == 0) {
      AsciiStrToUnicodeStrS (value, UnicodeStr, SMBIOS_UNICODE_STRING_MAX_LENGTH);
      HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[2], UnicodeStr, NULL);
    }

    if (IniGetValueBySectionAndName ("CPU", "serial-number", value) == 0) {
      AsciiStrToUnicodeStrS (value, UnicodeStr, SMBIOS_UNICODE_STRING_MAX_LENGTH);
      HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[3], UnicodeStr, NULL);
    }

    if (IniGetValueBySectionAndName ("CPU", "frequency", value) == 0) {
      Status = AsciiStrDecimalToUintnS (value, &End, &Freq);
      if (RETURN_ERROR (Status) || (End - value > 4)) {
        return RETURN_UNSUPPORTED;
      }
      InputData->CurrentSpeed = Freq;
    }

    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type4Record,
      (VOID *)&InputData,
      sizeof (SMBIOS_TABLE_TYPE4),
      InputStrToken
      );
    if (Type4Record == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type4Record, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (Type4Record);
      return Status;
    }

    FreePool (Type4Record);
    Status = SmbiosPlatformDxeRestoreHiiDefaultString (InputStrToken);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    InputData++;
    InputStrToken++;
  }

  return EFI_SUCCESS;
}
