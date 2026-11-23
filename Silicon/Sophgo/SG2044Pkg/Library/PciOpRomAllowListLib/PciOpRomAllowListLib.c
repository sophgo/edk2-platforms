/** @file
  PCI Option ROM allow-list for SG2044.

  SG2044 enables the MMU in Sv48 mode, so EFI_MEMORY_XP is enforced and
  the MultiArchUefiPkg x64 emulator can thunk native->emulated calls via
  instruction page faults.  The MegaRAID 3108 OpROM is therefore allowed
  to run under the emulator on this platform.

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
// The only devices whose Option ROMs are probed and loaded on SG2044.
//
STATIC CONST OP_ROM_ALLOW_ENTRY  mOpRomAllowList[] = {
  { 0x1000, 0x005D },   // LSI/AVAGO MegaRAID SAS 9361-8i (3108)
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
