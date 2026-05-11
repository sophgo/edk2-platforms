/** @file
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __DWI2C_H__
#define __DWI2C_H__

#include <Uefi/UefiBaseType.h>

///
/// Global ID for the I2c Protocol
///
#define SOPHGO_I2C_MASTER_PROTOCOL_GUID  \
  { 0x79153B43, 0x18D3, 0x40E1,          \
    { 0x89, 0x4B, 0x06, 0x00, 0x80, 0x0F, 0xEE, 0x6F } }

//
// Protocol interface structure
//
typedef struct _SOPHGO_I2C_MASTER_PROTOCOL SOPHGO_I2C_MASTER_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *I2C_SMBUS_READ_BYTE) (
  IN  SOPHGO_I2C_MASTER_PROTOCOL  *This,
  IN  INT32                       I2c,
  IN  UINT8                       Addr,
  IN  UINT8                       Cmd,
  OUT UINT8                       *Data
  );

typedef
EFI_STATUS
(EFIAPI *I2C_SMBUS_WRITE_BYTE) (
  IN  SOPHGO_I2C_MASTER_PROTOCOL  *This,
  IN  INT32                       I2c,
  IN  UINT8                       Addr,
  IN  UINT8                       Cmd,
  IN  UINT8                       Data
  );

typedef
EFI_STATUS
(EFIAPI *I2C_SMBUS_READ) (
  IN  SOPHGO_I2C_MASTER_PROTOCOL  *This,
  IN  INT32                       I2c,
  IN  UINT8                       Addr,
  IN  UINT8                       Cmd,
  IN  UINT32                      Len,
  OUT UINT8                       *Data
  );

typedef
EFI_STATUS
(EFIAPI *I2C_SMBUS_WRITE) (
  IN        SOPHGO_I2C_MASTER_PROTOCOL  *This,
  IN        INT32                       I2c,
  IN        UINT8                       Addr,
  IN        UINT8                       Cmd,
  IN        UINT32                      Len,
  IN CONST  UINT8                       *Data
  );

struct _SOPHGO_I2C_MASTER_PROTOCOL {
  I2C_SMBUS_READ_BYTE   ReadByte;
  I2C_SMBUS_WRITE_BYTE  WriteByte;
  I2C_SMBUS_READ        Read;
  I2C_SMBUS_WRITE       Write;
};

extern EFI_GUID  gSophgoI2cMasterProtocolGuid;

#endif // __DWI2C_H__
