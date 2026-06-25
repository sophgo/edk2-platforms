/** @file
  Protocol published by IpmiFruInfoDxe after Builtin FRU is read from BMC.

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef IPMI_FRU_INFO_PROTOCOL_H_
#define IPMI_FRU_INFO_PROTOCOL_H_

#include <Uefi.h>

#define IPMI_FRU_INFO_PROTOCOL_REVISION  0x00010000

typedef enum {
  FruChassisSerialNumber = 0,
  FruBoardSerialNumber,
  FruProductSerialNumber,
  FruFieldIdMax
} IPMI_FRU_FIELD_ID;

typedef struct _IPMI_FRU_INFO_PROTOCOL IPMI_FRU_INFO_PROTOCOL;

/**
  Return a cached FRU serial number string.

  @param[in]  FieldId   Field selector.

  @return  Pointer to a null-terminated ASCII string (never NULL).
**/
typedef
CHAR8 *
(EFIAPI *IPMI_FRU_INFO_GET_SERIAL_NUMBER)(
  IN IPMI_FRU_FIELD_ID  FieldId
  );

struct _IPMI_FRU_INFO_PROTOCOL {
  UINT32                               Revision;
  IPMI_FRU_INFO_GET_SERIAL_NUMBER      GetSerialNumber;
};

extern EFI_GUID  gIpmiFruInfoProtocolGuid;

#endif // IPMI_FRU_INFO_PROTOCOL_H_
