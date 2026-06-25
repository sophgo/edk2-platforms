/** @file
  BMC FRU helpers for SMBIOS platform drivers (SRA3-40 / SRA3-40-8 only).

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "SmbiosPlatformDxe.h"
#include "SmbiosPlatformFru.h"

STATIC IPMI_FRU_INFO_PROTOCOL  *mIpmiFruInfoProtocol = NULL;

STATIC
IPMI_FRU_INFO_PROTOCOL *
SmbiosPlatformGetFruInfoProtocol (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mIpmiFruInfoProtocol != NULL) {
    return mIpmiFruInfoProtocol;
  }

  Status = gBS->LocateProtocol (
                  &gIpmiFruInfoProtocolGuid,
                  NULL,
                  (VOID **)&mIpmiFruInfoProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "SmbiosPlatformDxe: IpmiFruInfo protocol not found %r\n",
      Status
      ));
    return NULL;
  }

  return mIpmiFruInfoProtocol;
}

/**
  Set an HII string from a BMC FRU field when the field is non-empty.

  @param[in]  StringId  HII string token to update.
  @param[in]  FieldId   FRU field selector.

  @retval TRUE   The HII string was updated from FRU.
  @retval FALSE  FRU field is empty; the HII string is unchanged.
**/
BOOLEAN
SmbiosPlatformDxeSetHiiStringFromFru (
  IN EFI_STRING_ID      StringId,
  IN IPMI_FRU_FIELD_ID  FieldId
  )
{
  IPMI_FRU_INFO_PROTOCOL  *FruInfo;
  CONST CHAR8             *FruSerial;
  CHAR16                  UnicodeStr[SMBIOS_UNICODE_STRING_MAX_LENGTH];

  FruInfo = SmbiosPlatformGetFruInfoProtocol ();
  if (FruInfo == NULL) {
    return FALSE;
  }

  FruSerial = FruInfo->GetSerialNumber (FieldId);
  if (FruSerial[0] == '\0') {
    return FALSE;
  }

  AsciiStrToUnicodeStrS (FruSerial, UnicodeStr, SMBIOS_UNICODE_STRING_MAX_LENGTH);
  HiiSetString (mSmbiosPlatformDxeHiiHandle, StringId, UnicodeStr, NULL);
  DEBUG ((DEBUG_INFO, "SmbiosPlatformDxe: FRU field %u -> %a\n", (UINT32)FieldId, FruSerial));

  return TRUE;
}
