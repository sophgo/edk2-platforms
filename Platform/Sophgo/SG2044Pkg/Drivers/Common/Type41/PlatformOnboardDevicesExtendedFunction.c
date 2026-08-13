/** @file
  SMBIOS Type 41 (Onboard Devices Extended) - dynamic SBDF.

  Soldered onboard devices are described with a static device-path key
  (Domain + DevPath[]).  At ReadyToBoot, when PCI enumeration is complete,
  each device is found by matching its device path among the enumerated
  PciIo handles, and its runtime SBDF (Segment/Bus/Device/Function) is
  read via PciIo->GetLocation().  This is immune to bus-number
  reassignment by the OS (RISC-V Linux reassigns all busses).

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2025-2026. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>

#include "SmbiosPlatformDxe.h"

typedef struct {
  VOID  *RecordData;
  VOID  *StrToken;
} TYPE41_DEFER_CONTEXT;

/**
  Find an onboard device by device-path matching among the enumerated
  PCI devices and read its runtime SBDF.

  @param[in]  Entry    Onboard-device table entry (carries the match key).
  @param[out] Segment  Receives the device's PCI segment.
  @param[out] Bus      Receives the device's bus number.
  @param[out] DevFunc  Receives the device's Device/Function number.

  @retval EFI_SUCCESS    Device found.
  @retval EFI_NOT_FOUND  Device not enumerated.
**/
STATIC
EFI_STATUS
FindOnboardDevice (
  IN  CONST TYPE41_ONBOARD_ENTRY  *Entry,
  OUT UINT16                      *Segment,
  OUT UINT8                       *Bus,
  OUT UINT8                       *DevFunc
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
    if (!Topo.FoundRoot || (Topo.RootUid != Entry->Domain)) {
      continue;
    }

    //
    // Exact match: the device itself (PathLen equal, prefix equal).
    //
    if ((Topo.PciCount != Entry->PathLen) ||
        (CompareMem (Topo.PciDevices, Entry->DevPath, Entry->PathLen) != 0))
    {
      continue;
    }

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
    FreePool (Handles);
    return EFI_SUCCESS;
  }

  FreePool (Handles);
  return EFI_NOT_FOUND;
}

/**
  ReadyToBoot callback: emit one Type 41 record per onboard-device
  entry, with runtime SBDF filled from the enumerated topology.
**/
STATIC
VOID
EFIAPI
OnboardDevicesOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  TYPE41_DEFER_CONTEXT  *Ctx;
  TYPE41_ONBOARD_ENTRY  *Entry;
  STR_TOKEN_INFO        *InputStrToken;
  UINT16                Segment;
  UINT8                 Bus;
  UINT8                 DevFunc;
  EFI_STATUS            Status;
  SMBIOS_TABLE_TYPE41   Record;
  SMBIOS_TABLE_TYPE41   *RecordPtr;
  SMBIOS_TABLE_TYPE41   *Type41Record;

  gBS->CloseEvent (Event);

  Ctx           = (TYPE41_DEFER_CONTEXT *)Context;
  Entry         = (TYPE41_ONBOARD_ENTRY *)Ctx->RecordData;
  InputStrToken = (STR_TOKEN_INFO *)Ctx->StrToken;

  while (Entry->Record.Hdr.Type != NULL_TERMINATED_TYPE) {
    Status = FindOnboardDevice (Entry, &Segment, &Bus, &DevFunc);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "[%a] onboard device (domain %u) not enumerated, record skipped\n",
        __func__,
        Entry->Domain
        ));
      Entry++;
      InputStrToken++;
      continue;
    }

    //
    // Copy the static template, fill runtime SBDF, then let the shared
    // framework create + add the record.  CreateTable takes VOID **,
    // so we pass a pointer to a pointer to our stack-local copy.
    //
    CopyMem (&Record, &Entry->Record, sizeof (SMBIOS_TABLE_TYPE41));
    Record.SegmentGroupNum = Segment;
    Record.BusNum          = Bus;
    Record.DevFuncNum      = DevFunc;
    RecordPtr              = &Record;

    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type41Record,
      (VOID *)&RecordPtr,
      sizeof (SMBIOS_TABLE_TYPE41),
      InputStrToken
      );
    if (Type41Record == NULL) {
      DEBUG ((DEBUG_ERROR, "[%a] CreateTable failed\n", __func__));
      FreePool (Ctx);
      return;
    }

    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type41Record, NULL);
    FreePool (Type41Record);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a] AddRecord failed %r\n",
        __func__,
        Status
        ));
      FreePool (Ctx);
      return;
    }

    Entry++;
    InputStrToken++;
  }

  FreePool (Ctx);
}

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformOnboardDevicesExtended) {
  TYPE41_DEFER_CONTEXT  *Ctx;
  EFI_EVENT            Event;
  EFI_STATUS           Status;

  Ctx = AllocateZeroPool (sizeof (*Ctx));
  if (Ctx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ctx->RecordData = RecordData;
  Ctx->StrToken   = StrToken;

  //
  // SBDF must be read after PCI enumeration; defer to ReadyToBoot.
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             OnboardDevicesOnReadyToBoot,
             Ctx,
             &Event
             );
  if (EFI_ERROR (Status)) {
    FreePool (Ctx);
    return Status;
  }

  return EFI_SUCCESS;
}
