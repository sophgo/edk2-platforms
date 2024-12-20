/** @file
  Driver implementation for the Designware GPIO controller.

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DWGPIO_H__
#define __DWGPIO_H__

///
/// Global ID for the GPIO Protocol
///
#define SOPHGO_GPIO_PROTOCOL_GUID  \
  { 0x2381F2E4, 0x92E1, 0x4992,    \
    { 0xB1, 0x5E, 0x90, 0xA6, 0xA9, 0xA0, 0xBE, 0x1F } }

//
// Protocol interface structure
//
typedef struct _SOPHGO_GPIO_PROTOCOL SOPHGO_GPIO_PROTOCOL;

#define GPIO_IN   0
#define GPIO_OUT  1

typedef enum {
  GpioConfigOutLow = 0,
  GpioConfigOutHigh,
  GpioConfigIn,
  MaxGpioConfigMode
} GPIO_CONFIG_MODE;

/**
  Used to set or clear GPIO pin value.

  @param[in]  This               Protocol instance structure
  @param[in]  Bus                GPIO bus number.
  @param[in]  Pin                GPIO pin number.
  @param[in]  Val                1 to set, 0 to clear.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  pin number is out of range.

**/
typedef
EFI_STATUS
(EFIAPI *GPIO_SET_VALUE) (
  IN  SOPHGO_GPIO_PROTOCOL  *This,
  IN  UINT32                Bus,
  IN  UINT32                Pin,
  IN  UINT32                Level
  );

/**
  When the pin is configured as an input, obtain its voltage level.
  And when it is configured as output, retrieve the value written
  to the data register last time.

  @param[in]   This              Protocol instance structure
  @param[in]   Bus               GPIO bus number.
  @param[in]   Pin               GPIO pin number.
  @param[out]  Level             The read pin level (1 : On/High, 0 : Off/Low) when input.
                                 The value written to the data register last time when output.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  pin number is out of range.

**/
typedef
EFI_STATUS
(EFIAPI *GPIO_GET_VALUE) (
  IN   SOPHGO_GPIO_PROTOCOL  *This,
  IN   UINT32                Bus,
  IN   UINT32                Pin,
  OUT  UINT32                *Level
  );

/**
  Used to configure GPIO with a given Mode.

  @param[in]  This               Protocol instance structure
  @param[in]  Bus                GPIO bus number.
  @param[in]  Pin                GPIO pin number.
  @param[in]  Mode               GPIO configure mode.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Parameter error.

**/
typedef
EFI_STATUS
(EFIAPI *GPIO_MODE_CONFIG) (
  IN  SOPHGO_GPIO_PROTOCOL  *This,
  IN  UINT32                Bus,
  IN  UINT32                Pin,
  IN  GPIO_CONFIG_MODE      Mode
  );

/**
  Get the direction of the given GPIO pin.

  @param[in]  This               Protocol instance structure
  @param[in]  Bus                GPIO bus number.
  @param[in]  Pin                GPIO pin number.
  @param[out] Direction          GPIO pin direction.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Parameter error.

**/
typedef
EFI_STATUS
(EFIAPI *GPIO_GET_DIRECTION) (
  IN  SOPHGO_GPIO_PROTOCOL  *This,
  IN  UINT32                Bus,
  IN  UINT32                Pin,
  OUT UINT8                 *Direction
  );

struct _SOPHGO_GPIO_PROTOCOL {
  GPIO_SET_VALUE      SetValue;
  GPIO_GET_VALUE      GetValue;
  GPIO_MODE_CONFIG    ModeConfig;
  GPIO_GET_DIRECTION  GetDirection;
};

extern EFI_GUID  gSophgoGpioProtocolGuid;

#endif /* __DWGPIO_H__ */
