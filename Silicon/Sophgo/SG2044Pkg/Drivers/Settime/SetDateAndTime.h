/** @file

This file defines the structures and macros required by the SetDateAndTime driver.

Copyright (c) 2024, Sophgo Corporation. All rights reserved.<BR>
**/
#ifndef _SET_DATETIME_H_
#define _SET_DATETIME_H_

#include <Uefi.h>
#include <Library/UefiHiiServicesLib.h> 
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiPopup.h>
#include <Guid/MdeModuleHii.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/HiiFormMapMethodGuid.h>
#include <Guid/DriverSampleHii.h>
#include <Guid/ZeroGuid.h>
#include "SetTimeNv.h"
#include <Library/FileHandleLib.h>
#include <Protocol/HiiPackageList.h>
#include <Protocol/HiiDatabase.h>
#include <Guid/FileInfo.h>

/**
 * Extern declarations for .vfr and .uni data.
 */
extern UINT8  ShowTimeVfrBin[];
extern UINT8  SetDateAndTimeStrings[];

/**
 * Unique signature for the private data structure.
 */
#define SET_DATETIME_PRIVATE_SIGNATURE SIGNATURE_32 ('S', 'S', 'D', 'T')

/**
 * Private data structure for the SetDateAndTime driver.
 */
typedef struct {
  UINTN                            Signature;
  EFI_HANDLE                       DriverHandle;
  EFI_HII_HANDLE                   HiiHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting; // Consumed protocol
  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;      // Produced protocol
  EFI_TIME			   TimeData;
} DATE_TIME_PRIVATE_DATA;

#define SET_DATEANDTIME_PRIVATE_FROM_THIS(a) \
  CR (a, DATE_TIME_PRIVATE_DATA, ConfigAccess, SET_DATETIME_PRIVATE_SIGNATURE)

/**
 * HII specific Vendor Device Path definition.
 */
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

EFI_STATUS
UpdateSystemTime (
IN EFI_TIME *TimeData
);

#endif
