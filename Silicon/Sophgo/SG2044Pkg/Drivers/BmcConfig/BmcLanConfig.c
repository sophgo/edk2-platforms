/** @file
  BMC Configuration Lib

  Copyright (c) 2025, Sophgo Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "BmcLanConfig.h"
#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiLib.h>

#include <IndustryStandard/Ipmi.h>

EFI_STATUS
EFIAPI
IpmiGetBmcLanInfo (
  IN  UINT8                    BmcChannel,
  OUT UINT8                    *pAddrSrc,
  OUT IPMI_LAN_IP_ADDRESS      *BmcIpAddress,
  OUT IPMI_LAN_SUBNET_MASK     *BmcSubnetMask,
  OUT IPMI_LAN_DEFAULT_GATEWAY *BmcDefaultGateWay
  )
{
  EFI_STATUS                                      Status;
  IPMI_GET_CHANNEL_INFO_REQUEST                   GetChannelInfoRequest;
  IPMI_GET_CHANNEL_INFO_RESPONSE                  GetChannelInfoResponse;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_REQUEST   GetConfigurationParametersRequest;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE  *GetConfigurationParametersResponse;
  UINT32                                          ResponseSize;

  if ((pAddrSrc == NULL) || (BmcIpAddress == NULL) || (BmcSubnetMask == NULL) || (BmcDefaultGateWay == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get Channel Information
  //
  ZeroMem (&GetChannelInfoRequest, sizeof (GetChannelInfoRequest));
  GetChannelInfoRequest.ChannelNumber.Bits.ChannelNo = BmcChannel;
  ResponseSize                                       = sizeof (GetChannelInfoResponse);

  Status = IpmiGetChannelInfo (&GetChannelInfoRequest, &GetChannelInfoResponse, &ResponseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get BMC channel info\n", __func__));
    return Status;
  }

  //
  // Check for LAN interface
  //
  if (  EFI_ERROR (Status)
     || (GetChannelInfoResponse.CompletionCode != IPMI_COMP_CODE_NORMAL)
     /* || (GetChannelInfoResponse.MediumType.Bits.ChannelMediumType != IPMI_CHANNEL_MEDIA_TYPE_802_3_LAN) */)
  {
    return EFI_NOT_FOUND;
  }

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

  if (GetChannelInfoResponse.CompletionCode != IPMI_COMP_CODE_NORMAL) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  DEBUG ((DEBUG_INFO, "###-ip source = 0x%x\n", GetConfigurationParametersResponse->ParameterData[0]));
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

  if (GetChannelInfoResponse.CompletionCode != IPMI_COMP_CODE_NORMAL) {
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

  if (GetChannelInfoResponse.CompletionCode != IPMI_COMP_CODE_NORMAL) {
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

  if (GetChannelInfoResponse.CompletionCode != IPMI_COMP_CODE_NORMAL) {
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
GetBmcLanInfo (
  OUT UINT8                 *pAddrSrc,
  OUT IPMI_LAN_IP_ADDRESS   *BmcIpAddress,
  OUT IPMI_LAN_SUBNET_MASK  *BmcSubnetMask,
  OUT IPMI_LAN_DEFAULT_GATEWAY *BmcDefaultGateWay
  )
{
  EFI_STATUS                   Status;
//   IPMI_GET_DEVICE_ID_RESPONSE  DeviceId;
  UINT8                        BmcChannel;

  for (BmcChannel = IPMI_CHANNEL_NUMBER_IMPLEMENTATION_SPECIFIC_1; BmcChannel < IPMI_CHANNEL_NUMBER_IMPLEMENTATION_SPECIFIC_11; BmcChannel++) {
    Status = IpmiGetBmcLanInfo (BmcChannel, pAddrSrc, BmcIpAddress, BmcSubnetMask, BmcDefaultGateWay);
    if (  EFI_ERROR (Status) || (BmcIpAddress->IpAddress[0] == 0)) {
      continue;
    }
    break;
  }
  return Status;
}




EFI_STATUS
GetSetIpPram (
  IN     UINT8          Channel,
  IN     UINT8          LanParameter,
  IN OUT UINT8          *IpAddress,
  IN     BOOLEAN        CommandGet
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
IpmiSetBmcLanInfo (
  IN UINT8                     BmcChannel,
  IN UINT8                     *pAddrSrc,
  IN IPMI_LAN_IP_ADDRESS       *BmcIpAddress,
  IN IPMI_LAN_SUBNET_MASK      *BmcSubnetMask,
  IN IPMI_LAN_DEFAULT_GATEWAY  *BmcDefaultGateWay
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
  Commanddata[0] = BmcChannel;               // Channel number
  Commanddata[1] = IpmiLanIpAddressSource;        // Parameter selector
  Commanddata[2] = *pAddrSrc;  // IP Source (STATIC/ DYANAMIC)
  Commanddatasize = 3;
  Responsesize    = 10;

  //
  //If BMC LAN configuration IP source is not changed, then issue command once.
  //
  Status = IpmiSubmitCommand (
            IPMI_NETFN_TRANSPORT,           // NetFunction
            IPMI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,     // Command
            (UINT8 *) &Commanddata[0],  // *CommandData
            Commanddatasize,            // CommandDataSize
            (UINT8 *) &Response,        // *ResponseData
            (UINT32 *) &Responsesize     // *ResponseDataSize
            );
  // DEBUG((DEBUG_INFO," Setup Laninfo->NicIpSource = %x - %r\n",Laninfo->NicIpSource,Status));

  if ((*pAddrSrc) != 2) {
    //
    // IP Source is Static.Set Ip Address,Subnet Mask and Gateway Ip
    //
    GetSetIpPram (BmcChannel, IpmiLanIpAddress, &BmcIpAddress->IpAddress[0], FALSE);
    GetSetIpPram (BmcChannel, IpmiLanSubnetMask, &BmcSubnetMask->IpAddress[0], FALSE);
    GetSetIpPram (BmcChannel, IpmiLanDefaultGateway, &BmcDefaultGateWay->IpAddress[0], FALSE);
  }

  return Status;
}


EFI_STATUS
SetBmcLanInfo (
  OUT UINT8                    *pAddrSrc,
  OUT IPMI_LAN_IP_ADDRESS      *BmcIpAddress,
  OUT IPMI_LAN_SUBNET_MASK     *BmcSubnetMask,
  OUT IPMI_LAN_DEFAULT_GATEWAY *BmcDefaultGateWay
  )
{
  EFI_STATUS                   Status;
//   IPMI_GET_DEVICE_ID_RESPONSE  DeviceId;
  UINT8                        BmcChannel;

  for (BmcChannel = IPMI_CHANNEL_NUMBER_IMPLEMENTATION_SPECIFIC_1; BmcChannel < IPMI_CHANNEL_NUMBER_IMPLEMENTATION_SPECIFIC_11; BmcChannel++) {
    Status = IpmiSetBmcLanInfo (BmcChannel, pAddrSrc, BmcIpAddress, BmcSubnetMask, BmcDefaultGateWay);
    if ( EFI_ERROR (Status) ) {
      continue;
    }
    break;
  }
  return Status;

}

