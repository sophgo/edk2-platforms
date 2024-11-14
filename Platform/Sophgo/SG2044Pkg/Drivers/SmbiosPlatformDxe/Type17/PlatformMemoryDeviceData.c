/** @file

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2024. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE17, PlatformMemoryDevice) = {
  {                                   // Table 1
    {                                 // Hdr
      EFI_SMBIOS_TYPE_MEMORY_DEVICE,  // Type
      sizeof (SMBIOS_TABLE_TYPE17),   // Length
      SMBIOS_HANDLE_PI_RESERVED       // Handle
    },
    SMBIOS_HANDLE_MEMORY,             // array to which this module belongs
    0xFFFE,                           // no errors
    64,                               // single DIMM, no ECC is 64bits (for ecc this would be 72)
    16,                               // data width of this device (32-bits)
    0xFFFF,                                // Memory size obtained dynamically 32Gb
    MemoryFormFactorSip,	      // Memory factor
    0,                                // Not part of a set
    1,                                // Location
    0,                                // Bank 0
    MemoryTypeLpddr5,                 // DDR4
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},  // unbuffered
    0,                                // DRAM speed - requires update
    2,                                // varies between diffrent production runs
    0,                                // serial
    0,                                // asset tag
    0,                                // part number
    2,                                // rank
    0x4000,                           // ExtendedSize; (since Size < 32GB-1)
    0xffff,                           // ConfiguredMemoryClockSpeed - initialized at runtime
    0,                                // MinimumVoltage; (unknown)
    0,                                // MaximumVoltage; (unknown)
    500,                              // ConfiguredVoltage; (unknown)
    MemoryTechnologyDram,             // MemoryTechnology
    {{                                // MemoryOperatingModeCapability
      0,                              // Reserved                        :1;
      0,                              // Other                           :1;
      0,                              // Unknown                         :1;
      1,                              // VolatileMemory                  :1;
      0,                              // ByteAccessiblePersistentMemory  :1;
      0,                              // BlockAccessiblePersistentMemory :1;
      0                               // Reserved                        :10;
    }},
    0,                                // FirwareVersion
    2,                                // ModuleManufacturerID (unknown)
    0,                                // ModuleProductID (unknown)
    0,                                // MemorySubsystemControllerManufacturerID (unknown)
    0,                                // MemorySubsystemControllerProductID (unknown)
    0,                                // NonVolatileSize
    0x400000000,                      // VolatileSize - initialized at runtime
    0,                                // CacheSize
    0,                                // LogicalSize
    8533,                             // ExtendedSpeed,
    8533                              // ExtendedConfiguredMemorySpeed
  },
  {                                   // Null-terminated table
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
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformMemoryDevice) = {
  {                                                                 // Table 1
    {                                                               // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_MEMORY_DEVICE_DEVICE_LOCATOR),
      STRING_TOKEN (STR_PLATFORM_DXE_MEMORY_DEVICE_MANUFACTURER),
    },
    ADDITIONAL_STR_INDEX_2                                          // Size of Tokens array
  }
};
