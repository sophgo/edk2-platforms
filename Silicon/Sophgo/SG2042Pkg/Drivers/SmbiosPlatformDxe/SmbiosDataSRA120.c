/** @file
  SMBIOS board data for the SRA1-20 (SG2042).

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"
#include <IndustryStandard/SmBios.h>

CONST SMBIOS_BOARD_DATA  gSmbiosBoardDataSra120 = {
  "SRA1-20",                           // SystemProductName (Type 1)
  "SRA1-20",                           // BoardProductName  (Type 2)
  "2.1",                               // ProductVersion    (Type 1/2/3)
  MiscChassisTypeMainServerChassis,    // ChassisType       (Type 3)
  0x20000000                           // MaxMemoryCapacityKb = 512 GB
};
