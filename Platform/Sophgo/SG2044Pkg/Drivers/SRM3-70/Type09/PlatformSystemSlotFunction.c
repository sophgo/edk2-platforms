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

/**
  Map an SRM3-70 PCIe controller domain (segment) to its board slot silkscreen.

  SRM3-70 does not label every PCIe connector, and several physical connectors
  are shared between the paired x4 controllers of one wrapper. The front/rear
  halves of such a shared connector are distinguished here with an A/B suffix
  (e.g. "PCIe2A" / "PCIe2B", "OCP0A" / "OCP0B"). This mapping is board specific
  and therefore lives in the SRM3-70 variant instead of the shared MapSlot(),
  which only maps even-numbered domains.

  @param  Domain   The PCIe controller domain (segment) number.

  @retval CHAR16*  The board slot designation string.
**/
STATIC
CHAR16 *
MapSlotName (
  IN UINT32  Domain
  )
{
  switch (Domain) {
    case 0:  return L"PCIe2A";
    case 1:  return L"PCIe2B";
    case 2:  return L"PCIe0";
    case 4:  return L"M.2";
    case 5:  return L"PCIe1";
    case 6:  return L"OCP0A";
    case 7:  return L"OCP0B";
    case 8:  return L"PCIeSW";
    default: return L"UNKNOWN";
  }
}

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformSystemSlot) {
  EFI_STATUS                         Status;
  UINT32                             Index, NumberOfControllers, SlotID;
  STR_TOKEN_INFO                     *InputStrToken;
  SMBIOS_TABLE_TYPE9                 *Type9Record;
  SMBIOS_TABLE_TYPE9                 *InputData;
  CHAR16                             SlotDesignation[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  PCIE_HOST_BRIDGE_TABLE             *PcieRcConfig;
  SMBIOS_TABLE_TYPE9_EXTENDED        SmbiosRecordExtended;
  UINTN                              TotalSize;
  SMBIOS_TABLE_TYPE9                 *SmbiosRecord;

  InputData     = (SMBIOS_TABLE_TYPE9 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;
  PcieRcConfig  = (PCIE_HOST_BRIDGE_TABLE *)PcdGetPtr (PcdPcieHostBridgeTable);
  if (PcieRcConfig == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] No PCIe host bridge configuration found\n", __func__));
    return EFI_NOT_FOUND;
  }

  NumberOfControllers = PcieRcConfig->NumOfControllers;

  for (Index = 0; Index < NumberOfControllers; ++Index) {
    TotalSize    = sizeof (SMBIOS_TABLE_TYPE9) + sizeof (SMBIOS_TABLE_TYPE9_EXTENDED);
    SmbiosRecord = NULL;
    SmbiosRecord = AllocateZeroPool (TotalSize);

    SlotID = PcieRcConfig->Controller[Index].Domain;

    CopyMem (SmbiosRecord, InputData, sizeof (SMBIOS_TABLE_TYPE9));

    SmbiosRecord->SlotType = SlotTypePCIExpressGen5X8;
    SmbiosRecord->SlotDataBusWidth = SlotDataBusWidth8X;

    UnicodeSPrint (SlotDesignation, sizeof (SlotDesignation), L"%s", MapSlotName (SlotID));
    HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[0], SlotDesignation, NULL);
    SmbiosRecord->SlotID = (UINT16)SlotID;
    SmbiosRecord->SegmentGroupNum = (UINT16)SlotID;

    SmbiosRecordExtended.SlotInformation   = 0;
    SmbiosRecordExtended.SlotPhysicalWidth = SmbiosRecord->SlotDataBusWidth;
    SmbiosRecordExtended.SlotPitch         = 0;
    SmbiosRecordExtended.SlotHeight        = SlotHeightFullHeight;

    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE9) + sizeof (SMBIOS_TABLE_TYPE9_EXTENDED);
    CopyMem ((UINT8 *)SmbiosRecord->PeerGroups + SmbiosRecord->PeerGroupingCount * sizeof (SmbiosRecord->PeerGroups), (UINT8 *)&SmbiosRecordExtended, sizeof (SMBIOS_TABLE_TYPE9_EXTENDED));

    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type9Record,
      (VOID *)&SmbiosRecord,
      sizeof (SMBIOS_TABLE_TYPE9) + sizeof (SMBIOS_TABLE_TYPE9_EXTENDED),
      InputStrToken
      );
    if (Type9Record == NULL) {
      FreePool (SmbiosRecord);
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type9Record, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (Type9Record);
      FreePool (SmbiosRecord);
      return Status;
    }

    FreePool (Type9Record);
    FreePool (SmbiosRecord);
  }
  return EFI_SUCCESS;
}
