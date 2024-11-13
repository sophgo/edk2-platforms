/** @file

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//#include <Guid/PlatformInfoHob.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#include "SmbiosPlatformDxe.h"

/**
  This function adds SMBIOS Table (Type 17) records.

  @param  RecordData                 Pointer to SMBIOS Table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS Table was successfully added.
  @retval Other                      Failed to update the SMBIOS Table.

**/
SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformMemoryDevice) {
  EFI_STATUS           Status;
  STR_TOKEN_INFO       *InputStrToken;
  SMBIOS_TABLE_TYPE17  *InputData;
  SMBIOS_TABLE_TYPE17  *Type17Record;
  CHAR16               UnicodeStr[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  CHAR8                value[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  UINT64	       Uint;
  CHAR8		       *End;
  InputData         = (SMBIOS_TABLE_TYPE17 *)RecordData;
  InputStrToken     = (STR_TOKEN_INFO *)StrToken;

  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
      Status = SmbiosPlatformDxeSaveHiiDefaultString (InputStrToken);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if (SearchValueBySectionAndName ("DDR", "vendor", value) == 0) {
              AsciiStrToUnicodeStrS (value, UnicodeStr, SMBIOS_UNICODE_STRING_MAX_LENGTH);
              HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[1], UnicodeStr, NULL);
      }

      if (SearchValueBySectionAndName ("DDR", "type", value) == 0) {
	if (!AsciiStrCmp(value, "LPDDR4x"))
		InputData->MemoryType = 0x1E; // LPDDR4

	if (!AsciiStrCmp(value, "LPDDR5x"))
		InputData->MemoryType = 0x23; // LPDDR5
      }

      if (SearchValueBySectionAndName ("DDR", "data-rate", value) == 0) {
          Status = AsciiStrDecimalToUint64S(value, &End, &Uint);
          if (RETURN_ERROR (Status)) {
            //
            // Number of hexadecimal digit characters is no more than 4.
            //
            return RETURN_UNSUPPORTED;
          }
          InputData->ExtendedSpeed = Uint;
          InputData->ExtendedConfiguredMemorySpeed = Uint;
      }

      if (SearchValueBySectionAndName ("DDR", "rank", value) == 0) {
          Status = AsciiStrDecimalToUint64S(value, &End, &Uint);
          if (RETURN_ERROR (Status)) {
            //
            // Number of hexadecimal digit characters is no more than 4.
            //
            return RETURN_UNSUPPORTED;
          }
          InputData->Attributes = Uint;
      }

      //
      // Create Table and fill up information.
      //
      SmbiosPlatformDxeCreateTable (
        (VOID *)&Type17Record,
        (VOID *)&InputData,
        sizeof (SMBIOS_TABLE_TYPE17),
        InputStrToken
        );
      if (Type17Record == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Add Table record and free pool.
      //
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
