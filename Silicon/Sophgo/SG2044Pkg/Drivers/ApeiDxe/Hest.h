/** @file
  Header file for HEST (Hardware Error Source Table) implementation.

  Copyright (c) 2025, Sophgo Technologies Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef HEST_H_
#define HEST_H_

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

//
//  Memory layout of MEM_SIZE_PER_GHES:
//     +-------------------------------+ <-+
// Low |Generic Error Status Block     |   |
//  |  |size: MEM_SIZE_PER_GHES -      |   |
//  |  |      GHES_READ_ACK_REG_LEN -  |   |
//  |  |      GHES_ERR_STATUS_REG_LEN  |   |
//  |  |                               |   |
//  |  |                               |   |
//  |  +-------------------------------+   |
//  |  |GHESv2.ErrorStatusAdr.Adr      |---+
//  |  |size: GHES_ERR_STATUS_REG_LEN  |
//  |  +-------------------------------+
//  V  |GHESv2.ReadAckReg.Adr          |
// High|size: GHES_READ_ACK_REG_LEN    |
//     +-------------------------------+
//
#define HEST_TABLE_SIZE                    0x4000
#define PCIE_MAX_ROOT_COMPLEXES            10
#define MEM_SIZE_PER_GHES                  0x1000  // The size of memory space one GHES used for storing
                                                   // error blocks and registers.
#define GHES_READ_ACK_REG_LEN              8
#define GHES_READ_ACK_REG_PRESERVE         0
#define GHES_READ_ACK_REG_WRITE_VALUE      0x00000001
#define GHES_ERR_STATUS_REG_LEN            8
#define GHES_ERR_STATUS_BLOCK_MAX_SIZE     (MEM_SIZE_PER_GHES - GHES_READ_ACK_REG_LEN - GHES_ERR_STATUS_REG_LEN)

#define PCIE_ERROR_SOURCE_ID_BASE          0x0000  // PCIe error source id base
#define DDR_ECC_ERROR_SOURCE_ID_BASE       0x1000  // DDR ECC source id base, far from PCIe IDs

#define DDR_CTL0_START_ADDRESS             0x02000000
#define DDR_CTL1_START_ADDRESS             0x02400000
#define DDR_CFG_BASE_ARRAY_SIZE            16

//
// PCIe Root Complex configuration
//
typedef struct {
  UINT8   RcId;           ///< Root Complex ID
  BOOLEAN Enabled;        ///< Whether this RC is enabled
  UINT8   PortCount;      ///< Number of ports
  UINT8   IntxVector;     ///< INTx interrupt vector
  UINT64  AerBaseAddr;    ///< AER register base address
  UINT64  ConfigBase;     ///< PCIe RC configuration space base address
} PCIE_RC_CONFIG;

//
// HEST context structure
//
typedef struct {
  EFI_ACPI_6_5_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *HestHeader;
} HEST_CONTEXT;

EFI_STATUS
HestHeaderCreator (
  IN OUT HEST_CONTEXT  *Context,
  IN     UINT32        TableSize
  );

EFI_STATUS
HestAddErrorSourceDescriptor (
  IN OUT HEST_CONTEXT  *Context,
  IN     VOID          *ErrorSource,
  IN     UINT32        ErrorSourceSize
  );

EFI_STATUS
GhesV2ContextForHest (
  OUT EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  GhesV2[],
  IN  UINT8                                                           NumOfGhesV2,
  IN  UINTN                                                           ErrorBlockBase,
  OUT UINT32                                                          *MemUsedSize
  );

/**
  Get total number of error sources.

  @return Total number of error sources (PCIe AER + DDR ECC)
**/
UINT8
GetTotalErrorSources (
  VOID
  );

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
  );

HEST_CONTEXT *
GetHestContext (
  VOID
  );

VOID
GetGhesV2Count (
  OUT  UINT32  *DdrCount,
  OUT  UINT32  *PcieCount
  );

VOID
FreeHestContextHeader (
  VOID
  );

#endif // HEST_H_
