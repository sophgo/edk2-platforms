/** @file
  Header file for Error Injection Table (EINJ) implementation.

  Copyright (c) 2025, Sophgo Technologies Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef EINJ_H_
#define EINJ_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>
#include <Protocol/FdtClient.h>
#include <IndustryStandard/Acpi65.h>
#include <Guid/Cper.h>
#include <Include/SG2044AcpiHeader.h>

#define EINJ_TABLE_SIZE                             0x4000
#define EINJ_INJECTION_ENTRY_COUNT                  9
#define EINJ_BEGIN_OPERATION_OFF                    0
#define EINJ_GET_TRIGGER_TABLE_OFF                  0x8
#define EINJ_SET_ERROR_TYPE_OFF                     0x10
#define EINJ_GET_ERROR_TYPE_OFF                     0x18
#define EINJ_EXECUTE_OPERATION_OFF                  0x20
#define EINJ_CHECK_BUSY_STATUS_OFF                  0x28
#define EINJ_GET_COMMAND_STATUS_OFF                 0x30
#define EINJ_TRIGGER_ERROR_OFF                      0x38
#define EINJ_END_OPERATION_OFF                      0x40
#define EINJ_SET_ERR_TYPE_WITH_ADDR_OFF             0x60
#define EINJ_TRIGGER_ERROR_ACTION_TABLE_OFF         0x88
#define EINJ_MEM_USED_SIZE                          0xB8

#define ACTION_END_OPERATION_VAL                    0
#define ACTION_BEGIN_OPERATION_VAL                  1
#define ACTION_EXECUTE_OPERATION_VAL                2
#define ACTION_TRIGGER_ERROR_VAL                    3
#define ACTION_CHECK_BUSY_STATUS_VAL                1

//
// EINJ context structure
//
typedef struct {
  EFI_ACPI_6_5_ERROR_INJECTION_TABLE_HEADER  *EinjHeader;
} EINJ_CONTEXT;

/**
  Initializes the Error Injection Table (EINJ).

  @retval EFI_SUCCESS      EINJ table was initialized successfully.
  @retval EFI_DEVICE_ERROR Failed to create or initialize EINJ header.
**/
EFI_STATUS
EinjInitTable (
  IN  UINTN     ErrorBlockBase,
  OUT UINT32    *MemUsedSize
  );

EINJ_CONTEXT *
GetEinjContext (
  VOID
  );

VOID
FreeEinjContextHeader (
  VOID
  );

#endif // EINJ_H_
