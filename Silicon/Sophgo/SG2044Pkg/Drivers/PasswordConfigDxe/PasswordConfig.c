/** @file
HII Config Access protocol implementation of password configuration module.

Copyright (c) 2024, Sophgo. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "PasswordConfig.h"

#define ADMIN_USER_CLEAN_TOGETHER  1
#define DEVICE_PATH_0_GUID { 0x44fe1427, 0x41c9, 0x4c35, { 0x8a, 0xdc, 0xbc, 0x8e, 0x84, 0x48, 0x11, 0x7e }}

VOID                  *mStartOpCodeHandle;
VOID                  *mEndOpCodeHandle;
EFI_IFR_GUID_LABEL    *mStartLabel;
EFI_IFR_GUID_LABEL    *mEndLabel;
UINT8                 mCheckFlag;

extern UINT8 PasswordConfigUiBin[];
BOOLEAN  mFirstEnterPasswordConfigForm;
EFI_GUID mPasswordConfigGuid = PASSWORDCONFIG_FORMSET_GUID;
EFI_GUID gPasswordConfigVarGuid = PASSWORDCONFIG_VAR_GUID;
STATIC RESTORE_PROTOCOL gPasswordRestoreProtocol = {
 PasswordRestore
};

HII_VENDOR_DEVICE_PATH  mPasswordConfigHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    DEVICE_PATH_0_GUID
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

PASSWORD_CONFIG_PRIVATE_DATA gPasswordConfigPrivate = {
  PASSWORD_CONFIG_PRIVATE_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    PasswordConfigExtractConfig,
    PasswordConfigRouteConfig,
    PasswordConfigCallback
  }
};

EFI_STATUS
EFIAPI
PasswordRestore (
  VOID
  )
{
  EFI_STATUS Status;
  PASSWORD_CONFIG_DATA  PasswordConfigData;

  PasswordConfigData.UserPriv = 0;
  PasswordConfigData.UserPasswordEnable = 1;
  PasswordConfigData.AdminPasswordEnable = 1;
  StrCpyS(PasswordConfigData.UserPassword, PASSWD_MAXLEN, L"user1234");
  StrCpyS(PasswordConfigData.AdminPassword, PASSWD_MAXLEN, L"admin1234");
  Status = gRT->SetVariable(
              VAR_PASSWORD_CONFIG_NAME,
              &gPasswordConfigVarGuid,
              PLATFORM_SETUP_VARIABLE_FLAG,
              sizeof(PASSWORD_CONFIG_DATA),
              &PasswordConfigData
   );
   if(EFI_ERROR(Status)) {
      return Status;
   }
  return EFI_SUCCESS;
}

/**
  Password configuration restore defaults.

  @param[in,out]    PasswordConfigData    Point to password configuration struct.

**/
VOID
PasswordConfigDefault (
  IN OUT PASSWORD_CONFIG_DATA  *PasswordConfigData
  )
{
  PasswordConfigData->UserPasswordEnable = 1;
  PasswordConfigData->AdminPasswordEnable = 1;
  StrCpyS(PasswordConfigData->UserPassword, PASSWD_MAXLEN, L"user1234");
  StrCpyS(PasswordConfigData->AdminPassword, PASSWD_MAXLEN, L"admin1234");
}

/**
  Password configuration initialization.Get the current password configuration from variable.
  If the variable does not exist, restore the default configuration and save the variable.

  @return       EFI_SUCCESS    Initialize password configuration in the form successfully.
  @return       other          Initialize password configuration in the form failed.
**/
EFI_STATUS
PasswordConfigInit (
  VOID
  )
{
  EFI_STATUS            Status;
  PASSWORD_CONFIG_DATA  PasswordConfigData;
  UINTN                 VarSize;

  VarSize = sizeof(PASSWORD_CONFIG_DATA);
  Status = gRT->GetVariable (
                  VAR_PASSWORD_CONFIG_NAME,
                  &gPasswordConfigVarGuid,
                  NULL,
                  &VarSize,
                  &PasswordConfigData
                  );

  if (Status == EFI_NOT_FOUND) {
    PasswordConfigDefault (&PasswordConfigData);
    Status = gRT->SetVariable(
              VAR_PASSWORD_CONFIG_NAME,
              &gPasswordConfigVarGuid,
              PLATFORM_SETUP_VARIABLE_FLAG,
              sizeof(PASSWORD_CONFIG_DATA),
              &PasswordConfigData
   );
   if (EFI_ERROR(Status)) {
     DEBUG((DEBUG_ERROR, "SetVariable failed: %r\n", Status));
   }
  }

  return Status;
}


/**
  Get Privilege configuration data from variable and set browser data.

  @param[in]  PrivateData    A pointer to ADVANCED_CONFIG_CALLBACK_DATA.

  @retval     EFI_SUCCESS     Success.
  @retval     Other           Failed.
**/
EFI_STATUS
GetPrivData (
  IN PASSWORD_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS           Status;
  UINTN                VarSize;

  VarSize = sizeof (PASSWORD_CONFIG_DATA);
  Status = gRT->GetVariable (
                  VAR_PASSWORD_CONFIG_NAME,
                  &gPasswordConfigVarGuid,
                  NULL,
                  &VarSize,
                  &PrivateData->PasswordConfigData
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetPrivData: GetVariable() failed: %r\n", Status));
    return Status;
  }

  Status = HiiSetBrowserData (
              &mPasswordConfigGuid,
              PASSWORD_CONFIG_VARIABLE,
              sizeof (PASSWORD_CONFIG_DATA),
              (UINT8 *) &PrivateData->PasswordConfigData,
              NULL
              );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetPrivData: HiiSetBrowserData error: %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  Get password configuration from variable and update uncommitted data in the form.

  @param[in]    PrivateData    Point to PASSWORD_CONFIG_PRIVATE_DATA.

  @return       EFI_SUCCESS    Update password configuration in the form successfully.
  @return       other          Update password configuration in the form failed.
**/
EFI_STATUS
GetPasswordConfigData (
  IN PASSWORD_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS            Status;
  PASSWORD_CONFIG_DATA  PasswordConfigData;
  UINTN                 VarSize;

  VarSize = sizeof(PASSWORD_CONFIG_DATA);
  Status = gRT->GetVariable (
                  VAR_PASSWORD_CONFIG_NAME,
                  &gPasswordConfigVarGuid,
                  NULL,
                  &VarSize,
                  &PasswordConfigData
                  );

  if (!EFI_ERROR(Status)) {
    CopyMem (&PrivateData->PasswordConfigData, &PasswordConfigData, VarSize);
    HiiSetBrowserData (
      &mPasswordConfigGuid,
      PASSWORD_CONFIG_VARIABLE,
      sizeof (PASSWORD_CONFIG_DATA),
      (UINT8 *) &PrivateData->PasswordConfigData,
      NULL
      );
  }
  return Status;
}

/**
  Get password configuration from variable.

  @param[in]    PrivateData    Point to PASSWORD_CONFIG_PRIVATE_DATA.

  @return       EFI_SUCCESS    Get password configuration from variable successfully.
  @return       other          Get password configuration from variable failed.
**/
EFI_STATUS
UpdatePasswordConfig (
  IN PASSWORD_CONFIG_PRIVATE_DATA  *PrivateData
)
{
  EFI_STATUS            Status;
  PASSWORD_CONFIG_DATA  PasswordConfigData;
  UINTN                 VarSize;

  Status = EFI_SUCCESS;
  VarSize = sizeof (PASSWORD_CONFIG_DATA);
  Status = gRT->GetVariable (
                  VAR_PASSWORD_CONFIG_NAME,
                  &gPasswordConfigVarGuid,
                  NULL,
                  &VarSize,
                  &PasswordConfigData
                  );
  if (Status == EFI_SUCCESS) {
    CopyMem (&PrivateData->PasswordConfigData, &PasswordConfigData, VarSize);
  }

  return Status;
}

/**
  Get password configuration from the form and set the configuration to the variable.

  @param[in]    This           Point to EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]    PrivateData    Point to PASSWORD_CONFIG_PRIVATE_DATA.

  @return       EFI_SUCCESS    Set password configuration in the form successfully.
  @return       other          Set password configuration in the form failed.
**/
EFI_STATUS
PasswordConfigSetupConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN PASSWORD_CONFIG_PRIVATE_DATA           *PrivateData
  )
{
  EFI_STATUS                Status;
  PASSWORD_CONFIG_DATA      PasswordData;
  if (!HiiGetBrowserData (&mPasswordConfigGuid, PASSWORD_CONFIG_VARIABLE, sizeof (PASSWORD_CONFIG_DATA), (UINT8 *) &PasswordData)) {
    return EFI_NOT_FOUND;
  }

  CopyMem (&PrivateData->PasswordConfigData, &PasswordData, sizeof(PASSWORD_CONFIG_DATA));
  Status = gRT->SetVariable(
              VAR_PASSWORD_CONFIG_NAME,
              &gPasswordConfigVarGuid,
              PLATFORM_SETUP_VARIABLE_FLAG,
              sizeof(PASSWORD_CONFIG_DATA),
              &PasswordData
          );
  if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "SetVariable failed: %r\n", Status));
  }

  return Status;
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Request        A null-terminated Unicode string in <ConfigRequest> format.
  @param[out] Progress       On return, points to a character in the Request string.
                             Points to the string's null terminator if request was successful.
                             Points to the most recent '&' before the first failing name/value
                             pair (or the beginning of the string if the failure is in the
                             first name/value pair) if the request was not successful.
  @param[out] Results        A null-terminated Unicode string in <ConfigAltResp> format which
                             has all values filled in for the names in the Request string.
                             String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
PasswordConfigExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                     Status;
  PASSWORD_CONFIG_PRIVATE_DATA   *Private;
  EFI_STRING                     ConfigRequestHdr;
  EFI_STRING                     ConfigRequest;
  BOOLEAN                        AllocatedRequest;
  UINTN                          Size;
  UINTN                          BufferSize;

  Status = EFI_SUCCESS;
  BufferSize = sizeof (PASSWORD_CONFIG_DATA);

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Private = PASSWORD_CONFIG_PRIVATE_DATA_FROM_THIS (This);
  UpdatePasswordConfig (Private);
  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &mPasswordConfigGuid, PASSWORD_CONFIG_VARIABLE)) {
    return EFI_NOT_FOUND;
  }
  ConfigRequestHdr = NULL;
  ConfigRequest  = NULL;
  AllocatedRequest = FALSE;
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    ConfigRequestHdr = HiiConstructConfigHdr (&mPasswordConfigGuid, PASSWORD_CONFIG_VARIABLE, Private->DriverHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }
  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) &Private->PasswordConfigData,
                                sizeof (PASSWORD_CONFIG_DATA),
                                Results,
                                Progress
                                );
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }
  if (Request == NULL) {
    *Progress = NULL;
  }
  else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param[in] This                 Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in] Configuration        A null-terminated Unicode string in <ConfigResp> format.
  @param[out] Progress            A pointer to a string filled in with the offset of the most
                                  recent '&' before the first failing name/value pair (or the
                                  beginning of the string if the failure is in the first
                                  name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
PasswordConfigRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            Buffsize;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *ConfigRouting;
  PASSWORD_CONFIG_PRIVATE_DATA     *Private;

  if (Configuration == NULL || Progress == NULL) {
    DEBUG((DEBUG_ERROR, "PasswordConfigRouteConfig: Invalid parameters.\n"));
    return EFI_INVALID_PARAMETER;
  }

  Private = PASSWORD_CONFIG_PRIVATE_DATA_FROM_THIS (This);
  *Progress = Configuration;
  // Locate HII Config Routing Protocol
  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **)&ConfigRouting
                );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "PasswordConfigRouteConfig: LocateProtocol failed. Status=%r\n", Status));
    return Status;
  }

  // Check if the Configuration matches
  if (!HiiIsConfigHdrMatch(Configuration, &mPasswordConfigGuid, PASSWORD_CONFIG_VARIABLE)) {
    DEBUG((DEBUG_ERROR, "PasswordConfigRouteConfig: Configuration header does not match.\n"));
    return EFI_NOT_FOUND;
  }

  // Sync Browser Data to Private Data
  PasswordConfigSetupConfig(This, Private);

  // Convert Configuration to Block
  Buffsize = sizeof(PASSWORD_CONFIG_DATA);
  Status = ConfigRouting->ConfigToBlock(
                            ConfigRouting,
                            Configuration,
                            (UINT8 *)&Private->PasswordConfigData,
                            &Buffsize,
                            Progress
                          );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "PasswordConfigRouteConfig: ConfigToBlock failed. Status=%r\n", Status));
  }

  return Status;
}

/**
  Record password from a HII input string.

  @param[in]  Private             The pointer to the global private data structure.
  @param[in]  StringId            The QuestionId received from HII input.
  @param[in]  StringBuffer        The unicode string buffer to store password.
  @param[in]  StringBufferLen     The len of unicode string buffer.

  @retval EFI_INVALID_PARAMETER   Any input parameter is invalid.
  @retval EFI_NOT_FOUND           The password string is not found or invalid.
  @retval EFI_SUCCESS             The operation is completed successfully.

**/
EFI_STATUS
RecordPassword (
  IN   PASSWORD_CONFIG_PRIVATE_DATA  *Private,
  IN   EFI_STRING_ID                 StringId,
  IN   CHAR16                        *StringBuffer,
  IN   UINTN                         StringBufferLen
  )
{
  CHAR16  *InputString;

  if ((StringId == 0) || (StringBuffer == NULL) || (StringBufferLen <= 0)) {
    return EFI_INVALID_PARAMETER;
  }

  InputString = HiiGetString (Private->HiiHandle, StringId, NULL);
  if (InputString == NULL) {
    return EFI_NOT_FOUND;
  }

  if (StrLen(InputString) >= StringBufferLen) {
    FreePool (InputString);
    return EFI_BUFFER_TOO_SMALL;
  }
  StrnCpyS (StringBuffer, StringBufferLen, InputString, StrLen(InputString));
  ZeroMem (InputString, (StrLen(InputString) + 1) * sizeof(CHAR16));
  FreePool (InputString);
  HiiSetString (Private->HiiHandle, StringId, L"", NULL);

  return EFI_SUCCESS;
}

/**
  Clean User Password,need admin right

  @param[in]  Private       A pointer to ADVANCED_CONFIG_CALLBACK_DATA.
  @param[in]  QuestionId    A unique value which is sent to the original exporting driver
                            so that it can identify the type of data to expect.
  @param[in]  Value         A pointer to the data being sent to the original exporting driver.
EFI_NOT_READY
  @retval     EFI_SUCCESS                   Success.
  @retval     EFI_ALREADY_STARTED           Old password exist.
  @retval     EFI_NOT_READY                 Typed in old password incorrect.
**/
EFI_STATUS
CleanUserPasswordConfigData (
  IN  PASSWORD_CONFIG_PRIVATE_DATA  *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  )
{
  EFI_STATUS            Status;
  UINTN                 VarSize;
  PASSWORD_CONFIG_DATA  TempData;
  EFI_INPUT_KEY         Key;
  CHAR16               *ConfirmCleanPasswdString;

  VarSize = sizeof(PASSWORD_CONFIG_DATA);
  Status = gRT->GetVariable(
                  VAR_PASSWORD_CONFIG_NAME,
                  &gPasswordConfigVarGuid,
                  NULL,
                  &VarSize,
                  &TempData
                );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  ConfirmCleanPasswdString = HiiGetString (Private->HiiHandle, STRING_TOKEN(STR_CONFIRM_CLEAN_PASSWD), NULL);
  do {
    CreatePopUp (EFI_WHITE | EFI_BACKGROUND_BLUE, &Key, L" ", ConfirmCleanPasswdString, L" ",NULL);
  } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

  if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
    StrCpyS(TempData.UserPassword, PASSWD_MAXLEN, L"user1234");
    TempData.UserPasswordEnable = 1;
    Status = gRT->SetVariable (
                    VAR_PASSWORD_CONFIG_NAME,
                    &gPasswordConfigVarGuid,
                    PLATFORM_SETUP_VARIABLE_FLAG,
                    sizeof (PASSWORD_CONFIG_DATA),
                    &TempData
                    );
    if (!EFI_ERROR(Status)) {
      CopyMem(&Private->PasswordConfigData, &TempData, sizeof(TempData));
      HiiSetBrowserData(
        &mPasswordConfigGuid,
        PASSWORD_CONFIG_VARIABLE,
        sizeof(PASSWORD_CONFIG_DATA),
        (UINT8*)&Private->PasswordConfigData,
        NULL
      );
    }
  }

  return EFI_SUCCESS;
}

/**
  Process callback function PasswordCheck().
  Get Password configuration data from variable and set browser data.

  @param[in]  Private       A pointer to ADVANCED_CONFIG_CALLBACK_DATA.
  @param[in]  QuestionId    A unique value which is sent to the original exporting driver
                            so that it can identify the type of data to expect.
  @param[in]  Value         A pointer to the data being sent to the original exporting driver.

  @retval     EFI_SUCCESS                   Success.
  @retval     EFI_ALREADY_STARTED           Old password exist.
  @retval     EFI_NOT_READY                 Typed in old password incorrect.
**/
EFI_STATUS
ProcessPasswordConfigData (
  IN  PASSWORD_CONFIG_PRIVATE_DATA  *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  )
{
  CHAR16 *TempPassword;
  EFI_STATUS Status;
  UINTN VarSize;
  UINTN Length;
  PASSWORD_CONFIG_DATA PasswordConfigData;

  VarSize = sizeof(PASSWORD_CONFIG_DATA);
  Length = sizeof(CHAR16) * PASSWD_MAXLEN;
  TempPassword = AllocateZeroPool(Length);

  if (TempPassword == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = RecordPassword(Private, Value->string, TempPassword, PASSWD_MAXLEN);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Error: Failed to input password!\n"));
    FreePool(TempPassword);
    return Status;
  }

  Status = gRT->GetVariable(
    VAR_PASSWORD_CONFIG_NAME,
    &gPasswordConfigVarGuid,
    NULL,
    &VarSize,
    &PasswordConfigData
  );

  if (!EFI_ERROR(Status)) {
    if (QuestionId == FORM_ADMIN_PASSWD_OPEN) {
      CopyMem(Private->PasswordConfigData.AdminPassword, TempPassword, Length);
    } else if (QuestionId == FORM_USER_PASSWD_OPEN) {
      CopyMem(Private->PasswordConfigData.UserPassword, TempPassword, Length);
      Private->PasswordConfigData.UserPasswordEnable = (StrLen(TempPassword) > 0);
    }

    Status = gRT->SetVariable(
      VAR_PASSWORD_CONFIG_NAME,
      &gPasswordConfigVarGuid,
      PLATFORM_SETUP_VARIABLE_FLAG,
      sizeof(PASSWORD_CONFIG_DATA),
      &Private->PasswordConfigData
    );
    GetPasswordConfigData(Private);
  }

  FreePool(TempPassword);
  return Status;
}

/**
  This call back function is registered with Device Info infomation formset.
  When user selects a boot option, this call back function will
  be triggered. The boot option is saved for later processing.


  @param[in] This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in] Action          Specifies the type of action taken by the browser.
  @param[in] QuestionId      A unique value which is sent to the original exporting driver
                             so that it can identify the type of data to expect.
  @param[in] Type            The type of value for the question.
  @param[in] Value           A pointer to the data being sent to the original exporting driver.
  @param[out] ActionRequest  On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS       The callback successfully handled the action.
  @retval  Others            Other errors as indicated.

**/
EFI_STATUS
EFIAPI
PasswordConfigCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  PASSWORD_CONFIG_PRIVATE_DATA  *Private;
  EFI_STATUS                    Status;
  UINTN                         VarSize;
  PASSWORD_CONFIG_DATA          PasswordConfigData;

  Private = PASSWORD_CONFIG_PRIVATE_DATA_FROM_THIS (This);
  Status  = EFI_SUCCESS;
  VarSize = sizeof(PASSWORD_CONFIG_DATA);
  Status = gRT->GetVariable (
                  VAR_PASSWORD_CONFIG_NAME,
                  &gPasswordConfigVarGuid,
                  NULL,
                  &VarSize,
                  &PasswordConfigData
                );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "PasswordConfigCallback: GetVariable error: %r\n", Status));
    return Status;
  }
  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    switch (QuestionId) {

    case FORM_ADMIN_PASSWD_OPEN:
      Status = ProcessPasswordConfigData (Private, QuestionId, Value);
      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "ProcessPasswordConfigData(Admin) failed: %r\n", Status));
      }
      break;

    case FORM_USER_PASSWD_OPEN:
      Status = ProcessPasswordConfigData (Private, QuestionId, Value);
      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "ProcessPasswordConfigData(User) failed: %r\n", Status));
      }
      break;

    case FORM_CLEAN_USER_PASSWORD:
      Status = CleanUserPasswordConfigData (Private, QuestionId, Value);
      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "CleanUserPasswordConfigData failed: %r\n", Status));
      }
      break;

   case FORM_USER_PASSWD_ENABLE:
      Status = gRT->GetVariable(
                VAR_PASSWORD_CONFIG_NAME,
                &gPasswordConfigVarGuid,
                NULL,
                &VarSize,
                &PasswordConfigData
              );
      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "GetVariable failed: %r\n", Status));
        break;
      }
      PasswordConfigData.UserPasswordEnable = Value->u8;
      Status = gRT->SetVariable(
                  VAR_PASSWORD_CONFIG_NAME,
                  &gPasswordConfigVarGuid,
                  PLATFORM_SETUP_VARIABLE_FLAG,
                  sizeof(PASSWORD_CONFIG_DATA),
                  &PasswordConfigData
                );
      HiiSetBrowserData(
                  &mPasswordConfigGuid,
    		  PASSWORD_CONFIG_VARIABLE,
    		  sizeof(PASSWORD_CONFIG_DATA),
    		  (UINT8*)&PasswordConfigData,
    		  NULL
		  );

      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to update UserPasswordEnable. %r\n", Status));
        break;
      }
    break;

    default:
      break;
    }
  }

  return Status;
}

/**
  This function publish the Phytium password configuration Form.

  @param[in, out]  PrivateData   Points to password configuration private data.

  @retval EFI_SUCCESS            HII Form is installed successfully.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource for HII Form installation.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
InstallPasswordConfigForm (
  IN OUT PASSWORD_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS                      Status;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;

  DriverHandle = NULL;
  mCheckFlag = 0;
  mFirstEnterPasswordConfigForm = FALSE;
  ConfigAccess = &PrivateData->ConfigAccess;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mPasswordConfigHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  ConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = gBS->InstallProtocolInterface(
                  &DriverHandle,
                  &gPasswordRestoreProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)&gPasswordRestoreProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PrivateData->DriverHandle = DriverHandle;
  HiiHandle = HiiAddPackages (
                &mPasswordConfigGuid,
                DriverHandle,
                PasswordConfigUiDxeStrings,
                PasswordConfigUiBin,
                NULL
                );
  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mPasswordConfigHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           ConfigAccess,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }
  PrivateData->HiiHandle = HiiHandle;
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (mStartOpCodeHandle == NULL) {
    UninstallPasswordConfigForm (PrivateData);
    return EFI_OUT_OF_RESOURCES;
  }
  mEndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (mEndOpCodeHandle == NULL) {
    UninstallPasswordConfigForm (PrivateData);
    return EFI_OUT_OF_RESOURCES;
  }
  mStartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                         mStartOpCodeHandle,
                                         &gEfiIfrTianoGuid,
                                         NULL,
                                         sizeof (EFI_IFR_GUID_LABEL)
                                         );
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  mEndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                       mEndOpCodeHandle,
                                       &gEfiIfrTianoGuid,
                                       NULL,
                                       sizeof (EFI_IFR_GUID_LABEL)
                                       );
  mEndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  PasswordConfigInit ();
  return EFI_SUCCESS;
}

/**
  This function removes phytium password configuration Form.

  @param[in, out]  PrivateData   Points to password configuration private data.

**/
VOID
UninstallPasswordConfigForm (
  IN OUT PASSWORD_CONFIG_PRIVATE_DATA    *PrivateData
  )
{
  if (PrivateData->HiiHandle != NULL) {
    HiiRemovePackages (PrivateData->HiiHandle);
    PrivateData->HiiHandle = NULL;
  }
  if (PrivateData->DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           PrivateData->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mPasswordConfigHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &PrivateData->ConfigAccess,
           NULL
           );
    PrivateData->DriverHandle = NULL;
  }
  FreePool (PrivateData);
  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }
  if (mEndOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mEndOpCodeHandle);
  }
}
