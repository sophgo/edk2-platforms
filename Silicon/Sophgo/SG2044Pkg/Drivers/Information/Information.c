/** @file
  Driver to dynamically update HII forms for system information display.

  Copyright (c) 2024 Sophgo Corporation. All rights reserved.
**/

#include "Information.h"
#include <Library/UefiBootServicesTableLib.h>

EFI_HANDLE                  DriverHandle;
INFORMATION_PAGE_CALLBACK_DATA *PrivateData;
INFORMATION_DATA gInformationData;
SMBIOS_PARSED_DATA ParsedData;
EFI_GUID mConfiginiGuid = CONFIG_INI_FORMSET_GUID;

HII_VENDOR_DEVICE_PATH mInformationHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof(VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    CONFIG_INI_FORMSET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

EFI_STATUS
EFIAPI
DriverCallback(
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION Action,
  IN EFI_QUESTION_ID QuestionId,
  IN UINT8 Type,
  IN EFI_IFR_TYPE_VALUE *Value,
  OUT EFI_BROWSER_ACTION_REQUEST *ActionRequest
) {

  if (Action != EFI_BROWSER_ACTION_CHANGED && Action != EFI_BROWSER_ACTION_SUBMITTED) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

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

EFI_STATUS
EFIAPI
ExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN CONST EFI_STRING Request,
  OUT EFI_STRING *Progress,
  OUT EFI_STRING *Results
) {
  EFI_STATUS Status;
  EFI_STRING ConfigRequestHdr;
  EFI_STRING ConfigRequest;
  UINTN BufferSize;

  if (Request == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Request;
  ConfigRequestHdr = HiiConstructConfigHdr(
    &mConfiginiGuid,
    L"InformationData",
    PrivateData->DriverHandle
  );

  if (ConfigRequestHdr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (Request == NULL) {
    BufferSize = StrSize(ConfigRequestHdr) + 32;
    ConfigRequest = AllocateZeroPool(BufferSize);
    if (ConfigRequest == NULL) {
      FreePool(ConfigRequestHdr);
      return EFI_OUT_OF_RESOURCES;
    }
    UnicodeSPrint(
      ConfigRequest,
      BufferSize,
      L"%s&OFFSET=0&WIDTH=%016LX",
      ConfigRequestHdr,
      (UINT64)sizeof(INFORMATION_DATA)
    );
    FreePool(ConfigRequestHdr);
  } else {
    if (!HiiIsConfigHdrMatch(Request, &mConfiginiGuid, L"InformationData")) {
      FreePool(ConfigRequestHdr);
      return EFI_NOT_FOUND;
    }
    ConfigRequest = Request;
  }

  Status = gHiiConfigRouting->BlockToConfig(
    gHiiConfigRouting,
    ConfigRequest,
    (UINT8 *)&gInformationData,
    sizeof(INFORMATION_DATA),
    Results,
    Progress
  );
  if (ConfigRequest != Request) {
    FreePool(ConfigRequest);
  }
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "ExtractConfig: BlockToConfig failed: %r\n", Status));
  }
  return Status;
}

EFI_STATUS
EFIAPI
RouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN CONST EFI_STRING Configuration,
  OUT EFI_STRING *Progress
) {
  EFI_STATUS Status;
  UINTN BufferSize;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Configuration;

  if (!HiiIsConfigHdrMatch(Configuration, &mConfiginiGuid, L"InformationData")) {
    return EFI_NOT_FOUND;
  }
  BufferSize = sizeof(INFORMATION_DATA);
  Status = gHiiConfigRouting->ConfigToBlock(
    gHiiConfigRouting,
    Configuration,
    (UINT8 *)&gInformationData,
    &BufferSize,
    Progress
  );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "RouteConfig: ConfigToBlock failed: %r\n", Status));
  }

  return Status;
}

EFI_STATUS
ParseSmbiosTable (VOID) {
  EFI_SMBIOS_PROTOCOL      *Smbios;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;
  EFI_STATUS               Status;

  ZeroMem(&ParsedData, sizeof(SMBIOS_PARSED_DATA));
  Status = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to locate SMBIOS protocol: %r\n", Status));
    return Status;
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
        ExtractString((CHAR8 *)((UINT8 *)Type0 + Type0->Hdr.Length), Type0->BiosVersion, &ParsedData.BiosVersion);
        ExtractString((CHAR8 *)((UINT8 *)Type0 + Type0->Hdr.Length), Type0->BiosReleaseDate, &ParsedData.BiosReleaseDate);
        ExtractString((CHAR8 *)((UINT8 *)Type0 + Type0->Hdr.Length), Type0->Vendor, &ParsedData.BiosVendor);
        break;
      }

      case SMBIOS_TYPE_SYSTEM_INFORMATION: {
        SMBIOS_TABLE_TYPE1 *Type1 = (SMBIOS_TABLE_TYPE1 *)Record;
        ExtractString((CHAR8 *)((UINT8 *)Type1 + Type1->Hdr.Length), Type1->Manufacturer, &ParsedData.SystemManufacturer);
        ExtractString((CHAR8 *)((UINT8 *)Type1 + Type1->Hdr.Length), Type1->ProductName, &ParsedData.SystemProductName);
        ExtractString((CHAR8 *)((UINT8 *)Type1 + Type1->Hdr.Length), Type1->SerialNumber, &ParsedData.SystemSerialNumber);
        ExtractString((CHAR8 *)((UINT8 *)Type1 + Type1->Hdr.Length), Type1->Version, &ParsedData.SystemVersion);
        break;
      }

      case SMBIOS_TYPE_BASEBOARD_INFORMATION: {
        SMBIOS_TABLE_TYPE2 *Type2 = (SMBIOS_TABLE_TYPE2 *)Record;
        ExtractString((CHAR8 *)((UINT8 *)Type2 + Type2->Hdr.Length), Type2->Manufacturer, &ParsedData.BaseboardManufacturer);
        ExtractString((CHAR8 *)((UINT8 *)Type2 + Type2->Hdr.Length), Type2->ProductName, &ParsedData.BaseboardProductName);
        break;
      }

      case SMBIOS_TYPE_SYSTEM_ENCLOSURE: {
        SMBIOS_TABLE_TYPE3 *Type3 = (SMBIOS_TABLE_TYPE3 *)Record;
        ExtractString((CHAR8 *)((UINT8 *)Type3 + Type3->Hdr.Length), Type3->Manufacturer, &ParsedData.ChassisManufacturer);
        break;
      }

      case SMBIOS_TYPE_PROCESSOR_INFORMATION: {
        SMBIOS_TABLE_TYPE4 *Type4 = (SMBIOS_TABLE_TYPE4 *)Record;
        ExtractString((CHAR8 *)((UINT8 *)Type4 + Type4->Hdr.Length), Type4->ProcessorVersion, &ParsedData.ProcessorVersion);
        ParsedData.ProcessorCurrentSpeed = Type4->CurrentSpeed;
        ParsedData.ProcessorCoreCount = Type4->CoreCount;
        ParsedData.ProcessorThreadCount = Type4->ThreadCount;
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
            ParsedData.L1ICacheSize = (UINT16)CacheSizeInKB;
          } else if (CacheType == CacheTypeData) {
            ParsedData.L1DCacheSize = (UINT16)CacheSizeInKB;
          } else if (CacheType == CacheTypeUnified) {
            ParsedData.L1ICacheSize = (UINT16)CacheSizeInKB;
            ParsedData.L1DCacheSize = (UINT16)CacheSizeInKB;
          }
        } else if (RawLevel == 1) {
          ParsedData.L2CacheSize = (UINT16)CacheSizeInKB;
        } else if (RawLevel == 2) {
          ParsedData.L3CacheSize = (UINT32)CacheSizeInKB;
        }
        break;
      }

      case SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION: {
        SMBIOS_TABLE_TYPE13 *Type13 = (SMBIOS_TABLE_TYPE13 *)Record;
        ParsedData.InstallableLanguages = Type13->InstallableLanguages;
        ParsedData.BiosLanguageFlags = Type13->Flags;
        break;
      }

      case SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY: {
        SMBIOS_TABLE_TYPE16 *Type16 = (SMBIOS_TABLE_TYPE16 *)Record;
        ParsedData.MemoryArrayLocation = Type16->Location;
        ParsedData.MemoryArrayUse = Type16->Use;
        break;
      }

      case SMBIOS_TYPE_MEMORY_DEVICE: {
  	    SMBIOS_TABLE_TYPE17 *Type17 = (SMBIOS_TABLE_TYPE17 *)Record;

        ExtractString((CHAR8 *)((UINT8 *)Type17 + Type17->Hdr.Length), Type17->Manufacturer, &ParsedData.MemoryManufacturer);
        ParsedData.MemorySize = (Type17->Size & 0x7FFF);
        if (Type17->Size & 0x8000) {
          ParsedData.MemorySize *= 1024;
        }
        ParsedData.ExtendSize = Type17->ExtendedSize;
        ParsedData.ExtendedSpeed = Type17->ExtendedSpeed;
        switch (Type17->MemoryType) {
          case 0x1E:
            StrCpyS(ParsedData.MemoryType, MAX_STRING_LENGTH, L"LPDDR4");
            break;
          case 0x23:
            StrCpyS(ParsedData.MemoryType, MAX_STRING_LENGTH, L"LPDDR5");
            break;
          case 0x18:
            StrCpyS(ParsedData.MemoryType, MAX_STRING_LENGTH, L"DDR3");
            break;
          case 0x1A:
            StrCpyS(ParsedData.MemoryType, MAX_STRING_LENGTH, L"DDR4");
            break;
          default:
            UnicodeSPrint(ParsedData.MemoryType, MAX_STRING_LENGTH * sizeof(CHAR16), L"Unknown (0x%X)", Type17->MemoryType);
            break;
        }
        ParsedData.MemoryRank = Type17->Attributes & 0x0F;
        break;
      }

      case SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS: {
        SMBIOS_TABLE_TYPE19 *Type19 = (SMBIOS_TABLE_TYPE19 *)Record;
        ParsedData.MemoryArrayMappedStartingAddress = Type19->StartingAddress;
        ParsedData.MemoryArrayMappedEndingAddress = Type19->EndingAddress;
        break;
      }

      case SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION: {
        SMBIOS_TABLE_TYPE32 *Type32 = (SMBIOS_TABLE_TYPE32 *)Record;
        ParsedData.BootStatus = Type32->BootStatus;
        break;
      }

      case SMBIOS_TYPE_IPMI_DEVICE_INFORMATION: {
        SMBIOS_TABLE_TYPE38 *Type38 = (SMBIOS_TABLE_TYPE38 *)Record;
        ParsedData.IPMIInterfaceType = Type38->InterfaceType;
        break;
      }

      default:
        break;
    }
  }

  return EFI_SUCCESS;
}

VOID
FillInformationData (
  IN SMBIOS_PARSED_DATA *ParsedData,
  OUT INFORMATION_DATA *InformationData
) {
  StrCpyS(InformationData->BiosVendor, MAX_STRING_LENGTH, ParsedData->BiosVendor);
  StrCpyS(InformationData->BiosVersion, MAX_STRING_LENGTH, ParsedData->BiosVersion);
  StrCpyS(InformationData->BiosReleaseDate, MAX_STRING_LENGTH, ParsedData->BiosReleaseDate);
  StrCpyS(InformationData->ProcessorVersion, MAX_STRING_LENGTH, ParsedData->ProcessorVersion);
  InformationData->ProcessorMaxSpeed = ParsedData->ProcessorCurrentSpeed;
  InformationData->L1ICacheSize = ParsedData->L1ICacheSize;
  InformationData->L1DCacheSize = ParsedData->L1DCacheSize;
  InformationData->L2CacheSize = ParsedData->L2CacheSize;
  InformationData->L3CacheSize = ParsedData->L3CacheSize;
  StrCpyS(InformationData->MemoryType, MAX_STRING_LENGTH, ParsedData->MemoryType);
  InformationData->ExtendedSpeed = ParsedData->ExtendedSpeed;
  InformationData->MemoryRank = ParsedData->MemoryRank;
  InformationData->ExtendSize = ParsedData->ExtendSize / (1024 * 1024);
  StrCpyS(InformationData->BoardProductName, MAX_STRING_LENGTH, ParsedData->BaseboardProductName);
  StrCpyS(InformationData->BoardVersion, MAX_STRING_LENGTH, ParsedData->BaseboardManufacturer);
  StrCpyS(InformationData->ProductName, MAX_STRING_LENGTH, ParsedData->SystemProductName);
  StrCpyS(InformationData->ProductVersion, MAX_STRING_LENGTH, ParsedData->SystemVersion);
  StrCpyS(InformationData->Manufacturer, MAX_STRING_LENGTH, ParsedData->SystemManufacturer);
}

EFI_STATUS
SyncToVarStore (
  IN INFORMATION_DATA *InformationData
) {
  EFI_STATUS Status;

  Status = gRT->SetVariable(
    L"InformationData",
    &mConfiginiGuid,
    EFI_VARIABLE_NON_VOLATILE |
    EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof(INFORMATION_DATA),
    InformationData
  );

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to set VarStore: %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
UpdateHiiBrowserData (
  VOID
) {
  EFI_STATUS Status;

  Status = HiiSetBrowserData(
    &mConfiginiGuid,
    L"InformationData",
    sizeof(INFORMATION_DATA),
    (UINT8 *)&gInformationData,
    NULL
  );

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to update HII browser data: %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InformationInit(
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
) {
  EFI_STATUS Status;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;

  ParseSmbiosTable();
  FillInformationData(&ParsedData, &gInformationData);
  SyncToVarStore(&gInformationData);
  PrivateData = AllocateZeroPool(sizeof(INFORMATION_PAGE_CALLBACK_DATA));
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->Signature = INFORMATION_PAGE_CALLBACK_DATA_SIGNATURE;
  PrivateData->ConfigAccess.ExtractConfig = ExtractConfig;
  PrivateData->ConfigAccess.RouteConfig = RouteConfig;
  PrivateData->ConfigAccess.Callback = DriverCallback;

  Status = gBS->LocateProtocol(&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&HiiConfigRouting);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  PrivateData->HiiConfigRouting = HiiConfigRouting;

  Status = gBS->InstallMultipleProtocolInterfaces(
    &DriverHandle,
    &gEfiDevicePathProtocolGuid,
    &mInformationHiiVendorDevicePath,
    &gEfiHiiConfigAccessProtocolGuid,
    &PrivateData->ConfigAccess,
    NULL
  );
  ASSERT_EFI_ERROR(Status);

  PrivateData->DriverHandle = DriverHandle;

  PrivateData->HiiHandle = HiiAddPackages(
    &mConfiginiGuid,
    DriverHandle,
    InformationVfrBin,
    InformationStrings,
    NULL
  );

  if (PrivateData->HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  UpdateHiiBrowserData();
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InformationUnload(
  IN EFI_HANDLE ImageHandle
) {
  if (DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces(
      DriverHandle,
      &gEfiDevicePathProtocolGuid,
      &mInformationHiiVendorDevicePath,
      &gEfiHiiConfigAccessProtocolGuid,
      &PrivateData->ConfigAccess,
      NULL
    );
    DriverHandle = NULL;
  }

  if (PrivateData->HiiHandle != NULL) {
    HiiRemovePackages(PrivateData->HiiHandle);
  }

  FreePool(PrivateData);
  PrivateData = NULL;

  return EFI_SUCCESS;
}

