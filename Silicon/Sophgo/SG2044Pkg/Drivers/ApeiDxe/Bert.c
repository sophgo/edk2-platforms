/** @file
  Implementation of Boot Error Record Table (BERT) functions according to
  ACPI 6.5 specification (Section 18.3.1).

  Copyright (c) 2025, Sophgo Technologies Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Bert.h"

static BERT_CONTEXT  mBertContext;

/**
  Creates and initializes a minimal Boot Error Record Table (BERT) header.

  @param[in]  Context  Pointer to BERT_CONTEXT structure.

  @retval EFI_SUCCESS           BERT header was created successfully.
  @retval EFI_INVALID_PARAMETER Context is NULL.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for BERT header.
**/
EFI_STATUS
BertHeaderCreator (
  IN BERT_CONTEXT  *Context
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  Header;

  if (Context == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Context parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Context->BertHeader = AllocateZeroPool (sizeof (EFI_ACPI_6_5_BOOT_ERROR_RECORD_TABLE_HEADER));
  if (Context->BertHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate BERT header\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize BERT header using RISCV_ACPI_HEADER macro
  //
  Header = (EFI_ACPI_DESCRIPTION_HEADER) RISCV_ACPI_HEADER (
    EFI_ACPI_6_5_BOOT_ERROR_RECORD_TABLE_SIGNATURE,
    EFI_ACPI_6_5_BOOT_ERROR_RECORD_TABLE_HEADER,
    EFI_ACPI_6_5_BOOT_ERROR_RECORD_TABLE_REVISION
    );

  //
  // Copy header to BERT table
  //
  CopyMem (&Context->BertHeader->Header, &Header, sizeof (EFI_ACPI_DESCRIPTION_HEADER));

  //
  // Initialize table length to header size
  //
  Context->BertHeader->Header.Length = sizeof (EFI_ACPI_6_5_BOOT_ERROR_RECORD_TABLE_HEADER);

  //
  // Initialize BERT specific fields
  //
  Context->BertHeader->BootErrorRegionLength = 0;  // No error region
  Context->BertHeader->BootErrorRegion = 0;        // No error region address

  return EFI_SUCCESS;
}

BERT_CONTEXT *
GetBertContext (
  VOID
  )
{
  return &mBertContext;
}

VOID
FreeBertContextHeader (
  VOID
  )
{
  if (mBertContext.BertHeader != NULL)
    FreePool (mBertContext.BertHeader);
}

/**
  Initializes Boot Error Record Table (BERT).

  @param[in] BootErrorRegion        64-bit physical address of the Boot Error Region.
  @param[in] BootErrorRegionLength  the length in bytes of the boot error region.

  @retval EFI_SUCCESS      BERT table was initialized successfully.
  @retval EFI_DEVICE_ERROR Failed to create or initialize BERT header.
**/
EFI_STATUS
BertInitTable (
  IN  UINT64  BootErrorRegion,
  IN  UINT32  BootErrorRegionLength
  )
{
  UINT8       Checksum;
  EFI_STATUS  Status;

  //
  // Clear BERT context
  //
  ZeroMem (&mBertContext, sizeof (BERT_CONTEXT));

  Status = BertHeaderCreator (&mBertContext);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create BERT header: %r\n", __func__, Status));
    return EFI_DEVICE_ERROR;
  }

  mBertContext.BertHeader->BootErrorRegionLength = BootErrorRegionLength;
  mBertContext.BertHeader->BootErrorRegion = BootErrorRegion;

  //
  // Calculate and update checksum
  //
  Checksum = CalculateCheckSum8 (
               (UINT8 *)(mBertContext.BertHeader),
               mBertContext.BertHeader->Header.Length
               );
  mBertContext.BertHeader->Header.Checksum = Checksum;

  DEBUG ((DEBUG_INFO, "%a: Empty BERT table initialized successfully\n", __func__));
  return EFI_SUCCESS;
}
