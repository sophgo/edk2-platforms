/** @file
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __I2C_H__
#define __I2C_H__

EFI_STATUS
EFIAPI
I2cSmbusReadByte (
  IN  UINT32  I2c,
  IN  UINT8   Addr,
  IN  UINT8   Cmd,
  OUT UINT8   *Data
  );

EFI_STATUS
EFIAPI
I2cSmbusWriteByte (
  IN  UINT32  I2c,
  IN  UINT8   Addr,
  IN  UINT8   Cmd,
  IN  UINT8   Data
  );

EFI_STATUS
EFIAPI
I2cSmbusRead (
  IN  UINT32  I2c,
  IN  UINT8   Addr,
  IN  UINT32  Len,
  IN  UINT8   Cmd,
  OUT UINT8   *Data
  );

EFI_STATUS
EFIAPI
I2cSmbusWrite (
  IN        UINT32  I2c,
  IN        UINT8   Addr,
  IN        UINT8   Cmd,
  IN        UINT32  Len,
  IN CONST  UINT8   *Data
  );

#endif // __I2C_H__
