/** @file
  SMBIOS board data for the SG2042-EVB.

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"
#include <IndustryStandard/SmBios.h>

CONST SMBIOS_BOARD_DATA  gSmbiosBoardDataSG2042Evb = {
  "SG2042-EVB",                        // SystemProductName (Type 1)
  "SG2042-EVB",                        // BoardProductName  (Type 2)
  "2.0",                               // ProductVersion    (Type 1/2/3)
  MiscChassisTypeDeskTop,              // ChassisType       (Type 3)
  0x10000000                           // MaxMemoryCapacityKb = 256 GB
};
