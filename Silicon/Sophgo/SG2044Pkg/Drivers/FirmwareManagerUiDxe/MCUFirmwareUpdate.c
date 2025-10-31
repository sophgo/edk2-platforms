/** @file
  The functions for firmware manager menu.

  Copyright (c) 2025, SOPHGO Technology Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "MCUFirmwareUpdate.h"
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseRiscVSbiLib.h>

#define MIN_FILE_SIZE       BASE_64KB
#define FWINFO_START(size)  ((size) - 128)
#define BOARD_TYPE_OFFSET   (0x14)

#define MCU_SLAVE_ADDR      (0x17)
#define REG_BOARD_TYPE      (0x00)
#define REG_FLASH_CMD       (0x63)
#define REG_FLASH_OFFSET    (0x7c)
#define REG_FLASH_DATA      (0x80)
#define REG_FLASH_FLUSH     (0xff)

#define FLASH_CMD_UNLOCK    (0x02)
#define FLASH_CMD_LOCK      (0x03)
#define FLASH_CMD_ERASE     (0x04)

#define FLASH_PAGE_SIZE     (8 * 1024)
#define FLASH_PAGE_MASK     (FLASH_PAGE_SIZE - 1)

#define ROUND_UP(x, align)    (((x) + ((align) - 1)) / (align) * (align))
#define ROUND_DOWN(x, align)  ((x) / (align) * (align))

STATIC UINTN CursorColumns, CursorRows;

EFI_STATUS
MCUDeviceMatch (
  VOID
  )
{
  if (!FixedPcdGetBool(PcdMcuExistence)) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MCUFlashSetOffset (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus,
  IN UINT32       Offset
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT8 Data[4];

  Data[0] = (Offset >> 24) & 0xFF;
  Data[1] = (Offset >> 16) & 0xFF;
  Data[2] = (Offset >> 8) & 0xFF;
  Data[3] = Offset & 0xFF;

  Status = I2cMasterProtocol->Write(
    I2cMasterProtocol,
    MCUI2cBus,
    MCU_SLAVE_ADDR,
    REG_FLASH_OFFSET,
    sizeof(Data),
    Data
    );
  if (EFI_ERROR(Status)) {
    Print (L"\rMCU flash set offset failed\n");
    return Status;
  }

  return Status;
}

STATIC
EFI_STATUS
MCUFlashSetBlockData (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus,
  IN UINT8        *Buffer,
  IN UINT8        Length
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT8 SmbusBlockMax = 16;
  UINT8 Reg, Slen, Left, Off = 0;

  Reg = REG_FLASH_DATA;

  while (Off < Length) {
    Left = Length - Off;
    if (Left >= SmbusBlockMax)
      Slen = SmbusBlockMax;
    else
      Slen = Left;

    Status = I2cMasterProtocol->Write(
      I2cMasterProtocol,
      MCUI2cBus,
      MCU_SLAVE_ADDR,
      Reg,
      Slen,
      (UINT8 *)Buffer + Off
      );
    if (EFI_ERROR(Status)) {
      Print (L"\rMCU flash set block data failed\n");
      return Status;
    }

    Off += Slen;
    Reg += Slen;
  }

  return Status;

}

STATIC
EFI_STATUS
MCUFlashGetBlockData (
  IN  SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN  UINT32       MCUI2cBus,
  OUT UINT8        *Buffer,
  IN  UINT8        Length
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT8 SmbusBlockMax = 16;
  UINT8 Reg, Slen, Left, Off = 0;

  Reg = REG_FLASH_DATA;

  while (Off < Length) {
    Left = Length - Off;
    if (Left >= SmbusBlockMax)
      Slen = SmbusBlockMax;
    else
      Slen = Left;

    Status = I2cMasterProtocol->Read(
      I2cMasterProtocol,
      MCUI2cBus,
      MCU_SLAVE_ADDR,
      Reg,
      Slen,
      (UINT8 *)Buffer + Off
      );
    if (EFI_ERROR(Status)) {
      Print (L"\rMCU flash Get block data failed\n");
      return Status;
    }

    Off += Slen;
    Reg += Slen;
  }

  return Status;
}

STATIC
EFI_STATUS
MCUFlashUnlock (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = I2cMasterProtocol->WriteByte(
    I2cMasterProtocol,
    MCUI2cBus,
    MCU_SLAVE_ADDR,
    REG_FLASH_CMD,
    FLASH_CMD_UNLOCK
    );

  if (EFI_ERROR(Status))
    Print (L"\rMCU flash unlock failed\n");


  return Status;
}

STATIC
EFI_STATUS
MCUFlashLock (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = I2cMasterProtocol->WriteByte(
    I2cMasterProtocol,
    MCUI2cBus,
    MCU_SLAVE_ADDR,
    REG_FLASH_CMD,
    FLASH_CMD_LOCK
    );

  if (EFI_ERROR(Status))
    Print (L"\rMCU flash lock failed\n");

  return Status;
}

STATIC
EFI_STATUS
MCUFlashErasePage (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32        MCUI2cBus,
  IN UINT32        Offset
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  if (Offset & FLASH_PAGE_MASK) {
    Print (L"\rOffset should page aligned when erase page\n");
		return EFI_INVALID_PARAMETER;
	}

  Status = MCUFlashSetOffset(
    I2cMasterProtocol,
    MCUI2cBus,
    Offset
    );

  Status = I2cMasterProtocol->WriteByte(
    I2cMasterProtocol,
    MCUI2cBus,
    MCU_SLAVE_ADDR,
    REG_FLASH_CMD,
    FLASH_CMD_ERASE
    );

  return Status;
}

STATIC
EFI_STATUS
MCUFlashErase (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32        MCUI2cBus,
  IN UINT32        Length
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32 Offset;

  if (Length & FLASH_PAGE_MASK) {
    Print (L"\rLength should page aligned when erase\n");
		return EFI_INVALID_PARAMETER;
	}

  for (Offset = 0; Offset < Length; Offset += FLASH_PAGE_SIZE) {
    gST->ConOut->SetCursorPosition (gST->ConOut, CursorColumns, CursorRows);
    Print (L"Erase page %08x", Offset);
    Status = MCUFlashErasePage(
      I2cMasterProtocol,
      MCUI2cBus,
      Offset
      );
    if (EFI_ERROR(Status)) {
      Print (L"\rMCU flash erase page failed\n");
      return Status;
    }
  }

  // DEBUG ((DEBUG_ERROR, "\n"));

  return Status;
}

STATIC
EFI_STATUS
MCUFlashWrite (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus,
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  CONST UINT8 FlashDataMax = 128;
  UINT32 Slen, Left, Xoff = 0;
  UINT8 Tmp;
  UINT8 FlashDataBuffer[FlashDataMax];
  BOOLEAN WriteFlag;

  while (Xoff <  Size) {
    Left = Size - Xoff;

    if (Left >= FlashDataMax)
      Slen = FlashDataMax;
    else
      Slen = Left;

    SetMem(FlashDataBuffer, sizeof(FlashDataBuffer), 0xFF);
    CopyMem(FlashDataBuffer, (UINT8 *)Buffer + Xoff, Slen);

    WriteFlag = FALSE;
    for (Tmp = 0; Tmp < FlashDataMax; Tmp ++) {
      if (FlashDataBuffer[Tmp] != 0xFF) {
        WriteFlag = TRUE;
        break;
      }
    }

    if (WriteFlag) {
      Status = MCUFlashSetOffset(
        I2cMasterProtocol,
        MCUI2cBus,
        Xoff
        );

      if (EFI_ERROR(Status))
        return Status;

      Status = MCUFlashSetBlockData(
        I2cMasterProtocol,
        MCUI2cBus,
        FlashDataBuffer,
        FlashDataMax
        );

      if (EFI_ERROR(Status))
        return Status;

    }
    gST->ConOut->SetCursorPosition (gST->ConOut, CursorColumns, CursorRows);
    Print (L"Program flash %08X %d%%", Xoff, Xoff * 100 / Size);

    Xoff += Slen;
  }

  gST->ConOut->SetCursorPosition (gST->ConOut, CursorColumns, CursorRows);
  Print (L"Program flash %08X 100%%", Xoff);

  return Status;
}

STATIC
EFI_STATUS
MCUFlashRead(
  IN  SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN  UINT32       MCUI2cBus,
  OUT VOID         *Data,
  IN  UINTN        Size
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  CONST UINT8 FlashDataMax = 128;
  UINT32 Slen, Left, Xoff = 0;

  while(Xoff < Size) {
    Left = Size - Xoff;
    if(Left >= FlashDataMax)
      Slen = FlashDataMax;
    else
      Slen = Left;

    Status = MCUFlashSetOffset(
      I2cMasterProtocol,
      MCUI2cBus,
      Xoff
      );
    if (EFI_ERROR(Status))
      return Status;

    Status = MCUFlashGetBlockData(
      I2cMasterProtocol,
      MCUI2cBus,
      (UINT8 *)Data + Xoff,
      Slen
      );

    if (EFI_ERROR(Status))
      return Status;

    gST->ConOut->SetCursorPosition (gST->ConOut, CursorColumns, CursorRows);
    Print (L"Verify  flash  %08X  %d%%", Xoff, Xoff * 100 / Size);

    Xoff += Slen;
  }

  gST->ConOut->SetCursorPosition (gST->ConOut, CursorColumns, CursorRows);
  Print (L"Verify  flash  %08X  100%%", Xoff);

  return Status;
}

EFI_STATUS
MCUFirmwareCheck (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus,
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT8 BoardTypeInFile = 0;
  UINT8 BoardType = 0;

  Status = I2cMasterProtocol->ReadByte(
    I2cMasterProtocol,
    MCUI2cBus,
    MCU_SLAVE_ADDR,
    REG_BOARD_TYPE,
    &BoardType
    );

  gST->ConOut->SetCursorPosition (gST->ConOut, CursorColumns, CursorRows);
  if (EFI_ERROR(Status)) {
    Print (L"Failed to retrieve board type\n");
    return Status;
  }

  if (Buffer == NULL || Size < MIN_FILE_SIZE)
    return EFI_BAD_BUFFER_SIZE;

  BoardTypeInFile = *((UINT8*)Buffer + FWINFO_START(Size) + BOARD_TYPE_OFFSET);
  if (BoardType != BoardTypeInFile) {
    Print (L"BoardType=0x%x, BoardTypeInFile=0x%x\n",
          BoardType, BoardTypeInFile);
    return EFI_CRC_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
MCUFirmwareErase (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus,
  IN UINTN        Size
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = MCUFlashUnlock(I2cMasterProtocol, MCUI2cBus);
  if (EFI_ERROR(Status))
    return Status;

  Status = MCUFlashErase(
    I2cMasterProtocol,
    MCUI2cBus,
    (UINT32)ROUND_UP(Size, FLASH_PAGE_SIZE)
    );
  if (EFI_ERROR(Status)) {
    Print (L"\rMCU flash erase failed\n");
    MCUFlashLock(I2cMasterProtocol, MCUI2cBus);
    return Status;
  }

  Status = MCUFlashLock(I2cMasterProtocol, MCUI2cBus);
  if (EFI_ERROR(Status))
    return Status;

  return EFI_SUCCESS;
}

EFI_STATUS
MCUFirmwareProgram (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus,
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = MCUFlashUnlock(I2cMasterProtocol, MCUI2cBus);
  if (EFI_ERROR(Status))
    return Status;

  Status = MCUFlashWrite(
    I2cMasterProtocol,
    MCUI2cBus,
    Buffer,
    Size
    );
  if (EFI_ERROR(Status)) {
    Print (L"\rMCU flash write failed");
    MCUFlashLock(I2cMasterProtocol, MCUI2cBus);
    return Status;
  }

  Status = MCUFlashLock(I2cMasterProtocol, MCUI2cBus);
  if (EFI_ERROR(Status))
    return Status;

  return EFI_SUCCESS;
}

EFI_STATUS
MCUFirmwareVerify (
  IN SOPHGO_I2C_MASTER_PROTOCOL  *I2cMasterProtocol,
  IN UINT32       MCUI2cBus,
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  VOID *ReadBack;

  ReadBack = AllocateZeroPool(Size);
  Status = MCUFlashRead(
    I2cMasterProtocol,
    MCUI2cBus,
    ReadBack,
    Size
    );
  if (EFI_ERROR(Status))
    goto ExitChannel;

  gST->ConOut->SetCursorPosition (gST->ConOut, CursorColumns, CursorRows);
  if (CompareMem(Buffer, ReadBack, Size)) {
    Print (L"\rMCU Flash verify failed");
    Status = EFI_CRC_ERROR;
  } else {
    Print (L"MCU Flash verify successfully");
  }


ExitChannel:
  FreePool (ReadBack);
  return Status;
}

VOID
MCUFirmwareCursorPosition(
  IN UINTN Columns,
  IN UINTN Rows
 )
{
  CursorColumns = Columns;
  CursorRows = Rows;

  return;
}