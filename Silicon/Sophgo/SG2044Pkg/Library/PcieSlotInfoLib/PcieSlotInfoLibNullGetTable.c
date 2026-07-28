/** @file
  Default (null) board table.  Returns NULL / count == 0.

  Overridden per-platform by an alternative library instance whose DSC
  mapping replaces the INF, supplying a real BOARD_SLOT[].

  Copyright (c) 2023-2026, SOPHGO Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/PcieSlotInfoLib.h>

CONST BOARD_SLOT *
EFIAPI
PcieSlotInfoGetBoardTable (
  OUT UINTN  *Count OPTIONAL
  )
{
  if (Count != NULL) {
    *Count = 0;
  }

  return NULL;
}
