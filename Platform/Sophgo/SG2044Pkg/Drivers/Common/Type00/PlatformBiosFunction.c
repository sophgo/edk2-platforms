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

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformBios) {
  EFI_STATUS          Status;
  STR_TOKEN_INFO      *InputStrToken;
  SMBIOS_TABLE_TYPE0  *Type0Record;
  SMBIOS_TABLE_TYPE0  *InputData;
  CHAR16               UnicodeStrDate[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  CHAR16              *BiosVersion;
  CONST CHAR8         *ReleaseDate = __DATE__;

  InputData     = (SMBIOS_TABLE_TYPE0 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;

  //
  // BIOS Version comes from PcdBiosVersionString (set in the DSC), so it is
  // part of the firmware image itself and stays correct regardless of the
  // boot medium (SPI flash or SD card). The Setup Information page derives its
  // value from this SMBIOS Type0 record, so both stay in sync from one source.
  //
  BiosVersion = (CHAR16 *)PcdGetPtr (PcdBiosVersionString);

  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
    Status = SmbiosPlatformDxeSaveHiiDefaultString (InputStrToken);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((BiosVersion == NULL) || (BiosVersion[0] == L'\0')) {
      HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[1], L"Not set", NULL);
    } else {
      HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[1], BiosVersion, NULL);
    }

    AsciiStrToUnicodeStrS (ReleaseDate, UnicodeStrDate, SMBIOS_UNICODE_STRING_MAX_LENGTH);
    HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[2], UnicodeStrDate, NULL);
    HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[0], L"SOPHGO", NULL);

    InputData->BiosSize = 0xFF;
    InputData->ExtendedBiosSize.Size = 0x10;
    InputData->ExtendedBiosSize.Unit = 0x0;

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
