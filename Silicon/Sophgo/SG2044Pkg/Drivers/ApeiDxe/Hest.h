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
// Constants for table sizes
//
#define HEST_TABLE_SIZE                    0x4000
#define PCIE_MAX_ROOT_COMPLEXES            10
#define SHARED_MEMORY_BASE                 0x70101D0000ULL
#define GENERIC_HARDWARE_ERROR_BLOCK_SIZE  4096
#define PCIE_AER_CAP_BASE                  0x100  // PCIe AER Capability Base Address
#define PCIE_AER_ERROR_STATUS_OFFSET       0x30  // AER Error Status Register Offset

#define PCIE_ERROR_SOURCE_ID_BASE         0x0000  // PCIe error source id base
#define DDR_ECC_ERROR_SOURCE_ID_BASE      0x1000  // DDR ECC source id base, far from PCIe IDs
#define DDR_ECC_STATUS_OFFSET             0x10608  // DDR ECC error status register offset
#define DDR_ECC_ERROR_ACK_PRESERVE        0xFFFFFFFF
#define DDR_ECC_ERROR_ACK_WRITE           0x00000001

#define DDR_CTL0_START_ADDRESS             0x02000000
#define DDR_CTL1_START_ADDRESS             0x02400000
#define DDR_CFG_BASE_ARRAY_SIZE            16
static const UINT64 DDR_CFG_BASE_ARRAY[DDR_CFG_BASE_ARRAY_SIZE] = {
    0x6B40000000, 0x6B44000000, 0x6B50000000, 0x6B54000000,
    0x6B60000000, 0x6B64000000, 0x6B70000000, 0x6B74000000,
    0x6B80000000, 0x6B84000000, 0x6B90000000, 0x6B94000000,
    0x6BA0000000, 0x6BA4000000, 0x6BB0000000, 0x6BB4000000
};

//
// PCIe AER Error Masks
// These values are based on PCIe specification and common error handling requirements
//
#define PCIE_AER_UNCORRECTABLE_MASK ( \
          BIT0  |  /* Undefined */\
          BIT4  |  /* Data Link Protocol Error */\
          BIT5  |  /* Surprise Down Error */\
          BIT6  |  /* Undefined */\
          BIT12 |  /* Poisoned TLP */\
          BIT13 |  /* Flow Control Protocol Error */\
          BIT14 |  /* Completion Timeout */\
          BIT15 |  /* Completer Abort */\
          BIT16 |  /* Unexpected Completion */\
          BIT17 |  /* Receiver Overflow */\
          BIT18 |  /* Malformed TLP */\
          BIT19 |  /* ECRC Error */\
          BIT20 |  /* Unsupported Request Error */\
          BIT21 |  /* ACS Violation */\
          BIT22 |  /* Internal Error */\
          BIT23 |  /* MC Blocked TLP */\
          BIT24 |  /* AtomicOp Egress Blocked */\
          BIT25 |  /* TLP Prefix Blocked Error */\
          BIT26   /* Poisoned TLP Egress Blocked */\
          )

#define PCIE_AER_CORRECTABLE_MASK ( \
          BIT0  |  /* Receiver Error */\
          BIT6  |  /* Bad TLP */\
          BIT7  |  /* Bad DLLP */\
          BIT8  |  /* REPLAY_NUM Rollover */\
          BIT12 |  /* Replay Timer Timeout */\
          BIT13 |  /* Advisory Non-Fatal Error */\
          BIT14    /* Corrected Internal Error */\
          )

//
// GHES register information
//
typedef struct {
  UINT64  Base;
  UINT32  Size;
  UINT32  ErrorSourceNum;
  UINT8   Type;
  UINT64  AckReg;
  UINT64  AckPreserve;
  UINT64  AckWrite;
} GHES_REGISTER;

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
// Memory Error Section definition according to UEFI spec
//
typedef struct {
  UINT64  ValidBits;
  UINT64  ErrorStatus;
  UINT64  PhysicalAddress;    // Memory address where error occurred
  UINT64  PhysicalAddressMask;
  UINT16  Node;              // Node where memory error occurred
  UINT16  Card;
  UINT16  Module;
  UINT16  Bank;
  UINT16  Device;
  UINT16  Row;
  UINT16  Column;
  UINT16  BitPosition;
  UINT64  RequestorId;
  UINT64  ResponderId;
  UINT64  TargetId;
  UINT8   ErrorType;
  UINT8   Extended;
  UINT16  RankNumber;
  UINT16  CardHandle;
  UINT16  ModuleHandle;
} EFI_MEMORY_ERROR_SECTION;

//
// HEST context structure
//
typedef struct {
  EFI_ACPI_6_5_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *HestHeader;
} HEST_CONTEXT;

extern HEST_CONTEXT  mHestContext;
extern UINTN         mPcieRcCount;

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
  IN  UINT8                                                           NumOfGhesV2
  );

/**
  Get total number of error sources.

  @return Total number of error sources (PCIe AER + DDR ECC)
**/
UINT8
GetTotalErrorSources (
  VOID
  );

#endif // HEST_H_
