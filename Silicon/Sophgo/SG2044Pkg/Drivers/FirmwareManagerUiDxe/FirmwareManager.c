/** @file
  The functions for firmware manager menu.

  Copyright (c) 2024, SOPHGO Technology Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/CustomizedDisplayLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/ResetSystemLib.h>
#include <Protocol/GraphicsOutput.h>

#include "FirmwareManager.h"
#include "FirmwareManagerFormGuid.h"

#define STRING_BUFFER_SIZE 64

EFI_GUID mFirmwareManagerGuid         = FIRMWARE_MANAGER_FORMSET_GUID;
STATIC   SPI_NOR                        *Nor;
STATIC   SOPHGO_NOR_FLASH_PROTOCOL      *NorFlashProtocol;
STATIC   SOPHGO_SPI_MASTER_PROTOCOL     *SpiMasterProtocol;

extern UINT8 FirmwareManagerVfrBin[];

EFI_HII_HANDLE gFirmwareUpdateHandle;

HII_VENDOR_DEVICE_PATH  mFirmwareManagerHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    { 0xF0EA4C6C, 0x47C5, 0x4AED, { 0xB3, 0xA2, 0x4E, 0xE4, 0x97, 0x05, 0x9B, 0x6C } }
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

FIRMWARE_MANAGER_CALLBACK_DATA gFirmwareManagerPrivate = {
  FIRMWARE_MANAGER_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    FirmwareManagerExtractConfig,
    FirmwareManagerRouteConfig,
    FirmwareManagerCallback
  }
};

/**
  Display an information popup.

  @param[in]  StringToken1    The first string token.
  @param[in]  StringToken2    The second string token.

  @retval  TRUE
**/
BOOLEAN
PopupInformation (
  IN CHAR16       *Str1,
  IN CHAR16       *Str2
  )
{
  EFI_INPUT_KEY  InputKey;
  UINTN          EventIndex;

  CreatePopUp (
    EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
    NULL,
    Str1,
    Str2,
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

  return TRUE;
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
FirmwareManagerExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                     Status;
  FIRMWARE_MANAGER_CALLBACK_DATA *Private;
  EFI_STRING                     ConfigRequestHdr;
  EFI_STRING                     ConfigRequest;
  BOOLEAN                        AllocatedRequest;
  UINTN                          Size;
  UINTN                          BufferSize;

  Status = EFI_SUCCESS;

  BufferSize = sizeof (FIRMWARE_MANAGER_DATA);
  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = FIRMWARE_MANAGER_CALLBACK_DATA_FROM_THIS (This);
  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &mFirmwareManagerGuid, FIRMWARE_MANAGER_VARIABLE)) {
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
		    &mFirmwareManagerGuid,
		    FIRMWARE_MANAGER_VARIABLE,
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
                                (UINT8 *) &Private->FirmwareManagerData,
                                sizeof (FIRMWARE_MANAGER_DATA),
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
FirmwareManagerRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            Buffsize;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *ConfigRouting;
  FIRMWARE_MANAGER_CALLBACK_DATA   *Private;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = FIRMWARE_MANAGER_CALLBACK_DATA_FROM_THIS (This);
  *Progress = Configuration;
  Status = gBS->LocateProtocol (
		  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **)&ConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!HiiIsConfigHdrMatch (
		Configuration,
		&mFirmwareManagerGuid,
		FIRMWARE_MANAGER_VARIABLE
		))
  {
    return EFI_NOT_FOUND;
  }

  Buffsize = sizeof (FIRMWARE_MANAGER_DATA);
  Status = ConfigRouting->ConfigToBlock (
                            ConfigRouting,
                            Configuration,
                            (UINT8 *) &Private->FirmwareManagerData,
                            &Buffsize,
                            Progress
                            );

  return Status;
}

/**
  Update firmware in spi nor flash.

  @param[in]  Nor                 Structure of nor flash.
  @param[in]  Address             Address to update.
  @param[in]  Buffer              A pointer to update firmware data.
  @param[in]  Size                Size of update firmware to update.
  @param[in]  String              Start info to print.
  @param[in]  PromptSkipVariable  Whether to prompt user to skip variable range.

  @retval     EFI_SUCCESS         Success.
              Other               Failed.
**/
EFI_STATUS
UpdateFirmware (
  IN SPI_NOR      *Nor,
  IN UINTN        Address,
  IN UINT8        *Buffer,
  IN UINTN        Size,
  IN CHAR16       *String,
  IN BOOLEAN      PromptSkipVariable
  )
{
  EFI_STATUS     Status;
  UINTN          Index;
  UINTN          Count;
  VOID           *TempBuffer;
  CHAR16         Space[]  = L"                 ";
  UINTN          Columns;
  UINTN          Rows;
  UINTN          StringLen;
  CHAR16         *WarningString;
  CHAR16         *PromptString;
  UINTN          VariableBase;
  UINT32         VariableSize;
  UINTN          BlockSize;
  BOOLEAN        SkipVariable;
  EFI_INPUT_KEY  Key;

  SkipVariable = FALSE;
  VariableBase = PcdGet64 (PcdFlashVariableOffset);
  VariableSize = PcdGet32 (PcdFlashNvStorageVariableSize);

  BlockSize = Nor->Info->SectorSize;
  TempBuffer = NULL;
  TempBuffer = AllocatePool (BlockSize);
  Status = EFI_SUCCESS;
  Count  = (Size / BlockSize);
  StringLen = StrLen (Space);

  WarningString = HiiGetString (
		  gFirmwareUpdateHandle,
		  STRING_TOKEN (STR_UPDATING_WARNING),
		  NULL
		  );
  if (PromptSkipVariable) {
    PromptString = HiiGetString (
		  gFirmwareUpdateHandle,
		  STRING_TOKEN (STR_UPDATE_PROMPT_MESSAGE),
		  NULL
		  );
    CreatePopUp (
	  EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          NULL,
          PromptString,
          L"[Y] Yes    [N] No",
          NULL);

    while (1) {
      gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, NULL);
      gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (Key.UnicodeChar == 'Y' || Key.UnicodeChar == 'y') {
        SkipVariable = TRUE;
	break;
      } else if (Key.UnicodeChar == 'N' || Key.UnicodeChar == 'n') {
        SkipVariable = FALSE;
	break;
      }
    }
  }

  CreatePopUp (
    EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
    NULL,
    WarningString,
    Space,
    NULL
    );

  gST->ConOut->SetAttribute (gST->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE);
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &Columns, &Rows);
  Columns = (Columns - StringLen) / 2;
  Rows = (Rows - (1 + 3)) / 2 + 3;
  gST->ConOut->SetCursorPosition (gST->ConOut, Columns, Rows);

  //
  // make sure Address & Size is BlockSize align
  //
  if (((Address % BlockSize) != 0)
	|| ((Size % BlockSize) != 0) ) {
    Count ++;
    Address = Address & (~ (BlockSize - 1));
  }

  for (Index = 0; Index < Count; Index ++) {
    //
    // Skip the Variable range
    //
    if (SkipVariable) {
      if (((VariableBase / BlockSize) <= Index)
		    && (Index < ((VariableBase + VariableSize) / BlockSize))) {
        continue;
      }
    }

    if (TempBuffer) {
      NorFlashProtocol->ReadData (
		      Nor,
		      Address + Index * BlockSize,
		      BlockSize,
		      TempBuffer
		      );
      if (CompareMem (TempBuffer, Buffer + Index * BlockSize, BlockSize) == 0) {
        gST->ConOut->SetCursorPosition (gST->ConOut, Columns, Rows);
        Print (L"%s %02d%%%", String, ((Index + 1) * 100) / Count);
        continue;
      }
    }

    Status = NorFlashProtocol->Erase (
		    Nor,
		    Address + Index * BlockSize,
		    BlockSize
		    );
    if (EFI_ERROR (Status)) {
      Print (L"\r%s Fail!\n", String);
      goto ProExit;
    }

    Status = NorFlashProtocol->WriteData (
		    Nor,
		    Address + Index * BlockSize,
		    BlockSize,
		    Buffer + Index * BlockSize
		    );
    if (EFI_ERROR (Status)) {
      Print (L"\r%s Fail!\n", String);
      goto ProExit;
    }

    gST->ConOut->SetCursorPosition (gST->ConOut, Columns, Rows);

    Print (L"%s %02d%%%", String, ((Index + 1) * 100) / Count);
  }

ProExit:
  if (Nor) {
    SpiMasterProtocol->FreeDevice (Nor);
  }

  if (TempBuffer != NULL) {
    FreePool (TempBuffer);
  }

  return Status;
}

/**
  Retrieve and format a string from HII.

  @param[in]      StringToken  The token for the string to retrieve.
  @param[in/out]  OutputStr    The buffer to store the formatted string.

  @retval  EFI_SUCCESS           The string was retrieved and formatted successfully.
  @retval  EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
EFI_STATUS
RetrieveAndFormatString (
  IN     UINT16   StringToken,
  IN OUT CHAR16   *OutputStr
  )
{
  CHAR16      *HiiFormatString;
  EFI_STATUS  Status;

  HiiFormatString = HiiGetString(
		  gFirmwareUpdateHandle,
		  StringToken,
		  NULL
		  );
  if (HiiFormatString == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = UnicodeSPrint (
		  OutputStr,
		  STRING_BUFFER_SIZE * sizeof(CHAR16),
		  L"%s",
		  HiiFormatString
		  );
  FreePool (HiiFormatString);
  return Status;
}

/**
  Clear the Display card buffer

  @param  NULL

  @retval  NULL

**/
VOID ClearScreen (
  VOID
  )
{

  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background = { 0x00, 0x00, 0x00, 0x00 };
  EFI_STATUS                    Status;

  //
  // Get current video resolution and text mode
  //
  Status = gBS->HandleProtocol (
		  gST->ConsoleOutHandle,
		  &gEfiGraphicsOutputProtocolGuid,
                  (VOID**)&GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;
    return;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "HorizontalResolution %d,VerticalResolution, %d \n",
    GraphicsOutput->Mode->Info->HorizontalResolution,
    GraphicsOutput->Mode->Info->VerticalResolution
    ));

  Status = GraphicsOutput->Blt (
		  GraphicsOutput,
                  &Background,
                  EfiBltVideoFill,
                  0,
                  0,
                  0,
                  0,
                  GraphicsOutput->Mode->Info->HorizontalResolution,
                  GraphicsOutput->Mode->Info->VerticalResolution,
                  0);
}

/**
  Press Enter to reboot.
**/
VOID
PressKeytoReset (
  EFI_STRING_ID    TokenToUpdate
  )
{
  CHAR16           Str1[64];
  CHAR16           *UpdateSuccString;

  UpdateSuccString = HiiGetString (gFirmwareUpdateHandle, TokenToUpdate, NULL);

  PopupInformation (Str1, UpdateSuccString);

  DEBUG ((DEBUG_VERBOSE, "The ENTER key is pressed, next clear screen and reboot!\n"));

  ClearScreen();

  ResetCold ();
}

/**
  Probe and initialize nor flash.

  @retval  EFI_SUCCESS       The Nor Flash probe successfully.
  @retval  EFI_NOT_FOUND     Nor flash does not exist.
  @retval  EFI_DEVICE_ERROR  Nor flash initialized failed.
**/
STATIC
EFI_STATUS
NorFlashProbe (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Read Nor Flash ID
  //
  Status = NorFlashProtocol->GetFlashid (Nor, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Read Nor flash ID failed!\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  //
  // Initialize Nor Flash
  //
  Status = NorFlashProtocol->Init (NorFlashProtocol, Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Cannot initialize flash device\n",
      __func__
      ));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Update file.

  @param[in]  FilePath        A pointer to update firmware data file path.
  @param[in]  QuestionId      A unique value which is sent to the original exporting driver
                              so that it can identify the type of data to expect.

  @retval  FALSE       Failed.
**/
BOOLEAN
UpdateFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL    *FilePath,
  IN EFI_QUESTION_ID             QuestionId
)
{
  VOID             *FileBuffer;
  UINTN            FileSize;
  UINT32           AuthStat;
  EFI_STATUS       Status;
  UINT8            *FirmwareAddress;
  UINT8            SelectedFlashNumber;
  CHAR16           *UpdatingFirmwareString;
  BOOLEAN          PromptSkipVariable;
  EFI_STRING_ID    TokenToUpdate1;
  EFI_STRING_ID    TokenToUpdate2;

  //
  // Locate SPI Master protocol
  //
  Status = gBS->LocateProtocol (
                  &gSophgoSpiMasterProtocolGuid,
                  NULL,
                  (VOID *)&SpiMasterProtocol
                  );
  if (EFI_ERROR (Status)) {
    Print (L"  Cannot locate SPI Master protocol!\n");
    return FALSE;
  }

  //
  // Locate Nor Flash protocol
  //
  Status = gBS->LocateProtocol (
                  &gSophgoNorFlashProtocolGuid,
                  NULL,
                  (VOID *)&NorFlashProtocol
                  );
  if (EFI_ERROR (Status)) {
    Print (L"  Cannot locate Nor Flash protocol!\n");
    return FALSE;
  }

  if (QuestionId == UPDATE_FIRMWARE_KEY) {
    SelectedFlashNumber = 0;
    TokenToUpdate1 = STRING_TOKEN (STR_UPDATING_FIRMWARE);
    TokenToUpdate2 = STRING_TOKEN (STR_FIRMWARE_UPDATE_SUCC);
    PromptSkipVariable = TRUE;
  } else if (QuestionId == UPDATE_INI_KEY) {
    SelectedFlashNumber = 1;
    TokenToUpdate1 = STRING_TOKEN (STR_UPDATING_INI);
    TokenToUpdate2 = STRING_TOKEN (STR_INI_UPDATE_SUCC);
    PromptSkipVariable = FALSE;
  }

  //
  // Setup and probe Nor flash
  //
  Nor = SpiMasterProtocol->SetupDevice (
                  SpiMasterProtocol,
                  Nor,
		  SelectedFlashNumber
                  );

  if (Nor == NULL) {
    Print (L"  Nor Flash %d not found!\n", SelectedFlashNumber);
    return FALSE;
  }

  Status = NorFlashProbe ();
  if (EFI_ERROR (Status)) {
    Print (L"  Error while performing Nor flash probe [Status=%r]\n", Status);
    return FALSE;
  }

  FileBuffer = GetFileBufferByFilePath (FALSE, FilePath, &FileSize, &AuthStat);

  if (!EFI_ERROR (Status)) {
    FirmwareAddress = FileBuffer;
    UpdatingFirmwareString = HiiGetString (
		    gFirmwareUpdateHandle,
		    TokenToUpdate1,
		    NULL
		    );
    Status = UpdateFirmware (
		    Nor,
		    0,
		    FirmwareAddress,
		    FileSize,
		    UpdatingFirmwareString,
		    PromptSkipVariable
		    );

    PressKeytoReset (TokenToUpdate2);
  }

  FreePool (FileBuffer);

  return FALSE;
}

/**
  Update the firmware.bin base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdateFirmwareFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return UpdateFromFile (FilePath, UPDATE_FIRMWARE_KEY);
}

/**
  Update the conf.ini base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdateIniFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return UpdateFromFile (FilePath, UPDATE_INI_KEY);
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
FirmwareManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *File;
  EFI_STATUS                Status;
  File = NULL;

  Status = EFI_SUCCESS;
  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    if (QuestionId == UPDATE_FIRMWARE_KEY) {
      Status = ChooseFile (NULL, NULL, UpdateFirmwareFromFile, &File);
    }

    if (QuestionId == UPDATE_INI_KEY) {
      Status = ChooseFile (NULL, NULL, UpdateIniFromFile, &File);
    }
  }

  return Status;
}

/**
  Install Firmware manager config Menu driver.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval  EFI_SUCEESS     Install Boot manager menu success.
  @retval  Other           Return error status.
**/
EFI_STATUS
EFIAPI
InstallFirmwareManagerForm (
  IN OUT FIRMWARE_MANAGER_CALLBACK_DATA   *PrivateData
  )
{
  EFI_STATUS Status;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  PrivateData->DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
		  &PrivateData->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mFirmwareManagerHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &PrivateData->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  PrivateData->HiiHandle = HiiAddPackages (
		  &mFirmwareManagerGuid,
		  PrivateData->DriverHandle,
		  FirmwareManagerVfrBin,
		  FirmwareManagerUiDxeStrings,
		  NULL
		  );
  ASSERT (PrivateData->HiiHandle != NULL);

  gFirmwareUpdateHandle = PrivateData->HiiHandle;

  return EFI_SUCCESS;
}

/**
  Unloads the form and its installed protocol.

  @param[in]  ImageHandle      Handle that identifies the image to be unloaded.
  @param[in]  SystemTable      The system table.

  @retval  EFI_SUCCESS      The image has been unloaded.
**/
EFI_STATUS
EFIAPI
UninstallFirmwareManagerForm (
  IN OUT FIRMWARE_MANAGER_CALLBACK_DATA   *PrivateData
  )
{
  EFI_STATUS Status;

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  PrivateData->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mFirmwareManagerHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &PrivateData->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  HiiRemovePackages (PrivateData->HiiHandle);

  return EFI_SUCCESS;
}
