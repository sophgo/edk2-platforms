/** @file

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2025-2026. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"  // TYPE41_ONBOARD_ENTRY / TYPE41_RECORD defined here

//
// Define data for SMBIOS Type 41 Table.
//
SMBIOS_PLATFORM_DXE_TABLE_DATA (TYPE41_ONBOARD_ENTRY, PlatformOnboardDevicesExtended) = {
  { TYPE41_REC_FIELD (OnBoardDeviceExtendedTypeOther,    1), 6, {0,0,4,0,0,0},   6}, // USB Controller 1
  { TYPE41_REC_FIELD (OnBoardDeviceExtendedTypeOther,    2), 6, {0,0,4,0,2,0},   6}, // USB Controller 2
  { TYPE41_REC_FIELD (OnBoardDeviceExtendedTypeVideo,    1), 6, {0,0,4,0,4,0,0}, 7}, // Video Adapter
  { TYPE41_REC_FIELD (OnBoardDeviceExtendedTypeEthernet, 1), 6, {0,0,4,0,6,0},   6}, // Ethernet Controller 1
  { TYPE41_REC_FIELD (OnBoardDeviceExtendedTypeEthernet, 2), 6, {0,0,4,0,7,0},   6}, // Ethernet Controller 2
  {
    { { NULL_TERMINATED_TYPE, 0, 0 }, 0, 0, 0, 0, 0, 0 },
    0, {0}, 0
  }
};

//
// Define string Tokens for additional strings.
//
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformOnboardDevicesExtended) = {
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_USB_CONTROLLER_1) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_USB_CONTROLLER_2) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_VGA_CONTROLLER_1) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_ETHERNET_CONTROLLER_1) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_ETHERNET_CONTROLLER_2) }, ADDITIONAL_STR_INDEX_1 }
};
