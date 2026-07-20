/** @file
  PEI driver for the Designware APB GPIO controller.

  Publishes SOPHGO_GPIO_PPI, the PEI-phase counterpart of
  SOPHGO_GPIO_PROTOCOL. All register-level work is delegated to DwGpioLib;
  because PEI runs with the MMU disabled (RiscVConfigureMmu is a DXE step),
  no GCD/attribute mapping is needed here -- MMIO is direct.

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/DwGpioLib.h>
#include <Ppi/SophgoGpio.h>

STATIC
EFI_STATUS
EFIAPI
GpioPpiSetValue (
  IN SOPHGO_GPIO_PPI  *This,
  IN UINT32           Bus,
  IN UINT32           Pin,
  IN UINT32           Level
  )
{
  return DwGpioSetValue (Bus, Pin, Level);
}

STATIC
EFI_STATUS
EFIAPI
GpioPpiGetValue (
  IN  SOPHGO_GPIO_PPI  *This,
  IN  UINT32           Bus,
  IN  UINT32           Pin,
  OUT UINT32           *Level
  )
{
  return DwGpioGetValue (Bus, Pin, Level);
}

STATIC
EFI_STATUS
EFIAPI
GpioPpiModeConfig (
  IN SOPHGO_GPIO_PPI      *This,
  IN UINT32               Bus,
  IN UINT32               Pin,
  IN DW_GPIO_CONFIG_MODE  Mode
  )
{
  return DwGpioModeConfig (Bus, Pin, Mode);
}

STATIC
EFI_STATUS
EFIAPI
GpioPpiGetDirection (
  IN  SOPHGO_GPIO_PPI  *This,
  IN  UINT32           Bus,
  IN  UINT32           Pin,
  OUT UINT8            *Direction
  )
{
  return DwGpioGetDirection (Bus, Pin, Direction);
}

STATIC SOPHGO_GPIO_PPI  mGpioPpi = {
  GpioPpiSetValue,
  GpioPpiGetValue,
  GpioPpiModeConfig,
  GpioPpiGetDirection
};

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mGpioPpiDescriptor = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gSophgoGpioPpiGuid,
  &mGpioPpi
};

EFI_STATUS
EFIAPI
DwGpioPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = PeiServicesInstallPpi (&mGpioPpiDescriptor);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: install SOPHGO_GPIO_PPI failed: %r\n",
            __func__, Status));
  }

  return Status;
}
