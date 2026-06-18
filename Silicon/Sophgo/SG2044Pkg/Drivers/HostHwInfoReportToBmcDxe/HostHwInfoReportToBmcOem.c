/** @file
  Submit NVMe / NIC inventory records to BMC over IPMI (UART transport).

  Copyright (c) 2026 SOPHGO Technologies Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <HostHwInfoReportToBmcOem.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

UINT8
HostHwInfoReportToBmcGetPlatformType (
  VOID
  )
{
  CONST CHAR16  *ProductName;

  ProductName = (CONST CHAR16 *)FixedPcdGetPtr (PcdProductName);
  if (ProductName == NULL) {
    return BMC_HW_INFO_PLATFORM_TYPE_RESERVED;
  }

  if (StrCmp (ProductName, L"SRA3-40-8") == 0) {
    return BMC_HW_INFO_PLATFORM_TYPE_SRA3_40_8;
  }

  if (StrCmp (ProductName, L"SRA3-40") == 0) {
    return BMC_HW_INFO_PLATFORM_TYPE_SRA3_40;
  }

  return BMC_HW_INFO_PLATFORM_TYPE_RESERVED;
}

VOID
HostHwInfoReportToBmcAsciiCopyPadded (
  OUT CHAR8        *Dst,
  IN  UINTN        DstSize,
  IN  CONST CHAR8  *Src,
  IN  UINTN        SrcMax
  )
{
  UINTN  Idx;

  if ((Dst == NULL) || (DstSize == 0)) {
    return;
  }

  ZeroMem (Dst, DstSize);
  if (Src == NULL) {
    return;
  }

  for (Idx = 0; (Idx < SrcMax) && (Idx < (DstSize - 1)); Idx++) {
    if (Src[Idx] == '\0') {
      break;
    }

    Dst[Idx] = Src[Idx];
  }
}

STATIC VOID
HostHwInfoReportToBmcDumpBytes (
  IN CONST CHAR8  *Label,
  IN CONST UINT8  *Data,
  IN UINT32       Size
  )
{
  UINT32  Offset;
  UINT32  Chunk;
  UINT32  Idx;
  CHAR8   Line[80];
  CHAR8   *Walk;
  UINTN   Pos;

  if ((Data == NULL) || (Size == 0)) {
    return;
  }

  DEBUG ((DEBUG_INFO, "%a (%u bytes):\n", Label, Size));
  for (Offset = 0; Offset < Size; Offset += 16) {
    Chunk = Size - Offset;
    if (Chunk > 16) {
      Chunk = 16;
    }

    Walk = Line;
    Pos  = 0;
    Pos += AsciiSPrint (Walk + Pos, sizeof (Line) - Pos, "  %04x:", Offset);
    for (Idx = 0; Idx < Chunk; Idx++) {
      Pos += AsciiSPrint (Walk + Pos, sizeof (Line) - Pos, " %02x", Data[Offset + Idx]);
    }

    DEBUG ((DEBUG_INFO, "%a\n", Line));
  }
}

STATIC VOID
HostHwInfoReportToBmcDumpNvmePayload (
  IN CONST BMC_HW_INFO_NVME_IPMI_PAYLOAD  *Payload
  )
{
  if (Payload == NULL) {
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "HostHwInfoReportToBmc IPMI NVMe request: NetFn=0x%02x Cmd=0x%02x len=%u\n",
    BMC_HW_INFO_IPMI_NETFN,
    BMC_HW_INFO_NVME_IPMI_CMD,
    (UINT32)sizeof (*Payload)
    ));
  DEBUG ((
    DEBUG_INFO,
    "  disk_index=%u platform_type=%u pci_segment=0x%04x vid=0x%04x did=0x%04x "
    "physical_slot=%u capacity_si_gb=%Lu\n",
    Payload->DiskIndex,
    Payload->PlatformType,
    Payload->PciSegment,
    Payload->PciVendorId,
    Payload->PciDeviceId,
    Payload->PhysicalSlot,
    Payload->CapacitySiGB
    ));
  DEBUG ((
    DEBUG_INFO,
    "  model=\"%a\" serial=\"%a\"\n",
    Payload->Model,
    Payload->Serial
    ));
  HostHwInfoReportToBmcDumpBytes (
    "  request_data_hex",
    (CONST UINT8 *)Payload,
    (UINT32)sizeof (*Payload)
    );
}

STATIC VOID
HostHwInfoReportToBmcDumpNicPayload (
  IN CONST BMC_HW_INFO_NIC_IPMI_PAYLOAD  *Payload
  )
{
  if (Payload == NULL) {
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "HostHwInfoReportToBmc IPMI NIC request: NetFn=0x%02x Cmd=0x%02x len=%u\n",
    BMC_HW_INFO_IPMI_NETFN,
    BMC_HW_INFO_NIC_IPMI_CMD,
    (UINT32)sizeof (*Payload)
    ));
  DEBUG ((
    DEBUG_INFO,
    "  nic_index=%u platform_type=%u pci_segment=0x%04x vid=0x%04x did=0x%04x "
    "physical_slot=%u\n",
    Payload->NicIndex,
    Payload->PlatformType,
    Payload->PciSegment,
    Payload->PciVendorId,
    Payload->PciDeviceId,
    Payload->PhysicalSlot
    ));
  DEBUG ((
    DEBUG_INFO,
    "  mac=%02x:%02x:%02x:%02x:%02x:%02x manufacturer=\"%a\" model=\"%a\"\n",
    Payload->MacAddress[0],
    Payload->MacAddress[1],
    Payload->MacAddress[2],
    Payload->MacAddress[3],
    Payload->MacAddress[4],
    Payload->MacAddress[5],
    Payload->Manufacturer,
    Payload->Model
    ));
  HostHwInfoReportToBmcDumpBytes (
    "  request_data_hex",
    (CONST UINT8 *)Payload,
    (UINT32)sizeof (*Payload)
    );
}

STATIC EFI_STATUS
HostHwInfoReportToBmcSubmitPayload (
  IN UINT8        Command,
  IN CONST VOID   *Payload,
  IN UINT32       PayloadSize
  )
{
  EFI_STATUS  Status;
  UINT8       Response[8];
  UINT32      ResponseSize;

  if ((Payload == NULL) || (PayloadSize == 0) ||
      (PayloadSize > BMC_HW_INFO_IPMI_MAX_LEN))
  {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Response, sizeof (Response));
  ResponseSize = sizeof (Response);

  Status = IpmiSubmitCommand (
             BMC_HW_INFO_IPMI_NETFN,
             Command,
             (UINT8 *)(UINTN)Payload,
             PayloadSize,
             Response,
             &ResponseSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: IpmiSubmitCommand NetFn=0x%02x Cmd=0x%02x %r\n",
      __func__,
      BMC_HW_INFO_IPMI_NETFN,
      Command,
      Status
      ));
    return Status;
  }

  if ((ResponseSize > 0) && (Response[0] != 0x00)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: BMC completion 0x%02x (NetFn=0x%02x Cmd=0x%02x)\n",
      __func__,
      Response[0],
      BMC_HW_INFO_IPMI_NETFN,
      Command
      ));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HostHwInfoReportToBmcSubmitNvme (
  IN CONST BMC_HW_INFO_NVME_IPMI_PAYLOAD  *Payload
  )
{
  if (Payload == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HostHwInfoReportToBmcDumpNvmePayload (Payload);

  return HostHwInfoReportToBmcSubmitPayload (
           BMC_HW_INFO_NVME_IPMI_CMD,
           Payload,
           (UINT32)sizeof (*Payload)
           );
}

EFI_STATUS
EFIAPI
HostHwInfoReportToBmcSubmitNic (
  IN CONST BMC_HW_INFO_NIC_IPMI_PAYLOAD  *Payload
  )
{
  if (Payload == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HostHwInfoReportToBmcDumpNicPayload (Payload);

  return HostHwInfoReportToBmcSubmitPayload (
           BMC_HW_INFO_NIC_IPMI_CMD,
           Payload,
           (UINT32)sizeof (*Payload)
           );
}
