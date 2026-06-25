/** @file
  Read Builtin FRU (device 0) from BMC via IPMI and cache serial numbers.

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <IndustryStandard/IpmiFruInformationStorage.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/Ipmi.h>

#include <Library/IpmiCommandLib.h>
#include <Library/IpmiFruInfoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>

#define FRU_DEVICE_ID                   0
#define FRU_READ_CHUNK                  32
#define FRU_READ_RETRY_MAX              64
#define FRU_READ_CHUNK_DELAY_US         200000
#define FRU_READ_AREA_DELAY_US          200000
#define FRU_READ_RETRY_LOG_INTERVAL     8
#define FRU_AREA_MAX                    256

//
// Uncomment the next line to enable FRU hex dump (per-area + full 256B image).
//
// #define FRU_DEBUG_HEX_DUMP
#define FRU_SERIAL_MAX          64
#define FRU_FIELD_DEFAULT       ""

STATIC CHAR8       mFruChassisSerial[FRU_SERIAL_MAX]  = FRU_FIELD_DEFAULT;
STATIC CHAR8       mFruBoardSerial[FRU_SERIAL_MAX]    = FRU_FIELD_DEFAULT;
STATIC CHAR8       mFruProductSerial[FRU_SERIAL_MAX] = FRU_FIELD_DEFAULT;
STATIC CHAR8       *mFruDataInfo[FruFieldIdMax] = {
  mFruChassisSerial,
  mFruBoardSerial,
  mFruProductSerial
};

STATIC BOOLEAN  mFruInfoLoaded = FALSE;

STATIC
VOID
FruLogIpmiRetry (
  IN CONST CHAR8   *Operation,
  IN UINTN         Attempt,
  IN EFI_STATUS    LastStatus
  )
{
  if ((Attempt == 1) ||
      (Attempt == (FRU_READ_RETRY_MAX - 1)) ||
      ((Attempt % FRU_READ_RETRY_LOG_INTERVAL) == 0))
  {
    DEBUG ((
      DEBUG_INFO,
      "IpmiFruInfo: retry %a attempt %u/%u last %r\n",
      Operation,
      Attempt,
      FRU_READ_RETRY_MAX - 1,
      LastStatus
      ));
  }
}

STATIC
EFI_STATUS
FruGetDeviceIdWithRetry (
  OUT IPMI_GET_DEVICE_ID_RESPONSE  *DeviceId
  )
{
  EFI_STATUS  Status;
  UINTN       Attempt;

  if (DeviceId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_DEVICE_ERROR;

  for (Attempt = 0; Attempt < FRU_READ_RETRY_MAX; Attempt++) {
    if (Attempt > 0) {
      FruLogIpmiRetry ("IpmiGetDeviceId", Attempt, Status);
      MicroSecondDelay (FRU_READ_CHUNK_DELAY_US);
    }

    Status = IpmiGetDeviceId (DeviceId);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (DeviceId->CompletionCode != IPMI_COMP_CODE_NORMAL) {
      Status = EFI_DEVICE_ERROR;
      continue;
    }

    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: IpmiGetDeviceId failed after %u attempts %r\n",
    __func__,
    FRU_READ_RETRY_MAX,
    Status
    ));
  return Status;
}

STATIC
EFI_STATUS
FruGetFruInventoryAreaInfoWithRetry (
  IN  UINT8                                      DeviceId,
  OUT IPMI_GET_FRU_INVENTORY_AREA_INFO_RESPONSE  *AreaResponse
  )
{
  EFI_STATUS                                 Status;
  IPMI_GET_FRU_INVENTORY_AREA_INFO_REQUEST   AreaRequest;
  UINTN                                      Attempt;

  if (AreaResponse == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_DEVICE_ERROR;

  for (Attempt = 0; Attempt < FRU_READ_RETRY_MAX; Attempt++) {
    if (Attempt > 0) {
      FruLogIpmiRetry ("IpmiGetFruInventoryAreaInfo", Attempt, Status);
      MicroSecondDelay (FRU_READ_CHUNK_DELAY_US);
    }

    AreaRequest.DeviceId = DeviceId;
    Status               = IpmiGetFruInventoryAreaInfo (&AreaRequest, AreaResponse);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (AreaResponse->CompletionCode != IPMI_COMP_CODE_NORMAL) {
      Status = EFI_DEVICE_ERROR;
      continue;
    }

    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: IpmiGetFruInventoryAreaInfo failed after %u attempts %r\n",
    __func__,
    FRU_READ_RETRY_MAX,
    Status
    ));
  return Status;
}

STATIC CONST CHAR8  mFruBcdPlus[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  ' ', '-', '.', ':', ',', '_'
};

STATIC
EFI_STATUS
FruReadBytes (
  IN  UINT16  Offset,
  IN  UINT16  Length,
  OUT UINT8   *Buffer
  )
{
  EFI_STATUS                   Status;
  IPMI_READ_FRU_DATA_REQUEST    Request;
  UINT8                        Temp[FRU_READ_CHUNK + sizeof (IPMI_READ_FRU_DATA_RESPONSE)];
  IPMI_READ_FRU_DATA_RESPONSE  *Response;
  UINT32                       ResponseSize;
  UINT16                       Cursor;
  UINT16                       Finish;

  if ((Buffer == NULL) || (Length == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Cursor = Offset;
  Finish = Offset + Length;

  while (Cursor < Finish) {
    UINT8   Count;
    UINTN   Attempt;
    BOOLEAN ChunkOk;

    if (Cursor > Offset) {
      MicroSecondDelay (FRU_READ_CHUNK_DELAY_US);
    }

    Count   = (UINT8)MIN ((UINT16)FRU_READ_CHUNK, Finish - Cursor);
    ChunkOk = FALSE;
    Status  = EFI_DEVICE_ERROR;

    for (Attempt = 0; Attempt < FRU_READ_RETRY_MAX; Attempt++) {
      if (Attempt > 0) {
        if ((Attempt == 1) ||
            (Attempt == (FRU_READ_RETRY_MAX - 1)) ||
            ((Attempt % FRU_READ_RETRY_LOG_INTERVAL) == 0))
        {
          DEBUG ((
            DEBUG_INFO,
            "%a: retry FRU read off=0x%04x len=%u attempt %u/%u last %r\n",
            __func__,
            Cursor,
            Count,
            Attempt,
            FRU_READ_RETRY_MAX - 1,
            Status
            ));
        }

        MicroSecondDelay (FRU_READ_CHUNK_DELAY_US);
      }

      ZeroMem (&Request, sizeof (Request));
      Request.DeviceId        = FRU_DEVICE_ID;
      Request.InventoryOffset = Cursor;
      Request.CountToRead     = Count;

      ResponseSize = sizeof (IPMI_READ_FRU_DATA_RESPONSE) + Count;
      Status       = IpmiReadFruData (&Request, (IPMI_READ_FRU_DATA_RESPONSE *)Temp, &ResponseSize);
      if (EFI_ERROR (Status)) {
        continue;
      }

      Response = (IPMI_READ_FRU_DATA_RESPONSE *)Temp;
      if (Response->CompletionCode != IPMI_COMP_CODE_NORMAL) {
        Status = EFI_DEVICE_ERROR;
        continue;
      }

      if (Response->CountReturned == 0) {
        Status = EFI_DEVICE_ERROR;
        continue;
      }

      CopyMem (Buffer, Response->Data, Response->CountReturned);
      Buffer += Response->CountReturned;
      Cursor += Response->CountReturned;
      ChunkOk = TRUE;
      break;
    }

    if (!ChunkOk) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: FRU read failed off=0x%04x len=%u after %u attempts %r\n",
        __func__,
        Cursor,
        Count,
        FRU_READ_RETRY_MAX,
        Status
        ));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
CHAR8 *
FruDecodeField (
  IN     UINT8   *AreaData,
  IN OUT UINT16  *Offset
  )
{
  UINT8   TypeCode;
  UINT8   Length;
  UINT16  Pos;
  CHAR8   *String;
  UINT8   Index;

  if ((AreaData == NULL) || (Offset == NULL)) {
    return NULL;
  }

  Pos      = *Offset;
  TypeCode = (AreaData[Pos] & 0xC0) >> 6;
  Length   = AreaData[Pos++] & 0x3F;
  if (Length == 0) {
    *Offset = Pos;
    return NULL;
  }

  if (TypeCode == 0) {
    *Offset = Pos + Length;
    return NULL;
  }

  String = AllocateZeroPool (Length + 1);
  if (String == NULL) {
    return NULL;
  }

  if (TypeCode == 3) {
    CopyMem (String, &AreaData[Pos], Length);
    String[Length] = '\0';
    Pos           += Length;
  } else if (TypeCode == 1) {
    for (Index = 0; Index < Length; Index++) {
      String[Index] = mFruBcdPlus[AreaData[Pos + Index] & 0x0F];
    }

    String[Length] = '\0';
    Pos           += Length;
  } else {
    FreePool (String);
    *Offset = Pos + Length;
    return NULL;
  }

  *Offset = Pos;
  return String;
}

#ifdef FRU_DEBUG_HEX_DUMP
#define FRU_FULL_IMAGE_PASS_MAX       16
#define FRU_FULL_IMAGE_PASS_DELAY_US  500000

STATIC
VOID
FruDumpHex (
  IN CONST CHAR8  *Label,
  IN CONST UINT8  *Data,
  IN UINT16       Size
  )
{
  UINT16  Offset;
  UINT16  Chunk;
  UINT16  Idx;
  CHAR8   Line[80];
  CHAR8   *Walk;
  UINTN   Pos;

  if ((Data == NULL) || (Size == 0)) {
    return;
  }

  if ((Label != NULL) && (Label[0] != '\0')) {
    DEBUG ((DEBUG_INFO, "IpmiFruInfo: %a (%u bytes):\n", Label, Size));
  } else {
    DEBUG ((DEBUG_INFO, "IpmiFruInfo: FRU raw data (%u bytes):\n", Size));
  }
  for (Offset = 0; Offset < Size; Offset += 16) {
    Chunk = Size - Offset;
    if (Chunk > 16) {
      Chunk = 16;
    }

    Walk = Line;
    Pos  = 0;
    Pos += AsciiSPrint (Walk + Pos, sizeof (Line) - Pos, "  %04x:", Offset);
    for (Idx = 0; Idx < Chunk; Idx++) {
      Pos += AsciiSPrint (Walk + Pos, sizeof (Line) - Pos, " %02x", Data[Offset + Idx]);
    }

    DEBUG ((DEBUG_INFO, "%a\n", Line));
  }
}
#endif // FRU_DEBUG_HEX_DUMP

STATIC
EFI_STATUS
FruReadAreaStrings (
  IN  CONST CHAR8  *AreaName,
  IN  UINT8        AreaOffsetUnits,
  IN  UINT8        FieldStartOffset,
  IN  UINTN        FieldCount,
  OUT CHAR8        FieldsOut[][FRU_SERIAL_MAX] OPTIONAL,
  IN  UINTN        FieldsOutCount
  )
{
  EFI_STATUS  Status;
  UINT8       Header[2];
  UINT8       *Area;
  UINT16      AreaLen;
  UINT16      Pos;
  UINTN       Index;
  CHAR8       *Field;

  if (AreaOffsetUnits == 0) {
    return EFI_NOT_FOUND;
  }

  MicroSecondDelay (FRU_READ_AREA_DELAY_US);

  Status = FruReadBytes ((UINT16)(AreaOffsetUnits * 8), sizeof (Header), Header);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AreaLen = (UINT16)Header[1] * 8;
  if ((AreaLen == 0) || (AreaLen > FRU_AREA_MAX)) {
    return EFI_DEVICE_ERROR;
  }

  Area = AllocateZeroPool (AreaLen);
  if (Area == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = FruReadBytes ((UINT16)(AreaOffsetUnits * 8), AreaLen, Area);
  if (EFI_ERROR (Status)) {
    FreePool (Area);
    return Status;
  }

#ifdef FRU_DEBUG_HEX_DUMP
  FruDumpHex (AreaName, Area, AreaLen);
#endif

  Pos = FieldStartOffset;
  for (Index = 0; Index < FieldCount; Index++) {
    Field = FruDecodeField (Area, &Pos);
    if ((Field != NULL) && (FieldsOut != NULL) && (Index < FieldsOutCount)) {
      AsciiStrnCpyS (FieldsOut[Index], FRU_SERIAL_MAX, Field, FRU_SERIAL_MAX - 1);
    }

    if (Field != NULL) {
      FreePool (Field);
    }
  }

  FreePool (Area);
  return EFI_SUCCESS;
}

#ifdef FRU_DEBUG_HEX_DUMP
STATIC
EFI_STATUS
FruDumpFullImage (
  IN UINT16  FruImageSize
  )
{
  UINT8       *Image;
  UINT16      Offset;
  UINT16      Chunk;
  UINTN       Pass;
  EFI_STATUS  Status;

  Image = AllocateZeroPool (FruImageSize);
  if (Image == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_DEVICE_ERROR;

  for (Pass = 0; Pass < FRU_FULL_IMAGE_PASS_MAX; Pass++) {
    if (Pass > 0) {
      DEBUG ((
        DEBUG_WARN,
        "%a: restart full image read pass %u/%u\n",
        __func__,
        Pass,
        FRU_FULL_IMAGE_PASS_MAX - 1
        ));
      MicroSecondDelay (FRU_FULL_IMAGE_PASS_DELAY_US);
    }

    ZeroMem (Image, FruImageSize);

    for (Offset = 0; Offset < FruImageSize; ) {
      Chunk = FruImageSize - Offset;
      if (Chunk > FRU_READ_CHUNK) {
        Chunk = FRU_READ_CHUNK;
      }

      Status = FruReadBytes (Offset, Chunk, Image + Offset);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_WARN,
          "%a: pass %u incomplete at off=0x%04x (%u/%u bytes) %r\n",
          __func__,
          Pass,
          Offset,
          Offset,
          FruImageSize,
          Status
          ));
        break;
      }

      Offset += Chunk;
    }

    if (Offset == FruImageSize) {
      DEBUG ((
        DEBUG_INFO,
        "%a: full FRU image read complete (%u bytes)\n",
        __func__,
        FruImageSize
        ));
      FruDumpHex ("FRU full inventory image", Image, FruImageSize);
      FreePool (Image);
      return EFI_SUCCESS;
    }
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: failed to read full FRU image (%u bytes) after %u passes last %r\n",
    __func__,
    FruImageSize,
    FRU_FULL_IMAGE_PASS_MAX,
    Status
    ));
  FreePool (Image);
  return Status;
}
#endif // FRU_DEBUG_HEX_DUMP

STATIC CONST CHAR8 *
FruFieldOrNone (
  IN CONST CHAR8  *Field
  )
{
  if ((Field == NULL) || (Field[0] == '\0')) {
    return "(none)";
  }

  return Field;
}

STATIC
VOID
FruPrintDeviceId (
  IN CONST IPMI_GET_DEVICE_ID_RESPONSE  *DeviceId
  )
{
  DEBUG ((
    DEBUG_INFO,
    "IpmiFruInfo: BMC Device ID CC=0x%02x FW=%u.%02u IPMI_spec=0x%02x FRU_support=%u\n",
    DeviceId->CompletionCode,
    DeviceId->FirmwareRev1.Bits.MajorFirmwareRev,
    DeviceId->MinorFirmwareRev,
    DeviceId->SpecificationVersion,
    DeviceId->DeviceSupport.Bits.FruInventorySupport
    ));
}

STATIC
VOID
FruPrintInventory (
  IN CONST CHAR8  ChassisFields[][FRU_SERIAL_MAX] OPTIONAL,
  IN CONST CHAR8  BoardFields[][FRU_SERIAL_MAX] OPTIONAL,
  IN CONST CHAR8  ProductFields[][FRU_SERIAL_MAX] OPTIONAL
  )
{
  DEBUG ((DEBUG_INFO, "IpmiFruInfo: === Builtin FRU Device (ID 0) ===\n"));

  if (ChassisFields != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "IpmiFruInfo: Chassis Part Number : %a\n",
      FruFieldOrNone (ChassisFields[0])
      ));
    DEBUG ((
      DEBUG_INFO,
      "IpmiFruInfo: Chassis Serial      : %a\n",
      FruFieldOrNone (ChassisFields[1])
      ));
  }

  if (BoardFields != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "IpmiFruInfo: Board Manufacturer  : %a\n",
      FruFieldOrNone (BoardFields[0])
      ));
    DEBUG ((
      DEBUG_INFO,
      "IpmiFruInfo: Board Product       : %a\n",
      FruFieldOrNone (BoardFields[1])
      ));
    DEBUG ((
      DEBUG_INFO,
      "IpmiFruInfo: Board Serial        : %a\n",
      FruFieldOrNone (BoardFields[2])
      ));
  }

  if (ProductFields != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "IpmiFruInfo: Product Manufacturer: %a\n",
      FruFieldOrNone (ProductFields[0])
      ));
    DEBUG ((
      DEBUG_INFO,
      "IpmiFruInfo: Product Name        : %a\n",
      FruFieldOrNone (ProductFields[1])
      ));
    DEBUG ((
      DEBUG_INFO,
      "IpmiFruInfo: Product Version     : %a\n",
      FruFieldOrNone (ProductFields[3])
      ));
    DEBUG ((
      DEBUG_INFO,
      "IpmiFruInfo: Product Serial      : %a\n",
      FruFieldOrNone (ProductFields[4])
      ));
  }
}

STATIC
EFI_STATUS
FruLoadSerialNumbers (
  VOID
  )
{
  EFI_STATUS                                 Status;
  IPMI_GET_DEVICE_ID_RESPONSE                DeviceId;
  IPMI_GET_FRU_INVENTORY_AREA_INFO_RESPONSE  AreaResponse;
  IPMI_FRU_COMMON_HEADER                     CommonHeader;
  UINT16                                     FruImageSize;
  CHAR8                                      ChassisFields[2][FRU_SERIAL_MAX];
  CHAR8                                      BoardFields[3][FRU_SERIAL_MAX];
  CHAR8                                      ProductFields[5][FRU_SERIAL_MAX];
  BOOLEAN                                    ChassisValid;
  BOOLEAN                                    BoardValid;
  BOOLEAN                                    ProductValid;

  ZeroMem (ChassisFields, sizeof (ChassisFields));
  ZeroMem (BoardFields, sizeof (BoardFields));
  ZeroMem (ProductFields, sizeof (ProductFields));
  ChassisValid = FALSE;
  BoardValid   = FALSE;
  ProductValid = FALSE;

  DEBUG ((
    DEBUG_INFO,
    "IpmiFruInfo: reading Builtin FRU via UART3 IPMI Serial (not LAN)\n"
    ));

  Status = FruGetDeviceIdWithRetry (&DeviceId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FruPrintDeviceId (&DeviceId);

  if (!DeviceId.DeviceSupport.Bits.FruInventorySupport) {
    DEBUG ((DEBUG_WARN, "%a: BMC does not report FRU inventory support\n", __func__));
    return EFI_UNSUPPORTED;
  }

  Status = FruGetFruInventoryAreaInfoWithRetry (FRU_DEVICE_ID, &AreaResponse);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FruImageSize = AreaResponse.InventoryAreaSize;
  if ((FruImageSize == 0) || (FruImageSize > FRU_AREA_MAX)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid FRU inventory area size %u\n",
      __func__,
      FruImageSize
      ));
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((
    DEBUG_INFO,
    "IpmiFruInfo: FRU inventory area size = %u bytes\n",
    FruImageSize
    ));

  Status = FruReadBytes (0, sizeof (CommonHeader), (UINT8 *)&CommonHeader);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: read FRU common header %r\n", __func__, Status));
    return Status;
  }

#ifdef FRU_DEBUG_HEX_DUMP
  FruDumpHex ("FRU common header", (UINT8 *)&CommonHeader, sizeof (CommonHeader));
#endif

  DEBUG ((
    DEBUG_INFO,
    "IpmiFruInfo: FRU common header offsets chassis=%u board=%u product=%u\n",
    CommonHeader.ChassisInfoStartingOffset,
    CommonHeader.BoardAreaStartingOffset,
    CommonHeader.ProductInfoStartingOffset
    ));

  if (CommonHeader.ChassisInfoStartingOffset != 0) {
    Status = FruReadAreaStrings (
               "FRU chassis area",
               CommonHeader.ChassisInfoStartingOffset,
               3,
               2,
               ChassisFields,
               2
               );
    if (!EFI_ERROR (Status)) {
      ChassisValid = TRUE;
      AsciiStrnCpyS (
        mFruChassisSerial,
        sizeof (mFruChassisSerial),
        ChassisFields[1],
        sizeof (mFruChassisSerial) - 1
        );
    } else {
      DEBUG ((DEBUG_WARN, "%a: chassis area read %r\n", __func__, Status));
    }
  }

  if (CommonHeader.BoardAreaStartingOffset != 0) {
    Status = FruReadAreaStrings (
               "FRU board area",
               CommonHeader.BoardAreaStartingOffset,
               6,
               3,
               BoardFields,
               3
               );
    if (!EFI_ERROR (Status)) {
      BoardValid = TRUE;
      AsciiStrnCpyS (
        mFruBoardSerial,
        sizeof (mFruBoardSerial),
        BoardFields[2],
        sizeof (mFruBoardSerial) - 1
        );
    } else {
      DEBUG ((DEBUG_WARN, "%a: board area read %r\n", __func__, Status));
    }
  }

  if (CommonHeader.ProductInfoStartingOffset != 0) {
    Status = FruReadAreaStrings (
               "FRU product area",
               CommonHeader.ProductInfoStartingOffset,
               3,
               5,
               ProductFields,
               5
               );
    if (!EFI_ERROR (Status)) {
      ProductValid = TRUE;
      AsciiStrnCpyS (
        mFruProductSerial,
        sizeof (mFruProductSerial),
        ProductFields[4],
        sizeof (mFruProductSerial) - 1
        );
    } else {
      DEBUG ((DEBUG_WARN, "%a: product area read %r\n", __func__, Status));
    }
  }

  FruPrintInventory (
    ChassisValid ? ChassisFields : NULL,
    BoardValid ? BoardFields : NULL,
    ProductValid ? ProductFields : NULL
    );

#ifdef FRU_DEBUG_HEX_DUMP
  //
  // Full-image hex dump only when all FruImageSize bytes are read successfully.
  //
  FruDumpFullImage (FruImageSize);
#endif

  if (ChassisValid || BoardValid || ProductValid) {
    return EFI_SUCCESS;
  }

  return EFI_DEVICE_ERROR;
}

CHAR8 *
EFIAPI
IpmiFruInfoGet (
  IN IPMI_FRU_FIELD_ID  FieldId
  )
{
  EFI_STATUS  Status;

  if (FieldId >= FruFieldIdMax) {
    return FRU_FIELD_DEFAULT;
  }

  if (!mFruInfoLoaded) {
    Status = FruLoadSerialNumbers ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: failed to load FRU serial numbers %r\n", __func__, Status));
    }

    mFruInfoLoaded = TRUE;
  }

  return mFruDataInfo[FieldId];
}
