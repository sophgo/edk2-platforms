/** @file
  Register-level library for the Designware APB GPIO controller.

  Phase-independent (BASE) implementation shared by DwGpioDxe (protocol) and
  DwGpioPei (PPI). Reads the controller layout directly from the fixed-at-build
  PCDs (PcdGpioControllerCount / PcdGpioBaseAddresses), so it needs neither a
  constructor nor dynamic allocation and works identically in PEI and DXE.

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/DwGpioLib.h>

#define GPIO_MUX_VAL(Gpio)        (0x00000001 << (Gpio))

//
// Address GPIO_REG Registers
//
#define GPIO_SWPORTA_DR           0x00000000
#define GPIO_SWPORTA_DDR          0x00000004
#define GPIO_EXT_PORTA            0x00000050

#define GPIO_PINS_PER_CONTROLLER  32

/**
  Resolve the register base of the controller that owns (Bus, Pin), validating
  both against the fixed-at-build GPIO PCD layout.

  @param[in]  Bus    GPIO controller (bank) number.
  @param[in]  Pin    GPIO pin number within the controller.
  @param[out] Base   Register base of the controller.

  @retval EFI_SUCCESS            Base resolved.
  @retval EFI_INVALID_PARAMETER  Bus/Pin out of range.
  @retval EFI_NOT_FOUND          GPIO PCD layout is empty.
**/
STATIC
EFI_STATUS
GetGpioBase (
  IN  UINT32  Bus,
  IN  UINT32  Pin,
  OUT UINTN   *Base
  )
{
  UINT32  GpioNum;
  UINT64  *GpioBaseAddresses;

  GpioNum           = FixedPcdGet32 (PcdGpioControllerCount);
  GpioBaseAddresses = (UINT64 *)PcdGetPtr (PcdGpioBaseAddresses);

  if ((GpioNum == 0) || (GpioBaseAddresses == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: no GPIO controller info in PCD\n", __func__));
    return EFI_NOT_FOUND;
  }

  if (Bus >= GpioNum) {
    return EFI_INVALID_PARAMETER;
  }

  if (Pin >= GPIO_PINS_PER_CONTROLLER) {
    return EFI_INVALID_PARAMETER;
  }

  *Base = (UINTN)GpioBaseAddresses[Bus];

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwGpioGetDirection (
  IN  UINT32  Bus,
  IN  UINT32  Pin,
  OUT UINT8   *Direction
  )
{
  UINTN       Base;
  UINT32      ReadVal;
  EFI_STATUS  Status;

  if (Direction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetGpioBase (Bus, Pin, &Base);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Get GPIO addr error: %r\n", Status));
    return Status;
  }

  ReadVal = MmioRead32 (Base + GPIO_SWPORTA_DDR);

  *Direction = (ReadVal & GPIO_MUX_VAL (Pin)) ? GPIO_OUT : GPIO_IN;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwGpioSetValue (
  IN  UINT32  Bus,
  IN  UINT32  Pin,
  IN  UINT32  Level
  )
{
  UINTN       Base;
  UINT32      ReadVal;
  EFI_STATUS  Status;

  Status = GetGpioBase (Bus, Pin, &Base);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Get GPIO addr error: %r\n", Status));
    return Status;
  }

  ReadVal = MmioRead32 (Base + GPIO_SWPORTA_DR);

  if (Level) {
    MmioWrite32 (Base + GPIO_SWPORTA_DR, ReadVal | GPIO_MUX_VAL (Pin));
  } else {
    MmioWrite32 (Base + GPIO_SWPORTA_DR, ReadVal & ~GPIO_MUX_VAL (Pin));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwGpioGetValue (
  IN  UINT32  Bus,
  IN  UINT32  Pin,
  OUT UINT32  *Level
  )
{
  UINTN       Base;
  UINT32      Val;
  UINT8       Direction;
  EFI_STATUS  Status;

  if (Level == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetGpioBase (Bus, Pin, &Base);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Get GPIO addr error: %r\n", Status));
    return Status;
  }

  Status = DwGpioGetDirection (Bus, Pin, &Direction);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Direction == GPIO_OUT) {
    Val = MmioRead32 (Base + GPIO_SWPORTA_DR);
  } else {
    Val = MmioRead32 (Base + GPIO_EXT_PORTA);
  }

  *Level = (Val & GPIO_MUX_VAL (Pin)) ? 1 : 0;

  return EFI_SUCCESS;
}

/**
  Set a pin's direction (input or output) without touching its data value.

  @param[in]  Bus     GPIO controller (bank) number.
  @param[in]  Pin     GPIO pin number within the controller.
  @param[in]  InOut   GPIO_IN or GPIO_OUT.
**/
STATIC
EFI_STATUS
GpioConfig (
  IN  UINT32  Bus,
  IN  UINT32  Pin,
  IN  UINT32  InOut
  )
{
  UINT32      Val;
  UINTN       Base;
  EFI_STATUS  Status;

  Status = GetGpioBase (Bus, Pin, &Base);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Get GPIO addr error: %r\n", Status));
    return Status;
  }

  Val = MmioRead32 (Base + GPIO_SWPORTA_DDR);

  if (InOut == GPIO_OUT) {
    MmioWrite32 (Base + GPIO_SWPORTA_DDR, Val | GPIO_MUX_VAL (Pin));
  } else {
    MmioWrite32 (Base + GPIO_SWPORTA_DDR, Val & ~GPIO_MUX_VAL (Pin));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwGpioModeConfig (
  IN  UINT32               Bus,
  IN  UINT32               Pin,
  IN  DW_GPIO_CONFIG_MODE  Mode
  )
{
  EFI_STATUS  Status;

  switch (Mode) {
    case DwGpioConfigOutLow:
      Status = GpioConfig (Bus, Pin, GPIO_OUT);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = DwGpioSetValue (Bus, Pin, 0);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      break;

    case DwGpioConfigOutHigh:
      Status = GpioConfig (Bus, Pin, GPIO_OUT);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = DwGpioSetValue (Bus, Pin, 1);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      break;

    case DwGpioConfigIn:
      Status = GpioConfig (Bus, Pin, GPIO_IN);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      break;

    default:
      DEBUG ((DEBUG_ERROR, "Invalid GPIO mode\n"));
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
