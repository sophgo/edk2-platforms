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
  TYPE41_RECORD (OnBoardDeviceExtendedTypeSATAController, 1, 8, 0x03, 0x00), // ASMedia SATA controller
  TYPE41_RECORD (OnBoardDeviceExtendedTypeOther,          1, 8, 0x04, 0x00), // Renesas USB controller 1
  TYPE41_RECORD (OnBoardDeviceExtendedTypeOther,          2, 8, 0x05, 0x00), // Renesas USB controller 2
  TYPE41_RECORD (OnBoardDeviceExtendedTypeVideo,          1, 8, 0x07, 0x00), // ASPEED VGA controller
  TYPE41_RECORD (OnBoardDeviceExtendedTypeOther,          3, 8, 0x07, 0x08), // ASPEED RTC
  TYPE41_RECORD (OnBoardDeviceExtendedTypeEthernet,       1, 8, 0x08, 0x00), // Intel I210 Ethernet 1
  TYPE41_RECORD (OnBoardDeviceExtendedTypeEthernet,       2, 8, 0x09, 0x00), // Intel I210 Ethernet 2
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
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_SATA_CONTROLLER_1) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_USB_CONTROLLER_1) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_USB_CONTROLLER_2) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_VGA_CONTROLLER_1) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_RTC_1) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_ETHERNET_CONTROLLER_1) }, ADDITIONAL_STR_INDEX_1 },
  { { STRING_TOKEN (STR_PLATFORM_DXE_ONBOARD_ETHERNET_CONTROLLER_2) }, ADDITIONAL_STR_INDEX_1 }
};
