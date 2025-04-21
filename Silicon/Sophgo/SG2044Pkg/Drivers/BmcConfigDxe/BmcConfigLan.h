#ifndef __BMC_CONFIG_LAN_H__
#define __BMC_CONFIG_LAN_H__

#include "BmcConfigNv.h"
#include "BmcConfig.h"


EFI_STATUS
EFIAPI
ConvertChar16ToIpmiLanIpAddress(
    IN CHAR16 *IpAddressStr,
    OUT UINT8 *IpAddress
  );

EFI_STATUS
EFIAPI
UpdateBmcLanConfigData(
    NET_PRIVATE_DATA *PrivateData
  );

BOOLEAN
IsValidIpAndGateway(
    IN CHAR16 *Ip
  );

BOOLEAN
IsValidSubnetMask(
    IN CHAR16 *SubnetMask
  );

EFI_STATUS
ProcessGateWayAddrSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  );

EFI_STATUS
ProcessSubnetMaskSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  );

EFI_STATUS
ProcessIpSourceSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  );

EFI_STATUS
ProcessIpAddrSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  );

VOID
UpdateNetworkForm(
    NET_PRIVATE_DATA *PrivateData
  );

EFI_STATUS
EFIAPI
SendBmcNetForm(
    NET_PRIVATE_DATA *PrivateData
  );
#endif
