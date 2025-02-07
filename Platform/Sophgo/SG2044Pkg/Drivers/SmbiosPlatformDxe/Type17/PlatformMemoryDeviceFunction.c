/** @file

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2024. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ConfigUtilsLib.h>
#include <Library/IniParserLib.h>

#include "SmbiosPlatformDxe.h"

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformMemoryDevice) {
  EFI_STATUS           Status;
  STR_TOKEN_INFO       *InputStrToken;
  SMBIOS_TABLE_TYPE17  *InputData;
  SMBIOS_TABLE_TYPE17  *Type17Record;
  CHAR16               UnicodeStr[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  CHAR8                value[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  UINT32               Size;
  UINT64	       Uint;
  CHAR8		       *End;
  InputData         = (SMBIOS_TABLE_TYPE17 *)RecordData;
  InputStrToken     = (STR_TOKEN_INFO *)StrToken;

  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
      Status = SmbiosPlatformDxeSaveHiiDefaultString (InputStrToken);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if (IniGetValueBySectionAndName ("DDR", "vendor", value) == 0) {
        AsciiStrToUnicodeStrS (value, UnicodeStr, SMBIOS_UNICODE_STRING_MAX_LENGTH);
        HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[1], UnicodeStr, NULL);
      }

      if (IniGetValueBySectionAndName ("DDR", "type", value) == 0) {
	if (!AsciiStrCmp(value, "LPDDR4x"))
	  InputData->MemoryType = 0x1E; // LPDDR4

	if (!AsciiStrCmp(value, "LPDDR5x"))
	  InputData->MemoryType = 0x23; // LPDDR5
      }

      if (IniGetValueBySectionAndName ("DDR", "data-rate", value) == 0) {
          Status = AsciiStrDecimalToUint64S(value, &End, &Uint);
          if (RETURN_ERROR (Status)) {
            return RETURN_UNSUPPORTED;
          }
          InputData->ExtendedSpeed = Uint / 1000000;
          InputData->ExtendedConfiguredMemorySpeed = Uint / 1000000;
      }

      if (IniGetValueBySectionAndName ("DDR", "rank", value) == 0) {
          Status = AsciiStrDecimalToUint64S(value, &End, &Uint);
          if (RETURN_ERROR (Status)) {
            return RETURN_UNSUPPORTED;
          }
          InputData->Attributes = Uint;
      }
      if (UpdateSmbiosFromEfuse(1, 352, 4, &Size) == 0) {
        if (((Size >> 13) & 0x1) ^ ((Size >> 12) & 0x1)) {
          UINT32 capacityBits = (Size >> 2) & 0x3;
          UINT32 dramCapacityMB;

          switch (capacityBits) {
          case 0:
            dramCapacityMB = 128 * 1024 * 1024;
            break;
          case 1:
            dramCapacityMB = 64 * 1024 * 1024;
            break;
          default:
            dramCapacityMB = 0;
            break;
          }
          if (dramCapacityMB == 0) {
            InputData->ExtendedSize = 0x4000000;
          } else {
            InputData->ExtendedSize = dramCapacityMB;
          }
        } else {
            InputData->ExtendedSize = 0x4000000;
        }
      }
      SmbiosPlatformDxeCreateTable (
        (VOID *)&Type17Record,
        (VOID *)&InputData,
        sizeof (SMBIOS_TABLE_TYPE17),
        InputStrToken
        );
      if (Type17Record == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type17Record, NULL);
      if (EFI_ERROR (Status)) {
        FreePool (Type17Record);
        return Status;
      }

      FreePool (Type17Record);
      Status = SmbiosPlatformDxeRestoreHiiDefaultString (InputStrToken);
      if (EFI_ERROR (Status)) {
        return Status;
      }

    InputData++;
    InputStrToken++;
  }
  return Status;
}
