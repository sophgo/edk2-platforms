/** @file
  FrontPage routines to handle the callbacks and browser calls

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FrontPage.h"
#include "FrontPageCustomizedUi.h"

#define MAX_STRING_LEN  500
UINTN EntryCount = 0;                  
INI_ENTRY gIniEntries[MAX_ENTRIES];    
#define DEVICE_PATH_0_GUID { 0x8e6d99ee, 0x7531, 0x48f8, { 0x87, 0x45, 0x7f, 0x61, 0x44, 0x46, 0x8f, 0xf2} }
#define DEVICE_PATH_1_GUID { 0x398b9173, 0xf5ed, 0x499a, { 0xa0, 0xbc, 0x8b, 0x51, 0x98, 0x7b, 0x55, 0xf9} }

EFI_GUID  mFrontPageGuid = FRONT_PAGE_FORMSET_GUID;
EFI_GUID  mConfiginiGuid = CONFIG_INI_FORMSET_GUID;

BOOLEAN  mResetRequired = FALSE;

EFI_FORM_BROWSER2_PROTOCOL  *gFormBrowser2;
CHAR8                       *mLanguageString;
BOOLEAN                     mModeInitialized = FALSE;
UINT32  mBootHorizontalResolution = 0;
UINT32  mBootVerticalResolution   = 0;
UINT32  mBootTextModeColumn       = 0;
UINT32  mBootTextModeRow          = 0;
UINT32  mSetupTextModeColumn       = 0;
UINT32  mSetupTextModeRow          = 0;
UINT32  mSetupHorizontalResolution = 0;
UINT32  mSetupVerticalResolution   = 0;

FRONT_PAGE_CALLBACK_DATA  gFrontPagePrivate = {
  FRONT_PAGE_CALLBACK_DATA_SIGNATURE,
  {NULL,NULL},
  {NULL,NULL},
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    FrontPageCallback
  }
};

HII_VENDOR_DEVICE_PATH  mFrontPageHiiVendorDevicePath0 = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    DEVICE_PATH_0_GUID
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

HII_VENDOR_DEVICE_PATH  mFrontPageHiiVendorDevicePath1 = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
   DEVICE_PATH_1_GUID
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

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  return EFI_NOT_FOUND;
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
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
	return UiFrontPageCallbackHandler(gFrontPagePrivate.HiiHandle[0], Action, QuestionId, Type, Value, ActionRequest);
}

VOID
UpdateFrontPageForm (
  VOID
  )
{
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartGuidLabel;
  EFI_IFR_GUID_LABEL  *EndGuidLabel;

  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);
  StartGuidLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartGuidLabel->Number       = LABEL_FRONTPAGE_INFORMATION;
  EndGuidLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndGuidLabel->Number       = LABEL_END;
  UiCustomizeFrontPage (
    gFrontPagePrivate.HiiHandle[0],
    StartOpCodeHandle
    );

  HiiUpdateForm (
    gFrontPagePrivate.HiiHandle[0],
    &mFrontPageGuid,
    FRONT_PAGE_FORM_ID,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
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

  gFrontPagePrivate.DriverHandle[0] = NULL;
  gFrontPagePrivate.DriverHandle[1] = NULL;
  Status      = gBS->InstallMultipleProtocolInterfaces (
                                          &gFrontPagePrivate.DriverHandle[0],
                                          &gEfiDevicePathProtocolGuid,
                                          &mFrontPageHiiVendorDevicePath0,
                                          &gEfiHiiConfigAccessProtocolGuid,
                                          &gFrontPagePrivate.ConfigAccess,
                                          NULL
                                          );
  ASSERT_EFI_ERROR(Status);

  Status      = gBS->InstallMultipleProtocolInterfaces (
                                          &gFrontPagePrivate.DriverHandle[1],
                                          &gEfiDevicePathProtocolGuid,
                                          &mFrontPageHiiVendorDevicePath1,
                                          &gEfiHiiConfigAccessProtocolGuid,
                                          &gFrontPagePrivate.ConfigAccess,
                                          NULL
                                          );
  ASSERT_EFI_ERROR(Status);
  gFrontPagePrivate.HiiHandle[0] = HiiAddPackages (
                                  &mFrontPageGuid,
                                  gFrontPagePrivate.DriverHandle[0],
                                  FrontPageVfrBin,
                                  UiAppStrings,
                                  NULL
                                  );

  ASSERT (gFrontPagePrivate.HiiHandle[0] != NULL);
  gFrontPagePrivate.HiiHandle[1] = HiiAddPackages (
                                  &mConfiginiGuid,
                                  gFrontPagePrivate.DriverHandle[1],
                                  ConfiginiVfrBin,
                                  UiAppStrings,
                                  NULL
                                  );
  
  ASSERT (gFrontPagePrivate.HiiHandle[1] != NULL);
  UpdateFrontPageForm ();
  UpdateHiiData ();
  return Status;
}

EFI_STATUS
CallFrontPage (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;
  EFI_HII_HANDLE 	      HiiHandles[2];
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_INPUT_WAIT)
    );
  
  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  
  HiiHandles[0] = gFrontPagePrivate.HiiHandle[0];
  HiiHandles[1] = gFrontPagePrivate.HiiHandle[1];
  Status        = gFormBrowser2->SendForm (
                                   gFormBrowser2,
				   HiiHandles,
                                   2,
                                   &mFrontPageGuid,
                                   0,
                                   NULL,
                                   &ActionRequest);
   

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
                  gFrontPagePrivate.DriverHandle[0],
                  &gEfiDevicePathProtocolGuid,
                  &mFrontPageHiiVendorDevicePath0,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gFrontPagePrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  gFrontPagePrivate.DriverHandle[1],
                  &gEfiDevicePathProtocolGuid,
                  &mFrontPageHiiVendorDevicePath1,
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

INT32
IniHandler (
    VOID       *User,
    CONST CHAR8 *Section,
    CONST CHAR8 *Name,
    CONST CHAR8 *Value
 )
{
    if (!AsciiStrCmp(Section, "eof"))
      return 0;

    if (EntryCount >= MAX_ENTRIES)
      return 0;

    AsciiStrCpyS(gIniEntries[EntryCount].Section, sizeof(gIniEntries[EntryCount].Section), Section);
    AsciiStrCpyS(gIniEntries[EntryCount].Name, sizeof(gIniEntries[EntryCount].Name), Name);
    AsciiStrCpyS(gIniEntries[EntryCount].Value, sizeof(gIniEntries[EntryCount].Value), Value);
    EntryCount++;

    return 1;
}

INT32
IniConfIniParse (
    IN INI_HANDLER   Handler,
    IN VOID          *User
 )
{
    INT32 result = -1;

    if (IsIniFileExist ())
      result = IniParseString(MemoryData, Handler, User);

    return result;
}

EFI_STATUS
EFIAPI
UpdateHiiData (
  VOID
  )
{
    VOID *StartOpCodeHandle;
    VOID *EndOpCodeHandle;
    EFI_STRING_ID StringId;
    EFI_STATUS Status;
    EFI_STRING_ID       HelpToken;
    UINTN               Index;
    EFI_IFR_GUID_LABEL  *StartLabel;
    EFI_IFR_GUID_LABEL  *EndLabel;

    CHAR16 DisplayBuffer[256];
    ZeroMem(DisplayBuffer, sizeof(DisplayBuffer));

    StartOpCodeHandle = HiiAllocateOpCodeHandle();
    ASSERT(StartOpCodeHandle != NULL);

    EndOpCodeHandle = HiiAllocateOpCodeHandle();
    ASSERT(EndOpCodeHandle != NULL);
    
    gST->ConOut->ClearScreen(gST->ConOut);
    StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(
        StartOpCodeHandle,
        &gEfiIfrTianoGuid,
        NULL,
        sizeof(EFI_IFR_GUID_LABEL)
    );
    StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
    StartLabel->Number = LABEL_UPDATE_START;

    EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(
        EndOpCodeHandle,
        &gEfiIfrTianoGuid,
        NULL,
        sizeof(EFI_IFR_GUID_LABEL)
    );
    EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
    EndLabel->Number = LABEL_UPDATE_END;

    for (Index = 0; Index < EntryCount; Index++) {
      INI_ENTRY *Entry = &gIniEntries[Index];

      if (Index == 0 || AsciiStrCmp(gIniEntries[Index - 1].Section, Entry->Section) != 0) {
        UnicodeSPrint(DisplayBuffer, sizeof(DisplayBuffer), L"Section: %a\n", Entry->Section);
        StringId = HiiSetString(gFrontPagePrivate.HiiHandle[1], 0, DisplayBuffer, NULL);
        HelpToken = 0; 
        HiiCreateTextOpCode(StartOpCodeHandle, StringId, HelpToken, 0);
      }

      UnicodeSPrint(DisplayBuffer, sizeof(DisplayBuffer), L"%a: %a\n", Entry->Name, Entry->Value);
      StringId = HiiSetString(gFrontPagePrivate.HiiHandle[1], 0, DisplayBuffer, NULL); 
      HelpToken = 0;
      HiiCreateTextOpCode(StartOpCodeHandle, StringId, HelpToken, 0);
    }

    Status = HiiUpdateForm(
        gFrontPagePrivate.HiiHandle[1],
        &mConfiginiGuid,
        NEW_FORM_ID,
	  StartOpCodeHandle,
        EndOpCodeHandle
    );

    HiiFreeOpCodeHandle(StartOpCodeHandle);
    HiiFreeOpCodeHandle(EndOpCodeHandle);

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

  IniConfIniParse(IniHandler, NULL);
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
