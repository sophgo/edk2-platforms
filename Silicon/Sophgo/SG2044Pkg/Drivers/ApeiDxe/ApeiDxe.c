/** @file
  SOPHGO APEI (Advanced Platform Error Interface) Driver implementation.

  This file implements APEI initialization for SOPHGO platform, including:
  - BERT (Boot Error Record Table)
  - HEST (Hardware Error Source Table)

  Copyright (c) 2025, Sophgo Technologies Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Guid/Cper.h>
#include "Bert.h"
#include "Hest.h"
#include "Einj.h"

#define SHARED_MEMORY_BASE                 0x70101D0000ULL
#define APEI_READY_REGISTER                (SHARED_MEMORY_BASE)
#define APEI_READY_REGISTER_LEN            8
#define APEI_TABLE_INFO_BASE               (APEI_READY_REGISTER + APEI_READY_REGISTER_LEN)
#define APEI_TABLE_INFO_LEN                (0x400 - APEI_READY_REGISTER_LEN)
#define BOOT_ERROR_REGION_BASE             (APEI_TABLE_INFO_BASE + APEI_TABLE_INFO_LEN)
#define BOOT_ERROR_REGION_LEN              0x1000

#define MAX_ERROR_SOURCES                  0x100
#define BERT_INIT_DONE                     (1 << 0)
#define HEST_INIT_DONE                     (1 << 1)
#define EINJ_INIT_DONE                     (1 << 2)
#define APEI_TABLE_INFO_WRITTEN            (1 << 4)

typedef struct {
  UINT64  HestAddr;
  UINT64  EinjAddr;
  UINT16  SourceIdDdrBase;
  UINT16  SourceIdPcieBase;
  UINT32  GhesV2CountDdr;
  UINT32  GhesV2CountPcie;
  UINT32  Reserved;
} APEI_TABLE_INFO;

//
// Error section GUIDs
//
EFI_GUID gEfiPcieErrorSectionGuid = EFI_ERROR_SECTION_PCIE_GUID;
EFI_GUID gEfiPlatformMemoryErrorSectionGuid = EFI_ERROR_SECTION_PLATFORM_MEMORY_GUID;

//
// Global variables
//
EFI_ACPI_TABLE_PROTOCOL         *mAcpiTableProtocol = NULL;
EFI_ACPI_SDT_PROTOCOL           *mAcpiSdtProtocol = NULL;

//
// Add HEST, BERT and EINJ table key definitions
//
UINTN                          mHestTableKey = 0;
UINTN                          mBertTableKey = 0;
UINTN                          mEinjTableKey = 0;

/**
  Initialize HEST table and register error handlers.

  @param[in]  ErrorBlockBase  The base address to store error block and related register data
  @param[out] MemUsedSize     The byte size of the SHARED_MEMORY used by GHES

  @retval EFI_SUCCESS           HEST initialized successfully
  @retval Others                Initialization failed
**/
EFI_STATUS
InitHestTable (
  IN  UINTN     ErrorBlockBase,
  OUT UINT32    *MemUsedSize
  )
{
  EFI_STATUS   Status;
  HEST_CONTEXT *Context;

  //
  // Initialize HEST table using Hest.c implementation
  //
  Status = HestInitTable (ErrorBlockBase, MemUsedSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to initialize HEST table - %r\n", Status));
    return Status;
  }

  Context = GetHestContext();
  if (Context == NULL) {
    DEBUG ((DEBUG_ERROR, "Error, get a NULL pointer of mHestContext- %r\n", Status));
    return Status;
  }
  //
  // Install HEST table
  //
  Status = mAcpiTableProtocol->InstallAcpiTable (
                                mAcpiTableProtocol,
                                Context->HestHeader,
                                Context->HestHeader->Header.Length,
                                &mHestTableKey
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install HEST table - %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Initialize BERT table.

  @param[in] BootErrorRegion        64-bit physical address of the Boot Error Region.
  @param[in] BootErrorRegionLength  the length in bytes of the boot error region.

  @retval EFI_SUCCESS           BERT initialized successfully
  @retval Others                Initialization failed
**/
EFI_STATUS
InitBertTable (
  IN  UINT64  BootErrorRegion,
  IN  UINT32  BootErrorRegionLength
  )
{
  EFI_STATUS   Status;
  BERT_CONTEXT *Context;

  //
  // Initialize BERT table using Bert.c implementation
  //
  Status = BertInitTable (BootErrorRegion, BootErrorRegionLength);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to initialize BERT table - %r\n", Status));
    return Status;
  }

  Context = GetBertContext();

  //
  // Install BERT table
  //
  Status = mAcpiTableProtocol->InstallAcpiTable (
                                mAcpiTableProtocol,
                                Context->BertHeader,
                                Context->BertHeader->Header.Length,
                                &mBertTableKey
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install BERT table - %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Initialize EINJ table.

  @param[in]  ErrorBlockBase  The base address to place EINJ related register and data
  @param[out] MemUsedSize     The byte size of the SHARED_MEMORY used by EINJ

  @retval EFI_SUCCESS           EINJ initialized successfully
  @retval Others                Initialization failed
**/
EFI_STATUS
InitEinjTable (
  IN  UINTN     ErrorBlockBase,
  OUT UINT32    *MemUsedSize
  )
{
  EFI_STATUS   Status;
  EINJ_CONTEXT *Context;

  //
  // Initialize EINJ table using Einj.c implementation
  //
  Status = EinjInitTable (ErrorBlockBase, MemUsedSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to initialize EINJ table - %r\n", Status));
    return Status;
  }

  Context = GetEinjContext();

  //
  // Install EINJ table
  //
  Status = mAcpiTableProtocol->InstallAcpiTable (
                                mAcpiTableProtocol,
                                Context->EinjHeader,
                                Context->EinjHeader->Header.Length,
                                &mEinjTableKey
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install EINJ table - %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InitApeiTableInfoRegion (
  IN  UINTN   ShareMemTableWrittenBase,
  OUT UINTN  *ShareMemEndAdr
  )
{
  UINT8            *CurrentPtr;
  APEI_TABLE_INFO  ApeiTableInfo;

  HEST_CONTEXT  *HestContext;
  EINJ_CONTEXT  *EinjContext;

  HestContext = GetHestContext ();
  EinjContext = GetEinjContext ();

  ZeroMem (&ApeiTableInfo, sizeof(APEI_TABLE_INFO));

  CurrentPtr = (UINT8 *)ShareMemTableWrittenBase;
  CopyMem (CurrentPtr, HestContext->HestHeader, HestContext->HestHeader->Header.Length);
  ApeiTableInfo.HestAddr = (UINT64)CurrentPtr;

  CurrentPtr += HestContext->HestHeader->Header.Length;
  CopyMem (CurrentPtr, EinjContext->EinjHeader, EinjContext->EinjHeader->Header.Length);
  ApeiTableInfo.EinjAddr = (UINT64)CurrentPtr;

  *ShareMemEndAdr = (UINTN)(CurrentPtr + EinjContext->EinjHeader->Header.Length);
  ApeiTableInfo.SourceIdDdrBase = DDR_ECC_ERROR_SOURCE_ID_BASE;
  ApeiTableInfo.SourceIdPcieBase = PCIE_ERROR_SOURCE_ID_BASE;
  GetGhesV2Count ( &(ApeiTableInfo.GhesV2CountDdr), &(ApeiTableInfo.GhesV2CountPcie));

  CurrentPtr = (UINT8 *)APEI_TABLE_INFO_BASE;
  CopyMem (CurrentPtr, &(ApeiTableInfo), sizeof (APEI_TABLE_INFO));

  return EFI_SUCCESS;
}

/**
  Initialize APEI (Advanced Platform Error Interface) driver.

  @param[in]  ImageHandle   Image handle of this driver
  @param[in]  SystemTable   Pointer to the system table

  @retval EFI_SUCCESS           Initialization completed successfully
  @retval EFI_ALREADY_STARTED   The protocol has already been installed
  @retval Others                Initialization failed
**/
EFI_STATUS
EFIAPI
ApeiDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS            Status;
  UINTN                 ShareMemHestBase, ShareMemEinjBase, ShareMemTableWrittenBase;
  UINTN                 ShareMemUsedAllSize;
  UINTN                 ShareMemEndAdr = 0;
  UINT32                ShareMemUsedSizeHest = 0;
  UINT32                ShareMemUsedSizeEinj = 0;
  EFI_PHYSICAL_ADDRESS  SharedMemoryAddress = SHARED_MEMORY_BASE;

  ShareMemUsedAllSize = APEI_READY_REGISTER_LEN + APEI_TABLE_INFO_LEN + BOOT_ERROR_REGION_LEN
                        + MEM_SIZE_PER_GHES * MAX_ERROR_SOURCES + EINJ_MEM_USED_SIZE
                        + HEST_TABLE_SIZE + EINJ_TABLE_SIZE;
  //
  // Ensure memory attributes are correct
  //
  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  SharedMemoryAddress,
                  EFI_SIZE_TO_PAGES(ShareMemUsedAllSize) * EFI_PAGE_SIZE,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME  // Use write-back caching and execute protection
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to add memory space with correct attributes - %r\n", __func__, Status));
    return Status;
  }

  //
  // Set memory attributes
  //
  Status = gDS->SetMemorySpaceAttributes (
                  SharedMemoryAddress,
                  EFI_SIZE_TO_PAGES(ShareMemUsedAllSize) * EFI_PAGE_SIZE,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to set memory space attributes - %r\n", __func__, Status));
    return Status;
  }

  //
  // Locate ACPI table protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate ACPI table protocol - %r\n", Status));
    return Status;
  }

  //
  // Locate ACPI SDT protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiSdtProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate ACPI SDT protocol - %r\n", Status));
    return Status;
  }

  //
  // Initialize APEI tables
  //
  Status = InitBertTable (BOOT_ERROR_REGION_BASE, BOOT_ERROR_REGION_LEN);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to initialize BERT table - %r\n", Status));
    goto ErrInitBert;
  }

  ShareMemHestBase = ALIGN_VALUE ((BOOT_ERROR_REGION_BASE + BOOT_ERROR_REGION_LEN), 8);
  Status = InitHestTable (ShareMemHestBase, &ShareMemUsedSizeHest);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to initialize HEST table - %r\n", Status));
    goto ErrInitHest;
  }

  ShareMemEinjBase = ALIGN_VALUE ((ShareMemHestBase + ShareMemUsedSizeHest), 8);
  Status = InitEinjTable (ShareMemEinjBase, &ShareMemUsedSizeEinj);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to initialize EINJ table - %r\n", Status));
    goto ErrInitEinj;
  }

  ShareMemTableWrittenBase = ALIGN_VALUE ((ShareMemEinjBase + ShareMemUsedSizeEinj), 8);
  InitApeiTableInfoRegion (ShareMemTableWrittenBase, &ShareMemEndAdr);

  DEBUG ((DEBUG_INFO, "ShareMemUse: 0x%llx -- 0x%llx\n", SHARED_MEMORY_BASE, ShareMemEndAdr));

  MmioWrite64 (APEI_READY_REGISTER, (BERT_INIT_DONE | HEST_INIT_DONE
                                     | EINJ_INIT_DONE | APEI_TABLE_INFO_WRITTEN));

  DEBUG ((DEBUG_INFO, "APEI initialization completed successfully\n"));

  Status = EFI_SUCCESS;

ErrInitEinj:
  FreeEinjContextHeader ();

ErrInitHest:
  FreeHestContextHeader ();

ErrInitBert:
  FreeBertContextHeader ();

  return Status;
}
