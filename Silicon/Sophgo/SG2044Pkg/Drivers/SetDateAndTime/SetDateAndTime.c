/** @file
# This driver implements a UEFI module for configuring the system date and time through a custom HII-based interface.
#
# Copyright (c) 2024  Sophgo Corporation. All rights reserved.<BR>
**/

#include "SetDateAndTime.h"

EFI_HANDLE                  DriverHandle;
DATE_TIME_PRIVATE_DATA      *PrivateData = NULL;
EFI_GUID  mTimeSetFormSetGuid = TIME_SET_FORMSET_GUID;
EFI_GUID  gSetDateAndTimeDataGuid = VAR_TIME_DATA_GUID;

STATIC RESTORE_PROTOCOL gSetDateAndTimeRestoreProtocol = {
  TimeDataRestoreDefaults
};

/**
 * @brief   Defines a vendor-specific device path structure for the Set Date and Time HII formset.
 *
 * @details
 *          - This structure is used to identify the HII formset associated with the Time Set functionality.
 *          - It includes a hardware device path and a vendor GUID specific to the formset (TIME_SET_FORMSET_GUID).
 *          - The device path is terminated with an End Device Path node.
 *
 * @structure HII_VENDOR_DEVICE_PATH
 *            - HARDWARE_DEVICE_PATH: Indicates the device path type as hardware.
 *            - HW_VENDOR_DP: Indicates this is a vendor-specific device path.
 *            - VENDOR_DEVICE_PATH: Includes the size and GUID for the formset.
 *            - END_DEVICE_PATH_TYPE: Marks the end of the device path.
 */
HII_VENDOR_DEVICE_PATH  mSetDateAndTimeHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    TIME_SET_FORMSET_GUID
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

EFI_STATUS
EFIAPI
TimeDataRestoreDefaults (
  VOID
  )
{
  TIME_DATA TimeData;
  EFI_STATUS Status;
  PrivateData->TimeData.Year   = 2000;
  PrivateData->TimeData.Month  = 1;
  PrivateData->TimeData.Day    = 1;
  PrivateData->TimeData.Hour   = 0;
  PrivateData->TimeData.Minute = 0;
  PrivateData->TimeData.Second = 0;
  TimeData.Year   = 2000;
  TimeData.Month  = 1;
  TimeData.Day    = 1;
  TimeData.Hour   = 0;
  TimeData.Minute = 0;
  TimeData.Second = 0;

Status = gRT->SetVariable(
    L"DynamicTimeData",
    &gSetDateAndTimeDataGuid,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof(TIME_DATA),
    &TimeData
);
if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to set EFI variable: %r\n", Status));
    return Status;
}
Status = UpdateSystemTimeFromHiiInput(&PrivateData->TimeData);
if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to set EFI variable: %r\n", Status));
    return Status;
  }

return EFI_SUCCESS;
}

/**
 * @brief   Validates if the provided year, month, and day constitute a valid date.
 *
 * @param   Year   The year to validate (e.g., 2024).
 * @param   Month  The month to validate (1 for January, 12 for December).
 * @param   Day    The day of the month to validate.
 *
 * @return  BOOLEAN
 *          - TRUE  if the given year, month, and day form a valid date.
 *          - FALSE otherwise.
 *
 * @details
 *          - The function checks if the month is within the range [1, 12].
 *          - The day is checked against the number of days in the given month.
 *          - For February (Month = 2), the function correctly handles leap years:
 *            - Leap years are divisible by 4 but not by 100, unless divisible by 400.
 *          - Example:
 *            - IsValidDate(2024, 2, 29) returns TRUE (2024 is a leap year).
 *            - IsValidDate(2023, 2, 29) returns FALSE (2023 is not a leap year).
 */
BOOLEAN
IsValidDate (
  IN UINT16 Year,
  IN UINT8  Month,
  IN UINT8  Day
  )
{
  CONST UINT8 DaysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if (Month < 1 || Month > 12 || Day < 1) {
    return FALSE;
  }

  BOOLEAN IsLeapYear = ((Year % 4 == 0 && Year % 100 != 0) || (Year % 400 == 0));
  UINT8 MaxDays = DaysInMonth[Month - 1];
  if (Month == 2 && IsLeapYear) {
    MaxDays = 29;
  }

  return Day <= MaxDays;
}
/**
 * @brief   Updates the system time using the provided EFI_TIME structure.
 *
 * @param   TimeData  A pointer to an EFI_TIME structure containing the new date and time.
 *
 * @return  EFI_STATUS
 *          - EFI_SUCCESS: The system time was successfully updated.
 *          - EFI_INVALID_PARAMETER: The provided date in the EFI_TIME structure is invalid.
 *
 * @details
 *          - The function first validates the provided date using the IsValidDate() function.
 *          - If the date is valid, it updates the system time using the runtime service
 *            gRT->SetTime().
 *          - If the date is invalid, the function returns EFI_INVALID_PARAMETER without updating
 *            the system time.
 *          - This ensures that only valid date-time values are applied to the system.
 */
EFI_STATUS
UpdateSystemTimeFromHiiInput (
  IN EFI_TIME *TimeData
  )
{
  if (!IsValidDate(TimeData->Year, TimeData->Month, TimeData->Day)) {
    return EFI_INVALID_PARAMETER;
  }
  return gRT->SetTime(TimeData);
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request                A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param  Progress               On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param  Results                A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
ExtractConfig(
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN CONST EFI_STRING Request,
  OUT EFI_STRING *Progress,
  OUT EFI_STRING *Results
) {
  DATE_TIME_PRIVATE_DATA *PrivateData;
  EFI_STRING ConfigRequestHdr = NULL;
  EFI_STATUS Status;

  if (This == NULL || Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = SET_DATEANDTIME_PRIVATE_FROM_THIS(This);
  *Progress = Request;
  *Results = NULL;

  ConfigRequestHdr = HiiConstructConfigHdr(&mTimeSetFormSetGuid, L"DynamicTimeData", PrivateData->DriverHandle);
  if (ConfigRequestHdr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EFI_STRING EffectiveRequest = Request;
  if (Request == NULL) {
    EffectiveRequest = ConfigRequestHdr;
  } else if (!HiiIsConfigHdrMatch(Request, &mTimeSetFormSetGuid, L"DynamicTimeData")) {
    FreePool(ConfigRequestHdr);
    return EFI_NOT_FOUND;
  }

  Status = PrivateData->HiiConfigRouting->BlockToConfig(
    PrivateData->HiiConfigRouting,
    EffectiveRequest,
    (UINT8 *)&PrivateData->TimeData,
    sizeof(EFI_TIME),
    Results,
    Progress
  );

  FreePool(ConfigRequestHdr);
  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration          A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param  Progress               A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
RouteConfig(
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN CONST EFI_STRING Configuration,
  OUT EFI_STRING *Progress
) {
  DATE_TIME_PRIVATE_DATA *PrivateData;
  EFI_STATUS Status;
  UINTN DataSize;

  if (This == NULL || Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = SET_DATEANDTIME_PRIVATE_FROM_THIS(This);
  *Progress = Configuration;

  if (!HiiIsConfigHdrMatch(Configuration, &mTimeSetFormSetGuid, L"DynamicTimeData")) {
    DEBUG((DEBUG_ERROR, "Configuration header does not match.\n"));
    return EFI_NOT_FOUND;
  }

  DataSize = sizeof(EFI_TIME);
  Status = PrivateData->HiiConfigRouting->ConfigToBlock(
    PrivateData->HiiConfigRouting,
    Configuration,
    (UINT8 *)&PrivateData->TimeData,
    &DataSize,
    Progress
  );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "ConfigToBlock failed: %r. Progress: %s\n", Status, *Progress));
    return Status;
  }

  Status = UpdateSystemTimeFromHiiInput(&PrivateData->TimeData);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to update system time: %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS EFIAPI DriverCallback(
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION Action,
  IN EFI_QUESTION_ID QuestionId,
  IN UINT8 Type,
  IN EFI_IFR_TYPE_VALUE *Value,
  OUT EFI_BROWSER_ACTION_REQUEST *ActionRequest
) {
  if (Action != EFI_BROWSER_ACTION_CHANGED && Action != EFI_BROWSER_ACTION_SUBMITTED) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
SetDateAndTimeInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;
  CHAR16                    	  *NewString;

  NewString        = NULL;
  PrivateData = AllocateZeroPool (sizeof (DATE_TIME_PRIVATE_DATA));
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->Signature = SET_DATETIME_PRIVATE_SIGNATURE;

  PrivateData->ConfigAccess.ExtractConfig = ExtractConfig;
  PrivateData->ConfigAccess.RouteConfig = RouteConfig;
  PrivateData->ConfigAccess.Callback = DriverCallback;
  Status = gRT->GetTime(&PrivateData->TimeData, NULL);

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to get current time: %r\n", Status));
    PrivateData->TimeData.Year = 2000;
    PrivateData->TimeData.Month = 1;
    PrivateData->TimeData.Day = 1;
  }
  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **) &HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiConfigRouting = HiiConfigRouting;
  Status = gBS->InstallMultipleProtocolInterfaces(
                  &DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mSetDateAndTimeHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &PrivateData->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallProtocolInterface(
                  &DriverHandle,
                  &gSetDateAndTimeRestoreProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)&gSetDateAndTimeRestoreProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  PrivateData->DriverHandle = DriverHandle;
  //
  // Publish our HII data
  //
  HiiHandle = HiiAddPackages (
                   &mTimeSetFormSetGuid,
                   DriverHandle,
                   SetDateAndTimeStrings,
                   SetDateAndTimeVfrBin,
                   NULL
                   );
  if (HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->HiiHandle = HiiHandle;
  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
SetDateAndTimeUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
            DriverHandle,
            &gEfiDevicePathProtocolGuid,
            &mSetDateAndTimeHiiVendorDevicePath,
            &gEfiHiiConfigAccessProtocolGuid,
            &PrivateData->ConfigAccess,
            NULL
           );
    DriverHandle = NULL;
  }

  if (PrivateData->HiiHandle != NULL) {
    HiiRemovePackages (PrivateData->HiiHandle);
  }

  FreePool (PrivateData);
  PrivateData = NULL;

  return EFI_SUCCESS;
}
