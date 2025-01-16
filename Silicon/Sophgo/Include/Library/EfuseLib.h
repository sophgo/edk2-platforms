/** @file
Function declarations about eFuse.

Copyright (c) 2025, Sophgo. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef EFUSE_LIB_
#define EFUSE_LIB_

typedef struct {
  UINTN   Regs;
  UINT32  NumCells;
  UINT32  CellWidth;
} EFUSE_INFO;

/**
  Get eFuse information and number of controllers.

  @param[in]   BusNum            The index of the eFuse controller.
  @param[out]  EfuseInfo         The eFuse information, can be NULL.
  @param[out]  EfuseNum          The number of eFuse controllers, can be NULL.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Cannot get the eFuse controller with the given BusNum.

**/
EFI_STATUS
EFIAPI
GetEfuseInfo (
  IN   UINT32      BusNum,
  OUT  EFUSE_INFO  *EfuseInfo,
  OUT  UINT32      *EfuseNum
  );

/**
  Write data to eFuse in bytes.

  @param[in]   BusNum            The index of the eFuse controller.
  @param[in]   Offset            The offset for writing data.
  @param[in]   Count             The number of bytes of data to be written.
  @param[in]   Buffer            The buffer for storing data to be written.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Cannot get the eFuse controller with the given BusNum.

**/
EFI_STATUS
EFIAPI
EfuseWriteBytes (
  IN   UINT32    BusNum,
  IN   UINT32    Offset,
  IN   UINT32    Count,
  IN   VOID      *Buffer
  );

/**
  Read data from eFuse in bytes.

  @param[in]   BusNum            The index of the eFuse controller.
  @param[in]   Offset            The offset for reading data.
  @param[in]   Count             The number of bytes of data to be read.
  @param[out]  Buffer            The buffer for storing data to be read in.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Cannot get the eFuse controller with the given BusNum.

**/
EFI_STATUS
EFIAPI
EfuseReadBytes (
  IN   UINT32    BusNum,
  IN   UINT32    Offset,
  IN   UINT32    Count,
  OUT  VOID      *Buffer
  );

#endif /* EFUSE_LIB_ */
