#ifndef __BMC_LAN_CONFIG_H__
#define __BMC_LAN_CONFIG_H__

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
#include "BMCNv.h"

// Function declarations

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

#endif // __BMC_LAN_CONFIG_H__
