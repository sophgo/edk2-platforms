/** @file
This file declares the interface functions required by the BmcLanConfig driver.

Copyright (c) 2025  Sophgo Corporation. All rights reserved.<BR>
**/

#ifndef __BMC_CONFIG_IPMI_H__
#define __BMC_CONFIG_IPMI_H__


#include <IndustryStandard/Ipmi.h>
#include <IndustryStandard/IpmiNetFnApp.h>
#include <IndustryStandard/IpmiNetFnTransport.h>
#include <Library/IpmiCommandLib.h>
#include "BmcConfigNv.h"
#include "BmcConfig.h"

#define BMC_IPMI_CHANNEL_NO 1


typedef struct {
  IPMI_SET_USER_PASSWORD_USER_ID      UserId;
  IPMI_SET_USER_PASSWORD_OPERATION    Operation;
  UINT8                               PasswordData[20]; // 16 or 20 bytes, depending on the 'PasswordSize' field
} IPMI_SET_USER_PASSWORD_CMD;

/**
  Set BMC LAN IP Param.

  @param[in] Channel           BMC channel number
  @param[in] LanParameter      Param type
  @param[in] IpAddress         Pointer to IP address

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
SetIpParam (
  IN     UINT8          Channel,
  IN     UINT8          LanParameter,
  IN OUT UINT8          *IpAddress
  );

/**
  Set BMC LAN IP Source.
  @param[in] pAddrSrc          Address source

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
IpmiSetBmcLanIpSrc (
  IN UINT8                     *pAddrSrc
  );

/**
  Set BMC LAN IP Addr.
  @param[in] BmcIpAddress      Pointer to IP address

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
IpmiSetBmcLanIpAddr (
  IN IPMI_LAN_IP_ADDRESS       *BmcIpAddress
  );

/**
  Set BMC LAN SubnetMask.

  @param[in] BmcSubnetMask     Pointer to subnet mask

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
IpmiSetBmcLanSubnetMask (
  IN IPMI_LAN_SUBNET_MASK      *BmcSubnetMask
  );

/**
  Set BMC LAN GateWay.

  @param[in] BmcDefaultGateWay     Pointer to default gateway

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
IpmiSetBmcLanGateWay (
  IN IPMI_LAN_DEFAULT_GATEWAY  *BmcDefaultGateWay
  );

/**
  Retrieve BMC LAN information for a specific channel.

  @param[out] pAddrSrc         Address source pointer
  @param[out] BmcIpAddress     Pointer to store IP address
  @param[out] BmcSubnetMask    Pointer to store subnet mask
  @param[out] BmcDefaultGatewayPointer to store default gateway

  @retval EFI_SUCCESS          LAN information retrieved successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
EFIAPI
IpmiGetBmcLanInfo (
  OUT UINT8                    *pAddrSrc,
  OUT IPMI_LAN_IP_ADDRESS      *BmcIpAddress,
  OUT IPMI_LAN_SUBNET_MASK     *BmcSubnetMask,
  OUT IPMI_LAN_DEFAULT_GATEWAY *BmcDefaultGateway
  );

/**
  Set BMC LAN information for a specific channel.

  @param[in] pAddrSrc          Address source
  @param[in] BmcIpAddress      Pointer to IP address
  @param[in] BmcSubnetMask     Pointer to subnet mask
  @param[in] BmcDefaultGateway Pointer to default gateway

  @retval EFI_SUCCESS          LAN information set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
EFIAPI
IpmiSetBmcLanInfo (
  IN UINT8                    *pAddrSrc,
  IN IPMI_LAN_IP_ADDRESS      *BmcIpAddress,
  IN IPMI_LAN_SUBNET_MASK     *BmcSubnetMask,
  IN IPMI_LAN_DEFAULT_GATEWAY *BmcDefaultGateway
  );

EFI_STATUS
EFIAPI
IpmiGetBmcMacAddr (
  OUT IPMI_LAN_MAC_ADDRESS      *BmcMacAddr
  );

EFI_STATUS
EFIAPI
GetBmcBasicInfo (
  OUT CHAR16* BmcFwVersion,
  OUT CHAR16* IpmiVersion
  );

EFI_STATUS
EFIAPI
GetBmcBootOption (
  OUT UINT8    *BootDeviceSelector,
  OUT UINT8    *BootInitiator,
  OUT BOOLEAN  *IsPersistent
  );

EFI_STATUS
EFIAPI
IpmiGetUserName (
  IN  UINT8             UserId,
  OUT CHAR16           *UserName,
  IN  UINTN             UserNameMaxLen
  );

EFI_STATUS
IpmiSetBmcMacAddr (
  IN OUT UINT8                     *MacAddr
  );

EFI_STATUS
EFIAPI
IpmiSetUserPassword (
  IN UINT8 UserId,
  IN UINT8 PasswordSize,
  IN UINT8 Operation,
  IN CHAR8 *UserPassword
 );
#endif // __BMC_LAN_CONFIG_IPMI_H__
