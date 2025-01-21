/** @file
  Processor Properties Topology Table (PPTT) for SG2044 platform

  This file describes the topological structure of the processor block on the
  SG2044 platform in the form as defined by ACPI PPTT table. The SG2044 platform 
  includes sixteen four-thread CPUS. Each of the CPUs include 64KB L1 Data cache,
  64KB L1 Instruction cache. Each cluster share a single L2 Cache. All clusters
  share a L3 Cache.

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - ACPI 6.5, Chapter 5, Section 5.2.30, Processor Properties Topology Table
**/

#include <Library/AcpiLib.h>
#include <Library/PcdLib.h>
#include <Library/IniParserLib.h>
#include <IndustryStandard/Acpi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/AcpiTable.h>
#include "SG2044AcpiHeader.h"

EFI_ACPI_TABLE_PROTOCOL       *mAcpiTableProtocol = NULL;
EFI_ACPI_SDT_PROTOCOL         *mAcpiSdtProtocol   = NULL;

/** Define helper macro for populating processor core information.

  @param [in] PackageId Package instance number.
  @param [in] ClusterId Cluster instance number.
  @param [in] CpuId     CPU instance number.
**/
#define PPTT_CORE_INIT(PackageId, ClusterId, CpuId)                            \
  {                                                                            \
    /* Parameters for CPU Core */                                              \
    EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR_INIT (                               \
      OFFSET_OF (TH_PPTT_CORE, DCache),     /* Length */                       \
      PPTT_PROCESSOR_CORE_FLAGS,            /* Flag */                         \
      OFFSET_OF (EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE,             \
        Package.Cluster[ClusterId]),        /* Parent */                       \
      ((PackageId << 4) | (ClusterId << 2) | CpuId),    /* ACPI Id */          \
      2                                     /* Num of private resource */      \
    ),                                                                         \
                                                                               \
    /* Offsets of the private resources */                                     \
    {                                                                          \
      OFFSET_OF (EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE,             \
        Package.Cluster[ClusterId].Core[CpuId].DCache),                        \
      OFFSET_OF (EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE,             \
        Package.Cluster[ClusterId].Core[CpuId].ICache)                         \
    },                                                                         \
                                                                               \
    /* L1 data cache parameters */                                             \
    EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE_INIT (                                   \
      PPTT_CACHE_STRUCTURE_FLAGS,           /* Flag */                         \
      0,                                                                       \
                                            /* Next level of cache */          \
      SIZE_64KB,                            /* Size */                         \
      512,                                  /* Num of sets */                  \
      2,                                    /* Associativity */                \
      PPTT_DATA_CACHE_ATTR,                 /* Attributes */                   \
      64,                                   /* Line size */                    \
      TH_PPTT_CACHE_ID(PackageId, ClusterId, CpuId, L1DataCache) /* Cache id */\
    ),                                                                         \
                                                                               \
    /* L1 instruction cache parameters */                                      \
    EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE_INIT (                                   \
      PPTT_CACHE_STRUCTURE_FLAGS,           /* Flag */                         \
      0,                                                                       \
                                            /* Next level of cache */          \
      SIZE_64KB,                            /* Size */                         \
      512,                                  /* Num of sets */                  \
      2,                                    /* Associativity */                \
      PPTT_INST_CACHE_ATTR,                 /* Attributes */                   \
      64,                                   /* Line size */                    \
      TH_PPTT_CACHE_ID(PackageId, ClusterId, CpuId, L1InstructionCache)        \
                                            /* Cache id */                     \
    ),                                                                         \
  }

/** Define helper macro for populating processor container information.

  @param [in] PackageId Package instance number.
  @param [in] ClusterId Cluster instance number.
**/
#define PPTT_CLUSTER_INIT(PackageId, ClusterId)                                \
  {                                                                            \
    /* Parameters for Cluster */                                               \
    EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR_INIT (                               \
      OFFSET_OF (TH_PPTT_CLUSTER, L2Cache),                                    \
                                            /* Length */                       \
      PPTT_PROCESSOR_CLUSTER_THREADED_FLAGS,/* Flag */                         \
      OFFSET_OF (EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE,             \
        Package.RootPackage),               /* Parent */                       \
      ((PackageId << 4) | ClusterId),       /* ACPI Id */                      \
      1                                     /* Num of private resource */      \
    ),                                                                         \
                                                                               \
    /* Offsets of the private resources */                                     \
    OFFSET_OF (EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE,               \
      Package.Cluster[ClusterId].L2Cache),                                     \
                                                                               \
    /* L2 cache parameters */                                                  \
    EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE_INIT (                                   \
      PPTT_CACHE_STRUCTURE_FLAGS,           /* Flag */                         \
      0,                                    /* Next level of cache */          \
      SIZE_2MB,                             /* Size */                         \
      2048,                                 /* Num of sets */                  \
      16,                                   /* Associativity */                \
      PPTT_UNIFIED_CACHE_ATTR,              /* Attributes */                   \
      64,                                   /* Line size */                    \
      TH_PPTT_CACHE_ID(PackageId, ClusterId, 0, L2Cache)    /* Cache id */     \
    ),                                                                         \
                                                                               \
    /* Initialize child cores */                                               \
    {                                                                          \
      PPTT_CORE_INIT (PackageId, ClusterId, 0),                                \
      PPTT_CORE_INIT (PackageId, ClusterId, 1),                                \
      PPTT_CORE_INIT (PackageId, ClusterId, 2),                                \
      PPTT_CORE_INIT (PackageId, ClusterId, 3)                                 \
    }                                                                          \
  }

#define PPTT_PACKAGE_INIT(PackageId)                                           \
  {                                                                            \
    EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR_INIT (                               \
      OFFSET_OF (SG2044_PPTT_PACKAGE, RootPackage.L3Cache),                    \
      PPTT_PROCESSOR_PACKAGE_FLAGS,                                            \
      0,                                                                       \
      0,                                                                       \
      1                                                                        \
    ),                                                                         \
                                                                               \
    /* Offsets of the private resources */                                     \
    OFFSET_OF (EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE,               \
      Package.RootPackage.L3Cache),                                            \
                                                                               \
    /* L3 cache parameters */                                                  \
    EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE_INIT (                                   \
      PPTT_CACHE_STRUCTURE_FLAGS,                    /* Flag */                \
      0,                                             /* Next level of cache */ \
      SIZE_64MB,                                     /* Size */                \
      4096,                                          /* Num of sets */         \
      255,                                           /* Associativity */       \
      PPTT_UNIFIED_CACHE_ATTR,                       /* Attributes */          \
      64,                                            /* Line size */           \
      TH_PPTT_CACHE_ID(PackageId, 0, 0, L3Cache)     /* Cache id */            \
    )                                                                          \
  }


#pragma pack(1)
typedef struct {
  TH_PPTT_PACKAGE                        RootPackage;
  TH_PPTT_CLUSTER                        Cluster[CLUSTER_COUNT];
} SG2044_PPTT_PACKAGE;

/*
 * Processor Properties Topology Table
 */
typedef struct {
  EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER  Header;
  SG2044_PPTT_PACKAGE                                      Package;
} EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE;
#pragma pack ()

STATIC EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE Pptt = {
  {
    RISCV_ACPI_HEADER (
      EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE,
      EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE,
      EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION
    )
  },

  {
    PPTT_PACKAGE_INIT (0),
    {
      PPTT_CLUSTER_INIT (0, 0),
      PPTT_CLUSTER_INIT (0, 1),
      PPTT_CLUSTER_INIT (0, 2),
      PPTT_CLUSTER_INIT (0, 3),
      PPTT_CLUSTER_INIT (0, 4),
      PPTT_CLUSTER_INIT (0, 5),
      PPTT_CLUSTER_INIT (0, 6),
      PPTT_CLUSTER_INIT (0, 7),
      PPTT_CLUSTER_INIT (0, 8),
      PPTT_CLUSTER_INIT (0, 9),
      PPTT_CLUSTER_INIT (0, 10),
      PPTT_CLUSTER_INIT (0, 11),
      PPTT_CLUSTER_INIT (0, 12),
      PPTT_CLUSTER_INIT (0, 13),
      PPTT_CLUSTER_INIT (0, 14),
      PPTT_CLUSTER_INIT (0, 15)
    }
  }
};

STATIC
UINT64
GetCacheSize(
  IN CHAR8           *IniField
  )
{
  EFI_STATUS Status;
  CHAR8      value[128];
  CHAR8      *End;
  UINT64     Uint = -1;

  if (IniGetValueBySectionAndName("CPU", IniField, value) == 0) {
    Status = AsciiStrDecimalToUint64S(value, &End, &Uint);
    if (RETURN_ERROR(Status)) {
      return RETURN_UNSUPPORTED;
    }
  }
  return Uint;
}

STATIC
VOID
PpttSetAcpiTable(
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  UINTN                                         AcpiTableHandle;
  EFI_STATUS                                    Status;
  UINT8                                         Checksum;
  UINT64					size;
  UINT16					loop0, loop1;
  UINT32					L1IcacheSize, L1DcacheSize;

  gBS->CloseEvent (Event);

  Status = IniConfIniParse (NULL);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Config INI parse fail. %r\n", Status));
    return ;
  }

  size = GetCacheSize("l3-cache-size");
  if (size >= 0) {
	Pptt.Package.RootPackage.L3Cache.Size = size;
  }
  L1IcacheSize = GetCacheSize("l1-i-cache-size");
  L1DcacheSize = GetCacheSize("l1-d-cache-size");

  size = GetCacheSize("l2-cache-size");
  if (size >= 0) {
    for (loop0 = 0; loop0 < CLUSTER_COUNT; loop0++) {
      Pptt.Package.Cluster[loop0].L2Cache.Size = size;

      for (loop1 = 0; loop1 < CORE_COUNT; loop1++) {
	if (L1IcacheSize >= 0) {
	  Pptt.Package.Cluster[loop0].Core[loop1].ICache.Size = L1IcacheSize;
	}

	if (L1DcacheSize >= 0) {
	  Pptt.Package.Cluster[loop0].Core[loop1].DCache.Size = L1DcacheSize;
	}
      }
    }
  }

  Checksum = CalculateCheckSum8 ((UINT8 *)(&Pptt), Pptt.Header.Header.Length);
  Pptt.Header.Header.Checksum = Checksum;

  AcpiTableHandle = 0;
  Status = mAcpiTableProtocol->InstallAcpiTable (
                                   mAcpiTableProtocol,
                                   &Pptt,
                                   Pptt.Header.Header.Length,
                                   &AcpiTableHandle);
  return;
}

EFI_STATUS
EFIAPI
PpttEntryPoint(
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS              Status;
  EFI_EVENT               ReadyToBootEvent;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiTableProtocol);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiSdtProtocol);
  ASSERT_EFI_ERROR (Status);

  Status = EfiCreateEventReadyToBootEx (
             TPL_NOTIFY,
             PpttSetAcpiTable,
             NULL,
             &ReadyToBootEvent
             );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "Acpi Pptt init done.\n"));

  return Status;
}
