/** @file

This file defines the structures and macros required by the BMC driver.

Copyright (c) 2025  Sophgo Corporation. All rights reserved.<BR>
**/
#ifndef _BMC_LAN_CONFIG_H_
#define _BMC_LAN_CONFIG_H_

#include <Uefi.h>
#include <Library/PcdLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiPopup.h>
#include <Guid/MdeModuleHii.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/HiiFormMapMethodGuid.h>
#include <Guid/DriverSampleHii.h>
#include <Guid/ZeroGuid.h>
#include "BmcLanConfigNv.h"
#include <Library/FileHandleLib.h>
#include <Protocol/HiiPackageList.h>
#include <Protocol/HiiDatabase.h>
#include <Guid/FileInfo.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>
#include <Library/UefiHiiServicesLib.h>
#include "BmcLanConfigIpmi.h"

#include <Library/SmbiosInformationLib.h>
#include <string.h>
/**
 * Extern declarations for .vfr and .uni data.
 */
extern UINT8  BmcLanConfigVfrBin[];
extern UINT8  BMCStrings[];

/**
 * Unique signature for the private data structure.
 */
#define BMC_PRIVATE_SIGNATURE SIGNATURE_32 ('S', 'G', 'B', 'S')

/**
 * Private data structure for the BMC driver.
 */
typedef struct {
  UINTN                            Signature;
  EFI_HANDLE                       DriverHandle;
  EFI_HII_HANDLE                   HiiHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting; // Consumed protocol
  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;      // Produced protocol
  BMC_DATA                         BmcConfigData;
} NET_PRIVATE_DATA;

#define BMC_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, NET_PRIVATE_DATA, ConfigAccess, BMC_PRIVATE_SIGNATURE)

/**
 * HII specific Vendor Device Path definition.
 */
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;


#endif //_BMC_LAN_CONFIG_H_