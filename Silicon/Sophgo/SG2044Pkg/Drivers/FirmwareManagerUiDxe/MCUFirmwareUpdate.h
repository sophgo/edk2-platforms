/** @file
  The functions for firmware manager menu.

  Copyright (c) 2025, SOPHGO Technology Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef  MCU_FIRMWARE_UPDATE_H_
#define  MCU_FIRMWARE_UPDATE_H_

#include <Uefi/UefiBaseType.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Include/DwI2c.h>

/**
  Find MCU device.

  @retval  EFI_SUCCESS       Find MCU successfully.
  @retval  EFI_NOT_FOUND     MCU does not exist.
**/
EFI_STATUS
MCUDeviceMatch (
  VOID
  );

/**
  Check if the MCU firmware is applicable.

  @param[in]  I2cMasterProtocol   The pointer to SOPHGO_I2C_MASTER_PROTOCOL.
  @param[in]  MCUI2cBus           I2c bus number of MCU.
  @param[in]  Buffer              A pointer to update firmware data.
  @param[in]  Size                Size of update firmware to update.

  @retval     EFI_SUCCESS         Success.
              Other               Failed.
**/
EFI_STATUS
MCUFirmwareCheck (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus,
  IN UINT8        *Buffer,
  IN UINTN        Size
  );

/**
  Erase the MCU flash space.

  @param[in]  I2cMasterProtocol   The pointer to SOPHGO_I2C_MASTER_PROTOCOL.
  @param[in]  MCUI2cBus           I2c bus number of MCU.
  @param[in]  Size                Size of update firmware to update.

  @retval     EFI_SUCCESS         Success.
              Other               Failed.
**/
EFI_STATUS
MCUFirmwareErase (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus,
  IN UINTN        Size
  );

/**
  Program the MCU flash.

  @param[in]  I2cMasterProtocol   The pointer to SOPHGO_I2C_MASTER_PROTOCOL.
  @param[in]  MCUI2cBus           I2c bus number of MCU.
  @param[in]  Buffer              A pointer to update firmware data.
  @param[in]  Size                Size of update firmware to update.

  @retval     EFI_SUCCESS         Success.
              Other               Failed.
**/
EFI_STATUS
MCUFirmwareProgram (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus,
  IN UINT8        *Buffer,
  IN UINTN        Size
  );

/**
  Verify the MCU flash programming result.

  @param[in]  I2cMasterProtocol   The pointer to SOPHGO_I2C_MASTER_PROTOCOL.
  @param[in]  MCUI2cBus           I2c bus number of MCU.
  @param[in]  Buffer              A pointer to update firmware data.
  @param[in]  Size                Size of update firmware to update.

  @retval     EFI_SUCCESS         Success.
              Other               Failed.
**/
EFI_STATUS
MCUFirmwareVerify (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus,
  IN UINT8        *Buffer,
  IN UINTN        Size
  );

/**
  Set CursorPosition during the MCU flash programming process.
**/
VOID
MCUFirmwareCursorPosition(
  IN UINTN Columns,
  IN UINTN Rows
  );

#endif