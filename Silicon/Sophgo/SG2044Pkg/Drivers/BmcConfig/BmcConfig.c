/** @file
# This driver implements a UEFI module for configuring the system date and time through a custom HII-based interface.
#
# Copyright (c) 2024  Sophgo Corporation. All rights reserved.<BR>
**/

#include "BmcConfig.h"



EFI_HANDLE                  DriverHandle;
EFI_GUID  gBmcConfigFormSetGuid = BMC_FORMSET_GUID;
EFI_GUID  gBMCDataGuid = VAR_BMC_DATA_GUID;
NET_PRIVATE_DATA      *PrivateData = NULL;
SMBIOS_PARSED_DATA    *ParsedData = NULL;



#define SMBIO_INFO_MAX_LENGTH 64

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
HII_VENDOR_DEVICE_PATH  mBMCHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    BMC_FORMSET_GUID
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



BOOLEAN
IsStringStartsWithSRA (
  IN CHAR16 *Str)
{
  CHAR16 *Target = L"SRA";
  if (Str == NULL) {
      return FALSE;
  }
  if (StrnCmp(Str, Target, 3) == 0) {
      return TRUE;
  }
  return FALSE;
}



EFI_STATUS
EFIAPI
ConvertChar16ToIpmiLanIpAddress (
  IN  CHAR16                 *IpAddressStr,
  OUT UINT8                  *IpAddress
) {
  UINTN  Index = 0;
  UINTN  OctetValue = 0;

  if ((IpAddressStr == NULL) || (IpAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // ZeroMem(IpAddress, sizeof(IpAddress));

  while (*IpAddressStr != L'\0') {
    if ((*IpAddressStr >= L'0') && (*IpAddressStr <= L'9')) {
      OctetValue = OctetValue * 10 + (*IpAddressStr - L'0');
      if (OctetValue > 255) {
        return EFI_INVALID_PARAMETER;
      }
    } else if (*IpAddressStr == L'.') {
      if (Index >= 4) {
        return EFI_INVALID_PARAMETER;
      }
      IpAddress[Index++] = (UINT8)OctetValue;
      OctetValue = 0;
    } else {
      return EFI_INVALID_PARAMETER;
    }
    IpAddressStr++;
  }

  if (Index < 3) {
    return EFI_INVALID_PARAMETER;
  }
  IpAddress[Index] = (UINT8)OctetValue;

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
GetBmcIp (
  BMC_DATA *BmcData
) {
  EFI_STATUS              Status;
  UINT8                   AddrSrc;
  IPMI_LAN_IP_ADDRESS     BmcIpAddress;
  IPMI_LAN_SUBNET_MASK    BmcSubnetMask;
  IPMI_LAN_DEFAULT_GATEWAY BmcGateway;

  if (BmcData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetBmcLanInfo(&AddrSrc, &BmcIpAddress, &BmcSubnetMask, &BmcGateway);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to get BMC LAN information: %r\n", Status));
    return Status;
  }

  BmcData->EnableDHCP = AddrSrc;

  UnicodeSPrint (
      BmcData->IpAddress,
      sizeof (BmcData->IpAddress),
      L"%d.%d.%d.%d",
      BmcIpAddress.IpAddress[0],
      BmcIpAddress.IpAddress[1],
      BmcIpAddress.IpAddress[2],
      BmcIpAddress.IpAddress[3]
      );
  UnicodeSPrint (
      BmcData->SubnetMask,
      sizeof (BmcData->SubnetMask),
      L"%d.%d.%d.%d",
      BmcSubnetMask.IpAddress[0],
      BmcSubnetMask.IpAddress[1],
      BmcSubnetMask.IpAddress[2],
      BmcSubnetMask.IpAddress[3]
      );

  UnicodeSPrint (
      BmcData->Gateway,
      sizeof (BmcData->Gateway),
      L"%d.%d.%d.%d",
      BmcGateway.IpAddress[0],
      BmcGateway.IpAddress[1],
      BmcGateway.IpAddress[2],
      BmcGateway.IpAddress[3]
      );

  Status = gRT->SetVariable(
    L"BMCConfigData",
    &gBMCDataGuid,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof(BMC_DATA),
    BmcData
  );

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to store BMC LAN information in UEFI variable: %r\n", Status));
  }

  return Status;
}

EFI_STATUS
EFIAPI
SetBmcIp (
  BMC_DATA *BmcData
) {
  EFI_STATUS              Status;
  UINT8                   AddrSrc;
  IPMI_LAN_IP_ADDRESS     BmcIpAddress;
  IPMI_LAN_SUBNET_MASK    BmcSubnetMask;
  IPMI_LAN_DEFAULT_GATEWAY BmcGateway;

  if (BmcData == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  AddrSrc = BmcData->EnableDHCP;
  Status = ConvertChar16ToIpmiLanIpAddress(BmcData->IpAddress, BmcIpAddress.IpAddress);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Invalid IP Address format.\n"));
    return Status;
  }

  Status = ConvertChar16ToIpmiLanIpAddress(BmcData->SubnetMask, BmcSubnetMask.IpAddress);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Invalid Subnet Mask format.\n"));
    return Status;
  }

  Status = ConvertChar16ToIpmiLanIpAddress(BmcData->Gateway, BmcGateway.IpAddress);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Invalid Gateway Address format.\n"));
    return Status;
  }

  Status = SetBmcLanInfo(&AddrSrc, &BmcIpAddress, &BmcSubnetMask, &BmcGateway);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to set BMC LAN info: %r\n", Status));
    return Status;
  }

  Status = gRT->SetVariable(
    L"BMCConfigData",
    &gBMCDataGuid,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof(BMC_DATA),
    BmcData
  );

  return EFI_SUCCESS;
}

VOID
EFIAPI
InitializeBmcVarstore (
  VOID
  )
{
  EFI_STATUS Status;
  UINTN      VarSize;

  VarSize = sizeof(BMC_DATA);
  Status = gRT->GetVariable(
                  L"BMCConfigData",
                  &gBMCDataGuid,
                  NULL,
                  &VarSize,
                  &PrivateData->BmcConfigData
                );
  if (Status == EFI_NOT_FOUND) {
    StrCpyS(PrivateData->BmcConfigData.IpAddress,
        sizeof(PrivateData->BmcConfigData.IpAddress) / sizeof(CHAR16),
        L"0.0.0.0");
    StrCpyS(PrivateData->BmcConfigData.SubnetMask,
        sizeof(PrivateData->BmcConfigData.SubnetMask) / sizeof(CHAR16),
        L"255.255.255.0");
    StrCpyS(PrivateData->BmcConfigData.Gateway,
        sizeof(PrivateData->BmcConfigData.Gateway) / sizeof(CHAR16),
        L"0.0.0.0");
    PrivateData->BmcConfigData.EnableDHCP = 1;
  }

  gRT->SetVariable(
          L"BMCConfigData",
          &gBMCDataGuid,
          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
          sizeof(BMC_DATA),
          &PrivateData->BmcConfigData
    );
}

BOOLEAN
IsValidIpAndGateway (
  IN CHAR16 *Ip
  )
{
  UINTN OctetCount = 0;
  UINTN OctetValue = 0;
  CHAR16 *Current = Ip;
  if (Ip == NULL || StrLen(Ip) == 0) {
    return FALSE;
  }

  while (*Current != L'\0') {
    if (*Current == L'.') {
      if (OctetValue > 255) {
        return FALSE;
      }
      OctetCount++;
      OctetValue = 0;
    } else if (*Current >= L'0' && *Current <= L'9') {
      OctetValue = OctetValue * 10 + (*Current - L'0');
      if (OctetValue > 255) {
        return FALSE;
      }
    } else {
      return FALSE;
    }
    Current++;
  }

  if (OctetValue > 255) {
    return FALSE;
  }
  OctetCount++;

  return (OctetCount == 4);
}

BOOLEAN
IsValidSubnetMask (
  IN CHAR16 *SubnetMask
  )
{
  UINT32 Mask = 0;
  CHAR16 *Current = SubnetMask;
  if (SubnetMask == NULL || StrLen(SubnetMask) == 0) {
    return FALSE;
  }
  if (!IsValidIpAndGateway(SubnetMask)) {
    return FALSE;
  }
  while (*Current != L'\0') {
    Mask = (Mask << 8) | (UINT32)StrDecimalToUintn(Current);
    while (*Current != L'.' && *Current != L'\0') {
      Current++;
    }
    if (*Current == L'.') {
      Current++;
    }
  }
  UINT32 InverseMask = ~Mask;
  if ((InverseMask & (InverseMask + 1)) != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Update the network configuration form dynamically to display IP, Subnet, and Gateway.

  This function first synchronizes Varstore with the latest BMC configuration,
  then retrieves the values for IP Address, Subnet Mask, and Gateway from Varstore,
  and updates the HII form dynamically.

  @retval EFI_SUCCESS          The form was updated successfully.
  @retval Others               An error occurred while updating the form.
**/
VOID
UpdateNetworkConfigForm (
  VOID
  )
{
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartGuidLabel;
  EFI_IFR_GUID_LABEL  *EndGuidLabel;
  CHAR16              IpAddress[16];
  CHAR16              SubnetMask[16];
  CHAR16              Gateway[16];
  EFI_STATUS          Status;
  EFI_STRING_ID       IpAddressId, SubnetMaskId, GatewayId;
#ifndef BMC_TEST_PHASE
  Status = GetBmcIp(&PrivateData->BmcConfigData);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to get BMC data: %r\n", Status));
    return;
  }
#endif
  UINTN VarSize = sizeof(BMC_DATA);
  Status = gRT->GetVariable(
                  L"BMCConfigData",
                  &gBMCDataGuid,
                  NULL,
                  &VarSize,
                  &PrivateData->BmcConfigData
                );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to retrieve Varstore data: %r\n", Status));
    return;
  }

  StrCpyS(IpAddress, sizeof(IpAddress) / sizeof(CHAR16), PrivateData->BmcConfigData.IpAddress);
  StrCpyS(SubnetMask, sizeof(SubnetMask) / sizeof(CHAR16), PrivateData->BmcConfigData.SubnetMask);
  StrCpyS(Gateway, sizeof(Gateway) / sizeof(CHAR16), PrivateData->BmcConfigData.Gateway);

  StartOpCodeHandle = HiiAllocateOpCodeHandle();
  if (StartOpCodeHandle == NULL) {
    DEBUG((DEBUG_ERROR, "Failed to allocate StartOpCodeHandle\n"));
    return;
  }

  EndOpCodeHandle = HiiAllocateOpCodeHandle();
  if (EndOpCodeHandle == NULL) {
    DEBUG((DEBUG_ERROR, "Failed to allocate EndOpCodeHandle\n"));
    HiiFreeOpCodeHandle(StartOpCodeHandle);
    return;
  }

  StartGuidLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(
                     StartOpCodeHandle,
                     &gEfiIfrTianoGuid,
                     NULL,
                     sizeof(EFI_IFR_GUID_LABEL)
                   );
  if (StartGuidLabel == NULL) {
    DEBUG((DEBUG_ERROR, "Failed to create StartGuidLabel\n"));
    HiiFreeOpCodeHandle(StartOpCodeHandle);
    HiiFreeOpCodeHandle(EndOpCodeHandle);
    return;
  }
  StartGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartGuidLabel->Number       = LABEL_START;

  EndGuidLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(
                   EndOpCodeHandle,
                   &gEfiIfrTianoGuid,
                   NULL,
                   sizeof(EFI_IFR_GUID_LABEL)
                 );
  if (EndGuidLabel == NULL) {
    DEBUG((DEBUG_ERROR, "Failed to create EndGuidLabel\n"));
    HiiFreeOpCodeHandle(StartOpCodeHandle);
    HiiFreeOpCodeHandle(EndOpCodeHandle);
    return;
  }
  EndGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndGuidLabel->Number       = LABEL_END;

  IpAddressId = HiiSetString(PrivateData->HiiHandle, 0, IpAddress, NULL);
  SubnetMaskId = HiiSetString(PrivateData->HiiHandle, 0, SubnetMask, NULL);
  GatewayId = HiiSetString(PrivateData->HiiHandle, 0, Gateway, NULL);

  if (IpAddressId == 0 || SubnetMaskId == 0 || GatewayId == 0) {
    DEBUG((DEBUG_ERROR, "Failed to generate HII strings.\n"));
    return;
  }
  HiiCreateTextOpCode(StartOpCodeHandle, STRING_TOKEN(STR_IP_PROMPT), STRING_TOKEN(STR_IP_HELP), IpAddressId);
  HiiCreateTextOpCode(StartOpCodeHandle, STRING_TOKEN(STR_SUBNET_PROMPT), STRING_TOKEN(STR_SUBNET_HELP), SubnetMaskId);
  HiiCreateTextOpCode(StartOpCodeHandle, STRING_TOKEN(STR_GATEWAY_PROMPT), STRING_TOKEN(STR_GATEWAY_HELP), GatewayId);
  Status = HiiUpdateForm(
             PrivateData->HiiHandle,
             &gBmcConfigFormSetGuid,
             NETWORK_GET_FORM_ID,
             StartOpCodeHandle,
             EndOpCodeHandle
           );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to update network config form: %r\n", Status));
  }
  HiiFreeOpCodeHandle(StartOpCodeHandle);
  HiiFreeOpCodeHandle(EndOpCodeHandle);
}

VOID
UpdateNetworkForm (
 VOID
)
{
  EFI_STATUS Status;
  VOID *StartOpCodeHandle;
  VOID *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL *StartLabel;
  EFI_IFR_GUID_LABEL *EndLabel;
  EFI_STRING_ID IpStringId, SubnetStringId, GatewayStringId;
  // UINTN varsize = sizeof(BMC_DATA);
#ifndef BMC_TEST_PHASE
  Status = GetBmcIp(&PrivateData->BmcConfigData);
#endif
  StartOpCodeHandle = HiiAllocateOpCodeHandle();
  EndOpCodeHandle = HiiAllocateOpCodeHandle();

  if (StartOpCodeHandle == NULL || EndOpCodeHandle == NULL) {
    DEBUG((DEBUG_ERROR, "Failed to allocate opcode handles.\n"));
    return;
  }
  StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(
                  StartOpCodeHandle,
                  &gEfiIfrTianoGuid,
                  NULL,
                  sizeof(EFI_IFR_GUID_LABEL)
               );
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number = LABEL_SET;

  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(
                EndOpCodeHandle,
                &gEfiIfrTianoGuid,
                NULL,
                sizeof(EFI_IFR_GUID_LABEL)
             );
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number = LABEL_END;
  IpStringId = HiiSetString(PrivateData->HiiHandle, 0, PrivateData->BmcConfigData.IpAddress, NULL);
  SubnetStringId = HiiSetString(PrivateData->HiiHandle, 0, PrivateData->BmcConfigData.SubnetMask, NULL);
  GatewayStringId = HiiSetString(PrivateData->HiiHandle, 0, PrivateData->BmcConfigData.Gateway, NULL);

  if (IpStringId == 0 || SubnetStringId == 0 || GatewayStringId == 0) {
    DEBUG((DEBUG_ERROR, "Failed to set dynamic strings.\n"));
    HiiFreeOpCodeHandle(StartOpCodeHandle);
    HiiFreeOpCodeHandle(EndOpCodeHandle);
    return;
  }

  HiiCreateStringOpCode(
    StartOpCodeHandle,
    NETWORK_SET_IP_KEY_ID,
    VAR_BMC_VARID,
    OFFSET_OF(BMC_DATA, IpAddress),
    STRING_TOKEN(STR_IP_PROMPT),
    STRING_TOKEN(STR_IP_HELP),
    EFI_IFR_FLAG_CALLBACK,
    0,
    7,
    15,
    NULL
  );

  HiiCreateStringOpCode(
    StartOpCodeHandle,
    NETWORK_SET_SUBNET_KEY_ID,
    VAR_BMC_VARID,
    OFFSET_OF(BMC_DATA, SubnetMask),
    STRING_TOKEN(STR_SUBNET_PROMPT),
    STRING_TOKEN(STR_SUBNET_HELP),
    EFI_IFR_FLAG_CALLBACK,
    0,
    7,
    15,
    NULL
  );

  HiiCreateStringOpCode(
    StartOpCodeHandle,
    NETWORK_SET_GATEWAY_KEY_ID,
    VAR_BMC_VARID,
    OFFSET_OF(BMC_DATA, Gateway),
    STRING_TOKEN(STR_GATEWAY_PROMPT),
    STRING_TOKEN(STR_GATEWAY_HELP),
    EFI_IFR_FLAG_CALLBACK,
    0,
    7,
    15,
    NULL
  );

  Status = HiiUpdateForm(
             PrivateData->HiiHandle,
             &gBmcConfigFormSetGuid,
             NETWORK_SET_FORM_ID,
             StartOpCodeHandle,
             EndOpCodeHandle
           );

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to update the form: %r\n", Status));
  }

  HiiFreeOpCodeHandle(StartOpCodeHandle);
  HiiFreeOpCodeHandle(EndOpCodeHandle);
}

EFI_STATUS
EFIAPI
BmcConfigExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS        Status;
  NET_PRIVATE_DATA *PrivateData;
  EFI_STRING        ConfigRequestHdr;
  EFI_STRING        ConfigRequest;
  BOOLEAN           AllocatedRequest;
  UINTN             BufferSize;

  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = BMC_PRIVATE_DATA_FROM_THIS(This);

  *Progress = Request;
  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  BufferSize       = sizeof(BMC_DATA);

  if ((Request != NULL) && !HiiIsConfigHdrMatch(Request, &gBmcConfigFormSetGuid, L"BMCConfigData")) {
    return EFI_NOT_FOUND;
  }

  if (Request == NULL) {
    ConfigRequestHdr = HiiConstructConfigHdr(&gBmcConfigFormSetGuid, L"BMCConfigData", PrivateData->DriverHandle);
    if (ConfigRequestHdr == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    UINTN Size = (StrLen(ConfigRequestHdr) + 32 + 1) * sizeof(CHAR16);
    ConfigRequest = AllocateZeroPool(Size);
    if (ConfigRequest == NULL) {
      FreePool(ConfigRequestHdr);
      return EFI_OUT_OF_RESOURCES;
    }

    AllocatedRequest = TRUE;
    UnicodeSPrint(ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool(ConfigRequestHdr);
  } else {
    ConfigRequest = Request;
  }

  Status = gHiiConfigRouting->BlockToConfig(
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *)&PrivateData->BmcConfigData,
                                BufferSize,
                                Results,
                                Progress
                              );

  if (AllocatedRequest) {
    FreePool(ConfigRequest);
  }

  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr(Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen(Request);
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BmcConfigRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS        Status;
  NET_PRIVATE_DATA *PrivateData;
  UINTN             BufferSize;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *ConfigRouting;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;
  PrivateData = BMC_PRIVATE_DATA_FROM_THIS(This);

  Status = gBS->LocateProtocol(
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **)&ConfigRouting
                  );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "RouteConfig: Failed to locate HiiConfigRouting protocol: %r\n", Status));
    return Status;
  }

  if (!HiiIsConfigHdrMatch(Configuration, &gBmcConfigFormSetGuid, L"BMCConfigData")) {
    DEBUG((DEBUG_WARN, "RouteConfig: Configuration does not match ConfigHdr.\n"));
    return EFI_NOT_FOUND;
  }

  BufferSize = sizeof(BMC_DATA);
  Status = ConfigRouting->ConfigToBlock(
                             ConfigRouting,
                             Configuration,
                             (UINT8 *)&PrivateData->BmcConfigData,
                             &BufferSize,
                             Progress
                             );
  if (EFI_ERROR(Status)) {
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
) {
  EFI_STATUS Status;
  UINTN VarSize;
  CHAR16 *NewString;
  if (Action != EFI_BROWSER_ACTION_CHANGED) {
    return EFI_UNSUPPORTED;
  }
  if (ActionRequest != NULL) {
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  }
  VarSize = sizeof(BMC_DATA);
  Status = gRT->GetVariable(
    L"BMCConfigData",
    &gBMCDataGuid,
    NULL,
    &VarSize,
    &PrivateData->BmcConfigData
  );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to get Varstore data: %r\n", Status));
    return Status;
  }
  switch (QuestionId) {
    case DHCP_QUESTION_ID:
      PrivateData->BmcConfigData.EnableDHCP = Value->u8;
      Status = SetBmcIp(&PrivateData->BmcConfigData);
      if (EFI_ERROR(Status)) {
          DEBUG((DEBUG_ERROR, "Failed to update BMC with IP Address: %r\n", Status));
      }
      // if (PrivateData->BmcConfigData.EnableDHCP == 0x02) {
      //   Status = GetBmcIp(&PrivateData->BmcConfigData);
      //   if (EFI_ERROR(Status)) {
      //     DEBUG((DEBUG_ERROR, "Failed to fetch DHCP configuration. Retaining static configuration.\n"));
      //     return Status;
      //   }
      // }
      // gRT->SetVariable(
      //     L"BMCConfigData",
      //     &gBMCDataGuid,
      //     EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
      //     sizeof(BMC_DATA),
      //     &PrivateData->BmcConfigData
      // );
      break;
    case NETWORK_SET_IP_KEY_ID:
    NewString = HiiGetString(PrivateData->HiiHandle, Value->string, NULL);
    if (NewString == NULL) {
        DEBUG((DEBUG_ERROR, "Failed to retrieve string for IP Address.\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    if (!IsValidIpAndGateway(NewString)) {
        DEBUG((DEBUG_ERROR, "Invalid IP Address: %s\n", NewString));
#ifdef BMC_TEST_PHASE
        Status = GetBmcIp(&PrivateData->BmcConfigData);
        if (EFI_ERROR(Status)) {
          DEBUG((DEBUG_ERROR, "Failed to get BMC data: %r\n", Status));
        }
        UpdateNetworkForm();
#else
        UpdateNetworkForm();
#endif
        CreatePopUp(
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            NULL,
            L"Invalid IP Address!",
            L"The entered IP address is not valid.",
            L"Ensure the value is true",
            NULL
        );
        gBS->Stall(1000000);
        FreePool(NewString);
        return EFI_INVALID_PARAMETER;
    }
    StrCpyS(PrivateData->BmcConfigData.IpAddress,
            sizeof(PrivateData->BmcConfigData.IpAddress) / sizeof(CHAR16),
            NewString);
    FreePool(NewString);

    Status = SetBmcIp(&PrivateData->BmcConfigData);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to update BMC with IP Address: %r\n", Status));
    }
    break;

    case NETWORK_SET_SUBNET_KEY_ID:
        NewString = HiiGetString(PrivateData->HiiHandle, Value->string, NULL);
        if (NewString == NULL) {
            DEBUG((DEBUG_ERROR, "Failed to retrieve string for Subnet Mask.\n"));
            return EFI_OUT_OF_RESOURCES;
        }
        if (!IsValidSubnetMask(NewString)) {
            DEBUG((DEBUG_ERROR, "Invalid Subnet Mask: %s\n", NewString));
#ifdef BMC_TEST_PHASE
        Status = GetBmcIp(&PrivateData->BmcConfigData);
        if (EFI_ERROR(Status)) {
          DEBUG((DEBUG_ERROR, "Failed to get BMC data: %r\n", Status));
        }
        UpdateNetworkForm();
#else
        UpdateNetworkForm();
#endif
            CreatePopUp(
                EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                NULL,
                L"Invalid SUBNET Address!",
                L"The entered SUBNET address is not valid.",
                L"Ensure the value is true",
                NULL
            );
            gBS->Stall(1000000);
            FreePool(NewString);
            return EFI_INVALID_PARAMETER;
        }
        StrCpyS(PrivateData->BmcConfigData.SubnetMask,
                sizeof(PrivateData->BmcConfigData.SubnetMask) / sizeof(CHAR16),
                NewString);
        FreePool(NewString);
        Status = SetBmcIp(&PrivateData->BmcConfigData);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Failed to update BMC with Subnet Mask: %r\n", Status));
        }
        break;

    case NETWORK_SET_GATEWAY_KEY_ID:
        NewString = HiiGetString(PrivateData->HiiHandle, Value->string, NULL);
        if (NewString == NULL) {
            DEBUG((DEBUG_ERROR, "Failed to retrieve string for Gateway Address.\n"));
            return EFI_OUT_OF_RESOURCES;
        }

        if (!IsValidIpAndGateway(NewString)) {
            DEBUG((DEBUG_ERROR, "Invalid Gateway Address: %s\n", NewString));
#ifdef BMC_TEST_PHASE
            Status = GetBmcIp(&PrivateData->BmcConfigData);
            if (EFI_ERROR(Status)) {
              DEBUG((DEBUG_ERROR, "Failed to get BMC data: %r\n", Status));
            }
            UpdateNetworkForm();
#else
            UpdateNetworkForm();
#endif
            CreatePopUp(
                EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                NULL,
                L"Invalid SUBNET Address!",
                L"The entered SUBNET address is not valid.",
                L"Ensure the value is true",
                NULL
            );
            gBS->Stall(1000000);
            FreePool(NewString);
            return EFI_INVALID_PARAMETER;
        }
        StrCpyS(PrivateData->BmcConfigData.Gateway,
                sizeof(PrivateData->BmcConfigData.Gateway) / sizeof(CHAR16),
                NewString);
        FreePool(NewString);

        Status = SetBmcIp(&PrivateData->BmcConfigData);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Failed to update BMC with Gateway Address: %r\n", Status));
        }
        break;

    case REFRESH_QUESTION_ID:
#ifdef BMC_TEST_PHASE
      Status = GetBmcIp(&PrivateData->BmcConfigData);
      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to get BMC data: %r\n", Status));
      }
      UpdateNetworkConfigForm();
#else
      UpdateNetworkConfigForm();
#endif
    break;

    default:
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
BmcConfigEntry (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;
  BOOLEAN                         IsServerBoard;

  ParsedData = AllocSmbiosData();
  if (ParsedData == NULL) {
    DEBUG((DEBUG_ERROR, "%a :Failed to alloc smbios data\n",__func__));
    return EFI_OUT_OF_RESOURCES;
  }

  IsServerBoard = IsStringStartsWithSRA(ParsedData->BaseboardProductName);

  if (!IsServerBoard) {
    DEBUG((DEBUG_INFO, "Not server board\n",__func__));
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData = AllocateZeroPool (sizeof (NET_PRIVATE_DATA));
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  InitializeBmcVarstore();
  PrivateData->Signature = BMC_PRIVATE_SIGNATURE;
  PrivateData->ConfigAccess.ExtractConfig =BmcConfigExtractConfig;
  PrivateData->ConfigAccess.RouteConfig = BmcConfigRouteConfig;
  PrivateData->ConfigAccess.Callback = DriverCallback;

  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **) &HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiConfigRouting = HiiConfigRouting;
  Status = gBS->InstallMultipleProtocolInterfaces(
                  &DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mBMCHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &PrivateData->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  PrivateData->DriverHandle = DriverHandle;

  HiiHandle = HiiAddPackages (
                   &gBmcConfigFormSetGuid,
                   DriverHandle,
                   BMCStrings,
                   BmcConfigVfrBin,
                   NULL
                   );
  if (HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  PrivateData->HiiHandle = HiiHandle;
  UpdateNetworkConfigForm();
  UpdateNetworkForm();
  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
BMCUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS Status;

  if (DriverHandle != NULL) {
    Status = gBS->UninstallMultipleProtocolInterfaces(
                    DriverHandle,
                    &gEfiDevicePathProtocolGuid,
                    &mBMCHiiVendorDevicePath,
                    &gEfiHiiConfigAccessProtocolGuid,
                    &PrivateData->ConfigAccess,
                    NULL
                  );
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Failed to uninstall protocol interfaces: %r\n", Status));
    }
    DriverHandle = NULL;
  }

  if (PrivateData->HiiHandle != NULL) {
    HiiRemovePackages(PrivateData->HiiHandle);
    PrivateData->HiiHandle = NULL;
  }
  if (PrivateData != NULL) {
    FreePool(PrivateData);
    PrivateData = NULL;
  }
  if (ParsedData != NULL) {
    FreeSmbiosData(ParsedData);
    ParsedData = NULL;
  }
  return EFI_SUCCESS;
}

