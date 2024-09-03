/** @file
  PCI Host Bridge Library instance for SOPHGO SG2044 platforms.

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCI_PLATFORM_LIB_H__
#define PCI_PLATFORM_LIB_H__

#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PciHostBridgeLib.h>

/**
 * Init all PCIe controllers of this platform.
 *
 * @return Success or not.
 *
 */

RETURN_STATUS
EFIAPI
PciPlatformInit(
    VOID
    );

/**
 * Get PCIe roots.
 *
 * @param Count Number of PCIe roots
 *
 * @return PCIe roots as an arrary
 *
 */

PCI_ROOT_BRIDGE *
EFIAPI
PciPlatformGetRoot(
    OUT UINTN   *Count
    );

/**
  Read 8, 16 or 32 bits PCI configuration register specified by Address.

  This function must guarantee that all PCI read and write operations are serialized.

  @param  Address     Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  Width       Width of bits, 8, 16 or 32.

  @return Value read from PCI config register.

**/

UINT32
EFIAPI
PciSegmentRead (
  IN UINT64                         Address,
  IN UINT32                         Width
  );

/**
  Write 8, 16 or 32 bits PCI configuration register specified by Address.

  This function must guarantee that all PCI read and write operations are serialized.

  @param  Address     Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  Value       Value to be written.
  @param  Width       Width of bits, 8, 16 or 32.

  @return Value write to PCI config register.

**/

UINT32
EFIAPI
PciSegmentWrite (
  IN UINT64                         Address,
  IN UINT32                         Value,
  IN UINT32                         Width
  );

#endif
