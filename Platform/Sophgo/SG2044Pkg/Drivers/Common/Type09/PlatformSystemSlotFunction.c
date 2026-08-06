/** @file
  SMBIOS Type 9 (System Slots) generated from the board slot table
  (PcieSlotInfoLib).

  One record per BOARD_SLOT entry: slot attributes (ID, type,
  designation, widths) come from the table.  The SBDF of the slot's
  downstream port is resolved at ReadyToBoot -- only then is PCI
  enumeration complete and PciIo->GetLocation() returns final bus
  numbers.  The port is found by device-path matching (domain +
  device-number hops), which is immune to bus-number reassignment.

  Copyright (c) 2026. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcieSlotInfoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>

#include "SmbiosPlatformDxe.h"

typedef struct {
  VOID  *RecordData;
  VOID  *StrToken;
} TYPE9_DEFER_CONTEXT;

/**
  Find the slot's downstream port among the enumerated PCI devices by
  device-path matching, read its runtime SBDF, and detect whether a
  card sits behind it.

  @param[in]  Slot         Slot table entry.
  @param[out] Segment      Receives the port's PCI segment.
  @param[out] Bus          Receives the port's bus number.
  @param[out] DevFunc      Receives the port's Device/Function number
                           (Device in bits [7:3], Function in [2:0]).
  @param[out] CardPresent  Receives TRUE when a device exists directly
                           behind the downstream port.

  @retval EFI_SUCCESS    Port found.
  @retval EFI_NOT_FOUND  Port handle not present.
**/
STATIC
EFI_STATUS
FindSlotPort (
  IN  CONST BOARD_SLOT  *Slot,
  OUT UINT16            *Segment,
  OUT UINT8             *Bus,
  OUT UINT8             *DevFunc,
  OUT BOOLEAN           *CardPresent
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                *Handles;
  UINTN                     HandleCount;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCIE_SLOT_TOPOLOGY        Topo;
  UINTN                     Seg;
  UINTN                     BusNum;
  UINTN                     Dev;
  UINTN                     Func;
  BOOLEAN                   Found;

  Found        = FALSE;
  *CardPresent = FALSE;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    PcieSlotParseDevicePath (DevicePath, &Topo);
    if (!Topo.FoundRoot || (Topo.RootUid != Slot->Domain)) {
      continue;
    }

    if (((Topo.PciCount != Slot->PathLen) && (Topo.PciCount != Slot->PathLen + 1)) ||
        (CompareMem (Topo.PciDevices, Slot->DevPath, Slot->PathLen) != 0))
    {
      continue;
    }

    if (Topo.PciCount == Slot->PathLen + 1) {
      //
      // A device directly behind the downstream port: slot occupied.
      //
      *CardPresent = TRUE;
      continue;
    }

    //
    // The slot's downstream port itself.
    //
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = PciIo->GetLocation (PciIo, &Seg, &BusNum, &Dev, &Func);
    if (EFI_ERROR (Status)) {
      continue;
    }

    *Segment = (UINT16)Seg;
    *Bus     = (UINT8)BusNum;
    *DevFunc = (UINT8)((Dev << 3) | Func);
    Found    = TRUE;
  }

  FreePool (Handles);
  return Found ? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
  ReadyToBoot callback: emit one Type 9 record per board slot table
  entry, with runtime SBDF filled from the enumerated topology.
**/
STATIC
VOID
EFIAPI
PlatformSystemSlotOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  TYPE9_DEFER_CONTEXT         *Ctx;
  SMBIOS_TABLE_TYPE9          *InputData;
  STR_TOKEN_INFO              *InputStrToken;
  CONST BOARD_SLOT            *Table;
  CONST BOARD_SLOT            *Slot;
  UINTN                       Count;
  UINTN                       Index;
  UINTN                       TotalSize;
  UINT16                      Segment;
  UINT8                       Bus;
  UINT8                       DevFunc;
  BOOLEAN                     CardPresent;
  EFI_STATUS                  Status;
  SMBIOS_TABLE_TYPE9          *SmbiosRecord;
  SMBIOS_TABLE_TYPE9          *Type9Record;
  SMBIOS_TABLE_TYPE9_EXTENDED SmbiosRecordExtended;
  CHAR16                      SlotDesignation[SMBIOS_UNICODE_STRING_MAX_LENGTH];

  gBS->CloseEvent (Event);

  Ctx           = (TYPE9_DEFER_CONTEXT *)Context;
  InputData     = (SMBIOS_TABLE_TYPE9 *)Ctx->RecordData;
  InputStrToken = (STR_TOKEN_INFO *)Ctx->StrToken;

  Table = PcieSlotInfoGetBoardTable (&Count);
  if (Table == NULL) {
    DEBUG ((DEBUG_WARN, "[%a] no board slot table, no Type 9 records\n", __func__));
    FreePool (Ctx);
    return;
  }

  for (Index = 0; Index < Count; Index++) {
    Slot = &Table[Index];

    Status = FindSlotPort (Slot, &Segment, &Bus, &DevFunc, &CardPresent);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "[%a] slot %a (domain %u): downstream port not enumerated, record skipped\n",
        __func__,
        Slot->Designation,
        Slot->Domain
        ));
      continue;
    }

    TotalSize    = sizeof (SMBIOS_TABLE_TYPE9) + sizeof (SMBIOS_TABLE_TYPE9_EXTENDED);
    SmbiosRecord = AllocateZeroPool (TotalSize);
    if (SmbiosRecord == NULL) {
      FreePool (Ctx);
      return;
    }

    CopyMem (SmbiosRecord, InputData, sizeof (SMBIOS_TABLE_TYPE9));

    SmbiosRecord->SlotType         = Slot->SlotType;
    SmbiosRecord->SlotDataBusWidth = Slot->SlotDataBusWidth;
    SmbiosRecord->DataBusWidth     = Slot->DataBusWidth;
    SmbiosRecord->CurrentUsage     = CardPresent ? SlotUsageInUse : SlotUsageAvailable;
    SmbiosRecord->SlotID           = Slot->SlotNumber;
    SmbiosRecord->SegmentGroupNum  = Segment;
    SmbiosRecord->BusNum           = Bus;
    SmbiosRecord->DevFuncNum       = DevFunc;
    SmbiosRecord->SlotLength       = Slot->SlotLength;
    CopyMem (&SmbiosRecord->SlotCharacteristics1, &Slot->Characteristics1, sizeof (UINT8));
    CopyMem (&SmbiosRecord->SlotCharacteristics2, &Slot->Characteristics2, sizeof (UINT8));

    AsciiStrToUnicodeStrS (Slot->Designation, SlotDesignation, ARRAY_SIZE (SlotDesignation));
    HiiSetString (
      mSmbiosPlatformDxeHiiHandle,
      InputStrToken->TokenArray[0],
      SlotDesignation,
      NULL
      );

    SmbiosRecordExtended.SlotInformation   = Slot->SlotInformation;
    SmbiosRecordExtended.SlotPhysicalWidth = Slot->PhysicalWidth;
    SmbiosRecordExtended.SlotPitch         = Slot->SlotPitch;
    SmbiosRecordExtended.SlotHeight        = Slot->SlotHeight;

    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE9) + sizeof (SMBIOS_TABLE_TYPE9_EXTENDED);
    CopyMem (
      (UINT8 *)SmbiosRecord->PeerGroups + SmbiosRecord->PeerGroupingCount * sizeof (SmbiosRecord->PeerGroups),
      (UINT8 *)&SmbiosRecordExtended,
      sizeof (SMBIOS_TABLE_TYPE9_EXTENDED)
      );

    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type9Record,
      (VOID *)&SmbiosRecord,
      sizeof (SMBIOS_TABLE_TYPE9) + sizeof (SMBIOS_TABLE_TYPE9_EXTENDED),
      InputStrToken
      );
    if (Type9Record == NULL) {
      FreePool (SmbiosRecord);
      FreePool (Ctx);
      return;
    }

    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type9Record, NULL);
    FreePool (Type9Record);
    FreePool (SmbiosRecord);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a] slot %a: AddRecord failed %r\n",
        __func__,
        Slot->Designation,
        Status
        ));
      FreePool (Ctx);
      return;
    }
  }

  FreePool (Ctx);
}

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformSystemSlot) {
  TYPE9_DEFER_CONTEXT  *Ctx;
  EFI_EVENT            Event;
  EFI_STATUS           Status;

  Ctx = AllocateZeroPool (sizeof (*Ctx));
  if (Ctx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ctx->RecordData = RecordData;
  Ctx->StrToken   = StrToken;

  //
  // Records need runtime SBDF, which is only final after PCI
  // enumeration; defer generation to ReadyToBoot.
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             PlatformSystemSlotOnReadyToBoot,
             Ctx,
             &Event
             );
  if (EFI_ERROR (Status)) {
    FreePool (Ctx);
    return Status;
  }

  return EFI_SUCCESS;
}
