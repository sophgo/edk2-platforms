/** @file
  SD3-10-LB board-level PCIe slot table.

  This is the SD3-10-LB PcieSlotInfoLib instance: it supplies
  PcieSlotInfoGetBoardTable() with the 4 RC-direct slot entries.  The
  SD3-10-LB DSC maps LIBRARY_CLASS PcieSlotInfoLib to this INF so it
  links in place of the Null instance.

  Per SG2044-PCIe-Memory-Mapping Confluence page.  Unlike SD3-10,
  domain 2 (PCIE_SLOT1) is not enabled on this board:
    Domain 0 -> PCIE_SLOT2 (slot 2)   Domain 4 -> PCIE_SLOT3 (slot 3)
    Domain 6 -> PCIE_SLOT4 (slot 4)   Domain 8 -> PCIE_SLOT5 (slot 5)

  Copyright (c) 2023-2026, SOPHGO Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/PcieSlotInfoLib.h>

STATIC CONST BOARD_SLOT  mSd310LbSlots[] = {
  { 0, { 0x00 }, 1, 2, SlotTypePCIExpressGen5X8, "PCIE_SLOT2", SlotDataBusWidth8X, SlotDataBusWidth8X, SlotLengthLong, SlotHeightFullHeight, 5, 0x04, 0x04, 0 },
  { 4, { 0x00 }, 1, 3, SlotTypePCIExpressGen5X8, "PCIE_SLOT3", SlotDataBusWidth8X, SlotDataBusWidth8X, SlotLengthLong, SlotHeightFullHeight, 5, 0x04, 0x04, 0 },
  { 6, { 0x00 }, 1, 4, SlotTypePCIExpressGen5X8, "PCIE_SLOT4", SlotDataBusWidth8X, SlotDataBusWidth8X, SlotLengthLong, SlotHeightFullHeight, 5, 0x04, 0x04, 0 },
  { 8, { 0x00 }, 1, 5, SlotTypePCIExpressGen5X8, "PCIE_SLOT5", SlotDataBusWidth8X, SlotDataBusWidth8X, SlotLengthLong, SlotHeightFullHeight, 5, 0x04, 0x04, 0 },
};

CONST BOARD_SLOT *
EFIAPI
PcieSlotInfoGetBoardTable (
  OUT UINTN  *Count OPTIONAL
  )
{
  if (Count != NULL) {
    *Count = ARRAY_SIZE (mSd310LbSlots);
  }

  return mSd310LbSlots;
}
