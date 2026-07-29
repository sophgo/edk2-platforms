/** @file
  SRM3-70 board-level PCIe slot table.

  This is the SRM3-70 PcieSlotInfoLib instance: it supplies
  PcieSlotInfoGetBoardTable() with the 8 slot entries.  The SRM3-70
  DSC maps LIBRARY_CLASS PcieSlotInfoLib to this INF so it links in
  place of the Null instance.

  Per SG2044-PCIe-Memory-Mapping Confluence page.  Domains 0/1/2/4/5/
  6/7 are RC-direct; MCIO2 hangs off the ASM2824 switch under domain 8.
  Silk and slot numbers are 1-based except MCIO2, which deliberately
  uses slot number 0 -- it is the only slot behind the ASM2824 whose
  DSPs all report factory PSN 0, so all three surfaces agree.

  Copyright (c) 2023-2026, SOPHGO Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/PcieSlotInfoLib.h>

STATIC CONST BOARD_SLOT  mSrm370Slots[] = {
  /* Domain, DevPath, PathLen, SlotNumber, SlotType, Designation, DataBusWidth, PhysicalWidth */
  { 0, { 0x00 }, 1, 3, SlotTypePCIExpressGen5X4, "PCIE3A", SlotDataBusWidth4X, SlotDataBusWidth4X, SlotLengthLong, SlotHeightFullHeight, 5, 0x04, 0x04, 0 },
  { 1, { 0x00 }, 1, 6, SlotTypePCIExpressGen5X4, "PCIE3B", SlotDataBusWidth4X, SlotDataBusWidth4X, SlotLengthLong, SlotHeightFullHeight, 5, 0x04, 0x04, 0 },
  { 2, { 0x00 }, 1, 1, SlotTypePCIExpressGen5X8, "PCIE1", SlotDataBusWidth8X, SlotDataBusWidth8X, SlotLengthLong, SlotHeightFullHeight, 5, 0x04, 0x04, 0 },
  { 4, { 0x00 }, 1, 5, SlotTypeM2Socket3, "M.2_1", SlotDataBusWidth4X, SlotDataBusWidth4X, SlotLengthUnknown, SlotHeightNone, 0, 0x04, 0x04, 0 },
  { 5, { 0x00 }, 1, 2, SlotTypePCIExpressGen5X4, "PCIE2", SlotDataBusWidth4X, SlotDataBusWidth4X, SlotLengthLong, SlotHeightFullHeight, 5, 0x04, 0x04, 0 },
  { 6, { 0x00 }, 1, 4, SlotTypeOther, "MCIO1A", SlotDataBusWidth4X, SlotDataBusWidth4X, SlotLengthUnknown, SlotHeightNone, 0, 0x04, 0x04, 0 },
  { 7, { 0x00 }, 1, 7, SlotTypeOther, "MCIO1B", SlotDataBusWidth4X, SlotDataBusWidth4X, SlotLengthUnknown, SlotHeightNone, 0, 0x04, 0x04, 0 },
  { 8, { 0x00, 0x00, 0x08 }, 3, 0, SlotTypeOther, "MCIO2", SlotDataBusWidth8X, SlotDataBusWidth8X, SlotLengthUnknown, SlotHeightNone, 0, 0x04, 0x04, 0 },
};

CONST BOARD_SLOT *
EFIAPI
PcieSlotInfoGetBoardTable (
  OUT UINTN  *Count OPTIONAL
  )
{
  if (Count != NULL) {
    *Count = ARRAY_SIZE (mSrm370Slots);
  }

  return mSrm370Slots;
}
