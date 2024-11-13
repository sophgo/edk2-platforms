/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

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

#define SLC_SIZE(x)    (UINT16)(0x8000 | (((x) * (1 << 20)) / (64 * (1 << 10))))
#define SLC_SIZE_2(x)  (0x80000000 | (((x) * (1 << 20)) / (64 * (1 << 10))))

typedef enum {
  CacheModeWriteThrough = 0,  ///< Cache is write-through
  CacheModeWriteBack,         ///< Cache is write-back
  CacheModeVariesWithAddress, ///< Cache mode varies by address
  CacheModeUnknown,           ///< Cache mode is unknown
  CacheModeMax
} CACHE_OPERATION_MODE;

/**
  This function adds SMBIOS Table (Type 7) records for System Level Cache (SLC).

  @param  RecordData                 Pointer to SMBIOS Table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS Table was successfully added.
  @retval Other                      Failed to add the SMBIOS Table.

**/
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
	    if (SearchValueBySectionAndName ("CPU", "l1-i-cache-size", value) == 0) {
		    Status = AsciiStrDecimalToUint64S (value, &End, &Uint);
		    if (RETURN_ERROR (Status)) {
		      //
		      // Number of hexadecimal digit characters is no more than 4.
		      //
		      return RETURN_UNSUPPORTED;
		    }
		    InputData->MaximumCacheSize = Uint;
		    InputData->InstalledSize = Uint;
		    InputData->MaximumCacheSize2 = Uint;
		    InputData->InstalledSize2 = Uint;
	    }
    }

    if (!StrCmp(UnicodeStr, L"L1 Data Cache")) {
	    if (SearchValueBySectionAndName ("CPU", "l1-d-cache-size", value) == 0) {
		    Status = AsciiStrDecimalToUint64S (value, &End, &Uint);
		    if (RETURN_ERROR (Status)) {
		      //
		      // Number of hexadecimal digit characters is no more than 4.
		      //
		      return RETURN_UNSUPPORTED;
		    }
		    InputData->MaximumCacheSize = Uint;
		    InputData->InstalledSize = Uint;
		    InputData->MaximumCacheSize2 = Uint;
		    InputData->InstalledSize2 = Uint;
	    }
    }

    if (!StrCmp(UnicodeStr, L"L2 Cache")) {
	    if (SearchValueBySectionAndName ("CPU", "l2-cache-size", value) == 0) {
		    Status = AsciiStrDecimalToUint64S (value, &End, &Uint);
		    if (RETURN_ERROR (Status)) {
		      //
		      // Number of hexadecimal digit characters is no more than 4.
		      //
		      return RETURN_UNSUPPORTED;
		    }
		    InputData->MaximumCacheSize = Uint;
		    InputData->InstalledSize = Uint;
		    InputData->MaximumCacheSize2 = Uint;
		    InputData->InstalledSize2 = Uint;
	    }
    }

    if (!StrCmp(UnicodeStr, L"L3 Cache (SLC)")) {
	    if (SearchValueBySectionAndName ("CPU", "l3-cache-size", value) == 0) {
		    Status = AsciiStrDecimalToUint64S (value, &End, &Uint);
		    if (RETURN_ERROR (Status)) {
		      //
		      // Number of hexadecimal digit characters is no more than 4.
		      //
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
