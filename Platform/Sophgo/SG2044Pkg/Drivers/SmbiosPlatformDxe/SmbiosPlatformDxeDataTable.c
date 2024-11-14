/** @file
  This file provides SMBIOS Type.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2023 - 2024, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  Copyright (c) 2024. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE0,
  PlatformBios
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE1,
  PlatformSystem
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE2,
  PlatformBoard
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE3,
  PlatformChassis
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE4,
  PlatformProcessor
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE7,
  PlatformCache
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE13,
  PlatformLanguage
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE16,
  PlatformPhysicalMemoryArray
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE17,
  PlatformMemoryDevice
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE19,
  PlatformMemoryArrayMappedAddress
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE32,
  PlatformSystemBoot
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE38,
  PlatformIpmiDevice
  )
SMBIOS_PLATFORM_DXE_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE44,
  PlatformProcessorAdditional
  )

SMBIOS_PLATFORM_DXE_DATA_TABLE mSmbiosPlatformDxeDataTable[] = {
  // Type0
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformBios
    ),
  // Type1
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformSystem
    ),
  // Type2
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformBoard
    ),
  // Type3
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformChassis
    ),
  // Type4
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformProcessor
    ),
  // Type7
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformCache
    ),
  // Type13
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformLanguage
    ),
  // Type16
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformPhysicalMemoryArray
    ),
  // Type17
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformMemoryDevice
    ),
  // Type19
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformMemoryArrayMappedAddress
    ),
  // Type32
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformSystemBoot
    ),
  // Type38
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformIpmiDevice
    ),
  // Type44
  SMBIOS_PLATFORM_DXE_TABLE_ENTRY_DATA_AND_FUNCTION (
    PlatformProcessorAdditional
    )
};

//
// Number of Data Table entries.
//
UINTN  mSmbiosPlatformDxeDataTableEntries = ARRAY_SIZE (mSmbiosPlatformDxeDataTable);
