/** @file
  Implementation for PlatformBootManagerLib library class interfaces.

  Copyright (c) 2023. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformBm.h"
EFI_GUID  mUiApp = {
  0x462CAA21, 0x7614, 0x4503, { 0x83, 0x6E, 0x8A, 0xB6, 0xF4, 0x66, 0x23, 0x31 }
};

EFI_GUID  mBootMenuFile = {
  0xEEC25BDC, 0x67F2, 0x4D95, { 0xB1, 0xD5, 0xF8, 0x1B, 0x20, 0x39, 0xD1, 0x1D }
};

STATIC PLATFORM_SERIAL_CONSOLE mSerialConsole = {
  //
  // VENDOR_DEVICE_PATH SerialDxe
  //
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, DP_NODE_LEN (VENDOR_DEVICE_PATH) },
    EDKII_SERIAL_PORT_LIB_VENDOR_GUID
  },

  //
  // UART_DEVICE_PATH Uart
  //
  {
    { MESSAGING_DEVICE_PATH, MSG_UART_DP, DP_NODE_LEN (UART_DEVICE_PATH) },
    0,                  // Reserved
    115200,             // BaudRate
    8,                  // DataBits
    1,                  // Parity
    1                   // StopBits
  },

  //
  // VENDOR_DEVICE_PATH TermType
  //
  {
    {
      MESSAGING_DEVICE_PATH, MSG_VENDOR_DP,
      DP_NODE_LEN (VENDOR_DEVICE_PATH)
    }
    //
    // Guid to be filled in dynamically
    //
  },

  //
  // EFI_DEVICE_PATH_PROTOCOL End
  //
  {
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    DP_NODE_LEN (EFI_DEVICE_PATH_PROTOCOL)
  }
};

STATIC PLATFORM_USB_KEYBOARD mUsbKeyboard = {
  //
  // USB_CLASS_DEVICE_PATH Keyboard
  //
  {
    {
      MESSAGING_DEVICE_PATH, MSG_USB_CLASS_DP,
      DP_NODE_LEN (USB_CLASS_DEVICE_PATH)
    },
    0xFFFF, // VendorId: any
    0xFFFF, // ProductId: any
    3,      // DeviceClass: HID
    1,      // DeviceSubClass: boot
    1       // DeviceProtocol: keyboard
  },

  //
  // EFI_DEVICE_PATH_PROTOCOL End
  //
  {
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    DP_NODE_LEN (EFI_DEVICE_PATH_PROTOCOL)
  }
};

/**
  Locate all handles that carry the specified protocol, filter them with a
  callback function, and pass each handle that passes the filter to another
  callback.

  @param[in] ProtocolGuid  The protocol to look for.

  @param[in] Filter        The filter function to pass each handle to. If this
                           parameter is NULL, then all handles are processed.

  @param[in] Process       The callback function to pass each handle to that
                           clears the filter.
**/
VOID
FilterAndProcess (
  IN EFI_GUID          *ProtocolGuid,
  IN FILTER_FUNCTION   Filter         OPTIONAL,
  IN CALLBACK_FUNCTION Process
  )
{
  EFI_STATUS Status;
  EFI_HANDLE *Handles;
  UINTN      NoHandles;
  UINTN      Idx;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  ProtocolGuid,
                  NULL /* SearchKey */,
                  &NoHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    //
    // This is not an error, just an informative condition.
    //
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: %g: %r\n",
      __func__,
      ProtocolGuid,
      Status
      ));
    return;
  }

  ASSERT (NoHandles > 0);
  for (Idx = 0; Idx < NoHandles; ++Idx) {
    CHAR16        *DevicePathText;
    STATIC CHAR16 Fallback[] = L"<device path unavailable>";

    //
    // The ConvertDevicePathToText () function handles NULL input transparently.
    //
    DevicePathText = ConvertDevicePathToText (
                       DevicePathFromHandle (Handles[Idx]),
                       FALSE, // DisplayOnly
                       FALSE  // AllowShortcuts
                       );
    if (DevicePathText == NULL) {
      DevicePathText = Fallback;
    }

    if ((Filter == NULL)
      || (Filter (Handles[Idx], DevicePathText)))
    {
      Process (Handles[Idx], DevicePathText);
    }

    if (DevicePathText != Fallback) {
      FreePool (DevicePathText);
    }
  }
  gBS->FreePool (Handles);
}

/**
  This FILTER_FUNCTION checks if a handle corresponds to a PCI display device.

  @param  Handle   The handle to check
  @param  ReportText   A pointer to a string at the time of the error.

  @retval    TURE     THe  handle corresponds to a PCI display device.
  @retval    FALSE    THe  handle does not corresponds to a PCI display device.
**/
BOOLEAN
EFIAPI
IsPciDisplay (
  IN EFI_HANDLE   Handle,
  IN CONST CHAR16 *ReportText
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00          Pci;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiPciIoProtocolGuid,
                  (VOID**)&PciIo
                  );
  if (EFI_ERROR (Status)) {
    //
    // This is not an error worth reporting.
    //
    return FALSE;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0 /* Offset */,
                        sizeof Pci / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %s: %r\n",
      __func__,
      ReportText,
      Status
      ));
    return FALSE;
  }

  return IS_PCI_DISPLAY (&Pci);
}

/**
  This FILTER_FUNCTION checks if a handle corresponds to a non-discoverable
  USB host controller.
**/
STATIC
BOOLEAN
EFIAPI
IsUsbHost (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText
  )
{
  NON_DISCOVERABLE_DEVICE  *Device;
  EFI_STATUS               Status;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEdkiiNonDiscoverableDeviceProtocolGuid,
                  (VOID **)&Device
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (CompareGuid (Device->Type, &gEdkiiNonDiscoverableUhciDeviceGuid) ||
      CompareGuid (Device->Type, &gEdkiiNonDiscoverableEhciDeviceGuid) ||
      CompareGuid (Device->Type, &gEdkiiNonDiscoverableXhciDeviceGuid))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  This CALLBACK_FUNCTION attempts to connect a handle non-recursively, asking
  the matching driver to produce all first-level child handles.

  @param  Handle       The handle to connect.
  @param  ReportText   A pointer to a string at the time of the error.

  @retval  VOID
**/
VOID
EFIAPI
Connect (
  IN EFI_HANDLE   Handle,
  IN CONST CHAR16 *ReportText
  )
{
  EFI_STATUS Status;

  Status = gBS->ConnectController (
                  Handle, // ControllerHandle
                  NULL,   // DriverImageHandle
                  NULL,   // RemainingDevicePath -- produce all children
                  FALSE   // Recursive
                  );
  DEBUG ((
    EFI_ERROR (Status) ? DEBUG_ERROR : DEBUG_VERBOSE,
    "%a: %s: %r\n",
    __func__,
    ReportText,
    Status
    ));
}

/**
  This CALLBACK_FUNCTION retrieves the EFI_DEVICE_PATH_PROTOCOL from the
  handle, and adds it to ConOut and ErrOut.

  @param  Handle   The handle to retrieves.
  @param  ReportText   A pointer to a string at the time of the error.

  @retval  VOID
**/
VOID
EFIAPI
AddOutput (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText
  )
{
  EFI_STATUS               Status;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;

  DevicePath = DevicePathFromHandle (Handle);
  if (DevicePath == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %s: handle %p: device path not found\n",
      __func__,
      ReportText,
      Handle
      ));
    return;
  }

  Status = EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %s: adding to ConOut: %r\n",
      __func__,
      ReportText,
      Status
      ));
    return;
  }

  Status = EfiBootManagerUpdateConsoleVariable (ErrOut, DevicePath, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %s: adding to ErrOut: %r\n",
      __func__,
      ReportText,
      Status)
      );
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: %s: added to ConOut and ErrOut\n",
    __func__,
    ReportText
    ));
}

/**
  Register the boot option.

  @param  FileGuid      File Guid.
  @param  Description   Option descriptor.
  @param  Attributes    Option  Attributes.

  @retval  VOID
**/
VOID
PlatformRegisterFvBootOption (
  IN EFI_GUID     *FileGuid,
  IN CHAR16       *Description,
  IN UINT32       Attributes,
  EFI_INPUT_KEY   *Key
  )
{
  EFI_STATUS                        Status;
  INTN                              OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION      NewOption;
  EFI_BOOT_MANAGER_LOAD_OPTION      *BootOptions;
  UINTN                             BootOptionCount;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH FileNode;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **) &LoadedImage
                  );
  ASSERT_EFI_ERROR (Status);

  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);
  DevicePath = DevicePathFromHandle (LoadedImage->DeviceHandle);
  ASSERT (DevicePath != NULL);
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *) &FileNode
                 );
  ASSERT (DevicePath != NULL);

  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             Attributes,
             Description,
             DevicePath,
             NULL,
             0
             );
  ASSERT_EFI_ERROR (Status);
  FreePool (DevicePath);

  BootOptions = EfiBootManagerGetLoadOptions (
                  &BootOptionCount,
                  LoadOptionTypeBoot
                  );

  OptionIndex = EfiBootManagerFindLoadOption (
                  &NewOption,
                  BootOptions,
                  BootOptionCount
                  );

  if (OptionIndex == -1) {
    Status = EfiBootManagerAddLoadOptionVariable (&NewOption, MAX_UINTN);
    ASSERT_EFI_ERROR (Status);
    Status = EfiBootManagerAddKeyOptionVariable (
               NULL,
               (UINT16)NewOption.OptionNumber,
               0,
               Key,
               NULL
               );
    ASSERT (Status == EFI_SUCCESS || Status == EFI_ALREADY_STARTED);
  }

  EfiBootManagerFreeLoadOption (&NewOption);
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
}

/** Boot a Fv Boot Option.

  This function is useful for booting the UEFI Shell as it is loaded
  as a non active boot option.

  @param[in] FileGuid      The File GUID.
  @param[in] Description   String describing the Boot Option.

**/
STATIC
VOID
PlatformBootFvBootOption (
  IN  CONST EFI_GUID  *FileGuid,
  IN  CHAR16          *Description
  )
{
  EFI_STATUS                         Status;
  EFI_BOOT_MANAGER_LOAD_OPTION       NewOption;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // The UEFI Shell was registered in PlatformRegisterFvBootOption ()
  // previously, thus it must still be available in this FV.
  //
  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);
  DevicePath = DevicePathFromHandle (LoadedImage->DeviceHandle);
  ASSERT (DevicePath != NULL);
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&FileNode
                 );
  ASSERT (DevicePath != NULL);

  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             LOAD_OPTION_ACTIVE,
             Description,
             DevicePath,
             NULL,
             0
             );
  ASSERT_EFI_ERROR (Status);
  FreePool (DevicePath);

  EfiBootManagerBoot (&NewOption);
}

/**
  Make a platform driver to create predefined boot options and related hot keys.

  @param  VOID

  @retval  VOID
**/

STATIC
VOID
GetPlatformOptions (
  VOID
  )
{
  EFI_STATUS                      Status;
  EFI_BOOT_MANAGER_LOAD_OPTION    *CurrentBootOptions;
  EFI_BOOT_MANAGER_LOAD_OPTION    *BootOptions;
  EFI_INPUT_KEY                   *BootKeys;
  PLATFORM_BOOT_MANAGER_PROTOCOL  *PlatformBootManager;
  UINTN                           CurrentBootOptionCount;
  UINTN                           Index;
  UINTN                           BootCount;

  Status = gBS->LocateProtocol (
                  &gPlatformBootManagerProtocolGuid,
                  NULL,
                  (VOID **)&PlatformBootManager
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = PlatformBootManager->GetPlatformBootOptionsAndKeys (
                                  &BootCount,
                                  &BootOptions,
                                  &BootKeys
                                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Fetch the existent boot options. If there are none, CurrentBootCount
  // will be zeroed.
  //
  CurrentBootOptions = EfiBootManagerGetLoadOptions (
                         &CurrentBootOptionCount,
                         LoadOptionTypeBoot
                         );
  //
  // Process the platform boot options.
  //
  for (Index = 0; Index < BootCount; Index++) {
    INTN   Match;
    UINTN  BootOptionNumber;

    //
    // If there are any preexistent boot options, and the subject platform boot
    // option is already among them, then don't try to add it. Just get its
    // assigned boot option number so we can associate a hotkey with it. Note
    // that EfiBootManagerFindLoadOption() deals fine with (CurrentBootOptions
    // == NULL) if (CurrentBootCount == 0).
    //
    Match = EfiBootManagerFindLoadOption (
              &BootOptions[Index],
              CurrentBootOptions,
              CurrentBootOptionCount
              );
    if (Match >= 0) {
      BootOptionNumber = CurrentBootOptions[Match].OptionNumber;
    } else {
      //
      // Add the platform boot options as a new one, at the end of the boot
      // order. Note that if the platform provided this boot option with an
      // unassigned option number, then the below function call will assign a
      // number.
      //
      Status = EfiBootManagerAddLoadOptionVariable (
                 &BootOptions[Index],
                 MAX_UINTN
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: failed to register \"%s\": %r\n",
          __func__,
          BootOptions[Index].Description,
          Status
          ));
        continue;
      }

      BootOptionNumber = BootOptions[Index].OptionNumber;
    }
  }
  EfiBootManagerFreeLoadOptions (CurrentBootOptions, CurrentBootOptionCount);
  EfiBootManagerFreeLoadOptions (BootOptions, BootCount);
  FreePool (BootKeys);
}

EFI_DEVICE_PATH *
FvFilePath (
  EFI_GUID  *FileGuid
  )
{
  EFI_STATUS                         Status;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
        
  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);
        
  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  ASSERT_EFI_ERROR (Status);
  return AppendDevicePathNode (
           DevicePathFromHandle (LoadedImage->DeviceHandle),
           (EFI_DEVICE_PATH_PROTOCOL *)&FileNode
           );
}

/**
 *   Create one boot option for BootManagerMenuApp.
 *
 *   @param  FileGuid          Input file guid for the BootManagerMenuApp.
 *   @param  Description       Description of the BootManagerMenuApp boot option.
 *   @param  Position          Position of the new load option to put in the ****Order variable.
 *   @param  IsBootCategory    Whether this is a boot category.
 *
 *   @retval OptionNumber      Return the option number info.
**/
UINTN
RegisterBootManagerMenuAppBootOption (
  EFI_GUID  *FileGuid,
  CHAR16    *Description,
  UINTN     Position,
  BOOLEAN   IsBootCategory
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_MANAGER_LOAD_OPTION  NewOption;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  UINTN                         OptionNumber;

  DevicePath = FvFilePath (FileGuid);
  Status     = EfiBootManagerInitializeLoadOption (
                 &NewOption,
                 LoadOptionNumberUnassigned,
                 LoadOptionTypeBoot,
                 IsBootCategory ? LOAD_OPTION_ACTIVE : LOAD_OPTION_CATEGORY_APP,
                 Description,
                 DevicePath,
                 NULL,
                 0
                 );
  ASSERT_EFI_ERROR (Status);
  FreePool (DevicePath);
    
  Status = EfiBootManagerAddLoadOptionVariable (&NewOption, Position);
  ASSERT_EFI_ERROR (Status);
      
  OptionNumber = NewOption.OptionNumber;
      
  EfiBootManagerFreeLoadOption (&NewOption);
        
  return OptionNumber;
}

/**
  Extracts the GUID from a device path string. This function converts the given 
  device path to a string format and then extracts the GUID part from the FvFile 
  node in the device path, if present. This function is specifically tailored 
  for FvFile type device paths.

  @param  DevicePath   The device path from which the GUID will be extracted.

  @retval EFI_GUID*    Pointer to the extracted GUID if successful.
  @retval NULL         If the device path does not contain an FvFile node or 
                       if any error occurs during processing.

  Note:
  - The function uses ConvertDevicePathToText to convert the device path to a 
    string format.
  - It assumes the GUID follows the "FvFile(" node in the string representation.
  - Only applicable for device paths containing FvFile nodes.
**/
EFI_GUID *
ExtractGuidFromDevicePathString (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CHAR16        *DevicePathStr;
  CHAR16        *GuidStart;
  STATIC EFI_GUID ExtractedGuid;
  RETURN_STATUS  Status;
  
  DevicePathStr = ConvertDevicePathToText(DevicePath, TRUE, TRUE);
  if (DevicePathStr == NULL) {
    DEBUG((DEBUG_ERROR, "Failed to convert device path to text\n"));
    return NULL;
  }

  GuidStart = StrStr(DevicePathStr, L"FvFile(");
  if (GuidStart == NULL) {
    FreePool(DevicePathStr);
    return NULL;
  }

  GuidStart += StrLen(L"FvFile(");
  Status = StrToGuid(GuidStart, &ExtractedGuid);
  if (RETURN_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to parse GUID from string: %r\n", Status));
    FreePool(DevicePathStr);
    return NULL;
  }

  FreePool(DevicePathStr);
  return &ExtractedGuid;
}

/**
  Removes duplicate boot options from the BootOrder variable and associated 
  Boot#### variables. The function identifies duplicates based on matching GUIDs 
  extracted from the FvFile nodes in the device paths. If no GUID is found, 
  it falls back to comparing the Description and FilePath.

  The function performs the following steps:
  - Retrieves the current BootOrder and Boot#### variables.
  - Iterates through the boot options, identifying duplicates by:
    - Matching GUIDs from the device paths.
    - Comparing the Description and FilePath if GUIDs are not present or do not match.
  - Deletes duplicate boot options and updates the BootOrder variable.

  @retval EFI_SUCCESS           Successfully removed duplicate boot options.
  @retval EFI_NOT_FOUND         No boot options found to process.
  @retval EFI_ERROR             If an error occurs during variable updates 
                                or boot option deletion.

  Note:
  - The function uses ExtractGuidFromDevicePathString to extract GUIDs from the 
    device paths for comparison.
  - Duplicate entries are removed from both Boot#### variables and the BootOrder variable.
  - Updates to BootOrder ensure a consistent boot order after removing duplicates.
**/
EFI_STATUS
EFIAPI
RemoveDuplicateBootOptions (
  VOID
)
{
    EFI_STATUS                     Status;
    EFI_BOOT_MANAGER_LOAD_OPTION   *BootOptions;
    UINTN                          BootOptionCount;
    UINTN                          i, j;
    UINT16                         *BootOrder;
    UINTN                          BootOrderSize;
    EFI_GUID                       *GuidI;
    EFI_GUID                       *GuidJ;
    BOOLEAN                        IsDuplicate;

    IsDuplicate = FALSE;
    BootOptions = EfiBootManagerGetLoadOptions(&BootOptionCount, LoadOptionTypeBoot);
    if (BootOptions == NULL) {
        return EFI_NOT_FOUND;
    }

    Status = GetEfiGlobalVariable2(L"BootOrder", (VOID **)&BootOrder, &BootOrderSize);
    if (EFI_ERROR(Status)) {
        EfiBootManagerFreeLoadOptions(BootOptions, BootOptionCount);
        return Status;
    }
    for (i = 0; i < BootOptionCount; i++) {
        GuidI = ExtractGuidFromDevicePathString(BootOptions[i].FilePath);
        for (j = i + 1; j < BootOptionCount; j++) {
            GuidJ = ExtractGuidFromDevicePathString(BootOptions[j].FilePath);
            if(GuidI ==NULL || GuidJ == NULL) {
	        continue;
	    }
            if (CompareGuid(GuidI, GuidJ)) {
                IsDuplicate = TRUE;
            } else if (StrCmp(BootOptions[i].Description, BootOptions[j].Description) == 0 &&
                CompareMem(BootOptions[i].FilePath, BootOptions[j].FilePath, sizeof(EFI_DEVICE_PATH_PROTOCOL)) == 0) {
                IsDuplicate = TRUE;
            }

            if (IsDuplicate) {
                Status = EfiBootManagerDeleteLoadOptionVariable(BootOptions[j].OptionNumber, LoadOptionTypeBoot);
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR, "Failed to delete duplicate BootOption %d: %r\n", BootOptions[j].OptionNumber, Status));
                    continue;
                }

                for (UINTN k = 0; k < BootOrderSize / sizeof(UINT16); k++) {
                    if (BootOrder[k] == BootOptions[j].OptionNumber) {
                        for (UINTN m = k; m < (BootOrderSize / sizeof(UINT16)) - 1; m++) {
                            BootOrder[m] = BootOrder[m + 1];
                        }
                        BootOrderSize -= sizeof(UINT16);
                        break;
                    }
                }

                for (UINTN k = j; k < BootOptionCount - 1; k++) {
                    BootOptions[k] = BootOptions[k + 1];
                }
                BootOptionCount--;  
                j--;
            }
        }
    }

    Status = gRT->SetVariable(
        L"BootOrder",
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
        BootOrderSize,
        BootOrder
    );
   if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to update BootOrder variable: %r\n", Status));
    }

    FreePool(BootOrder);
    EfiBootManagerFreeLoadOptions(BootOptions, BootOptionCount);

    return EFI_SUCCESS;
}
/**
 *   Return the boot option number.
 *
 *   If not found it in the current boot option, create a new one.
 *
 *   @retval OptionNumber   Return the boot option number.
 *
**/
UINTN
GetOption (
  IN CHAR16 *Description,
  EFI_GUID  guid
  )
{
  UINTN                         BootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         Index;
  UINTN                         OptionNumber;
  EFI_GUID  			*GuidFind;
  EFI_GUID                      *Guidin;
  
  OptionNumber = -1;
  Guidin = &guid;
  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  for (Index = 0; Index < BootOptionCount; Index++) {
      GuidFind = ExtractGuidFromDevicePathString(BootOptions[Index].FilePath);
      if(GuidFind == NULL || Guidin == NULL) {
	      continue;
      } 
      if(CompareGuid(Guidin, GuidFind)) {
         OptionNumber = BootOptions[Index].OptionNumber;
	 break;
      }
  }
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
  if(OptionNumber == -1) {
     OptionNumber = (UINT16)RegisterBootManagerMenuAppBootOption (Guidin, Description, (UINTN)-1, FALSE);
  }
  return OptionNumber;
}

/**
  Register the boot option And Keys.

  @param  VOID

  @retval  VOID
**/
VOID
PlatformRegisterOptionsAndKeys (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_INPUT_KEY                Enter;
  EFI_INPUT_KEY                F2;
  EFI_INPUT_KEY                F7;
  UINTN               OptionNumber;
  //
  // Load platform boot options
  //
  GetPlatformOptions ();
  RemoveDuplicateBootOptions();
  //
  // Register ENTER as CONTINUE key
  //
  Enter.ScanCode    = SCAN_NULL;
  Enter.UnicodeChar = CHAR_CARRIAGE_RETURN;
  Status = EfiBootManagerRegisterContinueKeyOption (0, &Enter, NULL);
  ASSERT_EFI_ERROR (Status);
  // F7: open boot device list menu
  F7.ScanCode    = SCAN_F7;
  F7.UnicodeChar = CHAR_NULL;
  OptionNumber   = GetOption (L"UEFI BootManagerMenuApp",mBootMenuFile);
  EfiBootManagerAddKeyOptionVariable (NULL, (UINT16)OptionNumber, 0, &F7, NULL);
  //
  // Map F2 and ESC to Boot Manager Menu
  //
  F2.ScanCode     = SCAN_F2;
  F2.UnicodeChar  = CHAR_NULL;
  OptionNumber   = GetOption (L"UEFI UiApp",mUiApp);
  EfiBootManagerAddKeyOptionVariable (NULL, (UINT16)OptionNumber, 0, &F2, NULL);
}

//
// BDS Platform Functions
//
/**
  Do the platform init, can be customized by OEM/IBV
  Possible things that can be done in PlatformBootManagerBeforeConsole:
  > Update console variable: 1. include hot-plug devices;
  >                          2. Clear ConIn and add SOL for AMT
  > Register new Driver#### or Boot####
  > Register new Key####: e.g.: F12
  > Signal ReadyToLock event
  > Authentication action: 1. connect Auth devices;
  >                        2. Identify auto logon user.
**/
VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  )
{
  RemoveDuplicateBootOptions();
  PlatformRegisterOptionsAndKeys ();
  //
  // Signal EndOfDxe PI Event
  //
  EfiEventGroupSignal (&gEfiEndOfDxeEventGroupGuid);

  //
  // Dispatch deferred images after EndOfDxe event.
  //
  EfiBootManagerDispatchDeferredImages ();

  //
  // Locate the PCI root bridges and make the PCI bus driver connect each,
  // non-recursively. This will produce a number of child handles with PciIo on
  // them.
  //
  FilterAndProcess (&gEfiPciRootBridgeIoProtocolGuid, NULL, Connect);

  //
  // Ensure that USB is initialized by connecting the PCI root bridge so
  // that the xHCI PCI controller gets enumerated.
  //
  FilterAndProcess (&gEfiUsb2HcProtocolGuid, NULL, Connect);

  //
  // Find all display class PCI devices (using the handles from the previous
  // step), and connect them non-recursively. This should produce a number of
  // child handles with GOPs on them.
  //
  FilterAndProcess (&gEfiPciIoProtocolGuid, IsPciDisplay, Connect);

  //
  // Now add the device path of all handles with GOP on them to ConOut and
  // ErrOut.
  //
  FilterAndProcess (&gEfiGraphicsOutputProtocolGuid, NULL, AddOutput);

  //
  // The core BDS code connects short-form USB device paths by explicitly
  // looking for handles with PCI I/O installed, and checking the PCI class
  // code whether it matches the one for a USB host controller. This means
  // non-discoverable USB host controllers need to have the non-discoverable
  // PCI driver attached first.
  //
  FilterAndProcess (&gEdkiiNonDiscoverableDeviceProtocolGuid, IsUsbHost, Connect);

  //
  // Add the hardcoded short-form USB keyboard device path to ConIn.
  //
  EfiBootManagerUpdateConsoleVariable (
    ConIn,
    (EFI_DEVICE_PATH_PROTOCOL *)&mUsbKeyboard,
    NULL
    );

  //
  // Add the hardcoded serial console device path to ConIn, ConOut, ErrOut.
  //
  // ASSERT (FixedPcdGet8 (PcdDefaultTerminalType) == 4);
  CopyGuid (&mSerialConsole.TermType.Guid, &gEfiTtyTermGuid);

  EfiBootManagerUpdateConsoleVariable (
    ConIn,
    (EFI_DEVICE_PATH_PROTOCOL *)&mSerialConsole,
    NULL
    );
  EfiBootManagerUpdateConsoleVariable (
    ConOut,
    (EFI_DEVICE_PATH_PROTOCOL *)&mSerialConsole,
    NULL
    );
  EfiBootManagerUpdateConsoleVariable (
    ErrOut,
    (EFI_DEVICE_PATH_PROTOCOL *)&mSerialConsole,
    NULL
    );
}

/**
  Do the platform specific action after the console is ready
  Possible things that can be done in PlatformBootManagerAfterConsole:
  > Console post action:
    > Dynamically switch output mode from 100x31 to 80x25 for certain senarino
    > Signal console ready platform customized event
  > Run diagnostics like memory testing
  > Connect certain devices
  > Dispatch aditional option roms
  > Special boot: e.g.: USB boot, enter UI
**/
VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
{
  EFI_STATUS                    Status;
  UINTN                         FirmwareVerLength;
  EFI_INPUT_KEY                 Key;

  FirmwareVerLength = StrLen (PcdGetPtr (PcdFirmwareVersionString));
  //
  // Show the splash screen.
  //
  Status = BootLogoEnableLogo ();

  //
  // Connect the rest of the devices.
  //
  EfiBootManagerConnectAll ();

  //
  // Enumerate all possible boot options, then filter and reorder them.
  //
  EfiBootManagerRefreshAllBootOption ();

  //
  // Register UEFI Shell
  //
  Key.ScanCode    = SCAN_NULL;
  Key.UnicodeChar = L's';
  PlatformRegisterFvBootOption (
    &gUefiShellFileGuid, 
    L"UEFI Shell",
    LOAD_OPTION_ACTIVE,
    &Key);
}

/**
  This function is called each second during the boot manager waits the
  timeout.

  @param TimeoutRemain  The remaining timeout.
**/
VOID
EFIAPI
PlatformBootManagerWaitCallback (
  IN UINT16          TimeoutRemain
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  Black;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  White;
  UINT16                               Timeout;
  EFI_STATUS                           Status;

  Timeout = PcdGet16 (PcdPlatformBootTimeOut);

  Black.Raw = 0x00000000;
  White.Raw = 0x00FFFFFF;

  Status = BootLogoUpdateProgress (
             White.Pixel,
             Black.Pixel,
             L"Press ESCAPE for boot options",
             White.Pixel,
             (Timeout - TimeoutRemain) * 100 / Timeout,
             0
             );
  if (EFI_ERROR (Status)) {
    Print (L".");
  }
}

/**
  The function is called when no boot option could be launched,
  including platform recovery options and options pointing to applications
  built into firmware volumes.

  If this function returns, BDS attempts to enter an infinite loop.
**/
VOID
EFIAPI
PlatformBootManagerUnableToBoot (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_MANAGER_LOAD_OPTION  BootManagerMenu;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         OldBootOptionCount;
  UINTN                         NewBootOptionCount;

  //
  // Record the total number of boot configured boot options
  //
  BootOptions = EfiBootManagerGetLoadOptions (
                  &OldBootOptionCount,
                  LoadOptionTypeBoot
                  );
  EfiBootManagerFreeLoadOptions (BootOptions, OldBootOptionCount);

  //
  // Connect all devices, and regenerate all boot options
  //
  EfiBootManagerConnectAll ();
  EfiBootManagerRefreshAllBootOption ();

  //
  // Boot the 'UEFI Shell' by default.
  //
  PlatformBootFvBootOption (
    &gUefiShellFileGuid,
    L"UEFI Shell (default)"
    );

  //
  // Record the updated number of boot configured boot options
  //
  BootOptions = EfiBootManagerGetLoadOptions (
                  &NewBootOptionCount,
                  LoadOptionTypeBoot
                  );
  EfiBootManagerFreeLoadOptions (BootOptions, NewBootOptionCount);

  //
  // If the number of configured boot options has changed, reboot
  // the system so the new boot options will be taken into account
  // while executing the ordinary BDS bootflow sequence.
  // *Unless* persistent varstore is being emulated, since we would
  // then end up in an endless reboot loop.
  //
  if (!PcdGetBool (PcdEmuVariableNvModeEnable)) {
    if (NewBootOptionCount != OldBootOptionCount) {
       DEBUG ((
        DEBUG_WARN,
        "%a: rebooting after refreshing all boot options\n",
        __func__
       ));

      gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
    }
  }

  Status = EfiBootManagerGetBootManagerMenu (&BootManagerMenu);
  if (EFI_ERROR (Status)) {
    return;
  }

  for ( ; ;) {
    EfiBootManagerBoot (&BootManagerMenu);
  }
}