/** @file
  Host-defined OEM IPMI payloads: BIOS reports hardware inventory to BMC.

  Protocol is owned by the host (UEFI). BMC implements parsing for these
  NetFn/Cmd/Data layouts.

  Copyright (c) 2026 SOPHGO Technologies Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HOST_HW_INFO_REPORT_TO_BMC_OEM_H_
#define HOST_HW_INFO_REPORT_TO_BMC_OEM_H_

#include <Uefi.h>

#include <Library/IpmiLib.h>

//
// BMC OEM inventory NetFn / commands
//
#define BMC_HW_INFO_IPMI_NETFN       0x3A
#define BMC_HW_INFO_NVME_IPMI_CMD    0x02
#define BMC_HW_INFO_NIC_IPMI_CMD     0x03

#define BMC_HW_INFO_IPMI_MAX_LEN     128
#define BMC_HW_INFO_NVME_MODEL_LEN   40
#define BMC_HW_INFO_NVME_SERIAL_LEN  20
#define BMC_HW_INFO_NIC_MFG_LEN      32
#define BMC_HW_INFO_NIC_MODEL_LEN    64

//
// Platform type in NVMe / NIC IPMI payloads (offset 1).
//
#define BMC_HW_INFO_PLATFORM_TYPE_SRA3_40     0
#define BMC_HW_INFO_PLATFORM_TYPE_SRA3_40_8   1
#define BMC_HW_INFO_PLATFORM_TYPE_RESERVED    2

//
// physical_slot (single byte at end of NVMe / NIC payloads).
// Semantics depend on platform_type: BMC must use offset-1 platform_type
//
#define BMC_HW_INFO_PHYSICAL_SLOT_UNKNOWN       0xFF

//
// SRA3-40 (platform_type = 0)
//
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE1               1
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE2               2
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE3               3
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE4               4
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_MB_M2               5
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_BACKPLANE_M2        6
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_ONBOARD_NIC_PORT0   7
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_ONBOARD_NIC_PORT1   8
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE5               9
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_PCIE6               10

//
// SRA3-40-8 (platform_type = 1)
//
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE1              1
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE2              2
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE3              3
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE4              4
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE5              5
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE6              6
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE7              7
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE8              8
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_MB_M2              9
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_BACKPLANE_M2_1     10
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_BACKPLANE_M2_2     11
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_BACKPLANE_M2_3     12
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_ONBOARD_NIC_PORT0  13
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_ONBOARD_NIC_PORT1  14
#define BMC_HW_INFO_PHYSICAL_SLOT_SRA3_40_8_PCIE9              15

#pragma pack(1)

typedef struct {
  UINT8    DiskIndex;
  UINT8    PlatformType;
  UINT16   PciSegment;
  UINT16   PciVendorId;
  UINT16   PciDeviceId;
  CHAR8    Model[BMC_HW_INFO_NVME_MODEL_LEN];
  CHAR8    Serial[BMC_HW_INFO_NVME_SERIAL_LEN];
  UINT64   CapacitySiGB;  // Total namespace bytes / 10^9 (SI decimal GB, truncate)
  UINT8    PhysicalSlot;
} BMC_HW_INFO_NVME_IPMI_PAYLOAD;

typedef struct {
  UINT8    NicIndex;
  UINT8    PlatformType;
  UINT16   PciSegment;
  UINT8    MacAddress[6];
  CHAR8    Manufacturer[BMC_HW_INFO_NIC_MFG_LEN];
  CHAR8    Model[BMC_HW_INFO_NIC_MODEL_LEN];
  UINT16   PciVendorId;
  UINT16   PciDeviceId;
  UINT8    PhysicalSlot;
} BMC_HW_INFO_NIC_IPMI_PAYLOAD;

#pragma pack()

UINT8
HostHwInfoReportToBmcGetPlatformType (
  VOID
  );

VOID
HostHwInfoReportToBmcAsciiCopyPadded (
  OUT CHAR8        *Dst,
  IN  UINTN        DstSize,
  IN  CONST CHAR8  *Src,
  IN  UINTN        SrcMax
  );

EFI_STATUS
EFIAPI
HostHwInfoReportToBmcSubmitNvme (
  IN CONST BMC_HW_INFO_NVME_IPMI_PAYLOAD  *Payload
  );

EFI_STATUS
EFIAPI
HostHwInfoReportToBmcSubmitNic (
  IN CONST BMC_HW_INFO_NIC_IPMI_PAYLOAD  *Payload
  );

#endif
