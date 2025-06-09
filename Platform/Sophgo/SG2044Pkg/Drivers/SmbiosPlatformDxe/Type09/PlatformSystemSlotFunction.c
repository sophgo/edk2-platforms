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
#include <Protocol/FdtClient.h>

#include "SmbiosPlatformDxe.h"

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformSystemSlot) {
  FDT_CLIENT_PROTOCOL  *FdtClient;
  CONST CHAR8          *CompatibleString;
  EFI_STATUS           FindNodeStatus, Status;
  INT32                Node;
  UINT32               Index, NumberOfControllers, SlotID, PropSize;
  CONST VOID           *Prop;
  STR_TOKEN_INFO       *InputStrToken;
  SMBIOS_TABLE_TYPE9   *Type9Record;
  SMBIOS_TABLE_TYPE9   *InputData;
  CHAR16               SlotDesignation[SMBIOS_UNICODE_STRING_MAX_LENGTH];

  InputData     = (SMBIOS_TABLE_TYPE9 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;
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
    ++Index;
  }

  if (Index == 0) {
    DEBUG ((DEBUG_ERROR, "[%a] Cannot get PCIe node from DTS (Status == %r)\n", __func__, Status));
    return EFI_NOT_FOUND;
  }
  NumberOfControllers = Index;

  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, CompatibleString, &Node), Index = 0;
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, CompatibleString, Node, &Node)) {
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "linux,pci-domain", &Prop, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] GetNodeProperty (linux,pci-domain) failed (Status == %r)\n", __func__, Status));
      continue;
    }
    SlotID = SwapBytes32 (((CONST UINT32 *)Prop)[0]);
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
    ++Index;
  }

  return EFI_SUCCESS;
}
