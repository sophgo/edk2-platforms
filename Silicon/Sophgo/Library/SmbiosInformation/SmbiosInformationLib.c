/**
  @file
  This library provides functionality to parse SMBIOS tables and extract relevant information.
  It defines a function ParseSmbiosTable, which retrieves system information from SMBIOS tables
  and populates the provided SMBIOS_PARSED_DATA structure.

  Copyright (c) Sophgo Inc. All rights reserved.
**/

#include <Library/SmbiosInformationLib.h>

EFI_STATUS
ExtractString(
  IN  CHAR8   *OptionalStrStart,
  IN  UINT8   Index,
  OUT CHAR16  **String
  )
{
  UINTN  StrSize;
  CHAR8  *CurrentStrPtr;

  if (String == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *String = NULL;

  if (Index == 0) {
    *String = AllocateZeroPool(sizeof(CHAR16));
    if (*String == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    return EFI_SUCCESS;
  }

  CurrentStrPtr = OptionalStrStart;
  while (Index > 1 && *CurrentStrPtr != '\0') {
    CurrentStrPtr += AsciiStrSize(CurrentStrPtr);
    Index--;
  }

  if (Index != 1 || *CurrentStrPtr == '\0') {
    *String = AllocateZeroPool(sizeof(CHAR16));
    if (*String == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    return EFI_NOT_FOUND;
  }

  StrSize = AsciiStrSize(CurrentStrPtr);
  *String = AllocatePool(StrSize * sizeof(CHAR16));
  if (*String == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (EFI_ERROR(AsciiStrToUnicodeStrS(CurrentStrPtr, *String, StrSize))) {
    FreePool(*String);
    *String = NULL;
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

SMBIOS_PARSED_DATA *
AllocSmbiosData (
  VOID
) {
  EFI_SMBIOS_PROTOCOL      *Smbios;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;
  EFI_STATUS               Status;
  SMBIOS_PARSED_DATA       *ParsedData;
  ParsedData = AllocateZeroPool(sizeof(SMBIOS_PARSED_DATA));
  if (ParsedData == NULL) {
    DEBUG((DEBUG_ERROR, "Failed to allocate memory for ParsedData\n"));
    return NULL;
  }
  ZeroMem(ParsedData, sizeof(SMBIOS_PARSED_DATA));
  Status = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to locate SMBIOS protocol: %r\n", Status));
    FreePool(ParsedData);
    return NULL;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  while (TRUE) {
    Status = Smbios->GetNext(Smbios, &SmbiosHandle, NULL, &Record, NULL);
    if (EFI_ERROR(Status)) {
      break;
    }

    switch (Record->Type) {
      case SMBIOS_TYPE_BIOS_INFORMATION: {
        SMBIOS_TABLE_TYPE0 *Type0 = (SMBIOS_TABLE_TYPE0 *)Record;
        ExtractString((CHAR8 *)((UINT8 *)Type0 + Type0->Hdr.Length), Type0->BiosVersion, &ParsedData->BiosVersion);
        ExtractString((CHAR8 *)((UINT8 *)Type0 + Type0->Hdr.Length), Type0->BiosReleaseDate, &ParsedData->BiosReleaseDate);
        ExtractString((CHAR8 *)((UINT8 *)Type0 + Type0->Hdr.Length), Type0->Vendor, &ParsedData->BiosVendor);
        break;
      }

      case SMBIOS_TYPE_SYSTEM_INFORMATION: {
        SMBIOS_TABLE_TYPE1 *Type1 = (SMBIOS_TABLE_TYPE1 *)Record;
        ExtractString((CHAR8 *)((UINT8 *)Type1 + Type1->Hdr.Length), Type1->Manufacturer, &ParsedData->SystemManufacturer);
        ExtractString((CHAR8 *)((UINT8 *)Type1 + Type1->Hdr.Length), Type1->ProductName, &ParsedData->SystemProductName);
        ExtractString((CHAR8 *)((UINT8 *)Type1 + Type1->Hdr.Length), Type1->SerialNumber, &ParsedData->SystemSerialNumber);
        ExtractString((CHAR8 *)((UINT8 *)Type1 + Type1->Hdr.Length), Type1->Version, &ParsedData->SystemVersion);
        break;
      }

      case SMBIOS_TYPE_BASEBOARD_INFORMATION: {
        SMBIOS_TABLE_TYPE2 *Type2 = (SMBIOS_TABLE_TYPE2 *)Record;
        ExtractString((CHAR8 *)((UINT8 *)Type2 + Type2->Hdr.Length), Type2->Manufacturer, &ParsedData->BaseboardManufacturer);
        ExtractString((CHAR8 *)((UINT8 *)Type2 + Type2->Hdr.Length), Type2->ProductName, &ParsedData->BaseboardProductName);
        break;
      }

      case SMBIOS_TYPE_SYSTEM_ENCLOSURE: {
        SMBIOS_TABLE_TYPE3 *Type3 = (SMBIOS_TABLE_TYPE3 *)Record;
        ExtractString((CHAR8 *)((UINT8 *)Type3 + Type3->Hdr.Length), Type3->Manufacturer, &ParsedData->ChassisManufacturer);
        break;
      }

      case SMBIOS_TYPE_PROCESSOR_INFORMATION: {
        SMBIOS_TABLE_TYPE4 *Type4 = (SMBIOS_TABLE_TYPE4 *)Record;
        ExtractString((CHAR8 *)((UINT8 *)Type4 + Type4->Hdr.Length), Type4->ProcessorVersion, &ParsedData->ProcessorVersion);
        ParsedData->ProcessorCurrentSpeed = Type4->CurrentSpeed;
        ParsedData->ProcessorCoreCount = Type4->CoreCount;
        ParsedData->ProcessorThreadCount = Type4->ThreadCount;
        break;
      }

      case SMBIOS_TYPE_CACHE_INFORMATION: {
        SMBIOS_TABLE_TYPE7 *Type7 = (SMBIOS_TABLE_TYPE7 *)Record;
        UINT8 RawLevel = (UINT8)(Type7->CacheConfiguration & 0x7);
        UINT64 CacheSizeInKB = 0;

        if (Record->Length >= 0x18 && Type7->InstalledSize2 != 0) {
          UINT32 InstalledSizeInBytes = Type7->InstalledSize2;
          CacheSizeInKB = InstalledSizeInBytes / 1024;
        } else {
          UINT16 RawSize = Type7->InstalledSize;
          if (RawSize & 0x8000) {
            CacheSizeInKB = ((RawSize & 0x7FFF) * 64ULL);
          } else {
            CacheSizeInKB = (RawSize & 0x7FFF);
          }
        }
        CHAR16 *LevelStr;
        switch (RawLevel) {
          case 0: LevelStr = L"L1"; break;
          case 1: LevelStr = L"L2"; break;
          case 2: LevelStr = L"L3"; break;
          default: LevelStr = L"Unknown"; break;
        }

        UINT8 CacheType = Type7->SystemCacheType;
        CHAR16 *CacheTypeStr = L"Unknown";
        if (CacheType == CacheTypeInstruction) {
          CacheTypeStr = L"Instruction";
        } else if (CacheType == CacheTypeData) {
          CacheTypeStr = L"Data";
        } else if (CacheType == CacheTypeUnified) {
          CacheTypeStr = L"Unified";
        }

        if (RawLevel == 0) {
          if (CacheType == CacheTypeInstruction) {
            ParsedData->L1ICacheSize = (UINT16)CacheSizeInKB;
          } else if (CacheType == CacheTypeData) {
            ParsedData->L1DCacheSize = (UINT16)CacheSizeInKB;
          } else if (CacheType == CacheTypeUnified) {
            ParsedData->L1ICacheSize = (UINT16)CacheSizeInKB;
            ParsedData->L1DCacheSize = (UINT16)CacheSizeInKB;
          }
        } else if (RawLevel == 1) {
          ParsedData->L2CacheSize = (UINT16)CacheSizeInKB;
        } else if (RawLevel == 2) {
          ParsedData->L3CacheSize = (UINT32)CacheSizeInKB;
        }
        break;
      }

      case SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION: {
        SMBIOS_TABLE_TYPE13 *Type13 = (SMBIOS_TABLE_TYPE13 *)Record;
        ParsedData->InstallableLanguages = Type13->InstallableLanguages;
        ParsedData->BiosLanguageFlags = Type13->Flags;
        break;
      }

      case SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY: {
        SMBIOS_TABLE_TYPE16 *Type16 = (SMBIOS_TABLE_TYPE16 *)Record;
        ParsedData->MemoryArrayLocation = Type16->Location;
        ParsedData->MemoryArrayUse = Type16->Use;
        break;
      }

      case SMBIOS_TYPE_MEMORY_DEVICE: {
  	SMBIOS_TABLE_TYPE17 *Type17 = (SMBIOS_TABLE_TYPE17 *)Record;
        ExtractString((CHAR8 *)((UINT8 *)Type17 + Type17->Hdr.Length), Type17->Manufacturer, &ParsedData->MemoryManufacturer);
        ParsedData->MemorySize = (Type17->Size & 0x7FFF);
        if (Type17->Size & 0x8000) {
          ParsedData->MemorySize *= 1024;
        }
        ParsedData->ExtendSize = Type17->ExtendedSize;
        ParsedData->ExtendedSpeed = Type17->ExtendedSpeed;
        switch (Type17->MemoryType) {
          case 0x1E:
            StrCpyS(ParsedData->MemoryType, MAX_STRING_LENGTH, L"LPDDR4");
            break;
          case 0x23:
            StrCpyS(ParsedData->MemoryType, MAX_STRING_LENGTH, L"LPDDR5");
            break;
          case 0x18:
            StrCpyS(ParsedData->MemoryType, MAX_STRING_LENGTH, L"DDR3");
            break;
          case 0x1A:
            StrCpyS(ParsedData->MemoryType, MAX_STRING_LENGTH, L"DDR4");
            break;
          default:
            UnicodeSPrint(ParsedData->MemoryType, MAX_STRING_LENGTH * sizeof(CHAR16), L"Unknown (0x%X)", Type17->MemoryType);
            break;
        }
        ParsedData->MemoryRank = Type17->Attributes & 0x0F;
        break;
      }

      case SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS: {
        SMBIOS_TABLE_TYPE19 *Type19 = (SMBIOS_TABLE_TYPE19 *)Record;
        ParsedData->MemoryArrayMappedStartingAddress = Type19->StartingAddress;
        ParsedData->MemoryArrayMappedEndingAddress = Type19->EndingAddress;
        break;
      }

      case SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION: {
        SMBIOS_TABLE_TYPE32 *Type32 = (SMBIOS_TABLE_TYPE32 *)Record;
        ParsedData->BootStatus = Type32->BootStatus;
        break;
      }

      case SMBIOS_TYPE_IPMI_DEVICE_INFORMATION: {
        SMBIOS_TABLE_TYPE38 *Type38 = (SMBIOS_TABLE_TYPE38 *)Record;
        ParsedData->IPMIInterfaceType = Type38->InterfaceType;
        break;
      }

      default:
        break;
    }
  }

  return ParsedData;
}

INT32
FreeSmbiosData (
  IN SMBIOS_PARSED_DATA *ParsedData
) {
  if (ParsedData == NULL) {
    DEBUG((DEBUG_ERROR, "%a:ParsedData Ptr NULL\n",__func__));
    return -1;
  }
  if (ParsedData->BiosVendor != NULL)
    FreePool(ParsedData->BiosVendor);

  if (ParsedData->BiosVersion != NULL)
    FreePool(ParsedData->BiosVersion);

  if (ParsedData->BiosReleaseDate != NULL)
    FreePool(ParsedData->BiosReleaseDate);

  if (ParsedData->SystemManufacturer != NULL)
    FreePool(ParsedData->SystemManufacturer);

  if (ParsedData->SystemProductName != NULL)
    FreePool(ParsedData->SystemProductName);

  if (ParsedData->SystemSerialNumber != NULL)
    FreePool(ParsedData->SystemSerialNumber);

  if (ParsedData->BaseboardManufacturer != NULL)
    FreePool(ParsedData->BaseboardManufacturer);

  if (ParsedData->BaseboardProductName != NULL)
    FreePool(ParsedData->BaseboardProductName);

  if (ParsedData->ChassisManufacturer != NULL)
    FreePool(ParsedData->ChassisManufacturer);

  if (ParsedData->ProcessorVersion != NULL)
    FreePool(ParsedData->ProcessorVersion);

  if (ParsedData->MemoryManufacturer != NULL)
    FreePool(ParsedData->MemoryManufacturer);

  FreePool(ParsedData);
  return 1;

}