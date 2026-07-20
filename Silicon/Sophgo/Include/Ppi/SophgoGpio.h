/** @file
  PEI PPI for the Designware APB GPIO controller.

  PEI-phase counterpart of SOPHGO_GPIO_PROTOCOL (DwGpio.h). The method set and
  semantics mirror the DXE protocol; both are thin wrappers over DwGpioLib.
  Consumed by early PEIMs that need GPIO access before the DXE protocol exists
  (e.g. driving PCIe PERST during PCIe bring-up).

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SOPHGO_GPIO_PPI_H_
#define SOPHGO_GPIO_PPI_H_

#include <Library/DwGpioLib.h>

///
/// Global ID for the SOPHGO GPIO PPI.
///
#define SOPHGO_GPIO_PPI_GUID  \
  { 0x9D7E4F2A, 0x3C61, 0x4B8E,    \
    { 0xA5, 0x12, 0x7F, 0x0D, 0x63, 0x9A, 0xE4, 0x11 } }

typedef struct _SOPHGO_GPIO_PPI SOPHGO_GPIO_PPI;

/**
  Set or clear a GPIO pin's output value.

  @param[in]  This               PPI instance pointer.
  @param[in]  Bus                GPIO controller (bank) number.
  @param[in]  Pin                GPIO pin number within the controller.
  @param[in]  Level              1 to set (high), 0 to clear (low).

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Bus/Pin out of range.
**/
typedef
EFI_STATUS
(EFIAPI *SOPHGO_GPIO_PPI_SET_VALUE) (
  IN  SOPHGO_GPIO_PPI  *This,
  IN  UINT32           Bus,
  IN  UINT32           Pin,
  IN  UINT32           Level
  );

/**
  Read a GPIO pin's level.

  @param[in]   This              PPI instance pointer.
  @param[in]   Bus               GPIO controller (bank) number.
  @param[in]   Pin               GPIO pin number within the controller.
  @param[out]  Level             1 : high, 0 : low.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Bus/Pin out of range, or Level is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *SOPHGO_GPIO_PPI_GET_VALUE) (
  IN   SOPHGO_GPIO_PPI  *This,
  IN   UINT32           Bus,
  IN   UINT32           Pin,
  OUT  UINT32           *Level
  );

/**
  Configure a GPIO pin with the given mode (out-low / out-high / input).

  @param[in]  This               PPI instance pointer.
  @param[in]  Bus                GPIO controller (bank) number.
  @param[in]  Pin                GPIO pin number within the controller.
  @param[in]  Mode               DW_GPIO_CONFIG_MODE value.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Bus/Pin out of range, or invalid mode.
**/
typedef
EFI_STATUS
(EFIAPI *SOPHGO_GPIO_PPI_MODE_CONFIG) (
  IN  SOPHGO_GPIO_PPI      *This,
  IN  UINT32               Bus,
  IN  UINT32               Pin,
  IN  DW_GPIO_CONFIG_MODE  Mode
  );

/**
  Get the direction of the given GPIO pin.

  @param[in]  This               PPI instance pointer.
  @param[in]  Bus                GPIO controller (bank) number.
  @param[in]  Pin                GPIO pin number within the controller.
  @param[out] Direction          GPIO_IN or GPIO_OUT.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Bus/Pin out of range, or Direction is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *SOPHGO_GPIO_PPI_GET_DIRECTION) (
  IN  SOPHGO_GPIO_PPI  *This,
  IN  UINT32           Bus,
  IN  UINT32           Pin,
  OUT UINT8            *Direction
  );

struct _SOPHGO_GPIO_PPI {
  SOPHGO_GPIO_PPI_SET_VALUE      SetValue;
  SOPHGO_GPIO_PPI_GET_VALUE      GetValue;
  SOPHGO_GPIO_PPI_MODE_CONFIG    ModeConfig;
  SOPHGO_GPIO_PPI_GET_DIRECTION  GetDirection;
};

extern EFI_GUID  gSophgoGpioPpiGuid;

#endif /* SOPHGO_GPIO_PPI_H_ */
