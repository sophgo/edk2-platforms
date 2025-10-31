/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2025. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>

#include "SmbiosPlatformDxe.h"
#include <Include/PcieHostPcd.h>

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformSystemSlot) {
  EFI_STATUS                         Status;
  UINT32                             Index, NumberOfControllers, SlotID;
  STR_TOKEN_INFO                     *InputStrToken;
  SMBIOS_TABLE_TYPE9                 *Type9Record;
  SMBIOS_TABLE_TYPE9                 *InputData;
  CHAR16                             SlotDesignation[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  PCIE_HOST_BRIDGE_TABLE            *PcieRcConfig;

  InputData     = (SMBIOS_TABLE_TYPE9 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;
  PcieRcConfig  = (PCIE_HOST_BRIDGE_TABLE *)PcdGetPtr (PcdPcieHostBridgeTable);
  if (PcieRcConfig == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] No PCIe host bridge configuration found\n", __func__));
    return EFI_NOT_FOUND;
  }

  NumberOfControllers = PcieRcConfig->NumOfControllers;

  for (Index = 0; Index < NumberOfControllers; ++Index) {
    SlotID = PcieRcConfig->PcieDomain[Index][0] | (PcieRcConfig->PcieDomain[Index][1] << 8) |
             (PcieRcConfig->PcieDomain[Index][2] << 16) | (PcieRcConfig->PcieDomain[Index][3] << 24);
    UnicodeSPrint (SlotDesignation, sizeof (SlotDesignation), L"SLOT%u", SlotID);
    HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[0], SlotDesignation, NULL);
    InputData->SlotID = (UINT16) SlotID;
    InputData->SegmentGroupNum = (UINT16) SlotID;

    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type9Record,
      (VOID *)&InputData,
      sizeof (SMBIOS_TABLE_TYPE9),
      InputStrToken
      );
    if (Type9Record == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type9Record, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (Type9Record);
      return Status;
    }

    FreePool (Type9Record);
  }
  return EFI_SUCCESS;
}
