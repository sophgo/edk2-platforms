/** @file Update Fimware

 Copyright (C) 2024, SOPHGO Technologies Inc. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Guid/FileInfo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/ResetSystemLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>

#include <Include/Spifmc.h>
#include <Include/SpiNorFlash.h>

#define STRING_BUFFER_SIZE 64

CHAR16 *FirmwareNames[1] = {L"FIRMWARE.BIN"};

STATIC SPI_NOR                        *Nor;
STATIC SOPHGO_NOR_FLASH_PROTOCOL      *NorFlashProtocol;
STATIC SOPHGO_SPI_MASTER_PROTOCOL     *SpiMasterProtocol;
EFI_HII_HANDLE                        gFirmwareUpdateHandle;

/**
  Show copyrights and warning.

  @retval    Null.
**/
EFI_STATUS
ShowCopyRightsAndWarning (
  VOID
  )
{
  UINTN         Columns;
  UINTN         Rows;
  UINTN         StringLen;
  UINTN         Index;
  CONST CHAR16  *String[5] = {
    L"************************************************************\n",
    L"*                  SOPHGO Firmware Update                  *\n",
    L"*        Copyright(C) 2024, SOPHGO Technologies Inc.       *\n",
    L"*                  All rights reserved                     *\n",
    L"************************************************************\n"
  };

  gST->ConOut->QueryMode (
	gST->ConOut,
	gST->ConOut->Mode->Mode,
	&Columns,
	&Rows
	);
  StringLen = StrLen (String[0]);
  Columns = (Columns - StringLen) / 2;
  Rows = 0;

  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  for (Index = 0; Index < 5; Index ++) {
    gST->ConOut->SetCursorPosition (gST->ConOut, Columns, Rows ++);
    Print (String[Index]);
  }

  Print (L"\n");

  return EFI_SUCCESS;
}

/**
  Read file from file system.

  @param[in]  RootFile      A pointer to EFI_FILE_PROTOCOL.
  @param[in]  FilePathName  File name to read.
  @param[out] OutFileData   File data after reading.
  @param[out] OutFileSize   File size.

  @retval     EFI_OUT_OF_RESOURCES
              EFI_SUCCESS
**/
STATIC
EFI_STATUS
GetFile (
  IN  EFI_FILE_PROTOCOL  *RootFile,
  IN  CHAR16             *FilePathName,
  OUT UINT8              **OutFileData,
  OUT UINTN              *OutFileSize
  )
{
  EFI_STATUS          Status;
  EFI_FILE_PROTOCOL   *File;
  EFI_FILE_INFO       *FileInfo;
  UINT8               *FileData;
  UINTN               FileInfoSize;
  UINTN               FileSize;

  File = NULL;
  FileSize = 0;
  FileInfo = NULL;
  FileInfoSize = 0;

  if (RootFile == NULL || FilePathName == NULL ||
      OutFileData == NULL || OutFileSize == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Invalid parameters!\n",
      __func__
      ));

    return EFI_INVALID_PARAMETER;
  }

  Status = RootFile->Open (
		  RootFile,
		  &File,
		  FilePathName,
		  EFI_FILE_MODE_READ,
		  0);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Open file failed! (Status = %r)\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Get file info
  //
  Status = File->GetInfo (
                   File,
                   &gEfiFileInfoGuid,
                   &FileInfoSize,
                   FileInfo
                   );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FileInfo = AllocatePool (FileInfoSize);
    if (FileInfo == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    Status = File->GetInfo (
		    File,
		    &gEfiFileInfoGuid,
		    &FileInfoSize,
		    FileInfo
		    );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a(): Get file info failed! (Status = %r)\n",
      __func__,
      Status
      ));
    goto Error;
  }

  if (FileInfo->Attribute & EFI_FILE_DIRECTORY) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_INFO,
      "%a(): FileInfo->Attribute! (Status = %r)\n",
      __func__,
      Status
      ));
    goto Error;
  }

  //
  // Get file data
  //
  FileSize = (UINTN)FileInfo->FileSize;
  FileData = AllocatePages (EFI_SIZE_TO_PAGES (FileSize));
  if (FileData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_INFO,
      "%a(): Allocate for file failed! (Status = %r)\n",
      __func__,
      Status
      ));
    goto Error;
  }

  Status = File->Read (File, &FileSize, FileData);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a(): Read file failed! (Status = %r)\n",
      __func__,
      Status
      ));
    goto Error;
  }

  *OutFileData = FileData;
  *OutFileSize = FileSize;

Error:
  if (EFI_ERROR (Status) && FileData != NULL) {
    FreePages (FileData, EFI_SIZE_TO_PAGES (FileSize));
  }

  if (FileInfo) {
    FreePool (FileInfo);
  }

  if (File) {
    File->Close (File);
  }

  return Status;
}

/**
  Return whether the USB device path is in a short form.

  @param[in]  DevicePath  The device path to be tested.

  @retval     TRUE   The device path is in short form.
  @retval     FALSE  The device path is not in short form.
**/
BOOLEAN
IsUsbShortForm (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      ((DevicePathSubType (DevicePath) == MSG_USB_DP)))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Read firmware form file system.

  @param[in]  FirmwareFileName    Firmware file name to read.
  @param[out] FirmwareData        A pointer to Firmware data after reading.
  @param[out] FirmwareSize        Firmware data size.

  @retval     EFI_NOT_FOUND
              EFI_SUCCESS
**/
EFI_STATUS
ReadFirmware (
  IN  CHAR16  *FirmwareFileName,
  OUT UINT8   **FirmwareData,
  OUT UINTN   *FirmwareSize
  )
{
  EFI_STATUS                       Status;
  UINTN                            HandleCount;
  EFI_HANDLE                       *Handles;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  EFI_FILE_PROTOCOL                *RootFile;
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;
  BOOLEAN                          IsUsb;
  UINTN                            Index;

  Handles = NULL;
  RootFile = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (EFI_ERROR (Status) || HandleCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Locate handle buffer failed! (Status = %r, HandleCount = %d)\n",
      __func__,
      Status,
      HandleCount
      ));
    goto Error;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    IsUsb = FALSE;
    Status = gBS->HandleProtocol (
		    Handles[Index],
		    &gEfiDevicePathProtocolGuid,
                    (VOID**)&DevicePath
                    );
    if (!EFI_ERROR(Status)) {
      while (!IsDevicePathEnd (DevicePath)) {
	IsUsb = IsUsbShortForm (DevicePath);
        if (IsUsb) {
           break;
        }

        DevicePath = NextDevicePathNode (DevicePath);
      }
    }

    if (!IsUsb) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID**)&FileSystem
                    );
    ASSERT (!EFI_ERROR (Status));

    if (RootFile) {
      RootFile->Close (RootFile);
      RootFile = NULL;
    }

    Status = FileSystem->OpenVolume (FileSystem, &RootFile);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
	"%a(): Open volume failed! (Status = %r)\n",
	__func__,
	Status
	));
      continue;
    }

    Status = GetFile (RootFile, FirmwareFileName, FirmwareData, FirmwareSize);
    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  if (Index >= HandleCount) {
    Status = EFI_NOT_FOUND;
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Index = %d\tHandleCount = %d! (Status = %r)\n",
      __func__,
      Index,
      HandleCount,
      Status
      ));
    goto Error;
  }

Error:
  if (Handles) {
    FreePool (Handles);
  }

  if (RootFile) {
    RootFile->Close (RootFile);
  }

  return Status;
}

/**
  Retrieve and format a string from HII.

  @param[in]      StringToken  The token for the string to retrieve.
  @param[in/out]  OutputStr    The buffer to store the formatted string.

  @retval     EFI_SUCCESS  The string was retrieved and formatted successfully.
  @retval     EFI_OUT_OF_RESOURCES  Memory allocation failed.
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
  Display an invalid information popup.

  @param[in]  StringToken1    The first string token.
  @param[in]  StringToken2    The second string token.

  @retval     TRUE
**/
BOOLEAN
PopupInvalidInformation (
  IN UINT16  StringToken1,
  IN UINT16  StringToken2
  )
{
  CHAR16         Str1[STRING_BUFFER_SIZE];
  CHAR16         Str2[STRING_BUFFER_SIZE];
  EFI_INPUT_KEY  InputKey;
  UINTN          EventIndex;

  // 
  // Retrieve and format the two strings
  //
  if (EFI_ERROR (RetrieveAndFormatString (StringToken1, Str1))) {
    Str1[0] = L'\0';
  }

  if (EFI_ERROR (RetrieveAndFormatString (StringToken2, Str2))) {
    Str2[0] = L'\0';
  }

  CreatePopUp (
    EFI_LIGHTGRAY | EFI_BACKGROUND_RED,
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
  Probe and initialize nor flash.

  @retval EFI_SUCCESS       The Nor Flash probe successfully.
  @retval EFI_NOT_FOUND     Nor flash does not exist.
  @retval EFI_DEVICE_ERROR  Nor flash initialized failed.
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
  Entry of the app.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
FirmwareUpdateEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  UINT8         *FirmwareData;
  UINTN         FirmwareSize;
  UINTN         Attribute;
  EFI_STATUS    Status;

  FirmwareData = NULL;
  FirmwareSize = 0;

  //
  // Turn off the watchdog timer
  //
  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  //
  // Locate SPI Master protocol
  //
  Status = gBS->LocateProtocol (
                  &gSophgoSpiMasterProtocolGuid,
                  NULL,
                  (VOID *)&SpiMasterProtocol
                  );
  if (EFI_ERROR (Status)) {
    Print (L"Cannot locate SPI Master protocol!\n");
    return Status;
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
    Print (L"Cannot locate Nor Flash protocol!\n");
    return Status;
  }

  //
  // Setup and probe Nor flash
  //
  Nor = SpiMasterProtocol->SetupDevice (
                  SpiMasterProtocol,
                  Nor,
		  0
                  );

  if (Nor == NULL) {
    Print (L"Nor Flash not found!\n");
    return EFI_NOT_FOUND;
  }

  Status = NorFlashProbe ();
  if (EFI_ERROR (Status)) {
    Print (L"Error while performing Nor flash probe [Status=%r]\n", Status);
    return Status;
  }

  Attribute = EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK;
  gST->ConOut->SetAttribute (gST->ConOut, Attribute);

  gFirmwareUpdateHandle = HiiAddPackages (
		  &gEfiCallerIdGuid,
		  gImageHandle,
		  FirmwareUpdateStrings,
		  NULL
		  );
  ASSERT (gFirmwareUpdateHandle != NULL);

  Status = ReadFirmware (FirmwareNames[0], &FirmwareData, &FirmwareSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%s not found\n",
      FirmwareNames[0]
      ));
    PopupInvalidInformation (STRING_TOKEN (STR_FIRMWARE_UPDATE_ERROR_INFO),
		             STRING_TOKEN (STR_PRESS_ENTER_CONTINUE));
    goto Error;
  }

  ShowCopyRightsAndWarning ();

  Print(L"Please do not power off or restart during the firmware update.\n");

  DEBUG ((DEBUG_INFO, "Firmware File (0x%X, 0x%X)\n", FirmwareData, FirmwareSize));

  Status = NorFlashProtocol->EraseChip (Nor);
  if (EFI_ERROR (Status)) {
    Print (L"Erase Flash Failed!\n");
    goto Error;
  }

  Status = NorFlashProtocol->WriteData (
		  Nor,
		  0x0,
		  FirmwareSize,
		  FirmwareData
		  );
  if (EFI_ERROR (Status)) {
    Print (L"Write Flash Failed!\n");
    goto Error;
  }

  Print (L"Firmware Update Success!\n");

Error:
  if (FirmwareData) {
    FreePages (FirmwareData, EFI_SIZE_TO_PAGES (FirmwareSize));
  }

  if (Nor) {
    SpiMasterProtocol->FreeDevice (Nor);
  }

  Attribute = EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK;
  gST->ConOut->SetAttribute (gST->ConOut, Attribute);

  if (!EFI_ERROR (Status)) {
    Print (L"\nFirmware has been updated, system will reboot now!\n");

    //
    // Stall 2s
    //
    gBS->Stall (2000 * 1000);

    ResetCold ();
  }

  HiiRemovePackages (gFirmwareUpdateHandle);

  return Status;
}
