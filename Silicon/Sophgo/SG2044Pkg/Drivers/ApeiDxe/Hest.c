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
  FDT_CLIENT_PROTOCOL     *FdtClient;
  CONST VOID             *Prop;
  INT32                   Node;
  EFI_STATUS              FindNodeStatus;

  //
  // Initialize RC count
  //
  mPcieRcCount = 0;

  //
  // Locate FDT Client Protocol
  //
  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate FDT Client Protocol: %r\n", __func__, Status));
    return Status;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: Starting PCIe RC configuration parsing\n", __func__));

  //
  // Find all PCIe nodes
  //
  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, "sophgo,sg2044-pcie-host", &Node);
       FindNodeStatus == EFI_SUCCESS;
       FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, "sophgo,sg2044-pcie-host", Node, &Node)) {

    //
    // Get reg property for base addresses
    //
    Status = FdtClient->GetNodeProperty (
                         FdtClient,
                         Node,
                         "reg",
                         &Prop,
                         NULL
                         );
    if (Status == EFI_NOT_FOUND) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to get reg property for RC%d\n", __func__, mPcieRcCount));
      continue;
    }

    //
    // Configure RC
    //
    mPcieRcConfig[mPcieRcCount].RcId = mPcieRcCount;
    mPcieRcConfig[mPcieRcCount].Enabled = TRUE;
    mPcieRcConfig[mPcieRcCount].PortCount = 1;  // Each RC has 1 port
    // dbi region is the first reg entry (index 0)
    mPcieRcConfig[mPcieRcCount].ConfigBase = SwapBytes64 (((CONST UINT64 *)Prop)[0]);

    DEBUG ((DEBUG_VERBOSE, "%a: Found RC%d: enabled=%d, port_count=%d, config_base=0x%lx\n",
            __func__,
            mPcieRcConfig[mPcieRcCount].RcId,
            mPcieRcConfig[mPcieRcCount].Enabled,
            mPcieRcConfig[mPcieRcCount].PortCount,
            mPcieRcConfig[mPcieRcCount].ConfigBase));

    mPcieRcCount++;
  }

  if (mPcieRcCount == 0) {
    DEBUG ((DEBUG_ERROR, "No PCIe RCs found\n"));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: Final PCIe RC count: %d\n", __func__, mPcieRcCount));
  return EFI_SUCCESS;
}

/**
  Initialize PCIe AER error source structure.

  @param[out] GhesV2       Pointer to PCIe AER structure to initialize
  @param[in]  RcConfig     Pointer to RC configuration
  @param[in]  ErrorBlock   Pointer to error status block
  @param[in]  BlockSize    Size of error status block

  @retval EFI_SUCCESS           Initialization successful
  @retval EFI_INVALID_PARAMETER Invalid parameter
**/
STATIC
EFI_STATUS
InitializePcieAerErrorSource (
  OUT EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  *GhesV2,
  IN  PCIE_RC_CONFIG                                                  *RcConfig,
  IN  VOID                                                            *ErrorBlock,
  IN  UINTN                                                           BlockSize
  )
{
  if (GhesV2 == NULL || RcConfig == NULL || ErrorBlock == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize GHES v2 basic fields
  //
  GhesV2->Type = EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_VERSION_2;
  GhesV2->SourceId = PCIE_ERROR_SOURCE_ID_BASE + RcConfig->RcId;
  GhesV2->RelatedSourceId = 0xFFFF;
  GhesV2->Flags = EFI_ACPI_6_5_ERROR_SOURCE_FLAG_FIRMWARE_FIRST;
  GhesV2->Enabled = TRUE;

  //
  // Configure error record parameters
  //
  GhesV2->NumberOfRecordsToPreAllocate = 1;
  GhesV2->MaxSectionsPerRecord = 1;
  GhesV2->MaxRawDataLength = GENERIC_HARDWARE_ERROR_BLOCK_SIZE;

  //
  // Configure error status parameters - using system memory space
  //
  GhesV2->ErrorStatusAddress.AddressSpaceId = EFI_ACPI_6_5_SYSTEM_MEMORY;
  GhesV2->ErrorStatusAddress.RegisterBitWidth = 64;
  GhesV2->ErrorStatusAddress.RegisterBitOffset = 0;
  GhesV2->ErrorStatusAddress.AccessSize = EFI_ACPI_6_5_QWORD;
  GhesV2->ErrorStatusAddress.Address = (UINT64)(UINTN)ErrorBlock;

  //
  // Configure notification
  //
  GhesV2->NotificationStructure.Type = EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_POLLED;
  GhesV2->NotificationStructure.PollInterval = 1000;  // 1 second polling interval
  GhesV2->NotificationStructure.Vector = 0;
  GhesV2->NotificationStructure.SwitchToPollingThresholdValue = 0;
  GhesV2->NotificationStructure.SwitchToPollingThresholdWindow = 0;
  GhesV2->NotificationStructure.ErrorThresholdValue = 0;
  GhesV2->NotificationStructure.ErrorThresholdWindow = 0;

  //
  // Configure error status block
  //
  GhesV2->ErrorStatusBlockLength = BlockSize;

  //
  // Configure read-ack register - using system memory space
  // Use DBI base address from RC config
  //
  GhesV2->ReadAckRegister.AddressSpaceId = EFI_ACPI_6_5_SYSTEM_MEMORY;
  GhesV2->ReadAckRegister.RegisterBitWidth = 64;
  GhesV2->ReadAckRegister.RegisterBitOffset = 0;
  GhesV2->ReadAckRegister.AccessSize = EFI_ACPI_6_5_QWORD;
  GhesV2->ReadAckRegister.Address = RcConfig->ConfigBase + PCIE_AER_CAP_BASE + PCIE_AER_ERROR_STATUS_OFFSET;
  GhesV2->ReadAckPreserve = 0xFFFFFFFF;
  GhesV2->ReadAckWrite = 0x00000001;

  return EFI_SUCCESS;
}

/**
  Initialize DDR ECC error source configuration.

  This function configures the Generic Hardware Error Source (GHES) for DDR ECC errors.
  It sets up notification type, polling interval, error status block parameters and
  error record configurations according to ACPI 6.5 specification.

  @param[out] GhesV2  Pointer to GHES v2 structure to be initialized

  @retval EFI_SUCCESS           DDR ECC error source initialized successfully
  @retval EFI_INVALID_PARAMETER GhesV2 is NULL
**/
STATIC
EFI_STATUS
InitializeDdrEccErrorSource (
  OUT EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  *GhesV2,
  IN  VOID                                                            *ErrorBlock,
  IN  UINTN                                                           BlockSize,
  IN  UINT8                                                           ControllerId
  )
{
  if (GhesV2 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Configure basic GHES v2 fields
  //
  GhesV2->Type = EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_VERSION_2;
  GhesV2->SourceId = DDR_ECC_ERROR_SOURCE_ID_BASE + ControllerId;
  GhesV2->RelatedSourceId = 0xFFFF;
  GhesV2->Flags = EFI_ACPI_6_5_ERROR_SOURCE_FLAG_FIRMWARE_FIRST;
  GhesV2->Enabled = TRUE;

  //
  // Configure error record parameters
  //
  GhesV2->NumberOfRecordsToPreAllocate = 1;
  GhesV2->MaxSectionsPerRecord = 1;
  GhesV2->MaxRawDataLength = GENERIC_HARDWARE_ERROR_BLOCK_SIZE;

  //
  // Configure error status parameters
  //
  GhesV2->ErrorStatusAddress.AddressSpaceId = EFI_ACPI_6_5_SYSTEM_MEMORY;
  GhesV2->ErrorStatusAddress.RegisterBitWidth = 64;
  GhesV2->ErrorStatusAddress.RegisterBitOffset = 0;
  GhesV2->ErrorStatusAddress.AccessSize = EFI_ACPI_6_5_QWORD;
  GhesV2->ErrorStatusAddress.Address = (UINT64)(UINTN)ErrorBlock;

  //
  // Configure notification
  //
  GhesV2->NotificationStructure.Type = EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_POLLED;
  GhesV2->NotificationStructure.PollInterval = 1000;  // 1 second polling interval
  GhesV2->NotificationStructure.Vector = 0;
  GhesV2->NotificationStructure.SwitchToPollingThresholdValue = 0;
  GhesV2->NotificationStructure.SwitchToPollingThresholdWindow = 0;
  GhesV2->NotificationStructure.ErrorThresholdValue = 0;
  GhesV2->NotificationStructure.ErrorThresholdWindow = 0;

  //
  // Configure error status block
  //
  GhesV2->ErrorStatusBlockLength = BlockSize;

  //
  // Configure read-ack register
  //
  GhesV2->ReadAckRegister.AddressSpaceId = EFI_ACPI_6_5_SYSTEM_MEMORY;
  GhesV2->ReadAckRegister.RegisterBitWidth = 64;
  GhesV2->ReadAckRegister.RegisterBitOffset = 0;
  GhesV2->ReadAckRegister.AccessSize = EFI_ACPI_6_5_QWORD;

  //
  // Calculate the correct controller address
  //
  UINT8 CfgIndex = ControllerId / 2;  // Get the configuration area index
  UINT8 CtlIndex = ControllerId % 2;  // Get the controller index (0 or 1)
  UINT64 CtlBase = DDR_CFG_BASE_ARRAY[CfgIndex] +
                   (CtlIndex == 0 ? DDR_CTL0_START_ADDRESS : DDR_CTL1_START_ADDRESS);
  GhesV2->ReadAckRegister.Address = CtlBase + DDR_ECC_STATUS_OFFSET;
  GhesV2->ReadAckPreserve = DDR_ECC_ERROR_ACK_PRESERVE;
  GhesV2->ReadAckWrite = DDR_ECC_ERROR_ACK_WRITE;

  DEBUG ((DEBUG_VERBOSE, "%a: DDR ECC error source initialized for Controller %d at 0x%lx\n",
          __func__, ControllerId, GhesV2->ErrorStatusAddress.Address));

  return EFI_SUCCESS;
}

/**
  Create GHES context for HEST table.

  This function creates Generic Hardware Error Source (GHES) structures for:
  1. PCIe Root Complex AER error sources
  2. LPDDR5x inline ECC error source

  @param[out] GhesV2       Array of GHES V2 structures
  @param[in]  NumOfGhesV2  Number of GHES structures to create

  @retval EFI_SUCCESS           GHES context created successfully
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory
  @retval Others                Other errors during initialization
**/
EFI_STATUS
GhesV2ContextForHest (
  OUT EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  GhesV2[],
  IN  UINT8                                                           NumOfGhesV2
  )
{
  EFI_STATUS  Status;
  UINT8       Index;
  VOID        *ErrorBlock;
  VOID        *CurrentBlock;
  UINT8       TotalErrorSources;
  UINT8       ValidSourceCount = 0;

  //
  // Get total number of error sources
  //
  TotalErrorSources = GetTotalErrorSources ();
  if (TotalErrorSources > NumOfGhesV2) {
    DEBUG ((DEBUG_ERROR, "%a: Not enough space for error sources\n", __func__));
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Use shared memory for error blocks
  //
  ErrorBlock = (VOID *)(UINTN)SHARED_MEMORY_BASE;
  CurrentBlock = ErrorBlock;

  DEBUG ((DEBUG_VERBOSE, "%a: Initial ErrorBlock at 0x%llx\n", __func__, (UINT64)(UINTN)ErrorBlock));
  DEBUG ((DEBUG_VERBOSE, "%a: Initial CurrentBlock at 0x%llx\n", __func__, (UINT64)(UINTN)CurrentBlock));

  //
  // Create PCIe AER error sources
  //
  for (Index = 0; Index < mPcieRcCount; Index++) {
    if (!mPcieRcConfig[Index].Enabled) {
      continue;
    }

    Status = InitializePcieAerErrorSource (
               &GhesV2[ValidSourceCount],
               &mPcieRcConfig[Index],
               CurrentBlock,
               GENERIC_HARDWARE_ERROR_BLOCK_SIZE
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to initialize AER source for RC%d: %r\n",
              __func__, mPcieRcConfig[Index].RcId, Status));
      continue;
    }

    DEBUG ((DEBUG_VERBOSE, "%a: Added PCIe error source %d with ID 0x%x at 0x%llx\n",
            __func__, ValidSourceCount, GhesV2[ValidSourceCount].SourceId,
            (UINT64)(UINTN)CurrentBlock));

    CurrentBlock = (VOID *)((UINTN)CurrentBlock + GENERIC_HARDWARE_ERROR_BLOCK_SIZE);
    ValidSourceCount++;
  }

  //
  // Create DDR ECC error sources for each controller
  //
  for (Index = 0; Index < DDR_CFG_BASE_ARRAY_SIZE * 2; Index++) {
    Status = InitializeDdrEccErrorSource (
               &GhesV2[ValidSourceCount],
               CurrentBlock,
               GENERIC_HARDWARE_ERROR_BLOCK_SIZE,
               Index
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to initialize DDR ECC error source for Controller %d: %r\n",
              __func__, Index, Status));
      continue;
    }

    DEBUG ((DEBUG_VERBOSE, "%a: Added DDR error source %d with ID 0x%x at 0x%llx\n",
            __func__, ValidSourceCount, GhesV2[ValidSourceCount].SourceId,
            (UINT64)(UINTN)CurrentBlock));

    CurrentBlock = (VOID *)((UINTN)CurrentBlock + GENERIC_HARDWARE_ERROR_BLOCK_SIZE);
    ValidSourceCount++;
  }

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
          __func__, mPcieRcCount, DDR_CFG_BASE_ARRAY_SIZE * 2,
          mPcieRcCount + (DDR_CFG_BASE_ARRAY_SIZE * 2)));

  return mPcieRcCount + (DDR_CFG_BASE_ARRAY_SIZE * 2);
}
