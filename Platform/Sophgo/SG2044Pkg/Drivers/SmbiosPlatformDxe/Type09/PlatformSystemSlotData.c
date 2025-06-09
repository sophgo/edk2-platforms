/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2025. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

//
// Define data for SMBIOS Type 9 Table.
//
SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE9, PlatformSystemSlot) = {
  {                                 // Table template
    {                               // Header
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS, // Type
      sizeof (SMBIOS_TABLE_TYPE9),  // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1,         // Slot Designation
    SlotTypePCIExpressGen5,         // Slot Type
    SlotDataBusWidth8X,             // Slot Data Bus Width
    SlotUsageAvailable,             // Current Usage
    SlotLengthLong,                 // Slot Length
    1,                              // Slot ID
    { 0, 0, 1},                     // Slot Characteristics 1
    { 0, 0, 1},                     // Slot Characteristics 2
    1,                              // Segment Group Number
    0,                              // Bus Number
    0,                              // Device Function Number
    SlotDataBusWidth8X,             // Data Bus Width (Base)
  }
};

//
// Define string Tokens for additional strings.
//
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformSystemSlot) = {
  {                                                              // Table template
    {                                                            // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_SYSTEM_SLOT_DESIGNATION)
    },
    ADDITIONAL_STR_INDEX_1                                       // Size of Tokens array
  },
};
