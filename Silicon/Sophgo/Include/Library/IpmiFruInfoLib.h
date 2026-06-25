/** @file
  Read Builtin FRU (device 0) from BMC via IPMI and cache serial numbers.

  Used by IpmiFruInfoDxe only.  Other modules should consume IPMI_FRU_INFO_PROTOCOL.

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef IPMI_FRU_INFO_LIB_H_
#define IPMI_FRU_INFO_LIB_H_

#include <Protocol/IpmiFruInfo.h>

/**
  Load Builtin FRU from BMC on first call and return a cached field string.

  @param[in]  FieldId   Field selector.

  @return  Pointer to a null-terminated ASCII string (never NULL).
**/
CHAR8 *
EFIAPI
IpmiFruInfoGet (
  IN IPMI_FRU_FIELD_ID  FieldId
  );

#endif // IPMI_FRU_INFO_LIB_H_
