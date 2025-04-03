/** @file
  Header file for Boot Error Record Table (BERT) initialization according to
  ACPI 6.5 specification (Section 18.3.1).

  Copyright (c) 2025, Sophgo Technologies Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef BERT_H_
#define BERT_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <IndustryStandard/Acpi65.h>
#include <Include/SG2044AcpiHeader.h>

//
// Default ACPI table information
//

//
// BERT context structure
//
typedef struct {
  EFI_ACPI_6_5_BOOT_ERROR_RECORD_TABLE_HEADER  *BertHeader;
} BERT_CONTEXT;

extern BERT_CONTEXT  mBertContext;

/**
  Creates and initializes a minimal Boot Error Record Table (BERT) header.

  This function creates an empty BERT table as a placeholder for future SoC
  implementations that may support boot error recording.

  @param[in]  Context  Pointer to BERT_CONTEXT structure.

  @retval EFI_SUCCESS           BERT header was created successfully.
  @retval EFI_INVALID_PARAMETER Context is NULL.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for BERT header.
**/
EFI_STATUS
BertHeaderCreator (
  IN BERT_CONTEXT  *Context
  );

/**
  Initializes an empty Boot Error Record Table (BERT).

  This function creates and initializes a minimal BERT table without error
  records, serving as a placeholder for future SoC implementations.

  @retval EFI_SUCCESS      BERT table was initialized successfully.
  @retval EFI_DEVICE_ERROR Failed to create or initialize BERT header.
**/
EFI_STATUS
BertInitTable (
  VOID
  );

#endif // BERT_H_
