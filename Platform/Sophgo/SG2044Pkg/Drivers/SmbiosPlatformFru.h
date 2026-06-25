/** @file
  BMC FRU helpers for SMBIOS platform drivers (SRA3-40 / SRA3-40-8 only).

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SMBIOS_PLATFORM_FRU_H_
#define SMBIOS_PLATFORM_FRU_H_

#include <Protocol/IpmiFruInfo.h>

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
  );

#endif // SMBIOS_PLATFORM_FRU_H_
