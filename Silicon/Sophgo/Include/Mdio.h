/** @file
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/
#ifndef __MDIO_H__
#define __MDIO_H__

#define SOPHGO_MDIO_PROTOCOL_GUID { 0xFF09CED9, 0xEE82, 0x4B9C, { 0xB4, 0x0C, 0xB4, 0x65, 0x0C, 0x0A, 0x7A, 0xFC } }

typedef struct _SOPHGO_MDIO_PROTOCOL SOPHGO_MDIO_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *SOPHGO_MDIO_READ) (
  IN CONST SOPHGO_MDIO_PROTOCOL  *This,
  IN UINT32                      PhyAddr,
  IN UINT32                      PhyReg,
  IN UINT32                      *Data
  );

typedef
EFI_STATUS
(EFIAPI *SOPHGO_MDIO_WRITE) (
  IN CONST SOPHGO_MDIO_PROTOCOL  *This,
  IN UINT32                      PhyAddr,
  IN UINT32                      PhyReg,
  IN UINT32                      Data
  );

struct _SOPHGO_MDIO_PROTOCOL {
  SOPHGO_MDIO_READ               Read;
  SOPHGO_MDIO_WRITE              Write;
  UINTN                          BaseAddress;
  UINT32                         MiiAddr; /* MII Address */
  UINT32                         MiiData; /* MII Data */
  UINT32                         MiiAddrShift; /* MII address shift */
  UINT32                         MiiAddrMask; /* MII address mask */
  UINT32                         MiiRegShift; /* MII reg shift */
  UINT32                         MiiRegMask; /* MII reg mask */
  UINT32                         MiiClkCsrShift;
  UINT32                         MiiClkCsrMask;
};

extern EFI_GUID gSophgoMdioProtocolGuid;

#endif // __MDIO_H__
