/** @file
  PCI Option ROM allow-list, supplied by the platform.

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCI_OPROM_ALLOW_LIST_LIB_H_
#define PCI_OPROM_ALLOW_LIST_LIB_H_

#include <Uefi.h>

/**
  Returns whether the Option ROM of VendorId:DeviceId may be probed and
  loaded by PciBusDxe.

  @param[in]  VendorId  PCI vendor ID.
  @param[in]  DeviceId  PCI device ID.

  @retval TRUE   Device is allow-listed; its Option ROM is probed.
  @retval FALSE  Device is not allow-listed; its Option ROM BAR is skipped.
**/
BOOLEAN
EFIAPI
PciOpRomIsAllowListed (
  IN UINT16  VendorId,
  IN UINT16  DeviceId
  );

#endif
