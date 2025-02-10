/** @file
This file declares the interface functions required by the BmcLanConfig driver.

Copyright (c) 2025  Sophgo Corporation. All rights reserved.<BR>
**/

#ifndef __BMC_LAN_CONFIG_IPMI_H__
#define __BMC_LAN_CONFIG_IPMI_H__

#include <Uefi.h>
#include <IndustryStandard/Ipmi.h>
#include <IndustryStandard/IpmiNetFnApp.h>
#include <IndustryStandard/IpmiNetFnTransport.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiCommandLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/IpmiProtocol.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Protocol/SmbusHc.h>
#include "BmcLanConfigNv.h"

/**
  Retrieve BMC LAN information.

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
GetBmcLanInfo (
  OUT UINT8                    *pAddrSrc,
  OUT IPMI_LAN_IP_ADDRESS      *BmcIpAddress,
  OUT IPMI_LAN_SUBNET_MASK     *BmcSubnetMask,
  OUT IPMI_LAN_DEFAULT_GATEWAY *BmcDefaultGateway
  );

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
SetIpPram (
  IN     UINT8          Channel,
  IN     UINT8          LanParameter,
  IN OUT UINT8          *IpAddress
  );

/**
  Set BMC LAN IP Source.

  @param[in] BmcChannel        BMC channel number
  @param[in] pAddrSrc          Address source

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
IpmiSetBmcLanIpSrc (
  IN UINT8                     BmcChannel,
  IN UINT8                     *pAddrSrc
  );

/**
  Set BMC LAN IP Addr.

  @param[in] BmcChannel        BMC channel number
  @param[in] BmcIpAddress      Pointer to IP address

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
IpmiSetBmcLanIpAddr (
  IN UINT8                     BmcChannel,
  IN IPMI_LAN_IP_ADDRESS       *BmcIpAddress
  );

/**
  Set BMC LAN SubnetMask.

  @param[in] BmcChannel        BMC channel number
  @param[in] BmcSubnetMask     Pointer to subnet mask

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
IpmiSetBmcLanSubnetMask (
  IN UINT8                     BmcChannel,
  IN IPMI_LAN_SUBNET_MASK      *BmcSubnetMask
  );

/**
  Set BMC LAN GateWay.

  @param[in] BmcChannel            BMC channel number
  @param[in] BmcDefaultGateWay     Pointer to default gateway

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
IpmiSetBmcLanGateWay (
  IN UINT8                     BmcChannel,
  IN IPMI_LAN_DEFAULT_GATEWAY  *BmcDefaultGateWay
  );

/**
  Set BMC LAN IP Source.

  @param[in] pAddrSrc     Pointer to ip source

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
SetBmcLanIpSrc (
  IN UINT8                    *pAddrSrc
  );

/**
  Set BMC LAN IP Addr.

  @param[in] BmcIpAddress     Pointer to ip addr

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
SetBmcLanIpAddr (
  IN IPMI_LAN_IP_ADDRESS      *BmcIpAddress
  );

/**
  Set BMC LAN Subnet Mask.

  @param[in] BmcSubnetMask     Pointer to subnet mask

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
SetBmcLanSubnetMask (
  IN IPMI_LAN_SUBNET_MASK     *BmcSubnetMask
  );

/**
  Set BMC LAN GateWay.

  @param[in] BmcDefaultGateWay     Pointer to GateWay

  @retval EFI_SUCCESS          set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
SetBmcLanGateWay (
  IN IPMI_LAN_DEFAULT_GATEWAY *BmcDefaultGateWay
  );

/**
  Set BMC LAN information.

  @param[in] pAddrSrc          Address source
  @param[in] BmcIpAddress      Pointer to IP address
  @param[in] BmcSubnetMask     Pointer to subnet mask
  @param[in] BmcDefaultGateway Pointer to default gateway

  @retval EFI_SUCCESS          LAN information set successfully
  @retval EFI_INVALID_PARAMETERInvalid parameter
  @retval Others               Error occurred
**/
EFI_STATUS
SetBmcLanInfo (
  IN UINT8                    *pAddrSrc,
  IN IPMI_LAN_IP_ADDRESS      *BmcIpAddress,
  IN IPMI_LAN_SUBNET_MASK     *BmcSubnetMask,
  IN IPMI_LAN_DEFAULT_GATEWAY *BmcDefaultGateway
  );

/**
  Retrieve BMC LAN information for a specific channel.

  @param[in]  BmcChannel       BMC channel number
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
  IN  UINT8                    BmcChannel,
  OUT UINT8                    *pAddrSrc,
  OUT IPMI_LAN_IP_ADDRESS      *BmcIpAddress,
  OUT IPMI_LAN_SUBNET_MASK     *BmcSubnetMask,
  OUT IPMI_LAN_DEFAULT_GATEWAY *BmcDefaultGateway
  );

/**
  Set BMC LAN information for a specific channel.

  @param[in] BmcChannel        BMC channel number
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
  IN UINT8                    BmcChannel,
  IN UINT8                    *pAddrSrc,
  IN IPMI_LAN_IP_ADDRESS      *BmcIpAddress,
  IN IPMI_LAN_SUBNET_MASK     *BmcSubnetMask,
  IN IPMI_LAN_DEFAULT_GATEWAY *BmcDefaultGateway
  );

#endif // __BMC_LAN_CONFIG_IPMI_H__