/** @file
  Implementation for PlatformBootManagerLib library class interfaces.

  Copyright (c) 2023. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformBm.h"

#define Hide  0x109
#define Default 0x0

extern EFI_GUID gSophgoEventAfterConsoleGuid;

EFI_GUID  mUiApp = {
  0x462CAA21, 0x7614, 0x4503, { 0x83, 0x6E, 0x8A, 0xB6, 0xF4, 0x66, 0x23, 0x31 }
};

EFI_GUID  mBootMenuFile = {
  0xEEC25BDC, 0x67F2, 0x4D95, { 0xB1, 0xD5, 0xF8, 0x1B, 0x20, 0x39, 0xD1, 0x1D }
};

EFI_GUID  mAutoCreateBootOptionGuid = {
  0x8108ac4e, 0x9f11, 0x4d59, { 0x85, 0x0e, 0xe2, 0x1a, 0x52, 0x2c, 0x59, 0xb2 }
};

extern VOID *
BmGetNextLoadOptionBuffer (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  Type,
  IN  EFI_DEVICE_PATH_PROTOCOL           *FilePath,
  OUT EFI_DEVICE_PATH_PROTOCOL           **FullPath,
  OUT UINTN                              *FileSize
  );

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
    CHAR16        Fallback[] = L"<device path unavailable>";

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
  Return TRUE when the boot option is auto-created instead of manually added.

  @param BootOption Pointer to the boot option to check.

  @retval TRUE  The boot option is auto-created.
  @retval FALSE The boot option is manually added.
**/
BOOLEAN
IsAutoCreateBootOption (
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  if ((BootOption->OptionalDataSize == sizeof (EFI_GUID)) &&
      CompareGuid ((EFI_GUID *)BootOption->OptionalData, &mAutoCreateBootOptionGuid)
      )
  {
    return TRUE;
  } else {
    return FALSE;
  }
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
  Extracts the GUID from a device path string. This function converts the given
  device path to a string format and then extracts the GUID part from the FvFile
  node in the device path, if present. This function is specifically tailored
  for FvFile type device paths.

  @param  DevicePath   The device path from which the GUID will be extracted.
  @param  Guid         Guid of this device path.

  @return EFI_SUCCESS on success, otherwise return an error

  Note:
  - The function uses ConvertDevicePathToText to convert the device path to a
    string format.
  - It assumes the GUID follows the "FvFile(" node in the string representation.
  - Only applicable for device paths containing FvFile nodes.
**/
EFI_STATUS
ExtractGuidFromDevicePathString (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT EFI_GUID                  *Guid
  )
{
  CHAR16        *DevicePathStr;
  CHAR16        *GuidStart;
  RETURN_STATUS  Status;

  DevicePathStr = ConvertDevicePathToText(DevicePath, TRUE, TRUE);
  if (DevicePathStr == NULL) {
    DEBUG((DEBUG_ERROR, "Failed to convert device path to text\n"));
    return EFI_NOT_FOUND;
  }

  GuidStart = StrStr(DevicePathStr, L"FvFile(");
  if (GuidStart == NULL) {
    FreePool(DevicePathStr);
    return EFI_NOT_FOUND;
  }

  GuidStart += StrLen(L"FvFile(");
  Status = StrToGuid(GuidStart, Guid);
  if (RETURN_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to parse GUID from string: %r\n", Status));
  }

  FreePool(DevicePathStr);
  return Status;
}

/**
  Sorting category for a boot option, used ONLY to order the entries that
  RefreshAllBootOption() appends at the tail of BootOrder within one boot.
  Entries already present in flash ("inherited") keep their slots untouched.
**/
typedef enum {
  BootSortCategoryHardDisk,    // fixed disks: NVMe / SATA / SAS
  BootSortCategoryRemovable,   // USB storage / SD / eMMC
  BootSortCategoryOptical,     // CD / DVD
  BootSortCategoryNetwork,     // iPXE application
  BootSortCategoryUnclassified,// auto-created but not recognizable
  BootSortCategoryFirmware,    // Shell / UiApp / BootManagerMenuApp
  BootSortCategoryMax
} BOOT_SORT_CATEGORY;

//
// iPXE UEFI application FFS GUID (edk2-non-osi Silicon/Sophgo/SG2044/iPXE).
//
STATIC EFI_GUID  mIPxeFileGuid = {
  0x39492805, 0x4E6C, 0x4015, { 0x91, 0x97, 0x69, 0x05, 0xFB, 0xAA, 0xF2, 0xB8 }
};

//
// Sophgo SdHostDxe host controller VenHw GUID: whole-controller boot options
// end with this node and carry no other bus/media node.
//
STATIC EFI_GUID  mSdHostDxeGuid = {
  0x11322596, 0xDD4F, 0x47FA, { 0x9E, 0x6C, 0xCE, 0x78, 0x7E, 0x11, 0xE4, 0xB1 }
};

/**
  Extract the FvFile GUID from a device path, if any.

  @param[in]  DevicePath  The device path to inspect.
  @param[out] Guid        Receives the FvFile GUID.

  @retval TRUE   An FvFile node was found and Guid was filled.
  @retval FALSE  No FvFile node in the device path.
**/
STATIC
BOOLEAN
GetFvFileGuidFromDevicePath (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT EFI_GUID                        *Guid
  )
{
  CONST MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  *FvFile;

  if ((DevicePath == NULL) || (Guid == NULL)) {
    return FALSE;
  }

  while (!IsDevicePathEnd (DevicePath)) {
    if ((DevicePathType (DevicePath) == MEDIA_DEVICE_PATH) &&
        (DevicePathSubType (DevicePath) == MEDIA_PIWG_FW_FILE_DP))
    {
      FvFile = (CONST MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)DevicePath;
      CopyGuid (Guid, &FvFile->FvFileName);
      return TRUE;
    }

    DevicePath = NextDevicePathNode (DevicePath);
  }

  return FALSE;
}

/**
  Map a boot option to its sorting category (see BOOT_SORT_CATEGORY).

  Firmware-internal entries (Shell / UiApp / BootManagerMenuApp) are detected
  by FvFile GUID; iPXE is detected by its FvFile GUID; everything else is only
  categorized when it is an auto-created option (OptionalData carries
  mBmAutoCreateBootOptionGuid). User / OS entries never reach this function's
  device-path classification because they are inherited by definition.
**/
STATIC
BOOT_SORT_CATEGORY
ClassifyBootOptionForSort (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  EFI_GUID                   Guid;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *Node;
  BOOLEAN                    HasUsb;
  BOOLEAN                    HasHardDrive;
  BOOLEAN                    HasCdrom;
  BOOLEAN                    HasSdMmc;
  BOOLEAN                    HasNvmeSata;

  if (GetFvFileGuidFromDevicePath (BootOption->FilePath, &Guid)) {
    if (CompareGuid (&Guid, &gUefiShellFileGuid) ||
        CompareGuid (&Guid, &mUiApp) ||
        CompareGuid (&Guid, &mBootMenuFile))
    {
      return BootSortCategoryFirmware;
    }

    if (CompareGuid (&Guid, &mIPxeFileGuid)) {
      return BootSortCategoryNetwork;
    }
  }

  if (!IsAutoCreateBootOption ((EFI_BOOT_MANAGER_LOAD_OPTION *)BootOption)) {
    //
    // User / OS entries are inherited (they keep their slot); reaching here
    // means a brand-new user entry appended this round - keep it first among
    // the appended entries (before device groups).
    //
    return BootSortCategoryHardDisk;
  }

  HasUsb        = FALSE;
  HasHardDrive  = FALSE;
  HasCdrom      = FALSE;
  HasSdMmc      = FALSE;
  HasNvmeSata   = FALSE;

  DevicePath = DuplicateDevicePath (BootOption->FilePath);
  if (DevicePath == NULL) {
    return BootSortCategoryUnclassified;
  }

  for (Node = DevicePath; !IsDevicePathEnd (Node); Node = NextDevicePathNode (Node)) {
    if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) &&
        (DevicePathSubType (Node) == MSG_USB_DP))
    {
      HasUsb = TRUE;
    } else if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) &&
               ((DevicePathSubType (Node) == MSG_NVME_NAMESPACE_DP) ||
                (DevicePathSubType (Node) == MSG_SATA_DP) ||
                (DevicePathSubType (Node) == MSG_ATAPI_DP) ||
                (DevicePathSubType (Node) == MSG_SCSI_DP)))
    {
      //
      // Whole-disk NVMe / SATA / SAS boot options carry no MEDIA_HARDDRIVE_DP
      // node (BmEnumerateBootOptions uses the whole-disk device path); the
      // bus node alone still identifies them as fixed disks.
      //
      HasNvmeSata = TRUE;
    } else if ((DevicePathType (Node) == MEDIA_DEVICE_PATH) &&
               (DevicePathSubType (Node) == MEDIA_CDROM_DP))
    {
      HasCdrom = TRUE;
    } else if ((DevicePathType (Node) == MEDIA_DEVICE_PATH) &&
               (DevicePathSubType (Node) == MEDIA_HARDDRIVE_DP))
    {
      HasHardDrive = TRUE;
    } else if ((DevicePathType (Node) == HARDWARE_DEVICE_PATH) &&
               (DevicePathSubType (Node) == HW_VENDOR_DP) &&
               CompareGuid (&((VENDOR_DEVICE_PATH *)Node)->Guid, &mSdHostDxeGuid))
    {
      //
      // Whole-controller SD / eMMC option from the Sophgo SdHostDxe stack:
      // the VenHw node is the only node in the path.
      //
      HasSdMmc = TRUE;
    }
  }

  FreePool (DevicePath);

  if (HasCdrom) {
    return BootSortCategoryOptical;
  }

  if (HasUsb || HasSdMmc) {
    return BootSortCategoryRemovable;
  }

  if (HasHardDrive || HasNvmeSata) {
    return BootSortCategoryHardDisk;
  }

  return BootSortCategoryUnclassified;
}

/**
  Check whether two boot option device paths are "equivalent" for the
  inherited/new decision.

  For FvFile entries (iPXE / Shell / UiApp / BootManagerMenuApp) the
  MemoryMapped(Base,End) prefix holds the runtime load address of the DXE FV,
  which changes across firmware layouts. Ignore the address and compare only
  the FvFile GUID - same semantics as BmAdjustFvFilePath()'s fall-back match.

  For everything else compare the full device path.
**/
STATIC
BOOLEAN
IsSameBootOptionPath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *Path1,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *Path2
  )
{
  EFI_GUID  Guid1;
  EFI_GUID  Guid2;

  if ((Path1 == NULL) || (Path2 == NULL)) {
    return FALSE;
  }

  if (GetFvFileGuidFromDevicePath (Path1, &Guid1) &&
      GetFvFileGuidFromDevicePath (Path2, &Guid2))
  {
    return (BOOLEAN)CompareGuid (&Guid1, &Guid2);
  }

  return (BOOLEAN)(CompareMem (Path1, Path2, GetDevicePathSize (Path1)) == 0);
}

/**
  Lift the entries appended by RefreshAllBootOption() to the front.

  The core refresh already guarantees everything else the rules need:
    - stale auto-created options are deleted (not in the enumerated set);
    - existing options keep their BootOrder slots untouched (full-field
      match on type/attributes/description/device-path/optional-data);
    - brand-new options are appended at the TAIL of BootOrder;
    - user / OS options are never touched.

  So the only remaining rule is placement: the appended entries (freshly
  inserted disks / USB sticks) must boot AHEAD of the inherited ones, so
  they never sit behind iPXE / Shell / firmware entries. This function
  finds the appended entries (an entry whose number was not in the
  pre-refresh order, or whose number is now backed by a different device -
  guards against number reuse by re-created options), sorts them stably by
  category (disk, removable, optical, network, unclassified, firmware) and
  writes them ahead of the inherited entries.

  Idempotent: a boot with no device change leaves BootOrder untouched.
**/
STATIC
VOID
SortAppendedBootOptions (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *OldOptions,
  IN UINTN                               OldOptionCount,
  IN CONST UINT16                        *OldOrder,
  IN UINTN                               OldOrderCount
  )
{
  EFI_STATUS                            Status;
  EFI_BOOT_MANAGER_LOAD_OPTION          *NewOptions;
  UINTN                                 NewOptionCount;
  UINT16                                *NewOrder;
  UINTN                                 NewOrderSize;
  UINTN                                 NewOrderCount;
  UINTN                                 Index;
  UINTN                                 Slot;
  UINTN                                 KeptCount;
  UINTN                                 AppendedCount;
  UINT16                                *Kept;
  UINT16                                *Appended;
  BOOT_SORT_CATEGORY                    *Category;
  EFI_BOOT_MANAGER_LOAD_OPTION          *Option;

  //
  // Post-refresh state.
  //
  NewOrder     = NULL;
  NewOrderSize = 0;
  Status = GetEfiGlobalVariable2 (
             L"BootOrder",
             (VOID **)&NewOrder,
             &NewOrderSize
             );
  if (EFI_ERROR (Status) || (NewOrder == NULL) || (NewOrderSize < sizeof (UINT16))) {
    if (NewOrder != NULL) {
      FreePool (NewOrder);
    }

    return;
  }

  NewOrderCount = NewOrderSize / sizeof (UINT16);

  NewOptions = EfiBootManagerGetLoadOptions (&NewOptionCount, LoadOptionTypeBoot);
  if ((NewOptions == NULL) || (NewOptionCount == 0)) {
    EfiBootManagerFreeLoadOptions (NewOptions, NewOptionCount);
    FreePool (NewOrder);
    return;
  }

  Kept     = AllocatePool (NewOrderCount * sizeof (UINT16));
  Appended = AllocatePool (NewOrderCount * sizeof (UINT16));
  Category = AllocatePool (NewOrderCount * sizeof (BOOT_SORT_CATEGORY));
  if ((Kept == NULL) || (Appended == NULL) || (Category == NULL)) {
    FreePool (Kept);
    FreePool (Appended);
    FreePool (Category);
    EfiBootManagerFreeLoadOptions (NewOptions, NewOptionCount);
    FreePool (NewOrder);
    return;
  }

  KeptCount     = 0;
  AppendedCount = 0;

  for (Index = 0; Index < NewOrderCount; Index++) {
    UINTN   OldOrderPos;
    BOOLEAN Inherited;

    Option = NULL;
    for (Slot = 0; Slot < NewOptionCount; Slot++) {
      if (NewOptions[Slot].OptionNumber == NewOrder[Index]) {
        Option = &NewOptions[Slot];
        break;
      }
    }

    //
    // Inherited = the number was in the pre-refresh order AND the option
    // behind it still has the same identity (guards against a number being
    // reused for a re-created option, e.g. after an FV address change).
    //
    Inherited = FALSE;
    if (Option != NULL) {
      OldOrderPos = OldOrderCount;
      if (OldOrder != NULL) {
        for (Slot = 0; Slot < OldOrderCount; Slot++) {
          if (OldOrder[Slot] == NewOrder[Index]) {
            OldOrderPos = Slot;
            break;
          }
        }
      }

      if (OldOrderPos != OldOrderCount) {
        for (Slot = 0; Slot < OldOptionCount; Slot++) {
          if ((OldOptions != NULL) &&
              (OldOptions[Slot].OptionNumber == NewOrder[Index]) &&
              IsSameBootOptionPath (OldOptions[Slot].FilePath, Option->FilePath))
          {
            Inherited = TRUE;
            break;
          }
        }
      }
    }

    if (Inherited) {
      Kept[KeptCount++] = NewOrder[Index];
    } else {
      Appended[AppendedCount] = NewOrder[Index];
      Category[AppendedCount] = (Option != NULL) ?
                                ClassifyBootOptionForSort (Option) :
                                BootSortCategoryUnclassified;
      AppendedCount++;
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "SortAppended: total=%d inherited=%d appended=%d\n",
    (UINT32)NewOrderCount,
    (UINT32)KeptCount,
    (UINT32)AppendedCount
    ));

  //
  // Stable selection sort of the appended entries by category.
  //
  if (AppendedCount != 0) {
    BOOLEAN  *Taken;
    UINT16   *SortedAppended;
    UINT32   MinRank;
    UINTN    MinIndex;
    UINTN    SortIdx;

    Taken          = AllocatePool (AppendedCount * sizeof (BOOLEAN));
    SortedAppended = AllocatePool (AppendedCount * sizeof (UINT16));
    if ((Taken != NULL) && (SortedAppended != NULL)) {
      for (Index = 0; Index < AppendedCount; Index++) {
        Taken[Index] = FALSE;
      }

      for (SortIdx = 0; SortIdx < AppendedCount; SortIdx++) {
        MinRank  = BootSortCategoryMax + 1;
        MinIndex = AppendedCount;
        for (Index = 0; Index < AppendedCount; Index++) {
          if (!Taken[Index] && (Category[Index] < MinRank)) {
            MinRank  = Category[Index];
            MinIndex = Index;
          }
        }

        Taken[MinIndex]         = TRUE;
        SortedAppended[SortIdx] = Appended[MinIndex];
      }

      DEBUG ((DEBUG_INFO, "SortAppended: appended before=["));
      for (Index = 0; Index < AppendedCount; Index++) {
        DEBUG ((DEBUG_INFO, "%04x(%d),", Appended[Index], (UINT32)Category[Index]));
      }

      DEBUG ((DEBUG_INFO, "] after=["));
      for (Index = 0; Index < AppendedCount; Index++) {
        DEBUG ((DEBUG_INFO, "%04x,", SortedAppended[Index]));
      }

      DEBUG ((DEBUG_INFO, "]\n"));

      CopyMem (Appended, SortedAppended, AppendedCount * sizeof (UINT16));
    }

    if (Taken != NULL) {
      FreePool (Taken);
    }

    if (SortedAppended != NULL) {
      FreePool (SortedAppended);
    }
  }

  //
  // Compose: appended entries FIRST, inherited entries behind them.
  // Write back only when the composition differs from the current order.
  //
  if (AppendedCount != 0) {
    UINT16  *Final;

    Final = AllocatePool (NewOrderCount * sizeof (UINT16));
    if (Final != NULL) {
      CopyMem (Final, Appended, AppendedCount * sizeof (UINT16));
      CopyMem (&Final[AppendedCount], Kept, KeptCount * sizeof (UINT16));

      if (CompareMem (Final, NewOrder, NewOrderSize) != 0) {
        Status = gRT->SetVariable (
                        L"BootOrder",
                        &gEfiGlobalVariableGuid,
                        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                        NewOrderSize,
                        Final
                        );
        DEBUG ((DEBUG_INFO, "SortAppended: SetVariable %r\n", Status));
      }

      FreePool (Final);
    }
  }

  FreePool (Kept);
  FreePool (Appended);
  FreePool (Category);
  EfiBootManagerFreeLoadOptions (NewOptions, NewOptionCount);
  FreePool (NewOrder);
}

/**
  Refresh the boot options and enforce the platform placement rule.

  Snapshot the current BootOrder and boot options, run the core
  EfiBootManagerRefreshAllBootOption(), then re-position the entries the
  refresh appended: freshly enumerated devices are lifted to the front of
  BootOrder (sorted by device category) so a newly inserted disk or USB
  stick boots ahead of the existing entries, which typically end with
  iPXE / Shell / firmware entries that are never the preferred target.
  Entries already stored in flash keep their relative order.
**/
STATIC
VOID
ReconcileBootOptions (
  VOID
  )
{
  EFI_BOOT_MANAGER_LOAD_OPTION  *OldOptions;
  UINTN                         OldOptionCount;
  UINT16                        *OldOrder;
  UINTN                         OldOrderSize;

  OldOptions   = EfiBootManagerGetLoadOptions (&OldOptionCount, LoadOptionTypeBoot);
  OldOrder     = NULL;
  OldOrderSize = 0;
  GetEfiGlobalVariable2 (L"BootOrder", (VOID **)&OldOrder, &OldOrderSize);

  EfiBootManagerRefreshAllBootOption ();

  SortAppendedBootOptions (
    OldOptions,
    OldOptionCount,
    OldOrder,
    (OldOrder != NULL) ? (OldOrderSize / sizeof (UINT16)) : 0
    );

  if (OldOrder != NULL) {
    FreePool (OldOrder);
  }

  EfiBootManagerFreeLoadOptions (OldOptions, OldOptionCount);
}


/**
  GetOption

  @param[in]  Description
  @param[in]  guid
  @param[in]  Attributes of the boot option
  @retval     OptionNumber
**/
UINTN
GetOption (
  IN CHAR16 *Description,
  EFI_GUID  Guid,
  UINT32    Attributes
  )
{
  UINTN                         BootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         Index;
  UINTN                         OptionNumber;
  EFI_GUID                      GuidFind;
  EFI_STATUS                    Status;

  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  for (Index = 0; Index < BootOptionCount; Index++) {
      Status = ExtractGuidFromDevicePathString(BootOptions[Index].FilePath, &GuidFind);
      if (EFI_ERROR(Status)) {
          continue;
      }
      if (CompareGuid(&Guid, &GuidFind)) {
        OptionNumber = BootOptions[Index].OptionNumber;
        break;
      }
  }
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);

  if (Index >= BootOptionCount) {
    return LoadOptionNumberUnassigned;
  } else {
    return OptionNumber;
  }
}

/**
  Register the boot Keys for current platform.
  @param  VOID

  @retval  VOID
**/
VOID
PlatformRegisterKeys (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_INPUT_KEY                Enter;
  EFI_INPUT_KEY                F2;
  EFI_INPUT_KEY                F7;
  EFI_INPUT_KEY                Key;
  UINTN                        OptionNumber;

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
  OptionNumber   = GetOption (L"UEFI BootManagerMenuApp",mBootMenuFile, Hide);
  EfiBootManagerAddKeyOptionVariable (NULL, (UINT16)OptionNumber, 0, &F7, NULL);
  //
  // Map F2 to Boot Manager Menu
  //
  F2.ScanCode     = SCAN_F2;
  F2.UnicodeChar  = CHAR_NULL;
  OptionNumber   = GetOption (L"UEFI UiApp",mUiApp, Hide);
  EfiBootManagerAddKeyOptionVariable (NULL, (UINT16)OptionNumber, 0, &F2, NULL);

  //
  // Add UEFI Shell Key F6 (hotkey only; do NOT touch the option attributes:
  // rewriting them makes the auto-created FV option diverge every refresh and
  // invalidates the Key#### CRC, breaking all hotkeys - measured on hw).
  //
  Key.ScanCode    = SCAN_F6;
  Key.UnicodeChar = CHAR_NULL;
  OptionNumber   = GetOption (L"UEFI Shell", gUefiShellFileGuid, Hide);
  EfiBootManagerAddKeyOptionVariable (NULL, (UINT16)OptionNumber, 0, &Key, NULL);
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
  EFI_EVENT                     AfterConsoleEvent;
  //
  // Show the splash screen.
  //
  Status = BootLogoEnableLogo ();

  //
  // Connect the rest of the devices.
  //
  EfiBootManagerConnectAll ();

  ReconcileBootOptions ();

  PlatformRegisterKeys();

  //
  // Signal After Console event
  //
  Status = gBS->CreateEventEx (
      EVT_NOTIFY_SIGNAL,
      TPL_CALLBACK,
      EfiEventEmptyFunction,
      NULL,
      &gSophgoEventAfterConsoleGuid,
      &AfterConsoleEvent
      );
  if (!EFI_ERROR (Status)) {
    gBS->SignalEvent (AfterConsoleEvent);
    gBS->CloseEvent (AfterConsoleEvent);
  }
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
             L"F2: Setup   F6: UEFI Shell   F7: Boot Menu",
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
