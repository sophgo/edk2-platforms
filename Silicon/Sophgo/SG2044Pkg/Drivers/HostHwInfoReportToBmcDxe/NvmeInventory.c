/** @file
  NVMe inventory dump (DEBUG) and BMC OEM IPMI report.

  Copyright (c) 2026 SOPHGO Technologies Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <HostHwInfoReportToBmcOem.h>
#include "HwInventoryInternal.h"

#include <IndustryStandard/Nvme.h>
#include <IndustryStandard/Pci22.h>

#include <Protocol/DevicePath.h>
#include <Protocol/NvmExpressPassthru.h>
#include <Protocol/PciIo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#define NVME_DUMP_TIMEOUT  EFI_TIMER_PERIOD_SECONDS (5)
#define NVME_SI_GIGA       1000000000ULL

STATIC VOID
AsciiTrimTrailingSpaces (
  CHAR8  *Buf,
  UINTN  BufSize
  )
{
  UINTN  Len;

  if (Buf == NULL || BufSize == 0) {
    return;
  }

  Buf[BufSize - 1] = '\0';
  Len              = BufSize - 1;
  while ((Len > 0) && (Buf[Len - 1] == ' ')) {
    Buf[Len - 1] = '\0';
    Len--;
  }
}

STATIC VOID
NvmeDumpCapacityReadable (
  IN UINT64  Bytes
  )
{
  UINT64        TmpRem;
  UINT64        TbSiWhole;
  UINT64        GbSiRemain;
  UINT64        TibWhole;
  UINT64        GibRemain;
  CONST UINT64  SiTera = 1000000000000ULL;
  CONST UINT64  SiGiga = 1000000000ULL;
  CONST UINT64  IecTi = SIZE_1TB;
  CONST UINT64  IecGi = 1024ULL * 1024 * 1024;

  TbSiWhole = DivU64x64Remainder (Bytes, SiTera, &TmpRem);
  GbSiRemain = DivU64x64Remainder (TmpRem, SiGiga, NULL);

  TibWhole = DivU64x64Remainder (Bytes, IecTi, &TmpRem);
  GibRemain = DivU64x64Remainder (TmpRem, IecGi, NULL);

  DEBUG ((
    DEBUG_INFO,
    "    Capacity SI   : %Lu TB + %Lu GB (after removing full TB; SI decimal)\n",
    TbSiWhole,
    GbSiRemain
    ));
  DEBUG ((
    DEBUG_INFO,
    "    Capacity IEC  : %Lu TiB + %Lu GiB (after removing full TiB; binary)\n",
    TibWhole,
    GibRemain
    ));
}

STATIC VOID
NvmePrintSubnqnIfPresent (
  IN CONST NVME_ADMIN_CONTROLLER_DATA  *Ctrl
  )
{
  UINTN   Idx;
  CHAR8   Qn[257];
  BOOLEAN Any;

  Any = FALSE;
  for (Idx = 0; Idx < sizeof (Ctrl->Subnqn); Idx++) {
    if (Ctrl->Subnqn[Idx] != 0) {
      Any = TRUE;
      break;
    }
  }

  if (!Any) {
    return;
  }

  CopyMem (Qn, Ctrl->Subnqn, sizeof (Ctrl->Subnqn));
  Qn[sizeof (Ctrl->Subnqn)] = '\0';
  AsciiTrimTrailingSpaces (Qn, sizeof (Qn));

  if (Qn[0] == '\0') {
    return;
  }

  DEBUG ((DEBUG_INFO, "  Subsystem NQN : %a\n", Qn));
}

STATIC BOOLEAN
NvmeReadPciReportInfo (
  IN  EFI_HANDLE  ControllerHandle,
  OUT UINT16      *Segment,
  OUT UINT16      *VendorId,
  OUT UINT16      *DeviceId
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINTN                Seg;
  UINTN                Bus;
  UINTN                Dev;
  UINTN                Fn;
  UINT16               Vid;
  UINT16               Did;

  if (Segment != NULL) {
    *Segment = 0;
  }

  if (VendorId != NULL) {
    *VendorId = 0;
  }

  if (DeviceId != NULL) {
    *DeviceId = 0;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  mHwInventoryImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (!EFI_ERROR (PciIo->GetLocation (PciIo, &Seg, &Bus, &Dev, &Fn)) && (Segment != NULL)) {
    *Segment = (UINT16)Seg;
  }

  Vid = 0;
  Did = 0;
  if (!EFI_ERROR (PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_VENDOR_ID_OFFSET, 1, &Vid)) &&
      (VendorId != NULL))
  {
    *VendorId = Vid;
  }

  if (!EFI_ERROR (PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_DEVICE_ID_OFFSET, 1, &Did)) &&
      (DeviceId != NULL))
  {
    *DeviceId = Did;
  }

  return TRUE;
}

/**
  Print PCI topology for the NVM Express PCI function (similar to Shell "pci").
  Helps map an NVMe SSD to connector / RP / switch downstream port via BDF and path text.
**/
STATIC VOID
NvmeDumpPciTopo (
  IN EFI_HANDLE  ControllerHandle
  )
{
  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  UINTN                         Seg;
  UINTN                         Bus;
  UINTN                         Dev;
  UINTN                         Fn;
  UINT16                        PciVid;
  UINT16                        PciDid;
  UINT16                        SubsysVid;
  UINT16                        SubsysId;
  UINT8                         PciProgIf;
  UINT8                         PciSubclass;
  UINT8                         PciBaseClass;
  EFI_DEVICE_PATH_PROTOCOL      *Dp;
  CHAR16                        *PathUni;
  CHAR8                         *PathAsc;
  UINTN                         Idx;
  UINTN                         PathLenBytes;

  PciVid     = 0;
  PciDid     = 0;
  SubsysVid  = 0;
  SubsysId   = 0;
  PathUni    = NULL;
  PathAsc    = NULL;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  mHwInventoryImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "NvmeInfoDumpDxe: no EFI_PCI_IO on handle %p (%r).\n",
      ControllerHandle,
      Status
      ));
  } else {
    Status = PciIo->GetLocation (
                    PciIo,
                    &Seg,
                    &Bus,
                    &Dev,
                    &Fn
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "NvmeInfoDumpDxe: PciIo->GetLocation failed %r.\n",
        Status
        ));
    } else {
      DEBUG ((DEBUG_INFO, "  PCI address (Seg:Bus:Dev.Fn): %04lx:%02lx:%02lx.%lx\n",
              (UINTN)Seg,
              (UINTN)Bus,
              (UINTN)Dev,
              (UINTN)Fn
              ));
    }

    if (!EFI_ERROR (PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_VENDOR_ID_OFFSET, 1, &PciVid))) {
      DEBUG ((DEBUG_INFO, "  PCI cfg VID    : 0x%04x\n", PciVid));
    }

    if (!EFI_ERROR (PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_DEVICE_ID_OFFSET, 1, &PciDid))) {
      DEBUG ((DEBUG_INFO, "  PCI cfg DID    : 0x%04x\n", PciDid));
    }

    if (!EFI_ERROR (PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_SUBSYSTEM_VENDOR_ID_OFFSET, 1, &SubsysVid))) {
      DEBUG ((DEBUG_INFO, "  PCI Subsys VID : 0x%04x\n", SubsysVid));
    }

    if (!EFI_ERROR (PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_SUBSYSTEM_ID_OFFSET, 1, &SubsysId))) {
      DEBUG ((DEBUG_INFO, "  PCI Subsys ID  : 0x%04x\n", SubsysId));
    }

    PciProgIf     = 0;
    PciSubclass   = 0;
    PciBaseClass  = 0;

    if (
        !EFI_ERROR (PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET,     1, &PciProgIf)) &&
        !EFI_ERROR (PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET + 1, 1, &PciSubclass)) &&
        !EFI_ERROR (PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET + 2, 1, &PciBaseClass)))
    {
      DEBUG ((DEBUG_INFO, "  PCI Class Code : Base 0x%02x Sub 0x%02x ProgIf 0x%02x\n", PciBaseClass, PciSubclass, PciProgIf));
    }
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&Dp,
                  mHwInventoryImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "NvmeInfoDumpDxe: no device path on handle %p (%r).\n", ControllerHandle, Status));
    return;
  }

  PathUni = ConvertDevicePathToText (Dp, FALSE, TRUE);
  if (PathUni == NULL) {
    DEBUG ((DEBUG_ERROR, "NvmeInfoDumpDxe: ConvertDevicePathToText returned NULL.\n"));
    return;
  }

  PathLenBytes = (StrLen (PathUni) + 1) * sizeof (CHAR8);
  PathAsc      = AllocatePool (PathLenBytes);
  if (PathAsc != NULL) {
    for (Idx = 0; Idx < StrLen (PathUni); Idx++) {
      PathAsc[Idx] = (CHAR8)(PathUni[Idx] & MAX_UINT8);
    }

    PathAsc[Idx] = '\0';
    DEBUG ((DEBUG_INFO, "  Device path   : %a\n", PathAsc));
    FreePool (PathAsc);
  }

  FreePool (PathUni);
}

STATIC EFI_STATUS
NvmeIdentifyController (
  IN  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL *PassThru,
  OUT NVME_ADMIN_CONTROLLER_DATA           *Ctrl
  )
{
  EFI_STATUS                                Status;
  EFI_NVM_EXPRESS_COMMAND                   NvmeCmd;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  Packet;

  ZeroMem (&Packet, sizeof (Packet));
  ZeroMem (&NvmeCmd, sizeof (NvmeCmd));
  ZeroMem (&Completion, sizeof (Completion));

  NvmeCmd.Cdw0.Opcode = NVME_ADMIN_IDENTIFY_CMD;
  NvmeCmd.Nsid       = 0;
  NvmeCmd.Cdw10      = 1;
  NvmeCmd.Flags      = CDW10_VALID;

  Packet.NvmeCmd        = &NvmeCmd;
  Packet.NvmeCompletion = &Completion;
  Packet.TransferBuffer = Ctrl;
  Packet.TransferLength = sizeof (*Ctrl);
  Packet.CommandTimeout = NVME_DUMP_TIMEOUT;
  Packet.QueueType      = NVME_ADMIN_QUEUE;

  Status = PassThru->PassThru (
                       PassThru,
                       0,
                       &Packet,
                       NULL
                       );
  return Status;
}

STATIC EFI_STATUS
NvmeIdentifyNamespace (
  IN  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL *PassThru,
  IN  UINT32                             Nsid,
  OUT NVME_ADMIN_NAMESPACE_DATA           *Ns
  )
{
  EFI_STATUS                                Status;
  EFI_NVM_EXPRESS_COMMAND                   NvmeCmd;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  Packet;

  ZeroMem (&Packet, sizeof (Packet));
  ZeroMem (&NvmeCmd, sizeof (NvmeCmd));
  ZeroMem (&Completion, sizeof (Completion));

  NvmeCmd.Cdw0.Opcode = NVME_ADMIN_IDENTIFY_CMD;
  NvmeCmd.Nsid       = Nsid;
  NvmeCmd.Cdw10      = 0;
  NvmeCmd.Flags      = CDW10_VALID;

  Packet.NvmeCmd        = &NvmeCmd;
  Packet.NvmeCompletion = &Completion;
  Packet.TransferBuffer = Ns;
  Packet.TransferLength = sizeof (*Ns);
  Packet.CommandTimeout = NVME_DUMP_TIMEOUT;
  Packet.QueueType      = NVME_ADMIN_QUEUE;

  Status = PassThru->PassThru (
                       PassThru,
                       Nsid,
                       &Packet,
                       NULL
                       );
  return Status;
}

STATIC VOID
NvmeReportControllerToBmc (
  IN UINT8         DiskIndex,
  IN CONST CHAR8   *MnStr,
  IN CONST CHAR8   *SnStr,
  IN UINT64        TotalBytes,
  IN UINT16        PciSegment,
  IN UINT16        PciVendorId,
  IN UINT16        PciDeviceId,
  IN EFI_HANDLE    ControllerHandle
  )
{
  BMC_HW_INFO_NVME_IPMI_PAYLOAD  Payload;
  EFI_STATUS                     Status;
  EFI_DEVICE_PATH_PROTOCOL       *DevicePath;

  ZeroMem (&Payload, sizeof (Payload));
  Payload.DiskIndex     = DiskIndex;
  Payload.PlatformType  = HostHwInfoReportToBmcGetPlatformType ();
  Payload.PciSegment    = PciSegment;
  Payload.PciVendorId   = PciVendorId;
  Payload.PciDeviceId   = PciDeviceId;
  HostHwInfoReportToBmcAsciiCopyPadded (
    Payload.Model,
    sizeof (Payload.Model),
    MnStr,
    AsciiStrLen (MnStr)
    );
  HostHwInfoReportToBmcAsciiCopyPadded (
    Payload.Serial,
    sizeof (Payload.Serial),
    SnStr,
    AsciiStrLen (SnStr)
    );
  Payload.CapacitySiGB = DivU64x64Remainder (TotalBytes, NVME_SI_GIGA, NULL);

  DevicePath = NULL;
  if (!EFI_ERROR (gBS->OpenProtocol (
                      ControllerHandle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID **)&DevicePath,
                      mHwInventoryImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      )))
  {
    HostHwInfoReportToBmcResolvePhysicalSlot (
      DevicePath,
      &Payload.PhysicalSlot
      );
  }

  Status = HostHwInfoReportToBmcSubmitNvme (&Payload);
  DEBUG ((
    DEBUG_INFO,
    "HostHwInfoReportToBmcDxe: BMC NVMe[%u] seg=%04x VID/DID %04x/%04x MN=%a SN=%a %Lu GB (SI) => %r\n",
    DiskIndex,
    Payload.PciSegment,
    Payload.PciVendorId,
    Payload.PciDeviceId,
    MnStr,
    SnStr,
    Payload.CapacitySiGB,
    Status
    ));
}

VOID
DumpNvmeInventory (
  VOID
  )
{
  EFI_STATUS                        Status;
  UINTN                             Index;
  UINTN                             NumHandles;
  EFI_HANDLE                       *Handles;
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL *PassThru;
  NVME_ADMIN_CONTROLLER_DATA         *Ctrl;
  NVME_ADMIN_NAMESPACE_DATA           *Ns;
  UINT32                              NsidWalker;
  UINT32                              NextNsid;
  UINT8                               DiskIndex;
  UINT16                              PciSegment;
  UINT16                              PciVendorId;
  UINT16                              PciDeviceId;

  Handles   = NULL;
  Ctrl      = NULL;
  Ns        = NULL;
  DiskIndex = 0;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiNvmExpressPassThruProtocolGuid,
                  NULL,
                  &NumHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "NvmeInfoDumpDxe: no NVMe PassThru (%r).\n",
      Status
      ));
    return;
  }

  Ctrl = AllocateZeroPool (sizeof (*Ctrl));
  Ns   = AllocateZeroPool (sizeof (*Ns));
  if ((Ctrl == NULL) || (Ns == NULL)) {
    goto Cleanup;
  }

  for (Index = 0; Index < NumHandles; Index++) {
    Status = gBS->OpenProtocol (
                    Handles[Index],
                    &gEfiNvmExpressPassThruProtocolGuid,
                    (VOID **)&PassThru,
                    mHwInventoryImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    PciSegment  = 0;
    PciVendorId = 0;
    PciDeviceId = 0;
    if (!NvmeReadPciReportInfo (
           Handles[Index],
           &PciSegment,
           &PciVendorId,
           &PciDeviceId
           ))
    {
      DEBUG ((
        DEBUG_ERROR,
        "NvmeInfoDumpDxe: PCI segment/VID/DID unavailable for handle %p\n",
        Handles[Index]
        ));
    }

    NvmeDumpPciTopo (Handles[Index]);

    Status = NvmeIdentifyController (PassThru, Ctrl);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "NvmeInfoDumpDxe: Identify Controller handle %p failed %r\n",
        Handles[Index],
        Status
        ));
      continue;
    }

    DEBUG ((DEBUG_INFO, "=== NVMe controller [handle %p] ===\n", Handles[Index]));
    DEBUG ((DEBUG_INFO, "  PCI VID     : 0x%04x\n", Ctrl->Vid));
    DEBUG ((DEBUG_INFO, "  PCI SSVID   : 0x%04x\n", Ctrl->Ssvid));
    DEBUG ((DEBUG_INFO, "  Num NS (NN) : %u\n", Ctrl->Nn));

    {
      CHAR8   MnStr[sizeof (Ctrl->Mn) + 1];
      CHAR8   SnStr[sizeof (Ctrl->Sn) + 1];
      CHAR8   FrStr[sizeof (Ctrl->Fr) + 1];
      UINT64  ControllerTotalBytes;

      ControllerTotalBytes = 0;

      CopyMem (MnStr, Ctrl->Mn, sizeof (Ctrl->Mn));
      CopyMem (SnStr, Ctrl->Sn, sizeof (Ctrl->Sn));
      CopyMem (FrStr, Ctrl->Fr, sizeof (Ctrl->Fr));
      MnStr[sizeof (Ctrl->Mn)] = '\0';
      SnStr[sizeof (Ctrl->Sn)] = '\0';
      FrStr[sizeof (Ctrl->Fr)] = '\0';
      AsciiTrimTrailingSpaces (MnStr, sizeof (MnStr));
      AsciiTrimTrailingSpaces (SnStr, sizeof (SnStr));
      AsciiTrimTrailingSpaces (FrStr, sizeof (FrStr));

      DEBUG ((DEBUG_INFO, "  Model (MN)  : %a\n", MnStr));
      NvmePrintSubnqnIfPresent (Ctrl);

      DEBUG ((DEBUG_INFO, "  Serial      : %a\n", SnStr));
      DEBUG ((DEBUG_INFO, "  Firmware    : %a\n", FrStr));

      NsidWalker = 0xFFFFFFFF;
      for ( ; ; ) {
      NextNsid = NsidWalker;
      Status   = PassThru->GetNextNamespace (PassThru, &NextNsid);
      if (Status == EFI_NOT_FOUND) {
        break;
      }

      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "NvmeInfoDumpDxe: GetNextNamespace failed %r\n",
          Status
          ));
        break;
      }

      NsidWalker = NextNsid;
      ZeroMem (Ns, sizeof (*Ns));
      Status = NvmeIdentifyNamespace (PassThru, NextNsid, Ns);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "NvmeInfoDumpDxe: Identify NS %u failed %r\n",
          NextNsid,
          Status
          ));
        continue;
      }

      {
        UINT32  LbaFmtIdx;
        UINT32  Lbads;
        UINT64  BlockSize;
        UINT64  TotalBytes;

        LbaFmtIdx = Ns->Flbas & 0xF;
        if (LbaFmtIdx > Ns->Nlbaf) {
          DEBUG ((
            DEBUG_ERROR,
            "NvmeInfoDumpDxe: NS %u FLBAS index %u invalid (Nlbaf %u)\n",
            NextNsid,
            LbaFmtIdx,
            Ns->Nlbaf
            ));
          continue;
        }

        if (Ns->LbaFormat[LbaFmtIdx].Ms != 0) {
          DEBUG ((
            DEBUG_ERROR,
            "NvmeInfoDumpDxe: NS %u metadata size %u (capacity below uses data LBA only)\n",
            NextNsid,
            Ns->LbaFormat[LbaFmtIdx].Ms
            ));
        }

        Lbads     = Ns->LbaFormat[LbaFmtIdx].Lbads;
        BlockSize = LShiftU64 (1, Lbads);
        TotalBytes = MultU64x64 (Ns->Nsze, BlockSize);

        DEBUG ((DEBUG_INFO, "  --- Namespace %u ---\n", NextNsid));
        DEBUG ((DEBUG_INFO, "    NSZE (blocks) : 0x%Lx\n", Ns->Nsze));
        DEBUG ((DEBUG_INFO, "    LBA bytes     : %Lu\n", BlockSize));
        DEBUG ((DEBUG_INFO, "    Capacity      : %Lu bytes\n", TotalBytes));
        NvmeDumpCapacityReadable (TotalBytes);
        ControllerTotalBytes += TotalBytes;
      }
    }

      NvmeReportControllerToBmc (
        DiskIndex,
        MnStr,
        SnStr,
        ControllerTotalBytes,
        PciSegment,
        PciVendorId,
        PciDeviceId,
        Handles[Index]
        );
      DiskIndex++;
    }
  }

Cleanup:
  if (Handles != NULL) {
    FreePool (Handles);
  }

  if (Ctrl != NULL) {
    FreePool (Ctrl);
  }

  if (Ns != NULL) {
    FreePool (Ns);
  }
}
