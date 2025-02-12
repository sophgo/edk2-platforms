/** @file

  This driver implements a UEFI module for reserving memory for TPU
  through a custom HII-based interface.

  Copyright (c) 2025, SOPHGO Technologies Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "ReserveMemoryDxe.h"

EFI_GUID  mReserveMemoryGuid     = RESERVE_MEMORY_FORMSET_GUID;
EFI_GUID  gReserveMemoryVarGuid  = RESERVE_MEMORY_VAR_GUID;

STATIC RESTORE_PROTOCOL gReserveMemoryRestoreProtocol = {
  RestoreReservedMemoryDefaults
};

extern UINT8 ReserveMemoryVfrBin[];
EFI_HII_HANDLE gReserveMemoryHandle;

HII_VENDOR_DEVICE_PATH  mReserveMemoryHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    { 0x5A68F8DC, 0x3254, 0x4B00, { 0x98, 0x0A, 0xFB, 0x45, 0x6B, 0xBC, 0x9A, 0xCD } }
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

RESERVE_MEMORY_CALLBACK_DATA gReserveMemoryPrivate = {
  RESERVE_MEMORY_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  NULL,
  {
    ReserveMemoryExtractConfig,
    ReserveMemoryRouteConfig,
    ReserveMemoryCallback
  }
};

EFI_STATUS
EFIAPI
RestoreReservedMemoryDefaults (
  VOID
  )
{
  RESERVE_MEMORY_DATA            ReservedMemData;
  EFI_STATUS                     Status;
  RESERVE_MEMORY_CALLBACK_DATA   *PrivateData;

  PrivateData = NULL;

  PrivateData->ReserveMemoryData.Value = 0;
  ReservedMemData.Value = 0;

  Status = gRT->SetVariable (
		  RESERVE_MEMORY_VARIABLE,
		  &gReserveMemoryVarGuid,
		  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
		  sizeof (RESERVE_MEMORY_DATA),
		  &ReservedMemData
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

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param[in]  ReservedMemorySize  0: Disable TPU; [8, DDR_SIZE]: Enable TPU.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
           otherwise.
 */
EFI_STATUS
IsValidReservedMemorySize (
  IN UINT32  ReservedMemorySize
  )
{
  UINT32             MemoryDeviceSize;
  SMBIOS_PARSED_DATA *ParsedData;
  EFI_STATUS         Status;
  CHAR16             *ErrorString;
  EFI_INPUT_KEY      InputKey;
  UINTN              EventIndex;

  Status = EFI_SUCCESS;
  ParsedData = AllocSmbiosData ();
  if (ParsedData == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get SMBIOS data!\n",
      __func__
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  MemoryDeviceSize = ParsedData->ExtendSize; // KB

  ErrorString = HiiGetString (
		  gReserveMemoryHandle,
                  STRING_TOKEN (STR_INVALID_SIZE_ERROR),
		  NULL
		  );

  if (ReservedMemorySize == 0) {
    Print (L"Disable TPU!\n");

    goto Exit;
  } else {
    if (ReservedMemorySize < 8
        || ReservedMemorySize > (MemoryDeviceSize / 1024 / 1024)) {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        NULL,
	ErrorString,
        L"Input must be 0 or [8, DDR Size]",
	NULL
	);
      while (1) {
        gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
        gST->ConIn->ReadKeyStroke (gST->ConIn, &InputKey);
        if (InputKey.ScanCode == SCAN_NULL
	  && InputKey.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          break;
        }
      }

      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }
  }

Exit:
  if (ParsedData != NULL) {
    FreeSmbiosData (ParsedData);
  }

  return Status;
}

/**
  Get reserve memory from variable.

  @param[in]    PrivateData    Point to RESERVE_MEMORY_CALLBACK_DATA.

  @return       EFI_SUCCESS    Get reserve memory from variable successfully.
  @return       other          Get reserve memory from variable failed.
**/
EFI_STATUS
UpdateReserveMemoryConfig (
  IN RESERVE_MEMORY_CALLBACK_DATA           *PrivateData
  )
{
  EFI_STATUS            Status;
  RESERVE_MEMORY_DATA   ReservedMemData;
  UINTN                 VarSize;

  VarSize = sizeof (RESERVE_MEMORY_DATA);
  Status = gRT->GetVariable (
		  RESERVE_MEMORY_VARIABLE,
		  &gReserveMemoryVarGuid,
                  NULL,
                  &VarSize,
		  &ReservedMemData
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Get variable failed!\n",
      __func__
      ));
    return Status;
  }

  CopyMem (&PrivateData->ReserveMemoryData, &ReservedMemData, VarSize);

  return EFI_SUCCESS;
}

/**
  Get reserve memory from the form and set the configuration to the variable.

  @param[in]    This           Point to EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]    PrivateData    Point to RESERVE_MEMORY_CALLBACK_DATA.

  @return       EFI_SUCCESS    Set reserve memory in the form successfully.
  @return       other          Set reserve memory in the form failed.
**/
EFI_STATUS
ReserveMemorySetupConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN RESERVE_MEMORY_CALLBACK_DATA           *PrivateData
  )
{
  EFI_STATUS           Status;
  RESERVE_MEMORY_DATA  ReservedMemData;

  if (!HiiGetBrowserData (
	&mReserveMemoryGuid,
	RESERVE_MEMORY_VARIABLE,
	sizeof (RESERVE_MEMORY_DATA),
	(UINT8 *) &ReservedMemData))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: SetVariable failed: not match\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  CopyMem (&PrivateData->ReserveMemoryData, &ReservedMemData,
		  sizeof (RESERVE_MEMORY_DATA));
  Status = gRT->SetVariable (
		  RESERVE_MEMORY_VARIABLE,
		  &gReserveMemoryVarGuid,
		  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
		  sizeof (RESERVE_MEMORY_DATA),
		  &ReservedMemData
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

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param[in]  This       Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Request    A null-terminated Unicode string in <ConfigRequest> format.
  @param[out] Progress   On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param[out]  Results   A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
ReserveMemoryExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
)
{
  EFI_STATUS                     Status;
  RESERVE_MEMORY_CALLBACK_DATA   *Private;
  EFI_STRING                     ConfigRequestHdr;
  EFI_STRING                     ConfigRequest;
  BOOLEAN                        AllocatedRequest;
  UINTN                          Size;
  UINTN                          BufferSize;

  Status = EFI_SUCCESS;

  BufferSize = sizeof (RESERVE_MEMORY_DATA);
  if (Progress == NULL || Results == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): invalid parameters\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  Private = RESERVE_MEMORY_CALLBACK_DATA_FROM_THIS (This);

  //
  // Update reserve memory
  //
  Status = UpdateReserveMemoryConfig (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &mReserveMemoryGuid, RESERVE_MEMORY_VARIABLE)) {
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

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (
		    &mReserveMemoryGuid,
		    RESERVE_MEMORY_VARIABLE,
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
                                (UINT8 *) &Private->ReserveMemoryData,
                                sizeof (RESERVE_MEMORY_DATA),
                                Results,
                                Progress
                                );

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }

  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration  A null-terminated Unicode string in <ConfigResp> format.
  @param[out] Progress       A pointer to a string filled in with the offset of the most
                             recent '&' before the first failing name/value pair (or the
                             beginning of the string if the failure is in the first
                             name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.
**/
EFI_STATUS
EFIAPI
ReserveMemoryRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
) {
  EFI_STATUS                    Status;
  UINTN                         BufferSize;
  RESERVE_MEMORY_CALLBACK_DATA  *Private;

  if (This == NULL || Configuration == NULL || Progress == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): invalid parameters\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  Private = RESERVE_MEMORY_CALLBACK_DATA_FROM_THIS (This);
  *Progress = Configuration;

  //
  // Check if the Configuration matches
  //
  if (!HiiIsConfigHdrMatch (
          Configuration,
	  &mReserveMemoryGuid,
	  RESERVE_MEMORY_VARIABLE
	  ))
  {
    DEBUG ((
      DEBUG_ERROR,
      "Configuration header does not match.\n"
      ));
    return EFI_NOT_FOUND;
  }

  //
  // Sync Browser Data to Private Data
  //
  Status = ReserveMemorySetupConfig (This, Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BufferSize = sizeof (RESERVE_MEMORY_DATA);
  Status = Private->HiiConfigRouting->ConfigToBlock (
		  Private->HiiConfigRouting,
		  Configuration,
		  (UINT8 *)&Private->ReserveMemoryData,
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

/**
  This function processes the results of changes in configuration.


  @param[in]  This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action          Specifies the type of action taken by the browser.
  @param[in]  QuestionId      A unique value which is sent to the original exporting driver
                              so that it can identify the type of data to expect.
  @param[in]  Type            The type of value for the question.
  @param[in]  Value           A pointer to the data being sent to the original exporting driver.
  @param[out] ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.
**/

EFI_STATUS
EFIAPI
ReserveMemoryCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_STATUS                     Status;
  RESERVE_MEMORY_CALLBACK_DATA   *Private;
  UINTN                          VarSize;
  RESERVE_MEMORY_DATA            InputSize;

  Private = RESERVE_MEMORY_CALLBACK_DATA_FROM_THIS (This);

  Status = EFI_SUCCESS;
  VarSize = sizeof (RESERVE_MEMORY_DATA);

  Status = gRT->GetVariable (
		  RESERVE_MEMORY_VARIABLE,
		  &gReserveMemoryVarGuid,
                  NULL,
                  &VarSize,
                  &InputSize
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

  if (QuestionId == RESERVED_MEM_QUESTION_ID &&
		  Action == EFI_BROWSER_ACTION_CHANGING) {
    InputSize.Value = (Value->u32);
    Status = IsValidReservedMemorySize (Value->u32);
    if (EFI_ERROR (Status)) {
      Print (L"Invalid!!!!\n");
      return Status;
    }

    //
    // Set variable
    //
    Status = gRT->SetVariable (
		    RESERVE_MEMORY_VARIABLE,
		    &gReserveMemoryVarGuid,
		    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
		    VarSize,
		    &InputSize
		    );

    if (EFI_ERROR (Status)) {
      Print (L"Failed to set ReservedMemorySize variable: %r\n", Status);
      return Status;
    }

    HiiSetBrowserData (
		  &mReserveMemoryGuid,
    		  RESERVE_MEMORY_VARIABLE,
		  VarSize,
    		  (UINT8*)&InputSize,
    		  NULL
		  );
  }

  return Status;
}

/**
  Reserved Memory initialization.
  Get the current reserved memory configuration frrom variable.
  If the variable does not exist, restore the default configuration
  and save the variable.

  @return       EFI_SUCCESS    Initialize reserved memory in the form successfully.
  @return       other          Initialize reserved memory in the form failed.
**/
EFI_STATUS
ReservedMemoryConfigInit (
  VOID
  )
{
  EFI_STATUS            Status;
  RESERVE_MEMORY_DATA   ReservedMemData;
  UINTN                 VarSize;

  VarSize = sizeof (RESERVE_MEMORY_DATA);
  Status = gRT->GetVariable (
		  RESERVE_MEMORY_VARIABLE,
		  &gReserveMemoryVarGuid,
                  NULL,
                  &VarSize,
                  &ReservedMemData
                  );
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: get variable: %r\n",
      __func__,
      Status
      ));
    //
    // Config default value
    //
    ReservedMemData.Value = 0;
    Status = gRT->SetVariable (
		    RESERVE_MEMORY_VARIABLE,
		    &gReserveMemoryVarGuid,
		    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VarSize,
                    &ReservedMemData
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

/**
  The entry point for Reserve Memory driver.

  @param[in] ImageHandle     Image handle this driver.
  @param[in] SystemTable     Pointer to SystemTable.

  @retval EFI_SUCCESS    This function always complete successfully.

**/
EFI_STATUS
EFIAPI
ReserveMemoryDriverEntryPoint (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                      Status;
  RESERVE_MEMORY_CALLBACK_DATA    *PrivateData;

  //
  // Create a private data structure.
  //
  PrivateData = AllocateCopyPool (sizeof (RESERVE_MEMORY_CALLBACK_DATA), &gReserveMemoryPrivate);
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (
		  &gEfiHiiConfigRoutingProtocolGuid,
		  NULL,
		  (VOID **) &PrivateData->HiiConfigRouting
		  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  PrivateData->DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces(
                  &PrivateData->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mReserveMemoryHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &PrivateData->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallProtocolInterface (
                  &PrivateData->DriverHandle,
                  &gReserveMemoryRestoreProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)&gReserveMemoryRestoreProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  PrivateData->HiiHandle = HiiAddPackages (
                   &mReserveMemoryGuid,
                   PrivateData->DriverHandle,
                   ReserveMemoryVfrBin,
                   ReserveMemoryDxeStrings,
                   NULL
                   );
  if (PrivateData->HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  gReserveMemoryHandle = PrivateData->HiiHandle;

  Status = ReservedMemoryConfigInit ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Unload the Reserve Memory configuration form.

  @param[in]  ImageHandle         The driver's image handle.

  @retval     EFI_SUCCESS         The SecureBoot configuration form is unloaded.
  @retval     Others              Failed to unload the form.

**/
EFI_STATUS
EFIAPI
ReserveMemoryDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                       Status;
  RESERVE_MEMORY_CALLBACK_DATA     *PrivateData;

  Status = gBS->HandleProtocol (
		  ImageHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &PrivateData
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (PrivateData->Signature == RESERVE_MEMORY_CALLBACK_DATA_SIGNATURE);

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
                  &mReserveMemoryHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &PrivateData->ConfigAccess,
                  NULL
                  );

  ASSERT_EFI_ERROR (Status);

  HiiRemovePackages (PrivateData->HiiHandle);

  return EFI_SUCCESS;
}
