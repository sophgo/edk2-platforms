/** @file
  SD3-12 board-level PCIe slot table.

  This is the SD3-12 PcieSlotInfoLib instance: it supplies
  PcieSlotInfoGetBoardTable() with the 6 slot entries.  The SD3-12
  DSC maps LIBRARY_CLASS PcieSlotInfoLib to this INF so it links in
  place of the Null instance.

  Per SG2044-PCIe-Memory-Mapping Confluence page.  Domains 0/2/4/5/6
  are RC-direct; MCIO2 hangs off the ASM2824 switch under domain 8.
  Silk and slot numbers are 1-based except MCIO2, which deliberately
  uses slot number 0 -- it is the only slot behind the ASM2824 whose
  DSPs all report factory PSN 0, so all three surfaces agree.

  Copyright (c) 2023-2026, SOPHGO Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/PcieSlotInfoLib.h>

STATIC CONST BOARD_SLOT  mSd312Slots[] = {
  /* Domain, DevPath, PathLen, SlotNumber, SlotType, Designation, DataBusWidth, PhysicalWidth */
  { 0, { 0x00 }, 1, 3, SlotTypePCIExpressGen5X16, "PCIE3", SlotDataBusWidth8X, SlotDataBusWidth16X },
  { 2, { 0x00 }, 1, 1, SlotTypePCIExpressGen5X16, "PCIE1", SlotDataBusWidth8X, SlotDataBusWidth16X },
  { 4, { 0x00 }, 1, 5, SlotTypeM2Socket3, "M.2_1", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 5, { 0x00 }, 1, 2, SlotTypePCIExpressGen5X4, "PCIE2", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 6, { 0x00 }, 1, 4, SlotTypeOther, "MCIO1", SlotDataBusWidth8X, SlotDataBusWidth8X },
  { 8, { 0x00, 0x00, 0x08 }, 3, 0, SlotTypeOther, "MCIO2", SlotDataBusWidth8X, SlotDataBusWidth8X },
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
