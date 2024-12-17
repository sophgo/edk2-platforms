/** @file
  The header file for Firmware manager menu.

  Copyright (c) 2024, SOPHGO Technologies Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef  FIRMWARE_MANAGER_H_
#define  FIRMWARE_MANAGER_H_

#include <Guid/GlobalVariable.h>
#include <Guid/MdeModuleHii.h>

#include <Protocol/DevicePathToText.h>
#include <Protocol/FormBrowserEx2.h>
#include <Protocol/HiiConfigAccess.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/FileExplorerLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Include/Spifmc.h>
#include <Include/SpiNorFlash.h>

typedef UINT16  STRING_REF;

typedef struct {
  UINT8 Value;
} FIRMWARE_MANAGER_DATA;

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;


extern EFI_GUID mFirmwareManagerGuid;

//
// These are the VFR compiler generated data representing our VFR data.
//
#define FIRMWARE_MANAGER_CALLBACK_DATA_SIGNATURE  SIGNATURE_32 ('F', 'I', 'W', 'M')
#define FIRMWARE_MANAGER_CALLBACK_DATA_FROM_THIS(a)  CR (a, FIRMWARE_MANAGER_CALLBACK_DATA, ConfigAccess, FIRMWARE_MANAGER_CALLBACK_DATA_SIGNATURE)

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

  //
  // Private data
  //
  FIRMWARE_MANAGER_DATA           FirmwareManagerData;
} FIRMWARE_MANAGER_CALLBACK_DATA;

extern FIRMWARE_MANAGER_CALLBACK_DATA  gFirmwareManagerPrivate;

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
  );

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
**/
EFI_STATUS
EFIAPI
FirmwareManagerExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );

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
  );

/**
  Install Firmware manager config Menu driver.

  @param[in] ImageHandle     The image handle.
  @param[in] SystemTable     The system table.

  @retval  EFI_SUCEESS  Install Boot manager menu success.
  @retval  Other        Return error status.
**/
EFI_STATUS
EFIAPI
InstallFirmwareManagerForm (
  IN OUT FIRMWARE_MANAGER_CALLBACK_DATA      *PrivateData
  );

/**
  Unloads the form and its installed protocol.

  @param[in]  ImageHandle      Handle that identifies the image to be unloaded.
  @param[in]  SystemTable      The system table.

  @retval  EFI_SUCCESS      The image has been unloaded.
**/
EFI_STATUS
EFIAPI
UninstallFirmwareManagerForm (
  IN OUT FIRMWARE_MANAGER_CALLBACK_DATA      *PrivateData
  );

#endif
