/** @file

  FrontPage routines to handle the callbacks and browser calls

  Copyright (c) 2025, SOPHGO Technologies Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "FrontPage.h"
#include "FrontPageCustomizedUi.h"

#define MAX_STRING_LEN             500
#define PASSWORD_CONFIG_ATTRIBUTES (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

extern EFI_HII_HANDLE       gStringPackHandle;
EFI_FORM_BROWSER2_PROTOCOL  *gFormBrowser2;
PASSWORD_TOGGLE_DATA        PassWordToggleData;
CHAR8                       *mLanguageString;
VOID                        *ProtocolPtr;

EFI_GUID mFrontPageGuid             = FORMSET_GUID;
EFI_GUID gEfiAcpiTableGuid          = ACPI_TABLE_GUID;
EFI_GUID gEfiLinuxDtbTableGuid      = LINUX_EFI_DT_TABLE_GUID;
BOOLEAN  mResetRequired             = FALSE;
BOOLEAN  mModeInitialized           = FALSE;
UINT32   mBootHorizontalResolution  = 0;
UINT32   mBootVerticalResolution    = 0;
UINT32   mBootTextModeColumn        = 0;
UINT32   mBootTextModeRow           = 0;
UINT32   mSetupTextModeColumn       = 0;
UINT32   mSetupTextModeRow          = 0;
UINT32   mSetupHorizontalResolution = 0;
UINT32   mSetupVerticalResolution   = 0;
SMBIOS_PARSED_DATA *ParsedData      = NULL;
VOID *AcpiTable = NULL;
VOID *Acpi20Table = NULL;

STATIC RESTORE_PROTOCOL gPassWordToggleRestoreProtocol = {
  PassWordToggleRestore
};

FRONT_PAGE_CALLBACK_DATA gFrontPagePrivate = {
  FRONT_PAGE_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  NULL,
  {
    ExtractConfig,
    RouteConfig,
    FrontPageCallback
  }
};

HII_VENDOR_DEVICE_PATH mFrontPageHiiVendorDevicePath0 = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    DEVICE_PATH_GUID
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

BOOLEAN
GetAcpiTable
(
 VOID
)
{
    UINTN TableCount = gST->NumberOfTableEntries;

    if (TableCount == 0 || gST->ConfigurationTable == NULL) {
        DEBUG((DEBUG_ERROR, "ConfigurationTable is NULL or TableCount is 0\n"));
        return FALSE;
    }

    for (UINTN Index = 0; Index < TableCount; Index++) {
        EFI_CONFIGURATION_TABLE *CurrentTable = &gST->ConfigurationTable[Index];

        if (CurrentTable == NULL) {
            DEBUG((DEBUG_ERROR, "ConfigurationTable entry is NULL at Index %u\n", Index));
            continue;
        }

        if (CompareGuid(&CurrentTable->VendorGuid, &gEfiAcpiTableGuid)) {
            AcpiTable = CurrentTable->VendorTable;
            return TRUE;
        }

        if (CompareGuid(&CurrentTable->VendorGuid, &gEfiAcpi20TableGuid)) {
            Acpi20Table = CurrentTable->VendorTable;
            return TRUE;
        }
    }

    return FALSE;
}

EFI_STATUS
EFIAPI
RemoveAcpiFromConfigTable
(
    VOID
)
{
    UINTN Index, NewIndex = 0;
    UINTN TableCount = gST->NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE *CurrentTable;
    EFI_STATUS Status;

    for (Index = 0; Index < TableCount; Index++) {
        CurrentTable = &gST->ConfigurationTable[Index];
        if (CurrentTable == NULL) {
            DEBUG((DEBUG_ERROR, "Current ConfigurationTable entry is NULL at Index %u\n", Index));
            continue;
        }

         if (CompareGuid(&CurrentTable->VendorGuid, &gEfiAcpiTableGuid)) {
            Status = gBS->InstallConfigurationTable(&gEfiAcpiTableGuid, NULL);
            if (EFI_ERROR(Status)) {
                DEBUG((DEBUG_ERROR, "Failed to remove ACPI 20 table from EFI System Table\n"));
            }
           continue;
        }

        if (CompareGuid(&CurrentTable->VendorGuid, &gEfiAcpi20TableGuid)) {
            Status = gBS->InstallConfigurationTable(&gEfiAcpi20TableGuid, NULL);
            if (EFI_ERROR(Status)) {
               DEBUG((DEBUG_ERROR, "Failed to remove ACPI table from EFI System Table\n"));
            }
           continue;
        }

        gST->ConfigurationTable[NewIndex++] = *CurrentTable;
    }

    if (NewIndex != TableCount) {
        for (Index = NewIndex; Index < TableCount; Index++) {
            ZeroMem(&gST->ConfigurationTable[Index], sizeof(EFI_CONFIGURATION_TABLE));
        }
    }

    gST->NumberOfTableEntries = NewIndex;
    return EFI_SUCCESS;
}

EFI_STATUS
InstallDtb
(
 VOID
)
{
    EFI_RISCV_FIRMWARE_CONTEXT  *FirmwareContext;
    VOID                        *DtbAddress;
    EFI_STATUS                  Status;

    FirmwareContext = NULL;
    GetFirmwareContextPointer (&FirmwareContext);

    if (FirmwareContext == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Firmware Context is NULL\n",
      __func__
      ));
      return EFI_UNSUPPORTED;
    }

    DtbAddress = (VOID *)FirmwareContext->FlattenedDeviceTree;
    if (DtbAddress == NULL) {
      DEBUG((DEBUG_ERROR, "DtbAddress is NULL!\n"));
      return EFI_INVALID_PARAMETER;
    }
    Status = gBS->InstallConfigurationTable(&gEfiLinuxDtbTableGuid, DtbAddress);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to install DTB: %r\n", Status));
        return Status;
    }
    return EFI_SUCCESS;
}

EFI_STATUS
InstallAcpiTables
(
 VOID
)
{
    EFI_STATUS Status;
    if (AcpiTable == NULL && Acpi20Table == NULL) {
       return EFI_NOT_FOUND;
    }
    if (AcpiTable != NULL) {
        Status = gBS->InstallConfigurationTable(&gEfiAcpiTableGuid, AcpiTable);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Failed to restore ACPI Table: %r\n", Status));
            return Status;
        }
    }

    if (Acpi20Table != NULL) {
        Status = gBS->InstallConfigurationTable(&gEfiAcpi20TableGuid, Acpi20Table);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Failed to restore ACPI 2.0 Table: %r\n", Status));
            return Status;
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS
UnloadAcpiTables
(
 VOID
)
{
    EFI_STATUS Status;
    Status = RemoveAcpiFromConfigTable();
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "[Error] Failed to remove ACPI Tables: %r\n", Status));
      return Status;
    }
    Status = InstallDtb();
    if (EFI_ERROR(Status)) {
       Status = InstallAcpiTables();
       if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Can not install dtb and acpi table\n"));
        return Status;
       }
       DEBUG((DEBUG_ERROR, "Can not install dtb, so reinstalled acpi\n"));
       return Status;
    }

    return EFI_SUCCESS;
}

BOOLEAN
ConfirmResetDefaults (
  CHAR16 *ConfirmPrompt
  )
{
  EFI_INPUT_KEY Key;

  CreatePopUp(EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, NULL,
                ConfirmPrompt,
                L"[Y] Yes    [N] No",
                NULL);

  while (TRUE) {
    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, NULL);
    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (Key.UnicodeChar == 'Y' || Key.UnicodeChar == 'y') {
      return TRUE;
    } else if (Key.UnicodeChar == 'N' || Key.UnicodeChar == 'n') {
      return FALSE;
    }
  }
}

EFI_STATUS
EFIAPI
RestoreFactoryDefaults (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_STATUS OverallStatus = EFI_SUCCESS;
  EFI_GUID *ModuleGuids[] = {
    &gSetDateAndTimeRestoreProtocolGuid,
    &gPasswordRestoreProtocolGuid,
    &gPassWordToggleRestoreProtocolGuid,
    &gReserveMemoryRestoreProtocolGuid
  };

  for (UINTN i = 0; i < ARRAY_SIZE (ModuleGuids); i++) {
    RESTORE_PROTOCOL *RestoreProtocol = NULL;
    Status = gBS->LocateProtocol (ModuleGuids[i], NULL, (VOID **)&RestoreProtocol);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to locate protocol for module %u: %r\n", i, Status));
      if (!EFI_ERROR (OverallStatus)) {
        OverallStatus = Status;
      }
      continue;
    }

    if (RestoreProtocol == NULL) {
      DEBUG ((DEBUG_ERROR, "RestoreProtocol is NULL for module %u.\n", i));
      if (!EFI_ERROR (OverallStatus)) {
        OverallStatus = EFI_DEVICE_ERROR;
      }
      continue;
    }

    if (RestoreProtocol->RestoreDefaults == NULL) {
      DEBUG((DEBUG_ERROR, "RestoreDefaults function pointer is NULL for module %u.\n", i));
      if (!EFI_ERROR (OverallStatus)) {
        OverallStatus = EFI_DEVICE_ERROR;
      }
      continue;
    }
    Status = RestoreProtocol->RestoreDefaults ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to restore defaults for module %u: %r\n", i, Status));
      if (!EFI_ERROR (OverallStatus)) {
        OverallStatus = Status;
      }
    }
  }

  if (EFI_ERROR (OverallStatus)) {
    DEBUG ((DEBUG_ERROR, "One or more modules failed to restore defaults.\n"));
  }

  return OverallStatus;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR      The variable could not be saved.
  @retval  EFI_UNSUPPORTED       The specified Action is not supported by the callback.
**/
EFI_STATUS
EFIAPI
FrontPageCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN  EFI_BROWSER_ACTION                   Action,
  IN  EFI_QUESTION_ID                      QuestionId,
  IN  UINT8                                Type,
  IN  EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST           *ActionRequest
  )
{
  EFI_STATUS Status;
  CHAR16     *ConfirmResetPrompt = NULL;
  EFI_INPUT_KEY Key;

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if (QuestionId == RESTORE_DEFAULTS_QUESTION_ID) {
      ConfirmResetPrompt = HiiGetString (gFrontPagePrivate.HiiHandle, STRING_TOKEN (STR_CONFIRM_RESET_PROMPT), NULL);
      if (ConfirmResetPrompt == NULL) {
        DEBUG ((DEBUG_ERROR, "Failed to get confirm reset prompt string.\n"));
        return EFI_OUT_OF_RESOURCES;
      }

      if (!ConfirmResetDefaults (ConfirmResetPrompt)) {
        FreePool (ConfirmResetPrompt);
        return EFI_SUCCESS;
      }

      FreePool (ConfirmResetPrompt);

      Status = RestoreFactoryDefaults ();
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to restore factory defaults: %r\n", Status));
        return Status;
      }
    }
    if (QuestionId == ACPI_DISABLE_QUESTION_ID) {
      if(Value->u8 == 0x0) {
        Status = UnloadAcpiTables();
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Failed to unload ACPI Tables: %r\n", Status));
            return Status;
        }
        CHAR16 *AcpiDisableMsg[] = {
            L"ACPI Disabled Successfully",
            L"Press ENTER to continue..."
        };
        CreatePopUp(EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, NULL, AcpiDisableMsg[0], AcpiDisableMsg[1], NULL);
        while (TRUE) {
            Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
            if (!EFI_ERROR(Status) && (Key.UnicodeChar == CHAR_CARRIAGE_RETURN)) {
                break;
            }
        }
      }
      if(Value->u8 == 0x1) {
        Status = InstallAcpiTables();
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Failed to load ACPI Tables: %r\n", Status));
            return Status;
        }
        CHAR16 *AcpiDisableMsg[] = {
            L"ACPI enabled Successfully",
            L"Press ENTER to continue..."
        };
        CreatePopUp(EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, NULL, AcpiDisableMsg[0], AcpiDisableMsg[1], NULL);
        while (TRUE) {
            Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
            if (!EFI_ERROR(Status) && (Key.UnicodeChar == CHAR_CARRIAGE_RETURN)) {
                break;
            }
        }
      }
    }
  }

  return UiFrontPageCallbackHandler (gFrontPagePrivate.HiiHandle, Action, QuestionId, Type, Value, ActionRequest);
}

EFI_STATUS
EFIAPI
PassWordToggleRestore (
  VOID
  )
{
  EFI_STATUS Status;
  UINTN      VarSize;

  VarSize = sizeof (PASSWORD_TOGGLE_DATA);
  PassWordToggleData.IsFirst = 0;
  PassWordToggleData.UserPriv = 0;

  Status = gRT->SetVariable (
		  EFI_PASSWORD_TOGGLE_VARIABLE_NAME,
		  &gEfiSophgoGlobalVariableGuid,
		  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
		  sizeof (PASSWORD_TOGGLE_DATA),
		  &PassWordToggleData
		  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to Restore PassWordToggleData. Status=%r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InitializePasswordToggleVariable (
  VOID
  )
{
  EFI_STATUS Status;
  UINTN      VarSize;

  VarSize = sizeof (PASSWORD_TOGGLE_DATA);

  Status = gRT->GetVariable (
		  EFI_PASSWORD_TOGGLE_VARIABLE_NAME,
                  &gEfiSophgoGlobalVariableGuid,
                  NULL,
                  &VarSize,
                  &PassWordToggleData
                  );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      PassWordToggleData.PasswordCheckEnabled = 0;
      PassWordToggleData.IsFirst = 0;
      PassWordToggleData.UserPriv = 0;
      PassWordToggleData.IsEvb = 0;
      PassWordToggleData.DefaultAcpi = 0;
      Status = gRT->SetVariable (
		      EFI_PASSWORD_TOGGLE_VARIABLE_NAME,
		      &gEfiSophgoGlobalVariableGuid,
		      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
		      sizeof (PASSWORD_TOGGLE_DATA),
		      &PassWordToggleData
		      );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to initialize PassWordToggleData. Status=%r\n", Status));
        return Status;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "Failed to read PassWordToggleData. Status=%r\n", Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

BOOLEAN
PassWordToggleEntry (
  VOID
  )
{
  UINTN      VarSize;
  EFI_STATUS Status;

  VarSize = sizeof(PassWordToggleData);
  Status = gRT->GetVariable (
		  EFI_PASSWORD_TOGGLE_VARIABLE_NAME,
		  &gEfiSophgoGlobalVariableGuid,
		  NULL,
		  &VarSize,
		  &PassWordToggleData
		  );
  if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to Get PassWordToggleData. Status=%r\n", Status));
        return Status;
  }
  return (PassWordToggleData.PasswordCheckEnabled == 1) ? TRUE : FALSE;
}

EFI_STATUS
FrontPagePasswordCheck (
  VOID
  )
{
  CHAR16                  TempStr[PASSWD_MAXLEN];
  CHAR16                  UsernameStr[PASSWD_MAXLEN];
  CHAR16                  ChanceStr[30];
  PASSWORD_CONFIG_DATA    PasswordConfigData;
  UINTN                   VarSize;
  EFI_STATUS              Status;
  EFI_INPUT_KEY           Key;
  CHAR16                  *EnterUsernameString;
  CHAR16                  *EnterPasswordString;
  CHAR16                  *PasswordIncorrectString;
  CHAR16                  *UsernameNotFoundString;
  CHAR16                  *YouHaveChanceLeftString;
  CHAR16                  *PressEnterContinueString;

  UINT8 Chance = 4;
  ZeroMem (TempStr,      sizeof(TempStr));
  ZeroMem (UsernameStr,  sizeof(UsernameStr));
  ZeroMem (&PasswordConfigData, sizeof (PasswordConfigData));
  VarSize = sizeof (PASSWORD_CONFIG_DATA);
  Status = gRT->GetVariable (
		  EFI_PASSWORD_CONFIG_VARIABLE_NAME,
                  &gEfiSophgoGlobalVariableGuid,
                  NULL,
                  &VarSize,
                  &PasswordConfigData
                  );
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "Failed to get password config variable. Status: %r\n", Status));
    return Status;
  }
  CONST CHAR16            *AdminName = L"admin";
  CONST CHAR16            *UserName = L"user";

  while (TRUE) {
    EnterUsernameString = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_ENTER_USERNAME), NULL);
    ReadString (EnterUsernameString, UsernameStr);
    if (StrCmp (UsernameStr, AdminName) == 0) {
      PasswordConfigData.UserPriv = 1;
      PassWordToggleData.UserPriv = 1;
      break;
    } else if (StrCmp(UsernameStr, UserName) == 0) {
      if (PasswordConfigData.UserPasswordEnable == 0) {
        DEBUG((DEBUG_WARN, "User password is not enabled. Cannot login.\n"));
        UsernameNotFoundString = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_USERNAME_NOT_ENABLED), NULL);
        if (UsernameNotFoundString == NULL) {
          UsernameNotFoundString = L"User password is disabled. No login.";
        }
        do {
          CreateDialog (&Key, UsernameNotFoundString, NULL);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

        gST->ConOut->ClearScreen (gST->ConOut);
        continue;
      }
      PasswordConfigData.UserPriv = 0;
      PassWordToggleData.UserPriv = 0;
      break;
    } else {
      UsernameNotFoundString = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_USERNAME_NOT_FOUND), NULL);
      if (UsernameNotFoundString == NULL) {
        UsernameNotFoundString = L"Username not found.";
      }
      do {
        CreateDialog (&Key, UsernameNotFoundString, NULL);
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
      gST->ConOut->ClearScreen (gST->ConOut);
    }
  }

  while (Chance > 0) {
    EnterPasswordString = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_ENTER_PASSWORD), NULL);
    ReadString (EnterPasswordString, TempStr);
    if ((PasswordConfigData.UserPriv == 1) &&
        (StrCmp (TempStr, PasswordConfigData.AdminPassword) == 0)) {
      break;
    } else if (
        (PasswordConfigData.UserPriv == 0) &&
        (StrCmp (TempStr, PasswordConfigData.UserPassword) == 0)
      ) {
      break;
    } else {
      DEBUG ((DEBUG_WARN, "Password validation failed. Remaining chances: %d\n", Chance - 1));
      gST->ConOut->ClearScreen (gST->ConOut);
      Chance--;
      if (Chance == 0) {
        DEBUG ((DEBUG_ERROR, "Maximum attempts reached. System resetting...\n"));
        gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
      } else {
        YouHaveChanceLeftString = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_YOU_HAVE_CHANCE_LEFT), NULL);
        UnicodeSPrint (ChanceStr, sizeof (ChanceStr), YouHaveChanceLeftString, Chance);

        PasswordIncorrectString = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_PASSWORD_INCORRECT), NULL);
        PressEnterContinueString = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_PRESEE_ENTER_CONTINUE), NULL);
        do {
          CreateDialog(&Key, PasswordIncorrectString, ChanceStr, PressEnterContinueString, NULL);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
      }
    }
    ZeroMem (TempStr, sizeof (TempStr));
  }

  Status = gRT->SetVariable (
		  EFI_PASSWORD_CONFIG_VARIABLE_NAME,
		  &gEfiSophgoGlobalVariableGuid,
		  PLATFORM_SETUP_VARIABLE_FLAG,
		  sizeof (PASSWORD_CONFIG_DATA),
		  &PasswordConfigData
		  );
   if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to set password config variable. Status: %r\n", Status));
    return Status;
  }

  Status = gRT->SetVariable (
		  EFI_PASSWORD_TOGGLE_VARIABLE_NAME,
		  &gEfiSophgoGlobalVariableGuid,
		  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
		  sizeof (PASSWORD_TOGGLE_DATA),
                  &PassWordToggleData
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to set passwordtoggle config variable. Status: %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InitializeFrontPage (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **)&gFormBrowser2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gFrontPagePrivate.DriverHandle = NULL;
  Status      = gBS->InstallMultipleProtocolInterfaces (
                                          &gFrontPagePrivate.DriverHandle,
                                          &gEfiDevicePathProtocolGuid,
                                          &mFrontPageHiiVendorDevicePath0,
                                          &gEfiHiiConfigAccessProtocolGuid,
                                          &gFrontPagePrivate.ConfigAccess,
                                          NULL
                                          );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallProtocolInterface(
                  &gFrontPagePrivate.DriverHandle,
                  &gPassWordToggleRestoreProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)&gPassWordToggleRestoreProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  gFrontPagePrivate.HiiHandle = HiiAddPackages (
                                  &mFrontPageGuid,
                                  gFrontPagePrivate.DriverHandle,
                                  FrontPageVfrBin,
                                  UiAppStrings,
                                  NULL
                                  );
  ASSERT (gFrontPagePrivate.HiiHandle != NULL);

  Status = UpdateFrontPageForm ();
  ASSERT_EFI_ERROR (Status);

  return Status;
}

EFI_STATUS
CallFrontPage (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_INPUT_WAIT)
    );

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status        = gFormBrowser2->SendForm (
                                   gFormBrowser2,
                                   &gFrontPagePrivate.HiiHandle,
                                   1,
                                   &mFrontPageGuid,
                                   0,
                                   NULL,
                                   &ActionRequest
				   );
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  return Status;
}

VOID
FreeFrontPage (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  gFrontPagePrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mFrontPageHiiVendorDevicePath0,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gFrontPagePrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  HiiRemovePackages (gFrontPagePrivate.HiiHandle);
  if (gFrontPagePrivate.LanguageToken != NULL) {
    FreePool (gFrontPagePrivate.LanguageToken);
    gFrontPagePrivate.LanguageToken = NULL;
  }
}

/**
  Convert Processor Frequency Data to a string.

  @param ProcessorFrequency The frequency data to process
  @param Base10Exponent     The exponent based on 10
  @param String             The string that is created

**/
VOID
ConvertProcessorToString (
  IN  UINT16  ProcessorFrequency,
  IN  UINT16  Base10Exponent,
  OUT CHAR16  **String
  )
{
  CHAR16  *StringBuffer;
  UINTN   Index;
  UINTN   DestMax;
  UINT32  FreqMhz;

  if (Base10Exponent >= 6) {
    FreqMhz = ProcessorFrequency;
    for (Index = 0; Index < (UINT32)Base10Exponent - 6; Index++) {
      FreqMhz *= 10;
    }
  } else {
    FreqMhz = 0;
  }

  DestMax      = 0x20 / sizeof (CHAR16);
  StringBuffer = AllocateZeroPool (0x20);
  ASSERT (StringBuffer != NULL);
  UnicodeValueToStringS (StringBuffer, sizeof (CHAR16) * DestMax, LEFT_JUSTIFY, FreqMhz / 1000, 3);
  Index = StrnLenS (StringBuffer, DestMax);
  StrCatS (StringBuffer, DestMax, L".");
  UnicodeValueToStringS (
    StringBuffer + Index + 1,
    sizeof (CHAR16) * (DestMax - (Index + 1)),
    PREFIX_ZERO,
    (FreqMhz % 1000) / 10,
    2
    );
  StrCatS (StringBuffer, DestMax, L" GHz");
  *String = (CHAR16 *)StringBuffer;
  return;
}

/**
  Convert Memory Size to a string.

  @param MemorySize      The size of the memory to process
  @param String          The string that is created

**/
VOID
ConvertMemorySizeToString (
  IN  UINT32  MemorySize,
  OUT CHAR16  **String
  )
{
  CHAR16  *StringBuffer;

  StringBuffer = AllocateZeroPool (0x24);
  ASSERT (StringBuffer != NULL);
  UnicodeValueToStringS (StringBuffer, 0x24, LEFT_JUSTIFY, MemorySize, 10);
  StrCatS (StringBuffer, 0x24 / sizeof (CHAR16), L" MB RAM");

  *String = (CHAR16 *)StringBuffer;

  return;
}

/**

  Acquire the string associated with the Index from smbios structure and return it.
  The caller is responsible for free the string buffer.

  @param    OptionalStrStart  The start position to search the string
  @param    Index             The index of the string to extract
  @param    String            The string that is extracted

  @retval   EFI_SUCCESS       The function returns EFI_SUCCESS always.

**/
EFI_STATUS
GetOptionalStringByIndex (
  IN      CHAR8   *OptionalStrStart,
  IN      UINT8   Index,
  OUT     CHAR16  **String
  )
{
  UINTN  StrSize;

  if (Index == 0) {
    *String = AllocateZeroPool (sizeof (CHAR16));
    return EFI_SUCCESS;
  }

  StrSize = 0;
  do {
    Index--;
    OptionalStrStart += StrSize;
    StrSize           = AsciiStrSize (OptionalStrStart);
  } while (OptionalStrStart[StrSize] != 0 && Index != 0);

  if ((Index != 0) || (StrSize == 1)) {
    *String = GetStringById (STRING_TOKEN (STR_MISSING_STRING));
  } else {
    *String = AllocatePool (StrSize * sizeof (CHAR16));
    AsciiStrToUnicodeStrS (OptionalStrStart, *String, StrSize);
  }

  return EFI_SUCCESS;
}

/**
  This function will change video resolution and text mode
  according to defined setup mode or defined boot mode

  @param  IsSetupMode   Indicate mode is changed to setup mode or boot mode.

  @retval  EFI_SUCCESS  Mode is changed successfully.
  @retval  Others             Mode failed to be changed.

**/
EFI_STATUS
UiSetConsoleMode (
  BOOLEAN  IsSetupMode
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL       *SimpleTextOut;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  UINT32                                MaxGopMode;
  UINT32                                MaxTextMode;
  UINT32                                ModeNumber;
  UINT32                                NewHorizontalResolution;
  UINT32                                NewVerticalResolution;
  UINT32                                NewColumns;
  UINT32                                NewRows;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *HandleBuffer;
  EFI_STATUS                            Status;
  UINTN                                 Index;
  UINTN                                 CurrentColumn;
  UINTN                                 CurrentRow;

  MaxGopMode  = 0;
  MaxTextMode = 0;
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **)&SimpleTextOut
                  );
  if (EFI_ERROR (Status)) {
    SimpleTextOut = NULL;
  }

  if ((GraphicsOutput == NULL) || (SimpleTextOut == NULL)) {
    return EFI_UNSUPPORTED;
  }

  if (IsSetupMode) {
    NewHorizontalResolution = mSetupHorizontalResolution;
    NewVerticalResolution   = mSetupVerticalResolution;
    NewColumns              = mSetupTextModeColumn;
    NewRows                 = mSetupTextModeRow;
  } else {
    NewHorizontalResolution = mBootHorizontalResolution;
    NewVerticalResolution   = mBootVerticalResolution;
    NewColumns              = mBootTextModeColumn;
    NewRows                 = mBootTextModeRow;
  }

  if (GraphicsOutput != NULL) {
    MaxGopMode = GraphicsOutput->Mode->MaxMode;
  }

  if (SimpleTextOut != NULL) {
    MaxTextMode = SimpleTextOut->Mode->MaxMode;
  }

  for (ModeNumber = 0; ModeNumber < MaxGopMode; ModeNumber++) {
    Status = GraphicsOutput->QueryMode (
                               GraphicsOutput,
                               ModeNumber,
                               &SizeOfInfo,
                               &Info
                               );
    if (!EFI_ERROR (Status)) {
      if ((Info->HorizontalResolution == NewHorizontalResolution) &&
          (Info->VerticalResolution == NewVerticalResolution))
      {
        if ((GraphicsOutput->Mode->Info->HorizontalResolution == NewHorizontalResolution) &&
            (GraphicsOutput->Mode->Info->VerticalResolution == NewVerticalResolution))
        {
          Status = SimpleTextOut->QueryMode (SimpleTextOut, SimpleTextOut->Mode->Mode, &CurrentColumn, &CurrentRow);
          ASSERT_EFI_ERROR (Status);
          if ((CurrentColumn == NewColumns) && (CurrentRow == NewRows)) {
            FreePool (Info);
            return EFI_SUCCESS;
          } else {
            for (Index = 0; Index < MaxTextMode; Index++) {
              Status = SimpleTextOut->QueryMode (SimpleTextOut, Index, &CurrentColumn, &CurrentRow);
              if (!EFI_ERROR (Status)) {
                if ((CurrentColumn == NewColumns) && (CurrentRow == NewRows)) {
                  Status = SimpleTextOut->SetMode (SimpleTextOut, Index);
                  ASSERT_EFI_ERROR (Status);
                  Status = PcdSet32S (PcdConOutColumn, mSetupTextModeColumn);
                  ASSERT_EFI_ERROR (Status);
                  Status = PcdSet32S (PcdConOutRow, mSetupTextModeRow);
                  ASSERT_EFI_ERROR (Status);
                  FreePool (Info);
                  return EFI_SUCCESS;
                }
              }
            }

            if (Index == MaxTextMode) {
              FreePool (Info);
              return EFI_UNSUPPORTED;
            }
          }
        } else {
          Status = GraphicsOutput->SetMode (GraphicsOutput, ModeNumber);
          if (!EFI_ERROR (Status)) {
            FreePool (Info);
            break;
          }
        }
      }

      FreePool (Info);
    }
  }

  if (ModeNumber == MaxGopMode) {
    return EFI_UNSUPPORTED;
  }

  Status = PcdSet32S (PcdVideoHorizontalResolution, NewHorizontalResolution);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdVideoVerticalResolution, NewVerticalResolution);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdConOutColumn, NewColumns);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdConOutRow, NewRows);
  ASSERT_EFI_ERROR (Status);
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleTextOutProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
    }

    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }

    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  EFI_STATUS                Status;
  PASSWORD_TOGGLE_DATA      PassWordToggleData;
  UINTN                     VarSize;
  EFI_STRING                ConfigRequestHdr;

  VarSize = sizeof (PASSWORD_TOGGLE_DATA);
  if ((Progress == NULL) || (Results == NULL)) {
    DEBUG ((DEBUG_ERROR, "ExtractConfig: Invalid parameters. Progress=%p, Results=%p\n", Progress, Results));
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;

  ConfigRequestHdr = HiiConstructConfigHdr (
		  &gEfiSophgoGlobalVariableGuid,
		  EFI_PASSWORD_TOGGLE_VARIABLE_NAME,
		  NULL
		  );
  if (ConfigRequestHdr == NULL) {
    DEBUG ((DEBUG_ERROR, "ExtractConfig: Failed to construct ConfigRequestHdr.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  if (Request == NULL) {
    *Results = AllocateCopyPool (StrSize (ConfigRequestHdr), ConfigRequestHdr);
    if (*Results == NULL) {
      FreePool (ConfigRequestHdr);
      return EFI_OUT_OF_RESOURCES;
    }

    FreePool (ConfigRequestHdr);
    return EFI_SUCCESS;
  }

  if (!HiiIsConfigHdrMatch (Request, &gEfiSophgoGlobalVariableGuid, EFI_PASSWORD_TOGGLE_VARIABLE_NAME)) {
    DEBUG ((DEBUG_ERROR, "ExtractConfig: Request does not match ConfigHdr.\n"));
    FreePool (ConfigRequestHdr);
    return EFI_NOT_FOUND;
  }

  Status = gRT->GetVariable (
		  EFI_PASSWORD_TOGGLE_VARIABLE_NAME,
		  &gEfiSophgoGlobalVariableGuid,
		  NULL,
		  &VarSize,
		  &PassWordToggleData
		  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "ExtractConfig: Failed to get variable. Status=%r\n", Status));
    FreePool (ConfigRequestHdr);
    return Status;
  }

  Status = gHiiConfigRouting->BlockToConfig (
		  gHiiConfigRouting,
                  Request,
                  (UINT8 *)&PassWordToggleData,
                  VarSize,
                  Results,
                  Progress
                  );

  FreePool (ConfigRequestHdr);

  return Status;
}

EFI_STATUS
EFIAPI
RouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  EFI_STATUS            Status;
  PASSWORD_TOGGLE_DATA  PassWordToggleData;
  UINTN                 VarSize;

  VarSize = sizeof (PASSWORD_TOGGLE_DATA);

  if ((Configuration == NULL) || (Progress == NULL)) {
    DEBUG ((DEBUG_ERROR, "RouteConfig: Invalid parameters. Configuration=%p, Progress=%p\n", Configuration, Progress));
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;
  if (!HiiIsConfigHdrMatch (Configuration, &gEfiSophgoGlobalVariableGuid, EFI_PASSWORD_TOGGLE_VARIABLE_NAME)) {
    DEBUG ((DEBUG_ERROR, "RouteConfig: Configuration does not match ConfigHdr.\n"));
    return EFI_NOT_FOUND;
  }

  Status = gRT->GetVariable (
		  EFI_PASSWORD_TOGGLE_VARIABLE_NAME,
                  &gEfiSophgoGlobalVariableGuid,
                  NULL,
                  &VarSize,
                  &PassWordToggleData
		  );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "RouteConfig: Failed to get variable. Status=%r\n", Status));
    return Status;
  }

  Status = gHiiConfigRouting->ConfigToBlock (
		  gHiiConfigRouting,
                  Configuration,
                  (UINT8 *)&PassWordToggleData,
                  &VarSize,
                  Progress
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
		  EFI_PASSWORD_TOGGLE_VARIABLE_NAME,
                  &gEfiSophgoGlobalVariableGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (PASSWORD_TOGGLE_DATA),
                  &PassWordToggleData
                  );

  return Status;
}

EFI_STATUS
UpdateTimeRegion (
  EFI_HII_HANDLE HiiHandle
  )
{
  EFI_STATUS         Status;
  VOID               *StartOpCodeHandle;
  VOID               *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL *StartGuidLabel;
  EFI_IFR_GUID_LABEL *EndGuidLabel;


  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (StartOpCodeHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (EndOpCodeHandle == NULL) {
    HiiFreeOpCodeHandle (StartOpCodeHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  StartGuidLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
		  StartOpCodeHandle,
                  &gEfiIfrTianoGuid,
                  NULL,
                  sizeof (EFI_IFR_GUID_LABEL)
		  );
  StartGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartGuidLabel->Number = LABEL_TIME_START;
  EndGuidLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
		  EndOpCodeHandle,
		  &gEfiIfrTianoGuid,
		  NULL,
                  sizeof (EFI_IFR_GUID_LABEL)
                  );
  EndGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndGuidLabel->Number = LABEL_END;
  HiiCreateDateOpCode (
		  StartOpCodeHandle,
                  0x0,
                  0x0,
                  0x0,
                  STRING_TOKEN (STR_DATE_PROMT),
                  STRING_TOKEN (STR_DATE_HELP),
                  EFI_IFR_FLAG_READ_ONLY,
                  QF_DATE_STORAGE_TIME,
                  NULL
		  );
  HiiCreateTimeOpCode (
                  StartOpCodeHandle,
                  0x0,
                  0x0,
                  0x0,
                  STRING_TOKEN (STR_TIME_PROMT),
                  STRING_TOKEN (STR_TIME_HELP),
                  EFI_IFR_FLAG_READ_ONLY,
                  QF_TIME_STORAGE_TIME,
                  NULL
                  );
  Status = HiiUpdateForm (
		  HiiHandle,
                  &mFrontPageGuid,
                  FRONT_PAGE_FORM_ID,
                  StartOpCodeHandle,
                  EndOpCodeHandle
                  );
  if (EFI_ERROR(Status)) {
     DEBUG((DEBUG_ERROR, "HiiUpdateForm failed: %r\n", Status));
     return Status;
  }
  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  return Status;
}

EFI_STATUS
UpdateLanguageRegion (
  EFI_HII_HANDLE HiiHandle
  )
{
  EFI_STATUS         Status;
  VOID               *StartOpCodeHandle;
  VOID               *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL *StartGuidLabel;
  EFI_IFR_GUID_LABEL *EndGuidLabel;

  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (StartOpCodeHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (EndOpCodeHandle == NULL) {
    HiiFreeOpCodeHandle (StartOpCodeHandle);
    return EFI_OUT_OF_RESOURCES;
  }
  StartGuidLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
		  StartOpCodeHandle,
                  &gEfiIfrTianoGuid,
                  NULL,
                  sizeof (EFI_IFR_GUID_LABEL)
                  );
  StartGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartGuidLabel->Number = LABEL_LANGUAGE;
  EndGuidLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
		  EndOpCodeHandle,
                  &gEfiIfrTianoGuid,
                  NULL,
                  sizeof (EFI_IFR_GUID_LABEL)
                  );
  EndGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndGuidLabel->Number = LABEL_END;
  UiCreateLanguageMenu (HiiHandle, StartOpCodeHandle);
  Status = HiiUpdateForm (
		  HiiHandle,
		  &mFrontPageGuid,
		  SYSTEM_SETTING_ID,
		  StartOpCodeHandle,
                  EndOpCodeHandle
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to update boot region form: %r\n", Status));
  }

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return Status;
}

EFI_STATUS
UpdateBootRegion (
  EFI_HII_HANDLE HiiHandle
  )
{
  EFI_STATUS         Status;
  VOID               *StartOpCodeHandle;
  VOID               *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL *StartGuidLabel;
  EFI_IFR_GUID_LABEL *EndGuidLabel;


  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (StartOpCodeHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (EndOpCodeHandle == NULL) {
    HiiFreeOpCodeHandle (StartOpCodeHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  StartGuidLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
		  StartOpCodeHandle,
                  &gEfiIfrTianoGuid,
                  NULL,
                  sizeof (EFI_IFR_GUID_LABEL)
                  );
  StartGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartGuidLabel->Number = LABEL_MANAGER;
  EndGuidLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                  EndOpCodeHandle,
                  &gEfiIfrTianoGuid,
                  NULL,
                  sizeof (EFI_IFR_GUID_LABEL)
                  );
  EndGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndGuidLabel->Number = LABEL_END;
  UiCustomizeFrontPage (HiiHandle, StartOpCodeHandle);
  Status = HiiUpdateForm (
		  HiiHandle,
                  &mFrontPageGuid,
                  FRONT_PAGE_FORM_ID,
                  StartOpCodeHandle,
                  EndOpCodeHandle
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to update boot region form: %r\n", Status));
  }

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return Status;
}

EFI_STATUS
UpdateFrontPageForm (
  VOID
  )
{
  EFI_STATUS Status;

  Status = UpdateBootRegion (gFrontPagePrivate.HiiHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UpdateTimeRegion (gFrontPagePrivate.HiiHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UpdateLanguageRegion (gFrontPagePrivate.HiiHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the image goes into a library that calls this
  function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeUserInterface (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HII_HANDLE                   HiiHandle;
  EFI_STATUS                       Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *SimpleTextOut;
  UINTN                            BootTextColumn;
  UINTN                            BootTextRow;
  BOOLEAN                          IsServerBoard;

  ParsedData = AllocSmbiosData ();
  if (ParsedData == NULL) {
    DEBUG((DEBUG_ERROR, "%a :Failed to alloc smbios data\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  IsServerBoard = IsServerProduct();

  if (!mModeInitialized) {
    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID **)&GraphicsOutput
                    );
    if (EFI_ERROR (Status)) {
      GraphicsOutput = NULL;
    }
    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiSimpleTextOutProtocolGuid,
                    (VOID **)&SimpleTextOut
                    );
    if (EFI_ERROR (Status)) {
      SimpleTextOut = NULL;
    }

    if (GraphicsOutput != NULL) {
      mBootHorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
      mBootVerticalResolution   = GraphicsOutput->Mode->Info->VerticalResolution;
    }

    if (SimpleTextOut != NULL) {
      Status = SimpleTextOut->QueryMode (
                                SimpleTextOut,
                                SimpleTextOut->Mode->Mode,
                                &BootTextColumn,
                                &BootTextRow
                                );
      mBootTextModeColumn = (UINT32)BootTextColumn;
      mBootTextModeRow    = (UINT32)BootTextRow;
    }

    mSetupHorizontalResolution = PcdGet32 (PcdSetupVideoHorizontalResolution);
    mSetupVerticalResolution   = PcdGet32 (PcdSetupVideoVerticalResolution);
    mSetupTextModeColumn       = PcdGet32 (PcdSetupConOutColumn);
    mSetupTextModeRow          = PcdGet32 (PcdSetupConOutRow);

    mModeInitialized = TRUE;
  }

  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);
  gST->ConOut->ClearScreen (gST->ConOut);
  HiiHandle = ExportFonts ();
  ASSERT (HiiHandle != NULL);

  InitializeStringSupport ();
  InitializePasswordToggleVariable ();
  PassWordToggleData.DefaultAcpi = GetAcpiTable() ? 1 : 0;
  PassWordToggleData.IsEvb = IsServerBoard ? 0 : 1;
  Status = gRT->SetVariable (
		  EFI_PASSWORD_TOGGLE_VARIABLE_NAME,
                  &gEfiSophgoGlobalVariableGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (PASSWORD_TOGGLE_DATA),
                  &PassWordToggleData
                  );
  if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "SetVariable(%s) failed: %r\n", EFI_PASSWORD_TOGGLE_VARIABLE_NAME, Status));
      return Status;
  }
  if (PassWordToggleEntry ()) {
    FrontPagePasswordCheck ();
    PassWordToggleData.IsFirst = 1;
    Status = gRT->SetVariable (
		    EFI_PASSWORD_TOGGLE_VARIABLE_NAME,
                    &gEfiSophgoGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (PASSWORD_TOGGLE_DATA),
                    &PassWordToggleData
                    );
    if (EFI_ERROR(Status)) {
  	DEBUG((DEBUG_ERROR, "SetVariable(%s) failed: %r\n", EFI_PASSWORD_TOGGLE_VARIABLE_NAME, Status));
        return Status;
    }
  }

  UiSetConsoleMode (TRUE);
  UiEntry (FALSE);
  UiSetConsoleMode (FALSE);

  UninitializeStringSupport ();
  HiiRemovePackages (HiiHandle);

  return EFI_SUCCESS;
}

/**
  This function is the main entry of the UI entry.
  The function will present the main menu of the system UI.

  @param ConnectAllHappened Caller passes the value to UI to avoid unnecessary connect-all.

**/
VOID
EFIAPI
UiEntry (
  IN BOOLEAN  ConnectAllHappened
  )
{
  EFI_STATUS              Status;
  EFI_BOOT_LOGO_PROTOCOL  *BootLogo;
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_USER_SETUP)
    );

  if (!ConnectAllHappened) {
    EfiBootManagerConnectAll ();
  }

  EfiBootManagerRefreshAllBootOption ();
  Status = gBS->LocateProtocol (&gEfiBootLogoProtocolGuid, NULL, (VOID **)&BootLogo);
  if (!EFI_ERROR (Status) && (BootLogo != NULL)) {
    BootLogo->SetBootLogo (BootLogo, NULL, 0, 0, 0, 0);
  }

  InitializeFrontPage ();
  CallFrontPage ();
  FreeFrontPage ();
  if (mLanguageString != NULL) {
    FreePool (mLanguageString);
    mLanguageString = NULL;
  }

  SetupResetReminder ();
}

VOID
EFIAPI
EnableResetRequired (
  VOID
  )
{
  mResetRequired = TRUE;
}

BOOLEAN
EFIAPI
IsResetRequired (
  VOID
  )
{
  return mResetRequired;
}

VOID
EFIAPI
SetupResetReminder (
  VOID
  )
{
  EFI_INPUT_KEY  Key;
  CHAR16         *StringBuffer1;
  CHAR16         *StringBuffer2;

  if (IsResetRequired ()) {
    StringBuffer1 = AllocateZeroPool (MAX_STRING_LEN * sizeof (CHAR16));
    ASSERT (StringBuffer1 != NULL);
    StringBuffer2 = AllocateZeroPool (MAX_STRING_LEN * sizeof (CHAR16));
    ASSERT (StringBuffer2 != NULL);
    StrCpyS (StringBuffer1, MAX_STRING_LEN, L"Configuration changed. Reset to apply it Now.");
    StrCpyS (StringBuffer2, MAX_STRING_LEN, L"Press ENTER to reset");
    do {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, NULL);
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

    FreePool (StringBuffer1);
    FreePool (StringBuffer2);

    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  }
}
