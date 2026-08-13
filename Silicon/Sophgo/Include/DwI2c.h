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

/**
  I2C read operation - write data to the I2C slave then read data bytes back.

  This performs a combined I2C transaction: first writes WriteLen bytes
  (typically a register address), then reads ReadLen bytes from the slave.

  @param[in]   This       The pointer to SOPHGO_I2C_MASTER_PROTOCOL.
  @param[in]   I2c        I2c bus number.
  @param[in]   Addr       I2c slave address.
  @param[in]   WriteLen   Number of bytes to write before reading.
  @param[in]   WriteData  Data to write before reading (e.g. register offset).
  @param[in]   ReadLen    Number of bytes to read.
  @param[out]  ReadData   Buffer to store the read data.

  @retval  EFI_SUCCESS              Read data success.
  @retval  EFI_NOT_FOUND            Unable to find i2c slave with the given address.
  @retval  EFI_DEVICE_ERROR         There was an error during the transmission.
  @retval  EFI_TIMEOUT              Waiting for bus busy timedout or transfer timeout.

**/
typedef
EFI_STATUS
(EFIAPI *I2C_MASTER_READ) (
  IN  SOPHGO_I2C_MASTER_PROTOCOL  *This,
  IN  INT32                       I2c,
  IN  UINT8                       Addr,
  IN  UINT32                      WriteLen,
  IN  UINT8                       *WriteData,
  IN  UINT32                      ReadLen,
  OUT UINT8                       *ReadData
  );

/**
  I2C write operation - write data bytes to the I2C slave.

  @param[in]  This  The pointer to SOPHGO_I2C_MASTER_PROTOCOL.
  @param[in]  I2c   I2c bus number.
  @param[in]  Addr  I2c slave address.
  @param[in]  Len   Number of bytes to write.
  @param[in]  Data  Data to be written.

  @retval  EFI_SUCCESS              Write data success.
  @retval  EFI_NOT_FOUND            Unable to find i2c slave with the given address.
  @retval  EFI_DEVICE_ERROR         There was an error during the transmission.
  @retval  EFI_TIMEOUT              Waiting for bus busy timedout or transfer timeout.
  @retval  EFI_INVALID_PARAMETER    Invalid function parameter.

**/
typedef
EFI_STATUS
(EFIAPI *I2C_MASTER_WRITE) (
  IN        SOPHGO_I2C_MASTER_PROTOCOL  *This,
  IN        INT32                       I2c,
  IN        UINT8                       Addr,
  IN        UINT32                      Len,
  IN CONST  UINT8                       *Data
  );

struct _SOPHGO_I2C_MASTER_PROTOCOL {
  I2C_MASTER_READ   Read;
  I2C_MASTER_WRITE  Write;
};

extern EFI_GUID  gSophgoI2cMasterProtocolGuid;

#endif // __DWI2C_H__
