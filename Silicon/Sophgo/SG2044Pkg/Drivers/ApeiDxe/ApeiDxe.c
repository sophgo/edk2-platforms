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

#define MAX_ERROR_SOURCES   0x1000
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
// Error handling statistics
//
typedef struct {
  UINT32 PcieAerCount;    // PCIe AER error count
  UINT32 DdrEccCount;     // DDR ECC error count
} ERROR_STATS;

ERROR_STATS mErrorStats = {0};

//
// Error handler function types
//
typedef EFI_STATUS (*HARDWARE_ERROR_HANDLER) (
  IN UINT8   *Buffer,
  IN UINT32  Length,
  IN UINT32  ErrorSeverity
  );

//
// Error source handler structure
//
typedef struct {
  EFI_GUID                *ErrorType;      // Error section type GUID
  HARDWARE_ERROR_HANDLER   Handler;         // Handler function
  CHAR8                   *Description;     // Handler description
} ERROR_SOURCE_HANDLER;

//
// Handler registration status
//
typedef struct {
  BOOLEAN PcieAerHandlerRegistered;
  BOOLEAN DdrEccHandlerRegistered;
} HANDLER_STATUS;

extern ERROR_SOURCE_HANDLER mErrorHandlers[];
extern HANDLER_STATUS mHandlerStatus;

//
// Global variables
//
HANDLER_STATUS mHandlerStatus = {
  .PcieAerHandlerRegistered = FALSE,
  .DdrEccHandlerRegistered = FALSE
};

//
// Forward declarations
//
STATIC
EFI_STATUS
HandlePcieAerError (
  IN UINT8   *Buffer,
  IN UINT32  Length,
  IN UINT32  ErrorSeverity
  );

STATIC
EFI_STATUS
HandleDdrEccError (
  IN UINT8   *Buffer,
  IN UINT32  Length,
  IN UINT32  ErrorSeverity
  );

//
// Error handlers table
//
ERROR_SOURCE_HANDLER mErrorHandlers[] = {
  {
    .ErrorType = &gEfiPcieErrorSectionGuid,
    .Handler = HandlePcieAerError,
    .Description = "PCIe AER Handler"
  },
  {
    .ErrorType = &gEfiPlatformMemoryErrorSectionGuid,
    .Handler = HandleDdrEccError,
    .Description = "DDR ECC Handler"
  },
  { NULL, NULL, NULL } // Terminator
};

//
// Add HEST and BERT table key definitions
//
UINTN                          mHestTableKey = 0;
UINTN                          mBertTableKey = 0;

/**
  Handle PCIe AER error.

  @param[in] Buffer        CPER buffer containing error data
  @param[in] Length        Length of CPER buffer
  @param[in] ErrorSeverity Error severity from CPER header

  @retval EFI_SUCCESS     Error handled successfully
  @retval Others          Error handling failed
**/
STATIC
EFI_STATUS
HandlePcieAerError (
  IN UINT8   *Buffer,
  IN UINT32  Length,
  IN UINT32  ErrorSeverity
  )
{
  EFI_STATUS                     Status;
  EFI_ERROR_SECTION_DESCRIPTOR   *Descriptor;
  EFI_PCIE_ERROR_DATA           *PcieError;

  DEBUG ((DEBUG_INFO, "%a: Processing PCIe AER error\n", __func__));

  //
  // Get error section descriptor
  //
  Descriptor = (EFI_ERROR_SECTION_DESCRIPTOR *)(Buffer +
               sizeof(EFI_COMMON_ERROR_RECORD_HEADER));

  //
  // Get PCIe error data
  //
  PcieError = (EFI_PCIE_ERROR_DATA *)((UINT8 *)Buffer + Descriptor->SectionOffset);

  //
  // Handle error based on severity
  //
  switch (ErrorSeverity) {
  case EFI_ACPI_6_5_ERROR_SEVERITY_CORRECTABLE:
    //
    // For correctable errors:
    // - Log error
    // - Update statistics
    // - Clear error status
    //
    DEBUG ((DEBUG_INFO, "PCIe correctable error on device %04x:%02x:%02x.%x\n",
            PcieError->DevBridge.Segment,
            PcieError->DevBridge.PrimaryOrDeviceBus,
            PcieError->DevBridge.Device,
            PcieError->DevBridge.Function));

    mErrorStats.PcieAerCount++;
    Status = EFI_SUCCESS;
    break;

  case EFI_ACPI_6_5_ERROR_SEVERITY_FATAL:
  case EFI_ACPI_6_5_ERROR_SEVERITY_CORRECTED:
    //
    // For fatal/uncorrectable errors:
    // - Log error
    // - Reset PCIe device if possible
    // - Update statistics
    //
    DEBUG ((DEBUG_ERROR, "PCIe fatal/uncorrectable error on device %04x:%02x:%02x.%x\n",
            PcieError->DevBridge.Segment,
            PcieError->DevBridge.PrimaryOrDeviceBus,
            PcieError->DevBridge.Device,
            PcieError->DevBridge.Function));

    // TODO: Implement PCIe device reset

    mErrorStats.PcieAerCount++;
    Status = EFI_SUCCESS;
    break;

  default:
    Status = EFI_UNSUPPORTED;
    break;
  }

  return Status;
}

/**
  Handle DDR ECC error.

  @param[in] Buffer        CPER buffer containing error data
  @param[in] Length        Length of CPER buffer
  @param[in] ErrorSeverity Error severity from CPER header

  @retval EFI_SUCCESS     Error handled successfully
  @retval Others          Error handling failed
**/
STATIC
EFI_STATUS
HandleDdrEccError (
  IN UINT8   *Buffer,
  IN UINT32  Length,
  IN UINT32  ErrorSeverity
  )
{
  EFI_STATUS                        Status;
  EFI_ERROR_SECTION_DESCRIPTOR      *Descriptor;
  EFI_MEMORY_ERROR_SECTION         *MemError;

  DEBUG ((DEBUG_INFO, "%a: Processing DDR ECC error\n", __func__));

  //
  // Get error section descriptor
  //
  Descriptor = (EFI_ERROR_SECTION_DESCRIPTOR *)(Buffer +
               sizeof(EFI_COMMON_ERROR_RECORD_HEADER));

  //
  // Get memory error data
  //
  MemError = (EFI_MEMORY_ERROR_SECTION *)((UINT8 *)Buffer +
             Descriptor->SectionOffset);

  //
  // Handle error based on severity
  //
  switch (ErrorSeverity) {
  case EFI_ACPI_6_5_ERROR_SEVERITY_CORRECTABLE:
    //
    // For correctable errors:
    // - Log error
    // - Update statistics
    // - Scrub memory if needed
    //
    DEBUG ((DEBUG_INFO, "DDR correctable ECC error at 0x%llx\n",
            MemError->PhysicalAddress));

    // TODO: Implement memory scrubbing

    mErrorStats.DdrEccCount++;
    Status = EFI_SUCCESS;
    break;

  case EFI_ACPI_6_5_ERROR_SEVERITY_FATAL:
  case EFI_ACPI_6_5_ERROR_SEVERITY_CORRECTED:
    //
    // For fatal/uncorrectable errors:
    // - Log error
    // - Offline memory page if possible
    // - Update statistics
    //
    DEBUG ((DEBUG_ERROR, "DDR fatal/uncorrectable ECC error at 0x%llx\n",
            MemError->PhysicalAddress));

    // TODO: Implement memory page offline

    mErrorStats.DdrEccCount++;
    Status = EFI_SUCCESS;
    break;

  default:
    Status = EFI_UNSUPPORTED;
    break;
  }

  return Status;
}

/**
  Initialize HEST table and register error handlers.

  @retval EFI_SUCCESS           HEST initialized successfully
  @retval Others                Initialization failed
**/
EFI_STATUS
InitHestTable (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE *GhesV2;
  UINT8 Index;
  UINT8 TotalErrorSources;

  //
  // Get total number of error sources from Hest.c
  //
  TotalErrorSources = GetTotalErrorSources ();

  //
  // Allocate memory for GHES structures
  //
  GhesV2 = AllocateZeroPool (TotalErrorSources * sizeof (EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE));
  if (GhesV2 == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate GHES structures\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create HEST header
  //
  Status = HestHeaderCreator (&mHestContext, HEST_TABLE_SIZE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create HEST header - %r\n", __func__, Status));
    FreePool (GhesV2);
    return Status;
  }

  //
  // Initialize GHES structures
  //
  Status = GhesV2ContextForHest (GhesV2, TotalErrorSources);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create GHES context - %r\n", __func__, Status));
    FreePool (GhesV2);
    return Status;
  }

  //
  // Add error source descriptors to HEST
  //
  for (Index = 0; Index < TotalErrorSources; Index++) {
    Status = HestAddErrorSourceDescriptor (
               &mHestContext,
               &GhesV2[Index],
               sizeof (EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE)
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to add error source %d - %r\n",
              __func__, Index, Status));
      FreePool (GhesV2);
      return Status;
    }
  }

  //
  // Free allocated memory
  //
  FreePool (GhesV2);

  //
  // Install HEST table
  //
  Status = mAcpiTableProtocol->InstallAcpiTable (
                                mAcpiTableProtocol,
                                mHestContext.HestHeader,
                                mHestContext.HestHeader->Header.Length,
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

  @retval EFI_SUCCESS           BERT initialized successfully
  @retval Others                Initialization failed
**/
EFI_STATUS
InitBertTable (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Initialize BERT table using Bert.c implementation
  //
  Status = BertInitTable ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to initialize BERT table - %r\n", Status));
    return Status;
  }

  //
  // Install BERT table
  //
  Status = mAcpiTableProtocol->InstallAcpiTable (
                                mAcpiTableProtocol,
                                mBertContext.BertHeader,
                                mBertContext.BertHeader->Header.Length,
                                &mBertTableKey
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install BERT table - %r\n", Status));
    return Status;
  }

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
  EFI_PHYSICAL_ADDRESS  SharedMemoryAddress = SHARED_MEMORY_BASE;

  //
  // Ensure memory attributes are correct
  //
  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeSystemMemory,
                  SharedMemoryAddress,
                  EFI_SIZE_TO_PAGES(sizeof(GHES_REGISTER) * MAX_ERROR_SOURCES) * EFI_PAGE_SIZE,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME | EFI_MEMORY_XP  // Use write-back caching and execute protection
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
                  EFI_SIZE_TO_PAGES(sizeof(GHES_REGISTER) * MAX_ERROR_SOURCES) * EFI_PAGE_SIZE,
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
  Status = InitBertTable ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to initialize BERT table - %r\n", Status));
    return Status;
  }

  Status = InitHestTable ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to initialize HEST table - %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "APEI initialization completed successfully\n"));
  return EFI_SUCCESS;
}
