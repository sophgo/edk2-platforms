/** @file

  This driver implements a UEFI module for debug configuration
  through a custom HII-based interface.

  Copyright (c) 2026, SOPHGO Technology Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "DebugConfigDxe.h"

EFI_GUID  mDebugConfigGuid     = DEBUG_CONFIG_FORMSET_GUID;

STATIC RESTORE_PROTOCOL gDebugConfigRestoreProtocol = {
  RestoreDebugConfigDefaults
};

extern UINT8 DebugConfigVfrBin[];
EFI_HII_HANDLE gDebugConfigHandle;

HII_VENDOR_DEVICE_PATH  mDebugConfigHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    { 0xD1B4A3C0, 0xF29E, 0x4718, { 0xB3, 0x44, 0x9A, 0xC7, 0x2D, 0x56, 0xE0, 0x8F } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

DEBUG_CONFIG_CALLBACK_DATA gDebugConfigPrivate = {
  DEBUG_CONFIG_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  NULL,
  {
    DebugConfigExtractConfig,
    DebugConfigRouteConfig,
    DebugConfigCallback
  }
};

EFI_STATUS
EFIAPI
RestoreDebugConfigDefaults (
  VOID
  )
{
  DEBUG_CONFIG_DATA            DebugConfigData;
  EFI_STATUS                   Status;

  DebugConfigData.EnableSerialPort = 0;

  Status = gRT->SetVariable (
          EFI_DEBUG_CONFIG_VARIABLE_NAME,
          &gEfiSophgoGlobalVariableGuid,
          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
          sizeof (DEBUG_CONFIG_DATA),
          &DebugConfigData
          );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to set EFI variable: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
UpdateDebugConfigConfig (
  IN DEBUG_CONFIG_CALLBACK_DATA           *PrivateData
  )
{
  EFI_STATUS            Status;
  DEBUG_CONFIG_DATA     DebugConfigData;
  UINTN                 VarSize;

  VarSize = sizeof (DEBUG_CONFIG_DATA);
  Status = gRT->GetVariable (
          EFI_DEBUG_CONFIG_VARIABLE_NAME,
          &gEfiSophgoGlobalVariableGuid,
                  NULL,
                  &VarSize,
          &DebugConfigData
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Get variable failed!\n",
      __func__
      ));
    return Status;
  }

  CopyMem (&PrivateData->DebugConfigData, &DebugConfigData, VarSize);

  return EFI_SUCCESS;
}

EFI_STATUS
DebugConfigSetupConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN DEBUG_CONFIG_CALLBACK_DATA             *PrivateData
  )
{
  EFI_STATUS           Status;
  DEBUG_CONFIG_DATA    DebugConfigData;

  if (!HiiGetBrowserData (
    &gEfiSophgoGlobalVariableGuid,
        EFI_DEBUG_CONFIG_VARIABLE_NAME,
    sizeof (DEBUG_CONFIG_DATA),
    (UINT8 *) &DebugConfigData))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: SetVariable failed: not match\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  CopyMem (&PrivateData->DebugConfigData, &DebugConfigData,
          sizeof (DEBUG_CONFIG_DATA));
  Status = gRT->SetVariable (
          EFI_DEBUG_CONFIG_VARIABLE_NAME,
          &gEfiSophgoGlobalVariableGuid,
          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
          sizeof (DEBUG_CONFIG_DATA),
          &DebugConfigData
          );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: SetVariable failed: %r\n",
      __func__,
      Status
      ));
  }

  return Status;
}

EFI_STATUS
EFIAPI
DebugConfigExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                     Status;
  DEBUG_CONFIG_CALLBACK_DATA     *Private;
  EFI_STRING                     ConfigRequestHdr;
  EFI_STRING                     ConfigRequest;
  BOOLEAN                        AllocatedRequest;
  UINTN                          Size;
  UINTN                          BufferSize;

  Status = EFI_SUCCESS;

  BufferSize = sizeof (DEBUG_CONFIG_DATA);
  if (Progress == NULL || Results == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): invalid parameters\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  Private = DEBUG_CONFIG_CALLBACK_DATA_FROM_THIS (This);

  Status = UpdateDebugConfigConfig (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gEfiSophgoGlobalVariableGuid, EFI_DEBUG_CONFIG_VARIABLE_NAME)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): HiiIsConfigHdrMatch not match\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest  = NULL;
  AllocatedRequest = FALSE;

  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    ConfigRequestHdr = HiiConstructConfigHdr (
            &gEfiSophgoGlobalVariableGuid,
            EFI_DEBUG_CONFIG_VARIABLE_NAME,
            Private->DriverHandle
            );
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest,
            Size,
            L"%s&OFFSET=0&WIDTH=%016LX",
            ConfigRequestHdr,
            (UINT64)BufferSize
            );
    FreePool (ConfigRequestHdr);
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) &Private->DebugConfigData,
                                sizeof (DEBUG_CONFIG_DATA),
                                Results,
                                Progress
                                );

  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }

  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

EFI_STATUS
EFIAPI
DebugConfigRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS                    Status;
  UINTN                         BufferSize;
  DEBUG_CONFIG_CALLBACK_DATA    *Private;

  if (This == NULL || Configuration == NULL || Progress == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): invalid parameters\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  Private = DEBUG_CONFIG_CALLBACK_DATA_FROM_THIS (This);
  *Progress = Configuration;

  if (!HiiIsConfigHdrMatch (
          Configuration,
      &gEfiSophgoGlobalVariableGuid,
      EFI_DEBUG_CONFIG_VARIABLE_NAME
      ))
  {
    DEBUG ((
      DEBUG_ERROR,
      "Configuration header does not match.\n"
      ));
    return EFI_NOT_FOUND;
  }

  Status = DebugConfigSetupConfig (This, Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BufferSize = sizeof (DEBUG_CONFIG_DATA);
  Status = Private->HiiConfigRouting->ConfigToBlock (
          Private->HiiConfigRouting,
          Configuration,
          (UINT8 *)&Private->DebugConfigData,
          &BufferSize,
          Progress
          );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: ConfigToBlock failed: %r. Progress: %s\n",
      __func__,
      Status,
      *Progress
      ));
  }

  return Status;
}

EFI_STATUS
EFIAPI
DebugConfigCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_STATUS                     Status;
  UINTN                          VarSize;
  DEBUG_CONFIG_DATA              DebugConfigData;

  Status = EFI_SUCCESS;
  VarSize = sizeof (DEBUG_CONFIG_DATA);

  Status = gRT->GetVariable (
          EFI_DEBUG_CONFIG_VARIABLE_NAME,
          &gEfiSophgoGlobalVariableGuid,
                  NULL,
                  &VarSize,
                  &DebugConfigData
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: GetVariable error: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (QuestionId == ENABLE_SERIAL_PORT_QUESTION_ID &&
          Action == EFI_BROWSER_ACTION_CHANGING) {
    DebugConfigData.EnableSerialPort = Value->u8;

    Status = gRT->SetVariable (
            EFI_DEBUG_CONFIG_VARIABLE_NAME,
            &gEfiSophgoGlobalVariableGuid,
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
            VarSize,
            &DebugConfigData
            );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    HiiSetBrowserData (
          &mDebugConfigGuid,
          EFI_DEBUG_CONFIG_VARIABLE_NAME,
          VarSize,
          (UINT8*)&DebugConfigData,
          NULL
          );
  }
  else if (QuestionId == ENABLE_SERIAL_PORT_QUESTION_ID &&
    Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD) {
    Value->u8 = 0;
  }

  return Status;
}

EFI_STATUS
DebugConfigConfigInit (
  VOID
  )
{
  EFI_STATUS            Status;
  DEBUG_CONFIG_DATA     DebugConfigData;
  UINTN                 VarSize;

  VarSize = sizeof (DEBUG_CONFIG_DATA);
  Status = gRT->GetVariable (
          EFI_DEBUG_CONFIG_VARIABLE_NAME,
          &gEfiSophgoGlobalVariableGuid,
                  NULL,
                  &VarSize,
                  &DebugConfigData
                  );
  if (Status == EFI_NOT_FOUND) {
    DebugConfigData.EnableSerialPort = 0;
    Status = gRT->SetVariable (
            EFI_DEBUG_CONFIG_VARIABLE_NAME,
            &gEfiSophgoGlobalVariableGuid,
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VarSize,
                    &DebugConfigData
            );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: SetVariable failed: %r\n",
        __func__,
        Status
        ));
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
DebugConfigDriverEntryPoint (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                      Status;
  DEBUG_CONFIG_CALLBACK_DATA      *PrivateData;

  PrivateData = AllocateCopyPool (sizeof (DEBUG_CONFIG_CALLBACK_DATA), &gDebugConfigPrivate);
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->LocateProtocol (
          &gEfiHiiConfigRoutingProtocolGuid,
          NULL,
          (VOID **) &PrivateData->HiiConfigRouting
          );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PrivateData->DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces(
                  &PrivateData->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mDebugConfigHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &PrivateData->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallProtocolInterface (
                  &PrivateData->DriverHandle,
                  &gDebugConfigRestoreProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)&gDebugConfigRestoreProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  PrivateData->HiiHandle = HiiAddPackages (
                   &mDebugConfigGuid,
                   PrivateData->DriverHandle,
                   DebugConfigVfrBin,
                   DebugConfigDxeStrings,
                   NULL
                   );
  if (PrivateData->HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  gDebugConfigHandle = PrivateData->HiiHandle;

  Status = DebugConfigConfigInit ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DebugConfigDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                       Status;
  DEBUG_CONFIG_CALLBACK_DATA       *PrivateData;

  Status = gBS->HandleProtocol (
          ImageHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &PrivateData
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (PrivateData->Signature == DEBUG_CONFIG_CALLBACK_DATA_SIGNATURE);

  Status = gBS->UninstallMultipleProtocolInterfaces (
          &ImageHandle,
          &gEfiCallerIdGuid,
          PrivateData,
          NULL
          );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
          PrivateData->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mDebugConfigHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &PrivateData->ConfigAccess,
                  NULL
                  );

  ASSERT_EFI_ERROR (Status);

  HiiRemovePackages (PrivateData->HiiHandle);

  return EFI_SUCCESS;
}