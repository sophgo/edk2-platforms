/** @file
Head file for front page.
Copyright (c) 2024, Sophgo. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _FRONT_PAGE_H_
#define _FRONT_PAGE_H_

#include "String.h"
#include "Ui.h"
#include <Protocol/BootLogo.h>
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/FileHandleLib.h>
#include <Library/IniParserLib.h>
#include <Library/IniParserLib/IniParserUtil.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/HiiPackageList.h>
#include <Protocol/HiiDatabase.h>
#include <Guid/FileInfo.h>
#include <Guid/MdeModuleHii.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/CustomizedDisplayLib/Colors.h>
#include <Protocol/HiiConfigAccess.h>
#include <stdio.h>
#include "FrontPageNVDataStruc.h"
#include "FrontPageCustomizedUiSupport.h"
#include <Library/PasswordRead.h>
#include <Library/RestoreDefaults.h>
#include <Library/SmbiosInformationLib.h>

#define PRINTABLE_LANGUAGE_NAME_STRING_ID  0x0001
#define FRONT_PAGE_CALLBACK_DATA_SIGNATURE  SIGNATURE_32 ('F', 'P', 'C', 'B')
#define EFI_FP_CALLBACK_DATA_FROM_THIS(a) \
  CR (a, \
      FRONT_PAGE_CALLBACK_DATA, \
      ConfigAccess, \
      FRONT_PAGE_CALLBACK_DATA_SIGNATURE \
      )
#define     QF_DATE_STORAGE_TIME    0x10

extern UINT8  FrontPageVfrBin[];
extern EFI_FORM_BROWSER2_PROTOCOL  *gFormBrowser2;
extern EFI_GUID mFrontPageGuid;
typedef struct {
  UINTN                             Signature;
  EFI_HII_HANDLE                    HiiHandle;
  EFI_HANDLE                        DriverHandle;
  EFI_STRING_ID                     *LanguageToken;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  EFI_HII_CONFIG_ROUTING_PROTOCOL   *HiiConfigRouting;
  EFI_GUID                          *FormSetGuid;
  CHAR16                            *VariableName;
} FRONT_PAGE_CALLBACK_DATA;

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         - A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        - On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         - A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
ExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
);

/**
  This function processes the results of changes in configuration.


  @param This            - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   - A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        - A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
RouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress

);

/**
  This function processes the results of changes in configuration.


  @param This            - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          - Specifies the type of action taken by the browser.
  @param QuestionId      - A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            - The type of value for the question.
  @param Value           - A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   - On return, points to the action requested by the callback function.

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
  );

/**
  Initialize HII information for the FrontPage

  @retval  EFI_SUCCESS        The operation is successful.
  @retval  EFI_DEVICE_ERROR   If the dynamic opcode creation failed.

**/
EFI_STATUS
InitializeFrontPage (
  VOID
  );

/**
  Acquire the string associated with the ProducerGuid and return it.


  @param ProducerGuid    - The Guid to search the HII database for
  @param Token           - The token value of the string to extract
  @param String          - The string that is extracted

  @retval  EFI_SUCCESS  The function returns EFI_SUCCESS always.

**/
EFI_STATUS
GetProducerString (
  IN      EFI_GUID       *ProducerGuid,
  IN      EFI_STRING_ID  Token,
  OUT     CHAR16         **String
  );

/**
  This function is the main entry of the UI entry.
  The function will present the main menu of the system UI.

  @param ConnectAllHappened Caller passes the value to UI to avoid unnecessary connect-all.

**/
VOID
EFIAPI
UiEntry (
  IN BOOLEAN  ConnectAllHappened
  );

/**
  Extract device path for given HII handle and class guid.

  @param Handle          The HII handle.

  @retval  NULL          Fail to get the device path string.
  @return  PathString    Get the device path string.

**/
CHAR16 *
ExtractDevicePathFromHiiHandle (
  IN      EFI_HII_HANDLE  Handle
  );

VOID
UpdateFrontPageForm (
VOID
);

EFI_STATUS
UpdateBootRegion (
EFI_HII_HANDLE HiiHandle
);

EFI_STATUS
UpdateTimeRegion (
EFI_HII_HANDLE HiiHandle
);

VOID
AppendAltCfgString (
  IN OUT EFI_STRING  *RequestResult,
  IN     EFI_STRING  ConfigRequestHdr
);

EFI_STATUS
EFIAPI
PassWordToggleRestore (
 VOID
);

#endif // _FRONT_PAGE_H_
