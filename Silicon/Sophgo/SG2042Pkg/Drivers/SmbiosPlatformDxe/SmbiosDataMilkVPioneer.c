/** @file
  SMBIOS board data for the MilkV-Pioneer (SG2042).

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"
#include <IndustryStandard/SmBios.h>

CONST SMBIOS_BOARD_DATA  gSmbiosBoardDataMilkVPioneer = {
  "MilkV-Pioneer",                     // SystemProductName (Type 1)
  "MilkV-Pioneer",                     // BoardProductName  (Type 2)
  "1.2",                               // ProductVersion    (Type 1/2/3)
  MiscChassisTypeDeskTop,              // ChassisType       (Type 3)
  0x10000000                           // MaxMemoryCapacityKb = 256 GB
};
