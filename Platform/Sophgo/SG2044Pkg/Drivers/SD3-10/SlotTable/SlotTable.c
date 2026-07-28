/** @file
  SD3-10 board-level PCIe slot table.

  This is the SD3-10 PcieSlotInfoLib instance: it supplies
  PcieSlotInfoGetBoardTable() with the 5 RC-direct slot entries.  The
  SD3-10 DSC maps LIBRARY_CLASS PcieSlotInfoLib to this INF so it links
  in place of the Null instance used by the other variants.

  Silk-screen mapping (per SG2044-PCIe-Memory-Mapping Confluence page;
  note SLOT1 and SLOT2 are NOT in domain order):
    Domain 0 -> PCIE_SLOT2 (slot 2)   Domain 2 -> PCIE_SLOT1 (slot 1)
    Domain 4 -> PCIE_SLOT3 (slot 3)   Domain 6 -> PCIE_SLOT4 (slot 4)
    Domain 8 -> PCIE_SLOT5 (slot 5)

  Copyright (c) 2023-2026, SOPHGO Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/PcieSlotInfoLib.h>

STATIC CONST BOARD_SLOT  mSd310Slots[] = {
  { 0, { 0x00 }, 1, 2, SlotTypePCIExpressGen5X8, "PCIE_SLOT2", SlotDataBusWidth8X, SlotDataBusWidth8X },
  { 2, { 0x00 }, 1, 1, SlotTypePCIExpressGen5X8, "PCIE_SLOT1", SlotDataBusWidth8X, SlotDataBusWidth8X },
  { 4, { 0x00 }, 1, 3, SlotTypePCIExpressGen5X8, "PCIE_SLOT3", SlotDataBusWidth8X, SlotDataBusWidth8X },
  { 6, { 0x00 }, 1, 4, SlotTypePCIExpressGen5X8, "PCIE_SLOT4", SlotDataBusWidth8X, SlotDataBusWidth8X },
  { 8, { 0x00 }, 1, 5, SlotTypePCIExpressGen5X8, "PCIE_SLOT5", SlotDataBusWidth8X, SlotDataBusWidth8X },
};

CONST BOARD_SLOT *
EFIAPI
PcieSlotInfoGetBoardTable (
  OUT UINTN  *Count OPTIONAL
  )
{
  if (Count != NULL) {
    *Count = ARRAY_SIZE (mSd310Slots);
  }

  return mSd310Slots;
}
