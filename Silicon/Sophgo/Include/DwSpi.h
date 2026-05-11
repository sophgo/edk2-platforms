/** @file
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __DWSPI_H__
#define __DWSPI_H__

#include <Uefi/UefiBaseType.h>

//
// Global ID for the SPI Protocol
//
#define SOPHGO_SPI_PROTOCOL_GUID         \
  { 0xCCEBDD3F, 0xCCD3, 0x4A1A,          \
    { 0xB4, 0x1A, 0x54, 0x4F, 0x56, 0xA2, 0x77, 0xF7 } }

//
// Protocol interface structure
//
typedef struct _SOPHGO_SPI_PROTOCOL SOPHGO_SPI_PROTOCOL;

typedef struct {
  UINT32      SpiBus;
  UINT8       Cs;
  UINT32      Mode;
  VOID        *ControllerState;
} SPI_DEVICE;

typedef struct {
  CONST VOID  *TxBuf;
  VOID        *RxBuf;
  UINT32      Len;              // size of TxBuf and RxBuf (in bytes)
  UINT32      BitsPerWord;      // select a BitsPerWord for this transfer. If 0 the default 8 is used.
  UINT32      SpeedHz;          // Select a speed for this transfer. If 0 the default is used.
  UINT32      EffectiveSpeedHz;
} SPI_TRANSFER;

typedef enum {
  SPI_MEM_NO_DATA,
  SPI_MEM_DATA_IN,
  SPI_MEM_DATA_OUT,
} SPI_MEM_DATA_DIR;

typedef struct {
  struct {
    UINT8  NBytes;      // number of opcode bytes (only 1 or 2 are valid).
                        // The opcode is sent MSB-first
    UINT16 OpCode;      // operation opcode
  } Cmd;

  struct {
    UINT8  NBytes;      // number of address bytes to send. Can be zero if
                        // the operation does not need to send an address
    UINT64 Val;         // address value. This value is always sent MSB first
                        // on the bus
  } Addr;

  struct {
    UINT8  NBytes;      // number of dummy bytes to send after an opcode or
                        // address. Can be zero if the operation does not
                        // require dummy bytes
  } Dummy;

  struct {
    SPI_MEM_DATA_DIR  Dir;
    UINT32            NBytes;  // number of data bytes to send/receive. Can be zero
                               // if the operation does not involve transferring data
    union {
      VOID        *In;  // input buffer
      CONST VOID  *Out; // output buffer
    } Buf;
  } Data;

  UINT32  SpeedHz;      // mem operation speed in HZ
} SPI_MEM_OP;

typedef
EFI_STATUS
(EFIAPI *SPI_SETUP_DEVICE) (
  IN     SOPHGO_SPI_PROTOCOL  *This,
  IN OUT SPI_DEVICE           *Spi,
  IN     UINT32               SpiBus,
  IN     UINT8                Cs,
  IN     UINT8                Mode
  );

typedef
EFI_STATUS
(EFIAPI *SPI_CLEANUP_DEVICE) (
  IN     SOPHGO_SPI_PROTOCOL  *This,
  IN OUT SPI_DEVICE           *Spi
  );

typedef
EFI_STATUS
(EFIAPI *SPI_TRANSFER_ONE) (
  IN     SOPHGO_SPI_PROTOCOL  *This,
  IN     SPI_DEVICE           *Spi,
  IN     SPI_TRANSFER         *Transfer
  );

typedef
EFI_STATUS
(EFIAPI *SPI_EXEC_MEM_OP) (
  IN     SOPHGO_SPI_PROTOCOL  *This,
  IN     SPI_DEVICE           *Spi,
  IN     SPI_MEM_OP           *Op
  );

struct _SOPHGO_SPI_PROTOCOL {
  SPI_SETUP_DEVICE    SpiSetupDevice;
  SPI_CLEANUP_DEVICE  SpiCleanupDevice;
  SPI_TRANSFER_ONE    SpiTransferOne;
  SPI_EXEC_MEM_OP     SpiExecMemOp;
};

extern EFI_GUID  gSophgoSpiProtocolGuid;

#endif // __DWSPI_H__
