/** @file
  This file defines the structures and macros required
  by the DebugConfigDxe driver.

  Copyright (c) 2026, SOPHGO Technology Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef DEBUG_CONFIG_H_
#define DEBUG_CONFIG_H_

#include <Uefi.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/HiiFormMapMethodGuid.h>
#include <Guid/DriverSampleHii.h>
#include <Guid/ZeroGuid.h>
#include <Guid/VendorGlobalVariables.h>

#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiPopup.h>
#include <Protocol/HiiPackageList.h>
#include <Protocol/HiiDatabase.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/RestoreDefaults.h>
#include <Library/PrintLib.h>

#include "DebugConfigNv.h"

typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

#define DEBUG_CONFIG_CALLBACK_DATA_SIGNATURE SIGNATURE_32 ('D', 'B', 'G', 'C')
#define DEBUG_CONFIG_CALLBACK_DATA_FROM_THIS(a)\
  CR (a, DEBUG_CONFIG_CALLBACK_DATA, ConfigAccess, DEBUG_CONFIG_CALLBACK_DATA_SIGNATURE)

typedef struct {
  UINTN                            Signature;

  EFI_HII_HANDLE                   HiiHandle;
  EFI_HANDLE                       DriverHandle;

  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;

  DEBUG_CONFIG_DATA                DebugConfigData;
} DEBUG_CONFIG_CALLBACK_DATA;

extern DEBUG_CONFIG_CALLBACK_DATA gDebugConfigPrivate;

EFI_STATUS
EFIAPI
DebugConfigCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );

EFI_STATUS
EFIAPI
DebugConfigExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );

EFI_STATUS
EFIAPI
DebugConfigRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  );

EFI_STATUS
EFIAPI
RestoreDebugConfigDefaults (
  VOID
  );

#endif