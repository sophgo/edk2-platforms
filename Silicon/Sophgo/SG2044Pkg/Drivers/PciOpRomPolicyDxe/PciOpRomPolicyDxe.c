/** @file
  PCI Option ROM allow-list policy driver.

  Installs EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL to restrict
  Option ROM processing to the curated list of VID/DID pairs below.
  Devices on the list proceed with normal OpROM loading (e.g. to run
  under the MultiArchUefiPkg x64 emulator); every other PCI device gets
  a ROM-skip resource descriptor, so PciBusDxe never probes its Option
  ROM BAR and the built-in firmware drivers (when present) own the
  device.

  To allow another card's OpROM, add its {VID, DID} pair to
  mOpRomAllowList below and rebuild.

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/IncompatiblePciDeviceSupport.h>

typedef struct {
  UINT16    VendorId;
  UINT16    DeviceId;
} OP_ROM_ALLOW_ENTRY;

//
// The only devices whose Option ROMs are probed and loaded.
//
STATIC CONST OP_ROM_ALLOW_ENTRY  mOpRomAllowList[] = {
  { 0x1000, 0x005D },   // LSI/AVAGO MegaRAID SAS 9361-8i (3108)
  { 0x1002, 0x6779 },   // AMD Radeon R5 230
  { 0x1002, 0x67DF },   // AMD Radeon RX 580
};

//
// QWORD Address Space Descriptor usage per PI spec Table 20: with
// AddrTranslationOffset == PCI_MAX_BAR (6) and SpecificFlag == 0,
// PciBusDxe skips the Option ROM BAR (does not probe the ROM).
// A private copy is handed to PciBusDxe on every CheckDevice call,
// so the caller can iterate and free it independently.
//
#pragma pack (1)
typedef struct {
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR    Desc;
  EFI_ACPI_END_TAG_DESCRIPTOR          End;
} ROM_SKIP_CONFIGURATION;
#pragma pack ()

/**
  Returns special resource configuration for devices NOT on the allow-list.

  @param[in]  This               Protocol instance.
  @param[in]  VendorId           Vendor ID.
  @param[in]  DeviceId           Device ID.
  @param[in]  RevisionId         Revision ID (unused).
  @param[in]  SubsystemVendorId  Subsystem vendor ID (unused).
  @param[in]  SubsystemDeviceId  Subsystem device ID (unused).
  @param[out] Configuration      ROM-skip descriptor for unlisted devices.

  @retval EFI_SUCCESS      Unlisted device; Configuration carries the
                           ROM-skip descriptor.
  @retval EFI_UNSUPPORTED  Allow-listed device; normal OpROM handling.

**/
STATIC
EFI_STATUS
EFIAPI
PciOpRomPolicyCheckDevice (
  IN  EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL  *This,
  IN  UINTN                                          VendorId,
  IN  UINTN                                          DeviceId,
  IN  UINTN                                          RevisionId,
  IN  UINTN                                          SubsystemVendorId,
  IN  UINTN                                          SubsystemDeviceId,
  OUT VOID                                           **Configuration
  )
{
  CONST ROM_SKIP_CONFIGURATION  Template = {
    {
      ACPI_ADDRESS_SPACE_DESCRIPTOR,                   // Desc
      sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR),      // Len
      0,                                               // ResType (ignored)
      0,                                               // GenFlag
      0,                                               // SpecificFlag
      0,                                               // AddrSpaceGranularity
      0,                                               // AddrRangeMin
      0,                                               // AddrRangeMax
      PCI_MAX_BAR,                                     // AddrTranslationOffset
      0                                                // AddrLen
    },
    {
      ACPI_END_TAG_DESCRIPTOR,                         // Desc
      0                                                // Checksum
    }
  };
  CONST OP_ROM_ALLOW_ENTRY  *Entry;
  UINTN                     Index;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < ARRAY_SIZE (mOpRomAllowList); Index++) {
    Entry = &mOpRomAllowList[Index];
    if ((Entry->VendorId == (UINT16)VendorId) &&
        (Entry->DeviceId == (UINT16)DeviceId))
    {
      //
      // Allow-listed: no special requirement, OpROM is probed and
      // loaded normally (subject to X64EMU_ENABLE dispatch policy).
      //
      *Configuration = NULL;
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Not allow-listed: tell PciBusDxe to skip the Option ROM entirely.
  //
  *Configuration = AllocateCopyPool (sizeof (Template), &Template);
  if (*Configuration == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

STATIC EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL  mPciOpRomPolicy = {
  PciOpRomPolicyCheckDevice
};

/**
  Driver entry point: install the policy protocol before PciBusDxe
  performs its first enumeration.

  @param[in] ImageHandle  Image handle.
  @param[in] SystemTable  System table.

  @retval EFI_SUCCESS  Protocol installed.

**/
EFI_STATUS
EFIAPI
PciOpRomPolicyDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle  = NULL;
  Status  = gBS->InstallProtocolInterface (
                   &Handle,
                   &gEfiIncompatiblePciDeviceSupportProtocolGuid,
                   EFI_NATIVE_INTERFACE,
                   &mPciOpRomPolicy
                   );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
