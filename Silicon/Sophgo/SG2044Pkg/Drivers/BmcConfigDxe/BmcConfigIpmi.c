/** @file
  This file defines the interface function required by the BmcLanConfig driver.

  Copyright (c) 2025  Sophgo Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "BmcConfig.h"
#include "BmcConfigIpmi.h"
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiLib.h>
#include <Library/HiiLib.h>
#include <IndustryStandard/Ipmi.h>
#include <AutoGen.h>


EFI_STATUS
EFIAPI
IpmiGetBmcMacAddr (
  OUT IPMI_LAN_MAC_ADDRESS      *BmcMacAddr
  )
{
  EFI_STATUS                                      Status;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_REQUEST   GetConfigurationParametersRequest;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE  *GetConfigurationParametersResponse;
  UINT32                                          ResponseSize;

  if (BmcMacAddr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  GetConfigurationParametersResponse = AllocateZeroPool (
                                         sizeof (*GetConfigurationParametersResponse)
                                         + sizeof (IPMI_LAN_IP_ADDRESS)
                                         );
  if (GetConfigurationParametersResponse == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Get Mac Address
  //
  ZeroMem (&GetConfigurationParametersRequest, sizeof (GetConfigurationParametersRequest));
  GetConfigurationParametersRequest.ChannelNumber.Uint8 = BMC_IPMI_CHANNEL_NO;
  GetConfigurationParametersRequest.ParameterSelector   = IpmiLanMacAddress;
  GetConfigurationParametersRequest.SetSelector         = 0;
  GetConfigurationParametersRequest.BlockSelector       = 0;

  ResponseSize = sizeof (*GetConfigurationParametersResponse) + sizeof (IPMI_LAN_IP_ADDRESS);

  Status = IpmiGetLanConfigurationParameters (&GetConfigurationParametersRequest, GetConfigurationParametersResponse, &ResponseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get the LAN configuration parameter\n", __func__));
    goto Exit;
  }

  if (GetConfigurationParametersResponse->CompletionCode != IPMI_COMP_CODE_NORMAL) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  CopyMem (
    BmcMacAddr->MacAddress,
    GetConfigurationParametersResponse->ParameterData,
    sizeof (IPMI_LAN_MAC_ADDRESS)
    );

Exit:
  FreePool (GetConfigurationParametersResponse);
  return Status;
}


EFI_STATUS
IpmiSetBmcMacAddrUnlock (
  VOID
  )
{
  UINT8       Commanddata[20];
  UINT8       Commanddatasize;
  UINT8       Response[20];
  UINT32       Responsesize;
  EFI_STATUS  Status = EFI_SUCCESS;

  ZeroMem (Commanddata, 20);
  ZeroMem (Response, 20);
  //
  // Set MAC Source Unlock
  //
  Commanddata[0] = BMC_IPMI_CHANNEL_NO;      // Channel number 0x01
  Commanddata[1] = 0xC2;                     // Parameter selector OEM define

  Commanddatasize = 2;
  Responsesize    = 1;

  Status = IpmiSubmitCommand (
            IPMI_NETFN_TRANSPORT,           // NetFunction 0x0c
            IPMI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,     // Command 0x01
            (UINT8 *) &Commanddata[0],  // *CommandData
            Commanddatasize,            // CommandDataSize
            (UINT8 *) &Response,        // *ResponseData
            (UINT32 *) &Responsesize     // *ResponseDataSize
            );
  return Status;
}

EFI_STATUS
IpmiSetBmcMacAddr (
  IN OUT UINT8       *MacAddr
  )
{
  UINT8       Commanddata[20];
  UINT8       Commanddatasize;
  UINT8       Response[20];
  UINT32       Responsesize;
  EFI_STATUS  Status;

  ZeroMem (Commanddata, 20);
  ZeroMem (Response, 20);
  //
  // Set IP Source
  //
  Commanddata[0] = BMC_IPMI_CHANNEL_NO;               // Channel number
  Commanddata[1] = IpmiLanMacAddress;        // Parameter selector
  CopyMem (&Commanddata[2], &MacAddr[0], 6);
  Commanddatasize = 8;
  Responsesize    = 10;

  Status = IpmiSubmitCommand (
            IPMI_NETFN_TRANSPORT,           // NetFunction
            IPMI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,     // Command
            (UINT8 *) &Commanddata[0],  // *CommandData
            Commanddatasize,            // CommandDataSize
            (UINT8 *) &Response,        // *ResponseData
            (UINT32 *) &Responsesize     // *ResponseDataSize
            );
  return Status;
}

EFI_STATUS
EFIAPI
IpmiGetBmcLanInfo (
  OUT UINT8                    *pAddrSrc,
  OUT IPMI_LAN_IP_ADDRESS      *BmcIpAddress,
  OUT IPMI_LAN_SUBNET_MASK     *BmcSubnetMask,
  OUT IPMI_LAN_DEFAULT_GATEWAY *BmcDefaultGateWay
  )
{
  EFI_STATUS                                      Status;
  // IPMI_GET_CHANNEL_INFO_REQUEST                   GetChannelInfoRequest;
  // IPMI_GET_CHANNEL_INFO_RESPONSE                  GetChannelInfoResponse;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_REQUEST   GetConfigurationParametersRequest;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE  *GetConfigurationParametersResponse;
  UINT32                                          ResponseSize;
  UINT8                                           BmcChannel;

  if ((pAddrSrc == NULL) || (BmcIpAddress == NULL) || (BmcSubnetMask == NULL) || (BmcDefaultGateWay == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BmcChannel = BMC_IPMI_CHANNEL_NO;

  GetConfigurationParametersResponse = AllocateZeroPool (
                                         sizeof (*GetConfigurationParametersResponse)
                                         + sizeof (IPMI_LAN_IP_ADDRESS)
                                         );
  if (GetConfigurationParametersResponse == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get LAN Address Source
  //
  ZeroMem (&GetConfigurationParametersRequest, sizeof (GetConfigurationParametersRequest));
  GetConfigurationParametersRequest.ChannelNumber.Uint8 = BmcChannel;
  GetConfigurationParametersRequest.ParameterSelector   = IpmiLanIpAddressSource;
  GetConfigurationParametersRequest.SetSelector         = 0;
  GetConfigurationParametersRequest.BlockSelector       = 0;

  ResponseSize = sizeof (*GetConfigurationParametersResponse) + sizeof (UINT8);

  Status = IpmiGetLanConfigurationParameters (&GetConfigurationParametersRequest, GetConfigurationParametersResponse, &ResponseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get the LAN ip source\n", __func__));
    goto Exit;
  }

  if (GetConfigurationParametersResponse->CompletionCode != IPMI_COMP_CODE_NORMAL) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  CopyMem (
    pAddrSrc,
    GetConfigurationParametersResponse->ParameterData,
    sizeof (UINT8)
    );

  //
  // Get LAN IP Address
  //
  ZeroMem (&GetConfigurationParametersRequest, sizeof (GetConfigurationParametersRequest));
  GetConfigurationParametersRequest.ChannelNumber.Uint8 = BmcChannel;
  GetConfigurationParametersRequest.ParameterSelector   = IpmiLanIpAddress;
  GetConfigurationParametersRequest.SetSelector         = 0;
  GetConfigurationParametersRequest.BlockSelector       = 0;

  ResponseSize = sizeof (*GetConfigurationParametersResponse) + sizeof (IPMI_LAN_IP_ADDRESS);

  Status = IpmiGetLanConfigurationParameters (&GetConfigurationParametersRequest, GetConfigurationParametersResponse, &ResponseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get the LAN configuration parameter\n", __func__));
    goto Exit;
  }

  if (GetConfigurationParametersResponse->CompletionCode != IPMI_COMP_CODE_NORMAL) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  CopyMem (
    BmcIpAddress->IpAddress,
    GetConfigurationParametersResponse->ParameterData,
    sizeof (IPMI_LAN_IP_ADDRESS)
    );

  //
  // Get Subnet Mask
  //
  ZeroMem (&GetConfigurationParametersRequest, sizeof (GetConfigurationParametersRequest));
  GetConfigurationParametersRequest.ChannelNumber.Uint8 = BmcChannel;
  GetConfigurationParametersRequest.ParameterSelector   = IpmiLanSubnetMask;
  GetConfigurationParametersRequest.SetSelector         = 0;
  GetConfigurationParametersRequest.BlockSelector       = 0;

  ResponseSize = sizeof (*GetConfigurationParametersResponse) + sizeof (IPMI_LAN_SUBNET_MASK);

  Status = IpmiGetLanConfigurationParameters (&GetConfigurationParametersRequest, GetConfigurationParametersResponse, &ResponseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get the LAN configuration parameter\n", __func__));
    goto Exit;
  }

  if (GetConfigurationParametersResponse->CompletionCode != IPMI_COMP_CODE_NORMAL) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  CopyMem (
    BmcSubnetMask->IpAddress,
    GetConfigurationParametersResponse->ParameterData,
    sizeof (IPMI_LAN_SUBNET_MASK)
    );

  //
  // Get default gateway
  //
  ZeroMem (&GetConfigurationParametersRequest, sizeof (GetConfigurationParametersRequest));
  GetConfigurationParametersRequest.ChannelNumber.Uint8 = BmcChannel;
  GetConfigurationParametersRequest.ParameterSelector   = IpmiLanDefaultGateway;
  GetConfigurationParametersRequest.SetSelector         = 0;
  GetConfigurationParametersRequest.BlockSelector       = 0;

  ResponseSize = sizeof (*GetConfigurationParametersResponse) + sizeof (IPMI_LAN_SUBNET_MASK);

  Status = IpmiGetLanConfigurationParameters (&GetConfigurationParametersRequest, GetConfigurationParametersResponse, &ResponseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get the LAN configuration parameter\n", __func__));
    goto Exit;
  }

  if (GetConfigurationParametersResponse->CompletionCode != IPMI_COMP_CODE_NORMAL) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  CopyMem (
    BmcDefaultGateWay->IpAddress,
    GetConfigurationParametersResponse->ParameterData,
    sizeof (IPMI_LAN_SUBNET_MASK)
    );

Exit:
  FreePool (GetConfigurationParametersResponse);
  return Status;
}

EFI_STATUS
SetIpParam (
  IN     UINT8          Channel,
  IN     UINT8          LanParameter,
  IN OUT UINT8          *IpAddress
  )
{
  UINT8       Commanddata[20];
  UINT8       Commanddatasize;
  UINT8       Response[10];
  UINT32       Responsesize;
  EFI_STATUS  Status;


  Commanddata[0]  = Channel;
  Commanddata[1]  = LanParameter;
  CopyMem (&Commanddata[2], &IpAddress[0], 4);
  Commanddatasize = 6;
  Responsesize = 10;

  Status = IpmiSubmitCommand (
            IPMI_NETFN_TRANSPORT,
            IPMI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,
            (UINT8 *) &Commanddata[0],
            Commanddatasize,
            (UINT8 *) &Response,
            (UINT32 *) &Responsesize
            );

  return Status;
}

EFI_STATUS
IpmiSetBmcLanIpSrc (
  IN UINT8                     *pAddrSrc
  )
{
  UINT8       Commanddata[20];
  UINT8       Commanddatasize;
  UINT8       Response[20];
  UINT32       Responsesize;
  EFI_STATUS  Status;

  ZeroMem (Commanddata, 20);
  ZeroMem (Response, 20);
  //
  // Set IP Source
  //
  Commanddata[0] = BMC_IPMI_CHANNEL_NO;// Channel number
  Commanddata[1] = IpmiLanIpAddressSource;        // Parameter selector
  Commanddata[2] = *pAddrSrc;  // IP Source (STATIC/ DYANAMIC)
  Commanddatasize = 3;
  Responsesize    = 10;

  Status = IpmiSubmitCommand (
            IPMI_NETFN_TRANSPORT,           // NetFunction
            IPMI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,     // Command
            (UINT8 *) &Commanddata[0],  // *CommandData
            Commanddatasize,            // CommandDataSize
            (UINT8 *) &Response,        // *ResponseData
            (UINT32 *) &Responsesize     // *ResponseDataSize
            );
  return Status;
}

EFI_STATUS
IpmiSetBmcLanIpAddr (
  IN IPMI_LAN_IP_ADDRESS       *BmcIpAddress
  )
{
  EFI_STATUS  Status;

  Status = SetIpParam (BMC_IPMI_CHANNEL_NO, IpmiLanIpAddress, &BmcIpAddress->IpAddress[0]);

  return Status;
}

EFI_STATUS
IpmiSetBmcLanSubnetMask (
  IN IPMI_LAN_SUBNET_MASK      *BmcSubnetMask
  )
{
  EFI_STATUS  Status;

  Status = SetIpParam (BMC_IPMI_CHANNEL_NO, IpmiLanSubnetMask, &BmcSubnetMask->IpAddress[0]);

  return Status;
}

EFI_STATUS
IpmiSetBmcLanGateWay (
  IN IPMI_LAN_DEFAULT_GATEWAY  *BmcDefaultGateWay
  )
{
  EFI_STATUS  Status;

  Status = SetIpParam (BMC_IPMI_CHANNEL_NO, IpmiLanDefaultGateway, &BmcDefaultGateWay->IpAddress[0]);

  return Status;
}

EFI_STATUS
IpmiSetBmcLanInfo (
  IN UINT8                     *pAddrSrc,
  IN IPMI_LAN_IP_ADDRESS       *BmcIpAddress,
  IN IPMI_LAN_SUBNET_MASK      *BmcSubnetMask,
  IN IPMI_LAN_DEFAULT_GATEWAY  *BmcDefaultGateWay
  )
{
  UINT8        Commanddata[20];
  UINT8        Commanddatasize;
  UINT8        Response[20];
  UINT32       Responsesize;
  EFI_STATUS   Status;
  UINT8        BmcChannel;

  BmcChannel = BMC_IPMI_CHANNEL_NO;
  ZeroMem (Commanddata, 20);
  ZeroMem (Response, 20);
  //
  // Set IP Source
  //
  Commanddata[0] = BmcChannel;               // Channel number
  Commanddata[1] = IpmiLanIpAddressSource;        // Parameter selector
  Commanddata[2] = *pAddrSrc;  // IP Source (STATIC/ DYANAMIC)
  Commanddatasize = 3;
  Responsesize    = 10;

  //
  //If BMC LAN configuration IP source is not changed, then issue command once.
  //
  Status = IpmiSubmitCommand (
            IPMI_NETFN_TRANSPORT,
            IPMI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,
            (UINT8 *) &Commanddata[0],
            Commanddatasize,
            (UINT8 *) &Response,
            (UINT32 *) &Responsesize
            );

  //
  // IP Source is Static.Set Ip Address,Subnet Mask and Gateway Ip
  //
  if ((*pAddrSrc) != 2) {
    Status = SetIpParam (BmcChannel, IpmiLanIpAddress, &BmcIpAddress->IpAddress[0]);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Failed to set bmc ip addr: %r\n", Status));
      return Status;
    }
    Status = SetIpParam (BmcChannel, IpmiLanSubnetMask, &BmcSubnetMask->IpAddress[0]);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Failed to set bmc subnetmask: %r\n", Status));
      return Status;
    }
    Status = SetIpParam (BmcChannel, IpmiLanDefaultGateway, &BmcDefaultGateWay->IpAddress[0]);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Failed to set bmc gateway: %r\n", Status));
      return Status;
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
IpmiGetUserName (
  IN  UINT8             UserId,
  OUT CHAR16           *UserName,
  IN  UINTN             UserNameMaxLen
  )
{
  IPMI_GET_USER_NAME_RESPONSE GetUserNameResponse;
  IPMI_GET_USER_NAME_REQUEST  GetUserNameRequest;
  UINT32                       DataSize;

  ZeroMem (&GetUserNameResponse, sizeof (IPMI_GET_USER_NAME_RESPONSE));
  ZeroMem (&GetUserNameRequest, sizeof (IPMI_GET_USER_NAME_REQUEST));
  GetUserNameRequest.UserId.Uint8 = UserId;
  DataSize = sizeof (IPMI_GET_USER_NAME_RESPONSE);

  IpmiSubmitCommand (
              IPMI_NETFN_APP,
              IPMI_APP_GET_USER_NAME,
              (UINT8 *)&GetUserNameRequest,
              sizeof (IPMI_GET_USER_NAME_REQUEST),
              (UINT8 *)&GetUserNameResponse,
              (UINT32 *)&DataSize
              );

  if (AsciiStrLen ((const CHAR8 *)GetUserNameResponse.UserName) > 0) {
    AsciiStrToUnicodeStrS((const CHAR8 *)GetUserNameResponse.UserName, UserName,UserNameMaxLen);
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  Set user password command:
  NetFunction Id is 0x06, command Id is 0x47.

  @param[in]   UserId        BMC User Id.
  @param[in]   PasswordSize  Password Size .
  @param[in]   Operation     Tells what operation to perform.
  @param[in]   UserPassword  Password.

  @retval      EFI_SUCCESS       Command completed successfully.
  @retval      EFI_DEVICE_ERROR  IPMI command failed.
  @retval      EFI_UNSUPPORTED   Command is not supported by BMC.
  @retval      EFI_BUFFER_TOO_SMALL   Response buffer is too small.
  @retval      EFI_INVALID_PARAMETER  RecvData or RecvLength is NULL.
**/
EFI_STATUS
EFIAPI
IpmiSetUserPassword (
  IN UINT8 UserId,
  IN UINT8 PasswordSize,
  IN UINT8 Operation,
  IN CHAR8 *UserPassword
 )
{
  IPMI_SET_USER_PASSWORD_CMD              SetPasswordRequest;
  UINT8                                   CompeleCode;
  EFI_STATUS                              Status;
  UINT32                                  DataSize;
  EFI_INPUT_KEY                           Key;
  CHAR16                                  StringBuffer1[32];
  CHAR16                                  StringBuffer2[64];
  CHAR16                                  StringBuffer3[32];


  ZeroMem (&SetPasswordRequest, sizeof (IPMI_SET_USER_PASSWORD_CMD));

  SetPasswordRequest.UserId.Uint8    = (UserId | (PasswordSize << 7));
  SetPasswordRequest.Operation.Uint8 = Operation;
  CompeleCode = 0;
  DataSize = sizeof (UINT8);
  CopyMem (SetPasswordRequest.PasswordData, UserPassword, AsciiStrLen (UserPassword));
  Status = IpmiSubmitCommand (
            IPMI_NETFN_APP,
            IPMI_APP_SET_USER_PASSWORD,
            (VOID *)&SetPasswordRequest,
            (sizeof(IPMI_SET_USER_PASSWORD_CMD) ),
            (UINT8 *) &CompeleCode,
            (UINT32 *) &DataSize
            );
  if (CompeleCode == 0x80 || CompeleCode == 0x81) {
    do {
      StrCpyS (StringBuffer1, StrLen (L"Invalid password.") + 1, L"Invalid password.");
      StrCpyS (StringBuffer2, StrLen (L"Config password in 6-20 characters with high complexity") + 1, L"Config password in 6-20 characters with high complexity");
      StrCpyS (StringBuffer3, StrLen (L"Press ENTER to continue") + 1, L"Press ENTER to continue");
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, StringBuffer3, NULL);
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
  }
  return Status;
}

EFI_STATUS
EFIAPI
GetBmcBasicInfo (
  OUT BMC_DATA *BmcData
  )
{
  EFI_STATUS                   Status;
  IPMI_GET_DEVICE_ID_RESPONSE  DeviceId;

  Status = IpmiGetDeviceId (&DeviceId);
  if (  !EFI_ERROR (Status)
     && (DeviceId.CompletionCode == IPMI_COMP_CODE_NORMAL))
  {
    //
    // Firmware Revision
    //
    DEBUG ((DEBUG_VERBOSE, "BMC Firmware Version:\n %d.%02d %2d %d %d\n",
      DeviceId.FirmwareRev1.Bits.MajorFirmwareRev,
      BcdToDecimal8 (DeviceId.MinorFirmwareRev),
      DeviceId.MinorFirmwareRev,
      sizeof (BmcData->FmVersion),
      sizeof (BmcData->IpmiVersion)));

    UnicodeSPrint (
      BmcData->FmVersion,
      sizeof (BmcData->FmVersion),
      L"%d.%02d",
      DeviceId.FirmwareRev1.Bits.MajorFirmwareRev,
      BcdToDecimal8 (DeviceId.MinorFirmwareRev)
      );

    //
    // IPMI Version
    //
    UnicodeSPrint (
      BmcData->IpmiVersion,
      sizeof (BmcData->IpmiVersion),
      L"%d.%d",
      DeviceId.SpecificationVersion & 0x0F,
      (DeviceId.SpecificationVersion >> 4) & 0x0F
      );
  }

  return Status;
}

EFI_STATUS
EFIAPI
GetBmcBootOption (
  OUT UINT8    *BootDeviceSelector,
  OUT UINT8    *BootInitiator,
  OUT BOOLEAN  *IsPersistent
  )
{
  EFI_STATUS                               Status;
  IPMI_GET_BOOT_OPTIONS_REQUEST            BootOptionsRequest;
  IPMI_GET_BOOT_OPTIONS_RESPONSE          *BootOptionsResponse;
  IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_4  *BootOptionsParameterData4;
  IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5  *BootOptionsParameterData5;


  ZeroMem (&BootOptionsRequest, sizeof (IPMI_GET_BOOT_OPTIONS_REQUEST));

  //
  // Retrieve Boot Info Acknowledge from BMC.
  //
  BootOptionsResponse = AllocateZeroPool (sizeof (IPMI_GET_BOOT_OPTIONS_REQUEST) + sizeof (IPMI_BOOT_OPTIONS_PARAMETERS));
  if (BootOptionsResponse == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BootOptionsRequest.ParameterSelector.Bits.ParameterSelector = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INFO_ACK;

  Status = IpmiGetSystemBootOptions (&BootOptionsRequest, BootOptionsResponse);
  if (EFI_ERROR (Status)) {
    FreePool (BootOptionsResponse);
    return Status;
  }

  BootOptionsParameterData4 = (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_4 *)BootOptionsResponse->ParameterData;
  *BootInitiator = BootOptionsParameterData4->BootInitiatorAcknowledgeData;
  //
  // Retrieve Boot Options parameter data
  //

  ZeroMem (&BootOptionsRequest, sizeof (IPMI_GET_BOOT_OPTIONS_REQUEST));

  BootOptionsRequest.ParameterSelector.Bits.ParameterSelector = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS;

  Status = IpmiGetSystemBootOptions (&BootOptionsRequest, BootOptionsResponse);
  if (EFI_ERROR (Status)) {
    FreePool (BootOptionsResponse);
    return Status;
  }

  BootOptionsParameterData5 = (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5 *)BootOptionsResponse->ParameterData;
  if (BootOptionsParameterData5->Data1.Bits.BootFlagValid != 0) {
    *IsPersistent       = (BootOptionsParameterData5->Data1.Bits.PersistentOptions != 0) ? TRUE : FALSE;
    *BootDeviceSelector = BootOptionsParameterData5->Data2.Bits.BootDeviceSelector;
    FreePool (BootOptionsResponse);
    return Status;
  }
  return Status;
}
