#ifndef __BMC_CONFIG_BASIC_INFO_H__
#define __BMC_CONFIG_BASIC_INFO_H__

#include "BmcConfigNv.h"
#include "BmcConfig.h"

EFI_STATUS
EFIAPI
UpdateBmcBasicInfo(
    NET_PRIVATE_DATA *PrivateData
  );

VOID
UpdateBmcInfoForm(
    NET_PRIVATE_DATA *PrivateData
  );

#endif
