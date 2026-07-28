/** @file
  SD3-12 board-level PCIe slot table.

  This is the SD3-12 PcieSlotInfoLib instance: it supplies
  PcieSlotInfoGetBoardTable() with the 6 slot entries.  The SD3-12
  DSC maps LIBRARY_CLASS PcieSlotInfoLib to this INF so it links in
  place of the Null instance.

  Per SG2044-PCIe-Memory-Mapping Confluence page.  Domains 0/2/4/5/6
  are RC-direct; MCIO1 hangs off a switch under domain 8.  Note PCIE0
  legitimately uses slot number 0 (PCIE_SLOT_NUMBER_NONE is 0xFFFF).

  Copyright (c) 2023-2026, SOPHGO Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/PcieSlotInfoLib.h>

STATIC CONST BOARD_SLOT  mSd312Slots[] = {
  /* Domain, DevPath, PathLen, SlotNumber, SlotType, Designation, DataBusWidth, PhysicalWidth */
  { 0, { 0x00 }, 1, 2, SlotTypePCIExpressGen5X16, "PCIE2", SlotDataBusWidth8X, SlotDataBusWidth16X },
  { 2, { 0x00 }, 1, 0, SlotTypePCIExpressGen5X16, "PCIE0", SlotDataBusWidth8X, SlotDataBusWidth16X },
  { 4, { 0x00 }, 1, 5, SlotTypeM2Socket3, "M.2_0", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 5, { 0x00 }, 1, 1, SlotTypePCIExpressGen5X4, "PCIE1", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 6, { 0x00 }, 1, 3, SlotTypeOther, "MCIO0", SlotDataBusWidth8X, SlotDataBusWidth8X },
  { 8, { 0x00, 0x00, 0x08 }, 3, 4, SlotTypeOther, "MCIO1", SlotDataBusWidth8X, SlotDataBusWidth8X },
};

CONST BOARD_SLOT *
EFIAPI
PcieSlotInfoGetBoardTable (
  OUT UINTN  *Count OPTIONAL
  )
{
  if (Count != NULL) {
    *Count = ARRAY_SIZE (mSd312Slots);
  }

  return mSd312Slots;
}
