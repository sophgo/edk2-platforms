/** @file
The header file of HII Config Access protocol implementation of password configuration module.

Copyright (c) 2024, Sophgo. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef  PASSWORD_CONFIG_H_
#define  PASSWORD_CONFIG_H_

#include <Guid/MdeModuleHii.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FormBrowserEx2.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/UefiHiiServicesLib.h>
#include "PasswordConfigData.h"
#include "PasswordConfigFormGuid.h"
#include <Library/RestoreDefaults.h>

typedef UINT16  STRING_REF;

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;


extern EFI_GUID mPasswordConfigGuid;

//
// These are the VFR compiler generated data representing our VFR data.
//

#define PASSWORD_CONFIG_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('S', 'O', 'P', 'A')
#define PASSWORD_CONFIG_PRIVATE_DATA_FROM_THIS(a)  CR (a, PASSWORD_CONFIG_PRIVATE_DATA, ConfigAccess, PASSWORD_CONFIG_PRIVATE_DATA_SIGNATURE)

typedef struct {
  UINTN                           Signature;

  //
  // HII relative handles
  //
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;

  //
  // Produced protocols
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;

  //data
  PASSWORD_CONFIG_DATA                PasswordConfigData;
} PASSWORD_CONFIG_PRIVATE_DATA;

extern PASSWORD_CONFIG_PRIVATE_DATA  gPasswordConfigPrivate;

/**
  This call back function is registered with Phytium PCIE config formset.
  When user selects a boot option, this call back function will
  be triggered. The boot option is saved for later processing.


  @param[in] This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in] Action          Specifies the type of action taken by the browser.
  @param[in] QuestionId      A unique value which is sent to the original exporting driver
                             so that it can identify the type of data to expect.
  @param[in] Type            The type of value for the question.
  @param[in] Value           A pointer to the data being sent to the original exporting driver.
  @param[out] ActionRequest  On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

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
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param[in] This            - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in] Request         - A null-terminated Unicode string in <ConfigRequest> format.
  @param[out] Progress       - On return, points to a character in the Request string.
                             Points to the string's null terminator if request was successful.
                             Points to the most recent '&' before the first failing name/value
                             pair (or the beginning of the string if the failure is in the
                             first name/value pair) if the request was not successful.
  @param[out] Results        - A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
PasswordConfigExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );

/**
  This function processes the results of changes in configuration.

  @param[in] This            - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in] Configuration   - A null-terminated Unicode string in <ConfigResp> format.
  @param[out] Progress       - A pointer to a string filled in with the offset of the most
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
  );

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
  );

/**
  This function removes phytium password configuration Form.

  @param[in, out]  PrivateData   Points to password configuration private data.

**/
VOID
UninstallPasswordConfigForm (
  IN OUT PASSWORD_CONFIG_PRIVATE_DATA    *PrivateData
  );

EFI_STATUS
EFIAPI
PasswordRestore (
  VOID
  );

#endif
