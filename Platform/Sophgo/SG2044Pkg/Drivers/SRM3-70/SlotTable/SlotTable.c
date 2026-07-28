/** @file
  SRM3-70 board-level PCIe slot table.

  This is the SRM3-70 PcieSlotInfoLib instance: it supplies
  PcieSlotInfoGetBoardTable() with the 8 slot entries.  The SRM3-70
  DSC maps LIBRARY_CLASS PcieSlotInfoLib to this INF so it links in
  place of the Null instance.

  Per SG2044-PCIe-Memory-Mapping Confluence page.  Domains 0/1/2/4/5/
  6/7 are RC-direct; MCIO1 hangs off a switch under domain 8.  Note
  PCIE0 legitimately uses slot number 0 (PCIE_SLOT_NUMBER_NONE is
  0xFFFF).

  Copyright (c) 2023-2026, SOPHGO Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/PcieSlotInfoLib.h>

STATIC CONST BOARD_SLOT  mSrm370Slots[] = {
  /* Domain, DevPath, PathLen, SlotNumber, SlotType, Designation, DataBusWidth, PhysicalWidth */
  { 0, { 0x00 }, 1, 2, SlotTypePCIExpressGen5X4, "PCIE2A", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 1, { 0x00 }, 1, 6, SlotTypePCIExpressGen5X4, "PCIE2B", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 2, { 0x00 }, 1, 0, SlotTypePCIExpressGen5X8, "PCIE0", SlotDataBusWidth8X, SlotDataBusWidth8X },
  { 4, { 0x00 }, 1, 5, SlotTypeM2Socket3, "M.2_0", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 5, { 0x00 }, 1, 1, SlotTypePCIExpressGen5X4, "PCIE1", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 6, { 0x00 }, 1, 3, SlotTypeOther, "MCIO0A", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 7, { 0x00 }, 1, 7, SlotTypeOther, "MCIO0B", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 8, { 0x00, 0x00, 0x08 }, 3, 4, SlotTypeOther, "MCIO1", SlotDataBusWidth8X, SlotDataBusWidth8X },
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
