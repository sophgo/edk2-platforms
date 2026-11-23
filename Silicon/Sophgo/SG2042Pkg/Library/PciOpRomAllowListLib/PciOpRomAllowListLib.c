/** @file
  PCI Option ROM allow-list for SG2042.

  SG2042 runs with the MMU disabled (its C920 only implements Sv39, and
  the SoC MMIO/DDR above 0x40_0000_0000 cannot be identity-mapped under
  Sv39), so EFI_MEMORY_XP can never be enforced and the x64 emulator has
  to rely on illegal-instruction trapping for every native->emulated
  transition.  OpROM drivers that publish protocols consumed by native
  code are therefore only allow-listed when that path is known to work.
  The MegaRAID 3108 ({ 0x1000, 0x005D }) is intentionally NOT listed:
  its DriverBinding->Supported() entry trips the DXE core in
  CoreConnectSingleController.  Linux drives the card via megaraid_sas.

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/PciOpRomAllowListLib.h>

typedef struct {
  UINT16    VendorId;
  UINT16    DeviceId;
} OP_ROM_ALLOW_ENTRY;

//
// The only devices whose Option ROMs are probed and loaded on SG2042.
//
STATIC CONST OP_ROM_ALLOW_ENTRY  mOpRomAllowList[] = {
  { 0x1002, 0x6779 },   // AMD Radeon R5 230
  { 0x1002, 0x67DF },   // AMD Radeon RX 580
  { 0x1002, 0x73DF },   // AMD Radeon RX 6700
  { 0x10DE, 0x2882 },   // NVIDIA GeForce RTX 4060
  { 0x10DE, 0x1287 },   // NVIDIA GeForce GT 730
  { 0x10DE, 0x128B },   // NVIDIA GeForce GT 710
};

BOOLEAN
EFIAPI
PciOpRomIsAllowListed (
  IN UINT16  VendorId,
  IN UINT16  DeviceId
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof (mOpRomAllowList) / sizeof (mOpRomAllowList[0]); Index++) {
    if ((mOpRomAllowList[Index].VendorId == VendorId) &&
        (mOpRomAllowList[Index].DeviceId == DeviceId))
    {
      return TRUE;
    }
  }

  return FALSE;
}
