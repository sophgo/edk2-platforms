/** @file

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2025. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Protocol/FdtClient.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>

#include "SmbiosPlatformDxe.h"

#define TYPE41_DEVICE_TYPE_OTHERS 0x81

/**
  This function adds SMBIOS Table (Type 41) records.

  @param  RecordData                 Pointer to SMBIOS Table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS Table was successfully added.
  @retval Other                      Failed to update the SMBIOS Table.

**/
SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformOnboardDevicesExtended) {
  EFI_STATUS           FindNodeStatus, Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  CONST CHAR8          *CompatibleString;
  STR_TOKEN_INFO       *InputStrToken;
  SMBIOS_TABLE_TYPE41  *InputData;
  SMBIOS_TABLE_TYPE41  *Type41Record;
  INT32                Node;
  UINT32               Index, SlotID, PropSize, InstanceNum;
  CONST VOID           *Prop;
  CHAR16               SlotDesignation[SMBIOS_UNICODE_STRING_MAX_LENGTH];

  InputData     = (SMBIOS_TABLE_TYPE41 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;

  InstanceNum = 0;
  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
    InputData->DeviceTypeInstance = InstanceNum;
    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type41Record,
      (VOID *)&InputData,
      sizeof (SMBIOS_TABLE_TYPE41),
      InputStrToken
      );
    if (Type41Record == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type41Record, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (Type41Record);
      return Status;
    }

    FreePool (Type41Record);
    InputData++;
    InputStrToken++;
    InstanceNum++;
  }

  InputData        = (SMBIOS_TABLE_TYPE41 *)RecordData;
  InputStrToken    = (STR_TOKEN_INFO *)StrToken;
  CompatibleString = "sophgo,sg2044-pcie-host";

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL, (VOID **)&FdtClient);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "[%a] No FDT client service found\n", __func__));
    return EFI_NOT_FOUND;
  }

  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, CompatibleString, &Node), Index = 0;
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, CompatibleString, Node, &Node)) {
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "linux,pci-domain", &Prop, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] GetNodeProperty (linux,pci-domain) failed (Status == %r)\n", __func__, Status));
      continue;
    }
    SlotID = SwapBytes32 (((CONST UINT32 *)Prop)[0]);
    UnicodeSPrint (SlotDesignation, sizeof (SlotDesignation), L"PCIE_SLOT%u", SlotID);
    HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[0], SlotDesignation, NULL);
    InputData->DeviceType = TYPE41_DEVICE_TYPE_OTHERS;
    InputData->DeviceTypeInstance = InstanceNum;
    InputData->SegmentGroupNum = (UINT16) SlotID;
    InputData->BusNum = 0;
    InputData->DevFuncNum = 0;

    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type41Record,
      (VOID *)&InputData,
      sizeof (SMBIOS_TABLE_TYPE41),
      InputStrToken
      );
    if (Type41Record == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type41Record, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (Type41Record);
      return Status;
    }

    FreePool (Type41Record);
    Index++;
    InstanceNum++;
  }

  return EFI_SUCCESS;
}
