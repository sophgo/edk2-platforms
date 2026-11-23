/** @file
  The header file for firmware verification.

  Copyright (c) 2025, SOPHGO Technologies Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Include/Spifmc.h>
#include <Include/SpiNorFlash.h>

/**
  Verify the version number and signature information of the firmware to be upgraded

  @param[in]   DataToVerify          The data buffer of the firmware to be upgraded that has
                                     been loaded into memory.
  @param[in]   DataToVerifyLen       The byte length of the firmware data buffer.
  @param[in]   Nor                   Structure of nor flash.
  @param[in]   NorFlashProtocol      Nor Flash protocol.

  @retval  EFI_UNSUPPORTED           1: Unable to obtain the version of the current firmware.
                                     2: The signature verification algorithm does not support.
  @retval  EFI_INCOMPATIBLE_VERSION  The version of the firmware to be upgraded is lower
                                     than the version of the current firmware.
  @retval  EFI_NOT_FOUND             Unable to find the public key file (public_key.der) from
                                     the firmware to be upgraded
  @retval  EFI_ABORTED               1: Failed to parse public key.
                                     2: Signature verification of the firmware to be upgraded
                                        failed.
  @retval  EFI_SUCCESS               The firmware to be upgraded has passed verification.
**/
EFI_STATUS
FirmwareVerify (
  IN UINT8    *DataToVerify,
  IN UINTN    DataToVerifyLen,
  IN SPI_NOR  *Nor,
  IN SOPHGO_NOR_FLASH_PROTOCOL *NorFlashProtocol
  );

/**
  Get the version information of the firmware currently in use from NOR flash.

  @param[in]   Nor               Structure of nor flash.
  @param[in]   NorFlashProtocol  Nor Flash protocol.
  @param[out]  CurrentVer        The firmware version read from NOR flash.
  @param[in]   CurrentVerSize    The buffer size of the CurrentVer.

  @retval  EFI_UNSUPPORTED       The buffer size of CurrentVer is not greater than VER_STR_MAX_LEN
  @retval  EFI_SUCCESS           Successfully obtained firmware version.
**/
EFI_STATUS
GetCurrentVer (
  IN  SPI_NOR *Nor,
  IN  SOPHGO_NOR_FLASH_PROTOCOL *NorFlashProtocol,
  OUT CHAR8   *CurrentVer,
  IN  UINTN   CurrentVerSize
  );
