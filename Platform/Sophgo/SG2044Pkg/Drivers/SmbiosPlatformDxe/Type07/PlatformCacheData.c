/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2024. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE7, PlatformCache) = {
  {                                         // Table 1
    {                                       // Header
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,    // Type
      sizeof (SMBIOS_TABLE_TYPE7),          // Length
      SMBIOS_HANDLE_PI_RESERVED             // Handle
    },
    1,
    0x180,                                  // L1 enabled, WB
    64,                                     // 64k I-cache max
    64,                                     // 64k installed
    {0,1},                                  // SRAM type
    {0,1},                                  // SRAM type
    0,                                      // speed unknown
    CacheErrorParity,                       // parity checking
    CacheTypeInstruction,                   // instruction cache
    CacheAssociativity2Way,                 // two way
    // SMBIOS 3.1.0 fields
    64,                                     //64k I-cache max
    64,                                     //64k installed
  },
  {                                         // Table 2
    {                                       // Header
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,    // Type
      sizeof (SMBIOS_TABLE_TYPE7),          // Length
      SMBIOS_HANDLE_PI_RESERVED             // Handle
    },
    1,
    0x180,                                  // L1 enabled, WB
    64,                                     // 64k D-cache max
    64,                                     // 64k installed
    {0,1},                                  // SRAM type
    {0,1},                                  // SRAM type
    0,                                      // speed unknown
    CacheErrorSingleBit,                    // ECC checking
    CacheTypeData,                          // data cache
    CacheAssociativity2Way,                 // two way
    // SMBIOS 3.1.0 fields
    64,                                     // 64k D-cache max
    64,                                     // 64k installed
  },
  {                                         // Table 3
    {                                       // Header
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,    // Type
      sizeof (SMBIOS_TABLE_TYPE7),          // Length
      SMBIOS_HANDLE_PI_RESERVED             // Handle
    },
    1,
    0x181,                                  // L2 enabled, WB
    512,                                    // 512k D-cache max
    512,                                    // 512k installed
    {0,1},                                  // SRAM type
    {0,1},                                  // SRAM type
    0,                                      // speed unknown
    CacheErrorSingleBit,                    // ECC checking
    CacheTypeUnified,                       // instruction cache
    CacheAssociativity16Way,                // 16 way associative
    // SMBIOS 3.1.0 fields
    512,                                    // 512k D-cache max
    512,                                    // 512k installed
  },
  {                                         // Table 4
    {                                       // Header
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,    // Type
      sizeof (SMBIOS_TABLE_TYPE7),          // Length
      SMBIOS_HANDLE_PI_RESERVED             // Handle
    },
    1,
    0x182,                                  // L3 enabled, WB
    1024,                                   // 1M cache max
    1024,                                   // 1M installed
    {0,1},                                  // SRAM type
    {0,1},                                  // SRAM type
    0,                                      // speed unknown
    CacheErrorSingleBit,                    // ECC checking
    CacheTypeUnified,                       // instruction cache
    CacheAssociativity8Way,                 // 8 way associative
    // SMBIOS 3.1.0 fields
    1024,                                   // 1M cache max
    1024,                                   // 1M installed
  },
  {                                         // Null-terminated table
    {
      NULL_TERMINATED_TYPE,
      0,
      0
    },
  }
};

//
// Define string Tokens for additional strings.
//
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformCache) = {
  {                                         // Table 1
    {                                       // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_CACHE_L1I)
    },
    ADDITIONAL_STR_INDEX_1                  // Size of Tokens array
  },
  {                                         // Table 2
    {                                       // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_CACHE_L1D)
    },
    ADDITIONAL_STR_INDEX_1                  // Size of Tokens array
  },
  {                                         // Table 3
    {                                       // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_CACHE_L2)
    },
    ADDITIONAL_STR_INDEX_1                  // Size of Tokens array
  },
  {                                         // Table 4
    {                                       // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_CACHE_L3)
    },
    ADDITIONAL_STR_INDEX_1                  // Size of Tokens array
  }
};
