/** @file
  The Designware GPIO controller driver.

  Publishes SOPHGO_GPIO_PROTOCOL. The register-level logic lives in DwGpioLib
  (shared with the PEI PPI driver); this driver adds the DXE-only pieces:
  mapping the controller MMIO regions into the GCD and installing the protocol.

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DwGpioLib.h>
#include <Include/DwGpio.h>

STATIC  SOPHGO_GPIO_PROTOCOL  *mGpioProtocol;

//
// Thin protocol shims: adapt the SOPHGO_GPIO_PROTOCOL signatures (which carry
// the This pointer) onto the phase-independent DwGpioLib entry points. The
// GPIO_CONFIG_MODE (DwGpio.h) and DW_GPIO_CONFIG_MODE (DwGpioLib.h) enums are
// numerically identical, so the mode value passes through unchanged.
//

STATIC
EFI_STATUS
EFIAPI
GpioProtocolSetValue (
  IN SOPHGO_GPIO_PROTOCOL  *This,
  IN UINT32                Bus,
  IN UINT32                Pin,
  IN UINT32                Level
  )
{
  return DwGpioSetValue (Bus, Pin, Level);
}

STATIC
EFI_STATUS
EFIAPI
GpioProtocolGetValue (
  IN  SOPHGO_GPIO_PROTOCOL  *This,
  IN  UINT32                Bus,
  IN  UINT32                Pin,
  OUT UINT32                *Level
  )
{
  return DwGpioGetValue (Bus, Pin, Level);
}

STATIC
EFI_STATUS
EFIAPI
GpioProtocolModeConfig (
  IN SOPHGO_GPIO_PROTOCOL  *This,
  IN UINT32                Bus,
  IN UINT32                Pin,
  IN GPIO_CONFIG_MODE      Mode
  )
{
  return DwGpioModeConfig (Bus, Pin, (DW_GPIO_CONFIG_MODE)Mode);
}

STATIC
EFI_STATUS
EFIAPI
GpioProtocolGetDirection (
  IN  SOPHGO_GPIO_PROTOCOL  *This,
  IN  UINT32                Bus,
  IN  UINT32                Pin,
  OUT UINT8                 *Direction
  )
{
  return DwGpioGetDirection (Bus, Pin, Direction);
}

/**
  Map every GPIO controller MMIO aperture into the GCD as MMIO/UC so the
  register accesses in DwGpioLib are valid once the DXE MMU is enabled.
**/
STATIC
EFI_STATUS
SetMemory (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      Index;
  UINT32      GpioNum;
  UINT64      *GpioBaseAddresses;

  GpioNum           = FixedPcdGet32 (PcdGpioControllerCount);
  GpioBaseAddresses = (UINT64 *)PcdGetPtr (PcdGpioBaseAddresses);
  if ((GpioNum == 0) || (GpioBaseAddresses == NULL)) {
    DEBUG ((DEBUG_ERROR, "No GPIO controller info found in PCD\n"));
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < GpioNum; Index++) {
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    GpioBaseAddresses[Index],
                    SIZE_4KB,
                    EFI_MEMORY_UC | EFI_MEMORY_XP
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a:%d] Add memory space failed: %r\n",
            __func__, __LINE__, Status));
      return Status;
    }

    Status = gDS->SetMemorySpaceAttributes (
                    GpioBaseAddresses[Index],
                    SIZE_4KB,
                    EFI_MEMORY_UC | EFI_MEMORY_XP
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a:%d] Set memory attributes failed: %r\n",
              __func__, __LINE__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwGpioEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = SetMemory ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mGpioProtocol = AllocateZeroPool (sizeof (SOPHGO_GPIO_PROTOCOL));
  if (mGpioProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory for mGpioProtocol\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  mGpioProtocol->SetValue     = GpioProtocolSetValue;
  mGpioProtocol->GetValue     = GpioProtocolGetValue;
  mGpioProtocol->ModeConfig   = GpioProtocolModeConfig;
  mGpioProtocol->GetDirection = GpioProtocolGetDirection;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gSophgoGpioProtocolGuid,
                  mGpioProtocol,
                  NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,"%a: InstallProtocolInterface(): %r\n",
            __func__, Status));
    FreePool (mGpioProtocol);
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwGpioUnload (
  IN  EFI_HANDLE  ImageHandle
  )
{
  if (mGpioProtocol != NULL) {
    FreePool (mGpioProtocol);
  }

  gBS->UninstallProtocolInterface (
         &ImageHandle,
         &gSophgoGpioProtocolGuid,
         NULL);

  return EFI_SUCCESS;
}
