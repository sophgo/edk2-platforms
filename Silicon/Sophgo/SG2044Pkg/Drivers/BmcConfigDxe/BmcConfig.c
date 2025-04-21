/** @file
 This driver implements a UEFI module for configuring the bmc lan
 parameters through a custom HII-based interface.

 Copyright (c) 2025  Sophgo Corporation. All rights reserved.<BR>
**/

#include "BmcConfig.h"
#include "BmcConfigIpmi.h"
#include "BmcConfigLan.h"
#include "BmcConfigMac.h"
#include "BmcConfigUser.h"
#include "BmcConfigOem.h"
#include "BmcConfigBasicInfo.h"

EFI_HANDLE        DriverHandle;
EFI_GUID          gBmcConfigFormSetGuid = BMC_FORMSET_GUID;
NET_PRIVATE_DATA *PrivateData = NULL;
BOOLEAN           IsFormatOpen = FALSE;

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
HII_VENDOR_DEVICE_PATH mBMCHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof(VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    BMC_FORMSET_GUID
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

EFI_STATUS
EFIAPI
UpdateBmcVarStore(
    IN  BMC_DATA             *BmcData
  )
{
  EFI_STATUS Status;
  Status = gRT->SetVariable(
      EFI_BMC_CONFIG_VARIABLE_NAME,
      &gEfiSophgoGlobalVariableGuid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
      sizeof(BMC_DATA),
      BmcData);
    if (EFI_ERROR(Status)) {
        DEBUG((
          DEBUG_ERROR,
          "SetVariable(%s) failed: %r\n",
          EFI_BMC_CONFIG_VARIABLE_NAME,
          Status
          ));
    }
  return Status;
}

VOID
EFIAPI
InitializeBmcVarstore(
    VOID
  )
{
  EFI_STATUS Status;
  UINTN VarSize;

  VarSize = sizeof(BMC_DATA);
  Status = gRT->GetVariable(
      EFI_BMC_CONFIG_VARIABLE_NAME,
      &gEfiSophgoGlobalVariableGuid,
      NULL,
      &VarSize,
      &PrivateData->BmcConfigData);
  if (Status == EFI_NOT_FOUND)
  {
    StrCpyS(PrivateData->BmcConfigData.IpAddress,
            sizeof(PrivateData->BmcConfigData.IpAddress) / sizeof(CHAR16),
            L"0.0.0.0");
    StrCpyS(PrivateData->BmcConfigData.SubnetMask,
            sizeof(PrivateData->BmcConfigData.SubnetMask) / sizeof(CHAR16),
            L"255.255.255.0");
    StrCpyS(PrivateData->BmcConfigData.Gateway,
            sizeof(PrivateData->BmcConfigData.Gateway) / sizeof(CHAR16),
            L"0.0.0.0");
    StrCpyS(PrivateData->BmcConfigData.UserName,
            sizeof(PrivateData->BmcConfigData.UserName) / sizeof(CHAR16),
            L"root");
    StrCpyS(PrivateData->BmcConfigData.FmVersion,
            sizeof(PrivateData->BmcConfigData.FmVersion) / sizeof(CHAR16),
            L"1.0");
    StrCpyS(PrivateData->BmcConfigData.IpmiVersion,
            sizeof(PrivateData->BmcConfigData.IpmiVersion) / sizeof(CHAR16),
            L"1.0");
    StrCpyS(PrivateData->BmcConfigData.MacAddr,
            sizeof(PrivateData->BmcConfigData.MacAddr) / sizeof(CHAR16),
            L"ff:ff:ff:ff:ff:ff");
    PrivateData->BmcConfigData.EnableDHCP = 1;
  }

  UpdateBmcVarStore(&PrivateData->BmcConfigData);
}



EFI_STATUS
EFIAPI
BmcConfigExtractConfig(
    IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
    IN CONST EFI_STRING Request,
    OUT EFI_STRING *Progress,
    OUT EFI_STRING *Results
  )
{
  EFI_STATUS Status;
  NET_PRIVATE_DATA *PrivateData;
  EFI_STRING ConfigRequestHdr;
  EFI_STRING ConfigRequest;
  BOOLEAN AllocatedRequest;
  UINTN BufferSize;

  if ((Progress == NULL) || (Results == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = BMC_PRIVATE_DATA_FROM_THIS(This);

  *Progress = Request;
  ConfigRequestHdr = NULL;
  ConfigRequest = NULL;
  AllocatedRequest = FALSE;
  BufferSize = sizeof(BMC_DATA);

  if (Request != NULL) {
    if(!HiiIsConfigHdrMatch(Request, &gEfiSophgoGlobalVariableGuid, EFI_BMC_CONFIG_VARIABLE_NAME)) {
      return EFI_NOT_FOUND;
    }
  }

  if (Request == NULL)
  {
    ConfigRequestHdr = HiiConstructConfigHdr(&gEfiSophgoGlobalVariableGuid, EFI_BMC_CONFIG_VARIABLE_NAME, PrivateData->DriverHandle);
    if (ConfigRequestHdr == NULL)
    {
      return EFI_OUT_OF_RESOURCES;
    }
    UINTN Size = (StrLen(ConfigRequestHdr) + 32 + 1) * sizeof(CHAR16);
    ConfigRequest = AllocateZeroPool(Size);
    if (ConfigRequest == NULL)
    {
      FreePool(ConfigRequestHdr);
      return EFI_OUT_OF_RESOURCES;
    }
    AllocatedRequest = TRUE;
    UnicodeSPrint(ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool(ConfigRequestHdr);
  }
  else
  {
    ConfigRequest = Request;
  }

  Status = gHiiConfigRouting->BlockToConfig(
      gHiiConfigRouting,
      ConfigRequest,
      (UINT8 *)&PrivateData->BmcConfigData,
      BufferSize,
      Results,
      Progress);

  if (AllocatedRequest)
  {
    FreePool(ConfigRequest);
  }

  if (EFI_ERROR(Status))
  {
    return Status;
  }

  if (Request == NULL)
  {
    *Progress = NULL;
  }
  else if (StrStr(Request, L"OFFSET") == NULL)
  {
    *Progress = Request + StrLen(Request);
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BmcConfigRouteConfig(
    IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
    IN CONST EFI_STRING Configuration,
    OUT EFI_STRING *Progress
  )
{
  EFI_STATUS Status;
  NET_PRIVATE_DATA *PrivateData;
  UINTN BufferSize;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *ConfigRouting;

  if ((Configuration == NULL) || (Progress == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;
  PrivateData = BMC_PRIVATE_DATA_FROM_THIS(This);

  Status = gBS->LocateProtocol(
      &gEfiHiiConfigRoutingProtocolGuid,
      NULL,
      (VOID **)&ConfigRouting);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "RouteConfig: Failed to locate HiiConfigRouting protocol: %r\n", Status));
    return Status;
  }

  if (!HiiIsConfigHdrMatch(Configuration, &gEfiSophgoGlobalVariableGuid, EFI_BMC_CONFIG_VARIABLE_NAME))
  {
    DEBUG((DEBUG_WARN, "RouteConfig: Configuration does not match ConfigHdr.\n"));
    return EFI_NOT_FOUND;
  }

  BufferSize = sizeof(BMC_DATA);
  Status = ConfigRouting->ConfigToBlock(
      ConfigRouting,
      Configuration,
      (UINT8 *)&PrivateData->BmcConfigData,
      &BufferSize,
      Progress);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "RouteConfig: ConfigToBlock failed. Status=%r\n", Status));
    return Status;
  }

  return Status;
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
EFI_STATUS
EFIAPI
DriverCallback(
    IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
    IN EFI_BROWSER_ACTION Action,
    IN EFI_QUESTION_ID QuestionId,
    IN UINT8 Type,
    IN EFI_IFR_TYPE_VALUE *Value,
    OUT EFI_BROWSER_ACTION_REQUEST *ActionRequest
  )
{
  EFI_STATUS                       Status;
  NET_PRIVATE_DATA                *Private;

  Private = BMC_PRIVATE_DATA_FROM_THIS(This);

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    if (!IsFormatOpen) {
      IsFormatOpen = TRUE;
      UpdateBmcLanConfigData(PrivateData);
      UpdateNetworkForm(PrivateData);
      UpdateBmcBasicInfo(PrivateData);
      UpdateBmcInfoForm(PrivateData);
      UpdateBmcMacInfo(PrivateData);
      UpdateBmcMacForm(PrivateData);
      UpdateBmcUserInfo(PrivateData);
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED)
  {
    if (ActionRequest != NULL)
    {
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    }

    switch (QuestionId)
    {
    case DHCP_QUESTION_ID:
      ProcessIpSourceSet(PrivateData, QuestionId, Value);
      break;
    case NETWORK_SET_IP_KEY_ID:
      Status = ProcessIpAddrSet(PrivateData, QuestionId, Value);
      break;
    case NETWORK_SET_SUBNET_KEY_ID:
      Status = ProcessSubnetMaskSet(PrivateData, QuestionId, Value);
      break;
    case NETWORK_SET_GATEWAY_KEY_ID:
      Status = ProcessGateWayAddrSet(PrivateData, QuestionId, Value);
      break;
    case MAC_ADDR_SET_KEY_ID:
      Status = ProcessMacAddrSet(PrivateData, QuestionId, Value);
      break;
    case REFRESH_QUESTION_ID:
      Status = UpdateBmcLanConfigData(PrivateData);
      UpdateNetworkForm(PrivateData);
      break;
    case BASIC_INFO_REFRESH_ID:
      Status = UpdateBmcBasicInfo(PrivateData);
      if (EFI_ERROR(Status)) {
        DEBUG((
          DEBUG_ERROR,
          "Failed to get BMC basic info: %r\n",
          Status
          ));
      }
      UpdateBmcInfoForm(PrivateData);
      break;
    case MAC_ADDR_REFRESH_ID:
      Status = UpdateBmcMacInfo(PrivateData);
      if (EFI_ERROR(Status))  {
        DEBUG((
          DEBUG_ERROR,
          "Failed to get BMC mac addr: %r\n",
          Status
          ));
      }
      UpdateBmcMacForm(PrivateData);
      break;
    case USER_REFRESH_ID:
      Status = UpdateBmcUserInfo(PrivateData);
      if (EFI_ERROR(Status))  {
        DEBUG((
          DEBUG_ERROR,
          "Failed to get BMC users info: %r\n",
          Status
          ));
      }
      // SendBmcUserForm(PrivateData);
      UpdateBmcUserForm(PrivateData);
      break;
    default:
      break;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGING) {
    if (QuestionId >= KEY_PASSWORD_1 && QuestionId <= KEY_PASSWORD_15) {
      Status = ProcessPasswordSet(PrivateData, QuestionId, Value);
    }
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
BmcConfigEntry(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_HII_HANDLE HiiHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;

  if (!IsServerProduct())
  {
    DEBUG((DEBUG_INFO, "%a: Non-server board detected. Skipping server-specific initialization.\n", __func__));
    return EFI_SUCCESS;
  }

  SendSmbiosOemToBmc();

  PrivateData = AllocateZeroPool(sizeof(NET_PRIVATE_DATA));
  if (PrivateData == NULL)
  {
    return EFI_OUT_OF_RESOURCES;
  }
  InitializeBmcVarstore();
  PrivateData->Signature = BMC_PRIVATE_SIGNATURE;
  PrivateData->ConfigAccess.ExtractConfig = BmcConfigExtractConfig;
  PrivateData->ConfigAccess.RouteConfig = BmcConfigRouteConfig;
  PrivateData->ConfigAccess.Callback = DriverCallback;

  Status = gBS->LocateProtocol(&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&HiiConfigRouting);
  if (EFI_ERROR(Status))
  {
    return Status;
  }
  PrivateData->HiiConfigRouting = HiiConfigRouting;
  Status = gBS->InstallMultipleProtocolInterfaces(
      &DriverHandle,
      &gEfiDevicePathProtocolGuid,
      &mBMCHiiVendorDevicePath,
      &gEfiHiiConfigAccessProtocolGuid,
      &PrivateData->ConfigAccess,
      NULL);
  ASSERT_EFI_ERROR(Status);
  PrivateData->DriverHandle = DriverHandle;

  HiiHandle = HiiAddPackages(
      &gBmcConfigFormSetGuid,
      DriverHandle,
      BMCStrings,
      BmcConfigVfrBin,
      NULL);
  if (HiiHandle == NULL)
  {
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
BmcConfigUnload(
    IN EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS Status;

  if (DriverHandle != NULL)
  {
    Status = gBS->UninstallMultipleProtocolInterfaces(
        DriverHandle,
        &gEfiDevicePathProtocolGuid,
        &mBMCHiiVendorDevicePath,
        &gEfiHiiConfigAccessProtocolGuid,
        &PrivateData->ConfigAccess,
        NULL);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "Failed to uninstall protocol interfaces: %r\n", Status));
    }
    DriverHandle = NULL;
  }

  if (PrivateData->HiiHandle != NULL)
  {
    HiiRemovePackages(PrivateData->HiiHandle);
    PrivateData->HiiHandle = NULL;
  }
  if (PrivateData != NULL)
  {
    FreePool(PrivateData);
    PrivateData = NULL;
  }
  return EFI_SUCCESS;
}
