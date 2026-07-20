/** @file
  Register-level library for the Designware APB GPIO controller.

  Phase-independent (BASE) helpers shared by the DXE protocol driver
  (DwGpioDxe) and the PEI PPI driver (DwGpioPei). The library owns the
  controller table (built from PcdGpioControllerCount / PcdGpioBaseAddresses)
  and exposes plain (Bus, Pin) accessors with no dependency on the DXE
  protocol or PEI PPI wrappers layered on top.

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DW_GPIO_LIB_H_
#define DW_GPIO_LIB_H_

#include <Uefi/UefiBaseType.h>

//
// Pin direction values, matching the DesignWare SWPORTA_DDR semantics.
//
#ifndef GPIO_IN
#define GPIO_IN   0
#endif
#ifndef GPIO_OUT
#define GPIO_OUT  1
#endif

//
// Pin configuration mode. Kept numerically identical to the DXE protocol's
// GPIO_CONFIG_MODE (DwGpio.h) so a caller can pass either enum through.
//
typedef enum {
  DwGpioConfigOutLow = 0,
  DwGpioConfigOutHigh,
  DwGpioConfigIn,
  DwMaxGpioConfigMode
} DW_GPIO_CONFIG_MODE;

/**
  Get the direction of the given GPIO pin.

  @param[in]  Bus                GPIO controller (bank) number.
  @param[in]  Pin                GPIO pin number within the controller.
  @param[out] Direction          GPIO_IN or GPIO_OUT.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Bus/Pin out of range, or Direction is NULL.
**/
EFI_STATUS
EFIAPI
DwGpioGetDirection (
  IN  UINT32  Bus,
  IN  UINT32  Pin,
  OUT UINT8   *Direction
  );

/**
  Set or clear a GPIO pin's output value.

  @param[in]  Bus                GPIO controller (bank) number.
  @param[in]  Pin                GPIO pin number within the controller.
  @param[in]  Level              1 to set (high), 0 to clear (low).

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Bus/Pin out of range.
**/
EFI_STATUS
EFIAPI
DwGpioSetValue (
  IN  UINT32  Bus,
  IN  UINT32  Pin,
  IN  UINT32  Level
  );

/**
  Read a GPIO pin's level. When configured as input, returns the pin voltage
  level; when configured as output, returns the last value written.

  @param[in]   Bus               GPIO controller (bank) number.
  @param[in]   Pin               GPIO pin number within the controller.
  @param[out]  Level             1 : high, 0 : low.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Bus/Pin out of range, or Level is NULL.
**/
EFI_STATUS
EFIAPI
DwGpioGetValue (
  IN  UINT32  Bus,
  IN  UINT32  Pin,
  OUT UINT32  *Level
  );

/**
  Configure a GPIO pin with the given mode (out-low / out-high / input).

  @param[in]  Bus                GPIO controller (bank) number.
  @param[in]  Pin                GPIO pin number within the controller.
  @param[in]  Mode               DW_GPIO_CONFIG_MODE value.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Bus/Pin out of range, or invalid mode.
**/
EFI_STATUS
EFIAPI
DwGpioModeConfig (
  IN  UINT32               Bus,
  IN  UINT32               Pin,
  IN  DW_GPIO_CONFIG_MODE  Mode
  );

#endif /* DW_GPIO_LIB_H_ */
