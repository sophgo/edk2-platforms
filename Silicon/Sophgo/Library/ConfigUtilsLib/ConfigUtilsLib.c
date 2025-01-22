#include <Library/ConfigUtilsLib.h>

INT32
UpdateSmbiosFromEfuse (
  UINT32    BusNum,
  UINT32    Offset,
  UINT32    Count,
  UINT32    *Size
  )
{
  EFI_STATUS Status;
  UINT8 *Buffer = NULL;

  if (Count == 0) {
    DEBUG((DEBUG_ERROR, "Invalid Count: %u\n", Count));
    return -1;
  }

   Buffer = AllocateZeroPool(Count);
   if (Buffer == NULL || Size == NULL) {
    return -1;
  }
  Status = EfuseReadBytes(BusNum, Offset, Count, Buffer);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to read from eFuse: %r\n", Status));
    return -1;
  }

  *Size = SwapBytes32(*(UINT32 *)Buffer);
  return 0;
}

INT32
ReadVersionAndDateFromFlash (
  CHAR8 *Version,
  CHAR8 *Date,
  UINTN StartAddress,
  UINTN EndAddress
  )
{
  EFI_STATUS Status;
  SOPHGO_NOR_FLASH_PROTOCOL *NorFlashProtocol = NULL;
  SOPHGO_SPI_MASTER_PROTOCOL *SpiMasterProtocol = NULL;
  SPI_NOR *Nor = NULL;
  UINTN RangeSize;
  UINT8 *Buffer = NULL;

  if (Version == NULL || Date == NULL || StartAddress >= EndAddress) {
    return -1;
  }

  RangeSize = EndAddress - StartAddress;

  Buffer = AllocateZeroPool(RangeSize);
  if (Buffer == NULL) {
    return -1;
  }

  Status = gBS->LocateProtocol(
    &gSophgoSpiMasterProtocolGuid,
    NULL,
    (VOID **)&SpiMasterProtocol
  );
  if (EFI_ERROR(Status)) {
    return -1;
  }

  Status = gBS->LocateProtocol(
    &gSophgoNorFlashProtocolGuid,
    NULL,
    (VOID **)&NorFlashProtocol
  );
  if (EFI_ERROR(Status)) {
    return -1;
  }

  Nor = SpiMasterProtocol->SetupDevice(
    SpiMasterProtocol,
    NULL,
    0
  );
  if(Nor == NULL) {
    return -1;
  }

  Status = NorFlashProtocol->GetFlashid(Nor, TRUE);
  if (EFI_ERROR(Status)) {
    return -1;
  }

  Status = NorFlashProtocol->Init(NorFlashProtocol, Nor);
  if (EFI_ERROR(Status)) {
    return -1;
  }

  Status = NorFlashProtocol->ReadData(
    Nor,
    StartAddress,
    RangeSize,
    Buffer
  );
  if(EFI_ERROR(Status)) {
    return -1;
  }

  CopyMem(Version, Buffer + VERSION_OFFSET, VERSION_SIZE);
  Version[VERSION_SIZE] = '\0';
  CopyMem(Date, Buffer + DATE_OFFSET, DATE_SIZE);
  Date[DATE_SIZE] = '\0';

  SpiMasterProtocol->FreeDevice(Nor);
  FreePool(Buffer);

  return 0;
}
