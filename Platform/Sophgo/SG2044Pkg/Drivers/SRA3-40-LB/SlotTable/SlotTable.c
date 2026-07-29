/** @file
  SRA3-40-LB board-level PCIe slot table.

  This is the SRA3-40-LB PcieSlotInfoLib instance: it supplies
  PcieSlotInfoGetBoardTable() with the 7 slot entries.  The
  SRA3-40-LB DSC maps LIBRARY_CLASS PcieSlotInfoLib to this INF so it
  links in place of the Null instance.

  Per SG2044-PCIe-Memory-Mapping Confluence page.  Unlike SRA3-40,
  domain 2 (PCIE2) is not enabled on this board.  Domains 0/4/8 are
  RC-direct; PCIE10/CON1/CON3/PCIE5 hang off the onboard PEX switch
  under domain 6.

  Switch-downstream SlotNumbers inherit the PEX switch factory PSNs
  (units shipped; switch firmware is not modified).

  Copyright (c) 2023-2026, SOPHGO Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/PcieSlotInfoLib.h>

STATIC CONST BOARD_SLOT  mSra340LbSlots[] = {
  /* Domain, DevPath, PathLen, SlotNumber, SlotType, Designation, DataBusWidth, PhysicalWidth */
  { 0, { 0x00 }, 1, 1, SlotTypePCIExpressGen5X16, "PCIE1", SlotDataBusWidth8X, SlotDataBusWidth16X },
  { 4, { 0x00 }, 1, 3, SlotTypePCIExpressGen5X16, "PCIE3", SlotDataBusWidth8X, SlotDataBusWidth16X },
  { 6, { 0x00, 0x00, 0x00, 0x00, 0x10 }, 5, 16, SlotTypePciExpressGen4X8, "PCIE10", SlotDataBusWidth8X, SlotDataBusWidth8X },
  { 6, { 0x00, 0x00, 0x00, 0x00, 0x0C }, 5, 12, SlotTypeM2Socket3, "CON1", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 6, { 0x00, 0x00, 0x00, 0x00, 0x08 }, 5, 8, SlotTypeM2Socket3, "CON3", SlotDataBusWidth4X, SlotDataBusWidth4X },
  { 6, { 0x00, 0x00, 0x00, 0x00, 0x18 }, 5, 24, SlotTypePciExpressGen4X16, "PCIE5", SlotDataBusWidth8X, SlotDataBusWidth16X },
  { 8, { 0x00 }, 1, 4, SlotTypePCIExpressGen5X16, "PCIE4", SlotDataBusWidth8X, SlotDataBusWidth16X },
};

CONST BOARD_SLOT *
EFIAPI
PcieSlotInfoGetBoardTable (
  OUT UINTN  *Count OPTIONAL
  )
{
  if (Count != NULL) {
    *Count = ARRAY_SIZE (mSra340LbSlots);
  }

  return mSra340LbSlots;
}
