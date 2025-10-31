/** @file

  Copyright (c) 2025, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/EfuseLib.h>

#define EFUSE_MODE             0x00
#define EFUSE_ADR              0x04
#define EFUSE_RD_DATA          0x0c

#define EMBEDDED_READ_MODE     0b10
#define EMBEDDED_WRITE_MODE    0b11

typedef struct {
  UINTN   Regs;
  UINT32  NumAddrBits;
  UINT32  NumCells;
  UINT32  CellWidth;
} SG_EFUSE_DEVICE;

STATIC  SG_EFUSE_DEVICE  *mEfuseArray;
STATIC  UINT32           mNumberOfControllers;

STATIC
VOID
EfuseWrite32 (
  IN  SG_EFUSE_DEVICE  *SgEfuse,
  IN  UINT32           OffSet,
  IN  UINT32           Value
  )
{
  MmioWrite32 ((UINTN)(SgEfuse->Regs + OffSet), Value);
}

STATIC
UINT32
EfuseRead32 (
  IN  SG_EFUSE_DEVICE  *SgEfuse,
  IN  UINT32           OffSet
  )
{
  return MmioRead32 ((UINTN)(SgEfuse->Regs + OffSet));
}

STATIC
VOID
EfuseModeWaitReady (
  IN  SG_EFUSE_DEVICE  *SgEfuse
  )
{
  while ((EfuseRead32 (SgEfuse, EFUSE_MODE) & 0b11) != 0)
    ;
}

STATIC
VOID
EfuseModeReset (
  IN  SG_EFUSE_DEVICE  *SgEfuse
  )
{
  EfuseWrite32 (SgEfuse, EFUSE_MODE, 0);
  EfuseModeWaitReady (SgEfuse);
}

STATIC
VOID
EfuseModeMdWrite (
  IN  SG_EFUSE_DEVICE  *SgEfuse,
  IN  UINT32           Val
  )
{
  UINT32 Mode = EfuseRead32 (SgEfuse, EFUSE_MODE);
  UINT32 New  = (Mode & 0xfffffffc) | (Val & 0b11);

  EfuseWrite32 (SgEfuse, EFUSE_MODE, New);
}

STATIC
UINT32
MakeAdrVal (
  IN  SG_EFUSE_DEVICE  *SgEfuse,
  IN  UINT32           Address,
  IN  UINT32           BitIndex
  )
{
  CONST UINT32  NumAddrBits = SgEfuse->NumAddrBits;
  CONST UINT32  AddressMask = (1 << NumAddrBits) - 1;

  return (Address & AddressMask) |
    ((BitIndex & 0x1f) << NumAddrBits);
}

STATIC
VOID
EfuseSetBit (
  IN  SG_EFUSE_DEVICE  *SgEfuse,
  IN  UINT32           Address,
  IN  UINT32           BitIndex
  )
{
  UINT32 AdrVal = MakeAdrVal (SgEfuse, Address, BitIndex);

  EfuseWrite32 (SgEfuse, EFUSE_ADR, AdrVal);
  EfuseModeMdWrite (SgEfuse, EMBEDDED_WRITE_MODE);
  EfuseModeWaitReady (SgEfuse);
}

STATIC
UINT32
EfuseEmbeddedRead (
  IN  SG_EFUSE_DEVICE  *SgEfuse,
  IN  UINT32           Address
  )
{
  UINT32  AdrVal;
  UINT32  ReadVal = 0;

  EfuseModeReset (SgEfuse);
  AdrVal = MakeAdrVal (SgEfuse, Address, 0);
  EfuseWrite32 (SgEfuse, EFUSE_ADR, AdrVal);
  EfuseModeMdWrite (SgEfuse, EMBEDDED_READ_MODE);
  EfuseModeWaitReady (SgEfuse);

  ReadVal = EfuseRead32 (SgEfuse, EFUSE_RD_DATA);

  return ReadVal;
}

STATIC
VOID
EfuseEmbeddedWrite (
  IN  SG_EFUSE_DEVICE  *SgEfuse,
  IN  UINT32           Address,
  IN  UINT32           Val
  )
{
  INT32 Loop;

  for (Loop = 0; Loop < 32; Loop++)
    if ((Val >> Loop) & 1)
      EfuseSetBit (SgEfuse, Address, Loop);
}

STATIC
UINT32
EfuseRead (
  IN   SG_EFUSE_DEVICE  *SgEfuse,
  IN   UINT32           Offset,
  IN   UINT32           Count,
  OUT  VOID             *Val
  )
{
  INT32  OpSize, Left, Start, Loop;
  UINT32 Tmp;
  UINT8  *Dst;

  Left = Count;
  Dst  = Val;

  //
  // head
  //
  if (Offset & 0x03) {
    OpSize = MIN (4 - (Offset & 0x03), Left);
    Start = (Offset & 0x03);
    Tmp = EfuseEmbeddedRead (SgEfuse, Offset >> 2);
    CopyMem (Dst, &((UINT8 *)&Tmp)[Start], OpSize);
    Dst    += OpSize;
    Left   -= OpSize;
    Offset += OpSize;
  }

  //
  // body
  //
  OpSize = Left >> 2;
  for (Loop = 0; Loop < OpSize; ++Loop) {
    Tmp = EfuseEmbeddedRead (SgEfuse, Offset >> 2);
    CopyMem (Dst, &Tmp, 4);
    Dst    += 4;
    Left   -= 4;
    Offset += 4;
  }

  //
  // tail
  //
  if (Left) {
    Tmp = EfuseEmbeddedRead (SgEfuse, Offset >> 2);
    CopyMem (Dst, &Tmp, Left);
  }

  return Count;
}

STATIC
UINT32
EfuseWrite (
  IN   SG_EFUSE_DEVICE  *SgEfuse,
  IN   UINT32           Offset,
  IN   UINT32           Count,
  IN   VOID             *Val
  )
{
  INT32  OpSize, Left, Start, Loop;
  UINT32 Tmp;
  UINT8  *Dst;

  Left = Count;
  Dst  = Val;

  //
  // head
  //
  if (Offset & 0x03) {
    Tmp = 0;
    OpSize = MIN (4 - (Offset & 0x03), Left);
    Start = (Offset & 0x03);
    CopyMem (&((UINT8 *)&Tmp)[Start], Dst, OpSize);
    EfuseEmbeddedWrite (SgEfuse, Offset >> 2, Tmp);
    Dst    += OpSize;
    Left   -= OpSize;
    Offset += OpSize;
  }

  //
  // body
  //
  OpSize = Left >> 2;
  for (Loop = 0; Loop < OpSize; ++Loop) {
    CopyMem (&Tmp, Dst, 4);
    EfuseEmbeddedWrite (SgEfuse, Offset >> 2, Tmp);
    Dst    += 4;
    Left   -= 4;
    Offset += 4;
  }

  //
  // tail
  //
  if (Left) {
    Tmp = 0;
    CopyMem (&Tmp, Dst, Left);
    EfuseEmbeddedWrite (SgEfuse, Offset >> 2, Tmp);
  }

  return Count;
}

EFI_STATUS
EFIAPI
EfuseReadBytes (
  IN   UINT32    BusNum,
  IN   UINT32    Offset,
  IN   UINT32    Count,
  OUT  VOID      *Buffer
  )
{
  SG_EFUSE_DEVICE  *SgEfuse;

  if (BusNum >= mNumberOfControllers) {
    DEBUG ((DEBUG_ERROR, "Efuse read error! Invalid BusNum\n"));
    return EFI_INVALID_PARAMETER;
  }
  SgEfuse = &mEfuseArray[BusNum];

  EfuseRead (SgEfuse, Offset, Count, Buffer);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EfuseWriteBytes (
  IN   UINT32    BusNum,
  IN   UINT32    Offset,
  IN   UINT32    Count,
  IN   VOID      *Buffer
  )
{
  SG_EFUSE_DEVICE  *SgEfuse;

  if (BusNum >= mNumberOfControllers) {
    DEBUG ((DEBUG_ERROR, "Efuse write error! Invalid BusNum\n"));
    return EFI_INVALID_PARAMETER;
  }
  SgEfuse = &mEfuseArray[BusNum];

  EfuseWrite (SgEfuse, Offset, Count, Buffer);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetEfuseInfo (
  IN   UINT32      BusNum,
  OUT  EFUSE_INFO  *EfuseInfo,
  OUT  UINT32      *EfuseNum
  )
{
  SG_EFUSE_DEVICE  *SgEfuse;

  if (EfuseNum != NULL)
    *EfuseNum = mNumberOfControllers;

  if (BusNum >= mNumberOfControllers) {
    DEBUG ((DEBUG_ERROR, "Efuse read error! Invalid BusNum\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (EfuseInfo != NULL) {
    SgEfuse = &mEfuseArray[BusNum];

    EfuseInfo->Regs      = SgEfuse->Regs;
    EfuseInfo->NumCells  = SgEfuse->NumCells;
    EfuseInfo->CellWidth = SgEfuse->CellWidth;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetControllerInfoByPcd (
  VOID
  )
{
  UINT64                *EfuseBaseAddresses;
  UINT32                *EfuseNumAddrBits;
  UINT32                *EfuseNumCells;
  UINT32                *EfuseCellWidth;
  UINT32                Index;

  mNumberOfControllers = FixedPcdGet32 (PcdEfuseControllerNum);
  EfuseBaseAddresses   = (UINT64 *)PcdGetPtr (PcdEfuseBase);
  EfuseNumAddrBits     = (UINT32 *)PcdGetPtr (PcdEfuseNumAddrBits);
  EfuseNumCells        = (UINT32 *)PcdGetPtr (PcdEfuseNumCells);
  EfuseCellWidth       = (UINT32 *)PcdGetPtr (PcdEfuseCellWidth);

  if ((mNumberOfControllers == 0) || (EfuseBaseAddresses == NULL) ||
      (EfuseNumAddrBits == NULL) || (EfuseNumCells == NULL) || (EfuseCellWidth == NULL)) {
    DEBUG ((DEBUG_ERROR, "No EFUSE controller found\n"));
    return EFI_NOT_FOUND;
  }

  mEfuseArray = AllocateZeroPool (mNumberOfControllers * sizeof (SG_EFUSE_DEVICE));
  if (mEfuseArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < mNumberOfControllers; Index++) {
    mEfuseArray[Index].Regs          = EfuseBaseAddresses[Index];
    mEfuseArray[Index].NumAddrBits   = EfuseNumAddrBits[Index];
    mEfuseArray[Index].NumCells      = EfuseNumCells[Index];
    mEfuseArray[Index].CellWidth     = EfuseCellWidth[Index];
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SetMemory (
  VOID
  )
{
  EFI_STATUS       Status;
  UINT32           Index;
  SG_EFUSE_DEVICE  *SgEfuse;

  for (Index = 0; Index < mNumberOfControllers; ++Index) {
    SgEfuse = &mEfuseArray[Index];
    Status  = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    SgEfuse->Regs,
                    SIZE_4KB,
                    EFI_MEMORY_UC | EFI_MEMORY_XP
                    );
    if (Status == EFI_ACCESS_DENIED)
      goto init;

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a:%d] Add memory space failed: %r\n",
            __func__, __LINE__, Status));
      return Status;
    }

  init:
    Status = gDS->SetMemorySpaceAttributes (
                    SgEfuse->Regs,
                    SIZE_4KB,
                    EFI_MEMORY_UC | EFI_MEMORY_XP
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a:%d] Set memory attributes failed: %r\n",
              __func__, __LINE__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EfuseLibConstructor (
  VOID
  )
{
  EFI_STATUS   Status;

  DEBUG ((DEBUG_INFO, "%a: GetControllerInfoByPcd\n", __func__));
  Status = GetControllerInfoByPcd ();
  if (EFI_ERROR (Status))
    return Status;

  Status = SetMemory ();
  if (EFI_ERROR (Status)) {
    FreePool (mEfuseArray);
    return Status;
  }

  return Status;
}

EFI_STATUS
EFIAPI
EfuseLibDestructor (
  VOID
  )
{
  if (mEfuseArray != NULL)
    FreePool (mEfuseArray);

  return EFI_SUCCESS;
}
