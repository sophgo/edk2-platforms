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

#define EFUSE_MODE             0x00
#define EFUSE_ADR              0x04
#define EFUSE_RD_DATA          0x0c

#define EMBEDDED_READ_MODE     0b10
#define EMBEDDED_WRITE_MODE    0b11

/* 88 is cell index
 * 4 is 32bits-per-cell
 */
#define EFUSE_DRAM_INFO_INDEX		(88)
#define EFUSE_CELL_SIZE			(4)
#define EFUSE_DRAM_INFO_OFFSET_0	(EFUSE_DRAM_INFO_INDEX * EFUSE_CELL_SIZE)
#define EFUSE_DRAM_INFO_OFFSET_1	((EFUSE_DRAM_INFO_INDEX + 1) * EFUSE_CELL_SIZE)

#define EFUSE_MISC_INFO_INDEX		(94)
#define EFUSE_MISC_INFO_OFFSET_0	(EFUSE_MISC_INFO_INDEX * EFUSE_CELL_SIZE)
#define EFUSE_MISC_INFO_OFFSET_1	((EFUSE_MISC_INFO_INDEX + 1) * EFUSE_CELL_SIZE)

#define SCS_CONFIG 0x804
#define PUBKEY_HASH 0x830

#define GB(n)	((n) * 1024 * 1024 * 1024)
#define MT(n)	((n) * 1000 * 1000)

typedef struct {
  UINTN   Regs;
  UINT32  NumAddrBits;
  UINT32  NumCells;
  UINT32  CellWidth;
} SG_EFUSE_DEVICE;

STATIC  SG_EFUSE_DEVICE  *mEfuseArray;
STATIC  UINT32           mNumberOfControllers;

// static const char *vendor[] = { "Micron", "Hynix", "CXMT", "" };
static const UINT64 capacity[] = { GB(16UL), GB(8UL), 0, 0 };
static const UINT64 data_rate[] = { MT(8533UL), MT(9600UL), 0, 0 };
static const UINT64 channel_number[] = {8, 4, 0, 0};
static const UINT64 channel_map[] = {
	(1 << 1) | (1 << 2) | (1 << 5) | (1 << 6),
	(1 << 0) | (1 << 3) | (1 << 4) | (1 << 7),
	0,
	0,
};

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
GetMemSizeFromEfusePei (
  OUT  UINT64    *MemSize
  )
{
  UINT32                 Flags  = 0;
  UINT32                 Flags0 = 0;
  UINT32                 Flags1 = 0;
  UINT64                *EfuseBaseAddresses;
  UINT32                *EfuseNumAddrBits;
  UINT32                *EfuseNumCells;
  UINT32                *EfuseCellWidth;
  UINT32                 Index;
  UINT64                 Capacity = 0;
  UINT64                 DataRate = 0;
  UINT64                 ChannelNum = 0;
  UINT64                 ChannelMap = 0;
  EFI_STATUS             Status;

  if (MemSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  mNumberOfControllers = FixedPcdGet32 (PcdEfuseControllerNum);
  EfuseBaseAddresses   = (UINT64 *)PcdGetPtr (PcdEfuseBase);
  EfuseNumAddrBits     = (UINT32 *)PcdGetPtr (PcdEfuseNumAddrBits);
  EfuseNumCells        = (UINT32 *)PcdGetPtr (PcdEfuseNumCells);
  EfuseCellWidth       = (UINT32 *)PcdGetPtr (PcdEfuseCellWidth);

  if ((mNumberOfControllers < 2) || (EfuseBaseAddresses == NULL) ||
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

  Status = EfuseReadBytes(1, EFUSE_MISC_INFO_OFFSET_0, sizeof(Flags0), &Flags0);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to read EFUSE_MISC_INFO_OFFSET_0\n"));
    return Status;
  }
  Status = EfuseReadBytes(1, EFUSE_MISC_INFO_OFFSET_1, sizeof(Flags1), &Flags1);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to read EFUSE_MISC_INFO_OFFSET_1\n"));
    return Status;
  }

  Flags = Flags0 | Flags1;
  if ((Flags >> 14) & 1) {
    *MemSize = 128 * 1024 * 1024 * 1024ULL;
  } else {
    EfuseReadBytes(1, EFUSE_DRAM_INFO_OFFSET_0, sizeof(Flags0), &Flags0);
    EfuseReadBytes(1, EFUSE_DRAM_INFO_OFFSET_1, sizeof(Flags1), &Flags1);
    Flags = Flags0 | Flags1;
    if (((Flags >> 12) & 0x03) == 0 || ((Flags >> 12) & 0x03) == 0x03) {
      *MemSize = 128 * 1024 * 1024 * 1024ULL;
      // DEBUG ((DEBUG_ERROR, "Failed to get memory size from efuse\n"));
      // return EFI_NOT_FOUND;
    } else {
      Capacity = capacity[(Flags >> 2) & 0x03];
      DataRate = data_rate[(Flags >> 4) & 0x03];
      ChannelNum = channel_number[(Flags >> 6) & 0x03];
      if (ChannelNum == 4) {
        ChannelMap = channel_map[(Flags >> 10) & 0x03];
      } else {
        ChannelMap = 0xFF;
      }

      *MemSize = Capacity * ChannelNum;
      DEBUG ((DEBUG_VERBOSE, "Efuse: Memory Capacity = %lu GB\n", *MemSize / (1024 * 1024 * 1024)));
    }
  }
  if (mEfuseArray != NULL)
    FreePool (mEfuseArray);

  return EFI_SUCCESS;
}