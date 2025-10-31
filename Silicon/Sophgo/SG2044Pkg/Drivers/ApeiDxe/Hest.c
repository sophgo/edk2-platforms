/** @file
  HEST (Hardware Error Source Table) implementation for SOPHGO SG2044 platform.

  This file implements HEST table creation and error source handling according to
  ACPI 6.5 specification, including:
  - PCIe Root Complex AER error sources
  - LPDDR5x inline ECC error source

  Copyright (c) 2025, Sophgo Technologies Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Hest.h"
#include <Include/PcieHostPcd.h>
PCIE_RC_CONFIG  mPcieRcConfig[PCIE_MAX_ROOT_COMPLEXES];
UINTN           mPcieRcCount = 0;
HEST_CONTEXT    mHestContext;
BOOLEAN         mPcieConfigParsed = FALSE;


/**
  Parse PCIe Root Complex configuration from conf.ini.

  This function reads PCIe configuration from conf.ini to support different PCIe topologies.
  Each RC section contains:
  - width: Number of lanes per port (4 or 8)
  - Memory and IO window configurations

  If no PCIe sections found in conf.ini, default to 5 ports.

  @retval EFI_SUCCESS          Configuration parsed successfully
  @retval EFI_OUT_OF_RESOURCES Too many RCs configured
  @retval EFI_INVALID_PARAMETER Invalid configuration parameters
**/
STATIC
EFI_STATUS
ParsePcieRcConfig (
  VOID
  )
{
  EFI_STATUS              Status;
  CONST VOID             *Prop;
  INT32                   Node, Index;
  EFI_STATUS              FindNodeStatus;
  PCIE_HOST_BRIDGE_TABLE  *PcieRcConfig;

  PcieRcConfig  = (PCIE_HOST_BRIDGE_TABLE *)PcdGetPtr (PcdPcieHostBridgeTable);
  if (PcieRcConfig == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] No PCIe host bridge configuration found\n", __func__));
    return EFI_NOT_FOUND;
  }

  mPcieRcCount = PcieRcConfig->NumOfControllers;
  if (mPcieRcCount > PCIE_MAX_ROOT_COMPLEXES) {
    DEBUG ((DEBUG_ERROR, "Too many PCIe controllers, only %d supported\n", PCIE_MAX_ROOT_COMPLEXES));
    return EFI_OUT_OF_RESOURCES;
  }
  if (mPcieRcCount == 0) {
    DEBUG ((DEBUG_ERROR, "No PCIe RCs found\n"));
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < mPcieRcCount; ++Index) {
    mPcieRcConfig[Index].RcId = Index;
    mPcieRcConfig[Index].Enabled = TRUE;
    mPcieRcConfig[Index].PortCount = 1;  // Each RC has 1 port
    DEBUG ((DEBUG_INFO, "RC%d: enabled=%d, port_count=%d\n",
            mPcieRcConfig[Index].RcId,
            mPcieRcConfig[Index].Enabled,
            mPcieRcConfig[Index].PortCount));
  }

  DEBUG ((DEBUG_VERBOSE, "%a: Final PCIe RC count: %d\n", __func__, mPcieRcCount));
  return EFI_SUCCESS;
}

/**
  Initialize generic hardware error source configuration.

  This function configures the Generic Hardware Error Source version 2 (GHESv2)
  with the given notification type ccording to ACPI 6.5 specification.

  @param[out] GhesV2        Pointer to GHES v2 structure to be initialized
  @param[in]  ErrorBlock    Pointer to the memory used for storing error blocks,
                            read ack register, and error status address
  @param[in]  SourceId      Uniquely identify the error source.
  @param[in]  Notification  Pointer to the hardware error notification structure

  @retval EFI_SUCCESS           Error source initialized successfully
  @retval EFI_INVALID_PARAMETER GhesV2 is NULL
**/
STATIC
EFI_STATUS
InitializeErrorSourceStruct (
  OUT EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  *GhesV2,
  IN  VOID                                                            *ErrorBlock,
  IN  UINT16                                                           SourceId,
  IN  EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_STRUCTURE              *Notification
  )
{
  if (GhesV2 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Configure basic GHES v2 fields
  //
  GhesV2->Type = EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_VERSION_2;
  GhesV2->SourceId = SourceId;
  GhesV2->RelatedSourceId = 0xFFFF;
  GhesV2->Flags = 0;
  GhesV2->Enabled = TRUE;

  //
  // Configure error record parameters
  //
  GhesV2->NumberOfRecordsToPreAllocate = 1;
  GhesV2->MaxSectionsPerRecord = 2;
  GhesV2->MaxRawDataLength = GHES_ERR_STATUS_BLOCK_MAX_SIZE;

  //
  // Configure error status address parameters
  //
  GhesV2->ErrorStatusAddress.AddressSpaceId = EFI_ACPI_6_5_SYSTEM_MEMORY;
  GhesV2->ErrorStatusAddress.RegisterBitWidth = 64;
  GhesV2->ErrorStatusAddress.RegisterBitOffset = 0;
  GhesV2->ErrorStatusAddress.AccessSize = EFI_ACPI_6_5_QWORD;
  GhesV2->ErrorStatusAddress.Address = (UINT64)(UINTN)(ErrorBlock + GHES_ERR_STATUS_BLOCK_MAX_SIZE);
  MmioWrite64 (GhesV2->ErrorStatusAddress.Address, (UINT64)(UINTN)ErrorBlock);

  //
  // Clear ErrorBlock.BlockStatus to initialize ErrorBlock memory region
  //
  ZeroMem (ErrorBlock, sizeof(EFI_ACPI_6_5_ERROR_BLOCK_STATUS));

  //
  // Configure notification
  //
  CopyMem (&(GhesV2->NotificationStructure), Notification, sizeof(EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_STRUCTURE));

  //
  // Configure error status block
  //
  GhesV2->ErrorStatusBlockLength = GHES_ERR_STATUS_BLOCK_MAX_SIZE;

  //
  // Configure read-ack register
  //
  GhesV2->ReadAckRegister.AddressSpaceId = EFI_ACPI_6_5_SYSTEM_MEMORY;
  GhesV2->ReadAckRegister.RegisterBitWidth = 64;
  GhesV2->ReadAckRegister.RegisterBitOffset = 0;
  GhesV2->ReadAckRegister.AccessSize = EFI_ACPI_6_5_QWORD;

  GhesV2->ReadAckRegister.Address = (UINT64)(UINTN)(ErrorBlock + MEM_SIZE_PER_GHES - GHES_READ_ACK_REG_LEN);
  GhesV2->ReadAckPreserve = GHES_READ_ACK_REG_PRESERVE;
  GhesV2->ReadAckWrite = GHES_READ_ACK_REG_WRITE_VALUE;

  //
  // Initialize ReadAckRegister
  //
  MmioWrite64(GhesV2->ReadAckRegister.Address, GhesV2->ReadAckWrite);

  return EFI_SUCCESS;
}

/**
  Create GHES context for HEST table.

  This function creates Generic Hardware Error Source (GHES) structures for:
  1. PCIe Root Complex AER error sources
  2. LPDDR5x inline ECC error source

  @param[out] GhesV2          Array of GHES V2 structures
  @param[in]  NumOfGhesV2     Number of GHES structures to create
  @param[in]  ErrorBlockBase  The base address to store error block and related register data
  @param[out] MemUsedSize     The byte size of the SHARED_MEMORY used by GHES

  @retval EFI_SUCCESS           GHES context created successfully
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory
  @retval Others                Other errors during initialization
**/
EFI_STATUS
GhesV2ContextForHest (
  OUT EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  GhesV2[],
  IN  UINT8                                                           NumOfGhesV2,
  IN  UINTN                                                           ErrorBlockBase,
  OUT UINT32                                                          *MemUsedSize
  )
{
  EFI_STATUS  Status;
  UINT8       Index;
  VOID        *ErrorBlock;
  VOID        *CurrentBlock;
  UINT8       TotalErrorSources;
  UINT8       ValidSourceCount = 0;
  EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_STRUCTURE *Notification;

  //
  // Get total number of error sources
  //
  TotalErrorSources = GetTotalErrorSources ();
  if (TotalErrorSources > NumOfGhesV2) {
    DEBUG ((DEBUG_ERROR, "%a: Not enough space for error sources\n", __func__));
    return EFI_BUFFER_TOO_SMALL;
  }

  ErrorBlock = (VOID *)ErrorBlockBase;
  CurrentBlock = ErrorBlock;

  DEBUG ((DEBUG_VERBOSE, "%a: Initial ErrorBlock at 0x%llx\n", __func__, (UINT64)(UINTN)ErrorBlock));
  DEBUG ((DEBUG_VERBOSE, "%a: Initial CurrentBlock at 0x%llx\n", __func__, (UINT64)(UINTN)CurrentBlock));

  Notification = (EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_STRUCTURE *) AllocateZeroPool (sizeof(EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_STRUCTURE));

  Notification->Type = EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_GSIV;
  Notification->Length = sizeof(EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_STRUCTURE);

  //
  // Create DDR ECC error sources for each controller
  //
  for (Index = 0; Index < DDR_CFG_BASE_ARRAY_SIZE; Index++) {
    Status = InitializeErrorSourceStruct (
               &GhesV2[ValidSourceCount],
               CurrentBlock,
               DDR_ECC_ERROR_SOURCE_ID_BASE + Index,
               Notification
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to initialize DDR ECC error source for Controller %d: %r\n",
              __func__, Index, Status));
      continue;
    }

    DEBUG ((DEBUG_VERBOSE, "%a: DDR ECC error source initialized for Controller %d at 0x%lx\n",
      __func__, Index, GhesV2[ValidSourceCount].ErrorStatusAddress.Address));

    DEBUG ((DEBUG_VERBOSE, "%a: Added DDR error source %d with ID 0x%x at 0x%llx\n",
            __func__, ValidSourceCount, GhesV2[ValidSourceCount].SourceId,
            (UINT64)(UINTN)CurrentBlock));

    CurrentBlock = (VOID *)((UINTN)CurrentBlock + MEM_SIZE_PER_GHES);
    ValidSourceCount++;
  }

  ZeroMem (Notification, sizeof (*Notification));
  Notification->Type = EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_POLLED;
  Notification->Length = sizeof(EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_STRUCTURE);
  Notification->PollInterval = 20000;

  //
  // Create PCIe AER error sources
  //
  for (Index = 0; Index < mPcieRcCount; Index++) {
    if (!mPcieRcConfig[Index].Enabled) {
      continue;
    }
    Status = InitializeErrorSourceStruct (
              &GhesV2[ValidSourceCount],
              CurrentBlock,
              PCIE_ERROR_SOURCE_ID_BASE + Index,
              Notification
              );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to initialize AER source for RC%d: %r\n",
              __func__, mPcieRcConfig[Index].RcId, Status));
      continue;
    }

    DEBUG ((DEBUG_VERBOSE, "%a: Added PCIe error source %d with ID 0x%x at 0x%llx\n",
            __func__, ValidSourceCount, GhesV2[ValidSourceCount].SourceId,
            (UINT64)(UINTN)CurrentBlock));

    CurrentBlock = (VOID *)((UINTN)CurrentBlock + MEM_SIZE_PER_GHES);
    ValidSourceCount++;
  }

  *MemUsedSize = (UINT32)((UINTN)CurrentBlock - ErrorBlockBase);

  DEBUG ((DEBUG_VERBOSE, "%a: Final CurrentBlock at 0x%llx\n", __func__, (UINT64)(UINTN)CurrentBlock));
  DEBUG ((DEBUG_VERBOSE, "%a: Initialized %d valid error sources out of %d total\n",
          __func__, ValidSourceCount, TotalErrorSources));

  return EFI_SUCCESS;
}

/**
  Creates and initializes HEST header.

  @param[in,out] Context    Pointer to HEST context
  @param[in]     TableSize  Size of HEST table

  @retval EFI_SUCCESS           HEST header created successfully
  @retval EFI_INVALID_PARAMETER Invalid Context pointer
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory
**/
EFI_STATUS
HestHeaderCreator (
  IN OUT HEST_CONTEXT  *Context,
  IN     UINT32        TableSize
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  Header;

  if (Context == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Context parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Context->HestHeader = AllocateZeroPool (TableSize);
  if (Context->HestHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate HEST header\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize HEST header using RISCV_ACPI_HEADER macro
  //
  Header = (EFI_ACPI_DESCRIPTION_HEADER) RISCV_ACPI_HEADER (
    EFI_ACPI_6_5_HARDWARE_ERROR_SOURCE_TABLE_SIGNATURE,
    EFI_ACPI_6_5_HARDWARE_ERROR_SOURCE_TABLE_HEADER,
    EFI_ACPI_6_5_HARDWARE_ERROR_SOURCE_TABLE_REVISION
    );

  //
  // Copy header to HEST table
  //
  CopyMem (&Context->HestHeader->Header, &Header, sizeof (EFI_ACPI_DESCRIPTION_HEADER));

  //
  // Initialize table length to header size
  //
  Context->HestHeader->Header.Length = sizeof (EFI_ACPI_6_5_HARDWARE_ERROR_SOURCE_TABLE_HEADER);

  //
  // Get total error sources and initialize count
  //
  Context->HestHeader->ErrorSourceCount = 0;  // Will be incremented as sources are added

  return EFI_SUCCESS;
}

/**
  Adds an error source descriptor to HEST.

  @param[in,out] Context          Pointer to HEST context
  @param[in]     ErrorSource     Pointer to error source descriptor
  @param[in]     ErrorSourceSize Size of error source descriptor

  @retval EFI_SUCCESS           Error source added successfully
  @retval EFI_INVALID_PARAMETER Invalid parameter
  @retval EFI_BUFFER_TOO_SMALL  Not enough space in HEST
**/
EFI_STATUS
HestAddErrorSourceDescriptor (
  IN OUT HEST_CONTEXT  *Context,
  IN     VOID          *ErrorSource,
  IN     UINT32        ErrorSourceSize
  )
{
  UINT8   *CurrentPtr;
  UINT32  NewLength;

  if (Context == NULL || ErrorSource == NULL || ErrorSourceSize == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate new table length
  //
  NewLength = Context->HestHeader->Header.Length + ErrorSourceSize;
  if (NewLength > HEST_TABLE_SIZE) {
    DEBUG ((DEBUG_ERROR, "%a: HEST table size exceeded\n", __func__));
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Add error source descriptor
  //
  CurrentPtr = (UINT8 *)Context->HestHeader + Context->HestHeader->Header.Length;
  CopyMem (CurrentPtr, ErrorSource, ErrorSourceSize);

  //
  // Update HEST header
  //
  Context->HestHeader->Header.Length = NewLength;
  Context->HestHeader->ErrorSourceCount++;

  //
  // Calculate and update checksum
  //
  Context->HestHeader->Header.Checksum = CalculateCheckSum8 (
                                          (UINT8 *)Context->HestHeader,
                                          Context->HestHeader->Header.Length
                                          );

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Added error source %d, new length: %d\n",
    __func__,
    Context->HestHeader->ErrorSourceCount,
    NewLength
    ));

  return EFI_SUCCESS;
}

/**
  Get total number of error sources.

  @return Total number of error sources (PCIe AER + DDR ECC)
**/
UINT8
GetTotalErrorSources (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Parse PCIe configuration only once
  //
  if (!mPcieConfigParsed) {
    Status = ParsePcieRcConfig ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to parse PCIe config - %r\n", __func__, Status));
      return 0;
    }
    mPcieConfigParsed = TRUE;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: Returning total error sources: PCIe RC=%d + DDR=%d = %d\n",
          __func__, mPcieRcCount, DDR_CFG_BASE_ARRAY_SIZE,
          mPcieRcCount + DDR_CFG_BASE_ARRAY_SIZE));

  return mPcieRcCount + DDR_CFG_BASE_ARRAY_SIZE;
}

HEST_CONTEXT *
GetHestContext (
  VOID
  )
{
  return &mHestContext;
}

VOID
GetGhesV2Count (
  OUT  UINT32  *DdrCount,
  OUT  UINT32  *PcieCount
  )
{
  *DdrCount = DDR_CFG_BASE_ARRAY_SIZE;
  *PcieCount = mPcieRcCount;
}

VOID
FreeHestContextHeader (
  VOID
  )
{
  if (mHestContext.HestHeader != NULL)
    FreePool (mHestContext.HestHeader);
}

/**
  Initialize HEST table and register error handlers.

  @param[in]  ErrorBlockBase  The base address to store error block and related register data
  @param[out] MemUsedSize     The byte size of the SHARED_MEMORY used by GHES

  @retval EFI_SUCCESS           HEST initialized successfully
  @retval Others                Initialization failed
**/
EFI_STATUS
HestInitTable (
  IN  UINTN     ErrorBlockBase,
  OUT UINT32    *MemUsedSize
  )
{
  EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE *GhesV2;
  UINT8       Index;
  UINT8       TotalErrorSources;
  EFI_STATUS  Status;

  if (IS_ALIGNED (ErrorBlockBase, 8) == 0){
    DEBUG ((DEBUG_ERROR, "%a: Invalid ErrorBlockBase: 0x%llx\n", __func__, ErrorBlockBase));
    DEBUG ((DEBUG_ERROR, "%a: ErrorBlockBase must be aligned on 8-byte boundaries!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

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
    goto InitError;
  }

  //
  // Initialize GHES structures
  //
  Status = GhesV2ContextForHest (GhesV2, TotalErrorSources, ErrorBlockBase, MemUsedSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create GHES context - %r\n", __func__, Status));
    goto InitError;
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
      goto InitError;
    }
  }

  //
  // Free allocated memory
  //
  FreePool (GhesV2);

  return EFI_SUCCESS;

InitError:
  FreePool (GhesV2);
  FreeHestContextHeader ();

  return Status;
}
