/** @file
  SMBIOS platform driver for Sophgo SG2042 boards.

  The driver itself is board-independent; the per-board strings, chassis type and
  memory array limit live in one of the SmbiosData<Board>.c files, selected at
  build time by gSophgoTokenSpaceGuid.PcdSmbiosBoardType.

  Copyright (c) 2023-2026, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_PLATFORM_DXE_H_
#define SMBIOS_PLATFORM_DXE_H_

#include <Uefi.h>

///
/// Values for gSophgoTokenSpaceGuid.PcdSmbiosBoardType
///
#define SMBIOS_BOARD_SG2042_EVB      0
#define SMBIOS_BOARD_MILKV_PIONEER   1
#define SMBIOS_BOARD_SRA1_20         2

///
/// Per-board data published through Type 1/2/3/11/16.
///
typedef struct {
  CONST CHAR8    *SystemProductName;      ///< Type 1 Product Name
  CONST CHAR8    *BoardProductName;       ///< Type 2 Product Name
  CONST CHAR8    *ProductVersion;         ///< Type 1/2/3 Version (same value)
  UINT8          ChassisType;             ///< Type 3 Chassis Type (MISC_CHASSIS_TYPE)
  UINT32         MaxMemoryCapacityKb;     ///< Type 16 Maximum Capacity, in KB
} SMBIOS_BOARD_DATA;

extern CONST SMBIOS_BOARD_DATA  gSmbiosBoardDataSG2042Evb;
extern CONST SMBIOS_BOARD_DATA  gSmbiosBoardDataMilkVPioneer;
extern CONST SMBIOS_BOARD_DATA  gSmbiosBoardDataSra120;

#endif // SMBIOS_PLATFORM_DXE_H_
