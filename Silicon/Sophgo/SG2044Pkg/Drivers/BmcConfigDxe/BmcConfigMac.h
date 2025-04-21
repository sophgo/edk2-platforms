#ifndef __BMC_CONFIG_MAC_H__
#define __BMC_CONFIG_MAC_H__


#include "BmcConfigNv.h"
#include "BmcConfig.h"


BOOLEAN
isValidMacFormat(
  IN CONST CHAR16 *MacStr
  );

UINT8
CharToHex (
  IN CHAR16  c
  );

EFI_STATUS
EFIAPI
ConvertChar16ToIpmiMac(
  IN  CHAR16  *MacStr,
  OUT UINT8   *Mac
  );

EFI_STATUS
EFIAPI
UpdateBmcMacInfo(
    NET_PRIVATE_DATA *PrivateData
  );

VOID
UpdateBmcMacForm(
    NET_PRIVATE_DATA *PrivateData
  );

EFI_STATUS
ProcessMacAddrSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  );

#endif
