/** @file

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2025-2026. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

#define TYPE41_ENABLED  (1 << 7)

#define TYPE41_RECORD(DeviceType, Instance, Segment, Bus, DevFunc) \
  {                                                                \
    {                                                              \
      EFI_SMBIOS_TYPE_ONBOARD_DEVICES_EXTENDED_INFORMATION,        \
      sizeof (SMBIOS_TABLE_TYPE41),                                \
      SMBIOS_HANDLE_PI_RESERVED                                    \
    },                                                             \
    ADDITIONAL_STR_INDEX_1,                                        \
    TYPE41_ENABLED | (DeviceType),                                 \
    (Instance),                                                    \
    (Segment),                                                     \
    (Bus),                                                         \
    (DevFunc)                                                      \
  }

//
// Define data for SMBIOS Type 41 Table.
//
SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE41, PlatformOnboardDevicesExtended) = {
  TYPE41_RECORD (OnBoardDeviceExtendedTypeOther,    1, 6, 0x0B, 0), // Renesas USB controller 1
  TYPE41_RECORD (OnBoardDeviceExtendedTypeOther,    2, 6, 0x0D, 0), // Renesas USB controller 2
  TYPE41_RECORD (OnBoardDeviceExtendedTypeVideo,    1, 6, 0x10, 0), // ASPEED VGA controller
  TYPE41_RECORD (OnBoardDeviceExtendedTypeEthernet, 1, 6, 0x12, 0), // Intel I210 Ethernet 1
  TYPE41_RECORD (OnBoardDeviceExtendedTypeEthernet, 2, 6, 0x13, 0), // Intel I210 Ethernet 2
  {
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
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformOnboardDevicesExtended) = {
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_USB_CONTROLLER_1) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_USB_CONTROLLER_2) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_VGA_CONTROLLER_1) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_ETHERNET_CONTROLLER_1) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_ETHERNET_CONTROLLER_2) }, ADDITIONAL_STR_INDEX_1 }
};
