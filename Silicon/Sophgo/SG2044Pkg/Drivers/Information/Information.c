/** @file
  Driver to dynamically update HII forms for system information display.

  Copyright (c) 2024 Sophgo Corporation. All rights reserved.
**/

#include "Information.h"
#include <Library/UefiBootServicesTableLib.h>

EFI_HANDLE                  DriverHandle;
INFORMATION_PAGE_CALLBACK_DATA *PrivateData;
INFORMATION_DATA gInformationData;
SMBIOS_PARSED_DATA *ParsedData;
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
    EFI_INFORMATION_VARIABLE_NAME,
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
    if (!HiiIsConfigHdrMatch(Request, &mConfiginiGuid, EFI_INFORMATION_VARIABLE_NAME)) {
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
  if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "BlockToConfig failed: %r\n", Status));
      return Status;
  }
  if (ConfigRequest != Request) {
    FreePool(ConfigRequest);
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

  if (!HiiIsConfigHdrMatch(Configuration, &mConfiginiGuid, EFI_INFORMATION_VARIABLE_NAME)) {
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

VOID
FillInformationData (
  OUT INFORMATION_DATA *InformationData
) {
  StrCpyS(InformationData->BiosVendor, MAX_LENGTH, ParsedData->BiosVendor);
  StrCpyS(InformationData->BiosVersion, MAX_LENGTH, ParsedData->BiosVersion);
  StrCpyS(InformationData->BiosReleaseDate, MAX_LENGTH, ParsedData->BiosReleaseDate);
  StrCpyS(InformationData->ProcessorVersion, MAX_LENGTH, ParsedData->ProcessorVersion);
  InformationData->ProcessorMaxSpeed = ParsedData->ProcessorCurrentSpeed;
  InformationData->L1ICacheSize = ParsedData->L1ICacheSize;
  InformationData->L1DCacheSize = ParsedData->L1DCacheSize;
  InformationData->L2CacheSize = ParsedData->L2CacheSize;
  InformationData->L3CacheSize = ParsedData->L3CacheSize;
  StrCpyS(InformationData->MemoryType, MAX_LENGTH, ParsedData->MemoryType);
  InformationData->ExtendedSpeed = ParsedData->ExtendedSpeed;
  InformationData->MemoryRank = ParsedData->MemoryRank;
  InformationData->ExtendSize = ParsedData->ExtendSize / (1024 * 1024);
  StrCpyS(InformationData->BoardProductName, MAX_LENGTH, ParsedData->BaseboardProductName);
  StrCpyS(InformationData->BoardVersion, MAX_LENGTH, ParsedData->BaseboardManufacturer);
  StrCpyS(InformationData->ProductName, MAX_LENGTH, ParsedData->SystemProductName);
  StrCpyS(InformationData->ProductVersion, MAX_LENGTH, ParsedData->SystemVersion);
  StrCpyS(InformationData->Manufacturer, MAX_LENGTH, ParsedData->SystemManufacturer);
}

EFI_STATUS
SyncToVarStore (
  IN INFORMATION_DATA *InformationData
) {
  EFI_STATUS Status;

  Status = gRT->SetVariable(
    EFI_INFORMATION_VARIABLE_NAME,
    &gEfiSophgoGlobalVariableGuid,
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
    EFI_INFORMATION_VARIABLE_NAME,
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
  ParsedData = AllocSmbiosData();
  if (ParsedData == NULL) {
    DEBUG((DEBUG_ERROR, "ParseSmbiosTable failed, initializing default values.\n"));
    ParsedData = AllocateZeroPool(sizeof(SMBIOS_PARSED_DATA));
    if (ParsedData == NULL) {
      DEBUG((DEBUG_ERROR, "Failed to allocate memory for ParsedData\n"));
      return EFI_OUT_OF_RESOURCES;
    }
    StrCpyS(ParsedData->BiosVendor, MAX_LENGTH, L"Unknown");
    StrCpyS(ParsedData->BiosVersion, MAX_LENGTH, L"Unknown");
    StrCpyS(ParsedData->BiosReleaseDate, MAX_LENGTH, L"Unknown");
    ParsedData->ProcessorCurrentSpeed = 0;
    ParsedData->L1ICacheSize = 0;
    ParsedData->L1DCacheSize = 0;
    ParsedData->L2CacheSize = 0;
    ParsedData->L3CacheSize = 0;
    StrCpyS(ParsedData->MemoryType, MAX_LENGTH, L"Unknown");
    ParsedData->ExtendedSpeed = 0;
    ParsedData->MemoryRank = 0;
    ParsedData->ExtendSize = 0;
  }
  FillInformationData(&gInformationData);
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

  if (PrivateData != NULL) {
    FreePool(PrivateData);
    PrivateData = NULL;
  }
  if (ParsedData != NULL) {
    FreeSmbiosData(ParsedData);
    ParsedData = NULL;
  }

  return EFI_SUCCESS;
}

