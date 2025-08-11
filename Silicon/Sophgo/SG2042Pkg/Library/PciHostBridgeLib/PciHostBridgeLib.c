/** @file

  PCI Host Bridge Library instance for Sophgo SG2042

  Copyright (c) 2023, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Include/PlatformPciLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/PciHostBridgeResourceAllocation.h>

#ifndef MDEPKG_NDEBUG
STATIC CHAR16 CONST * CONST mPciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};
#endif

#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

STATIC EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mEfiPciRootBridgeDevicePath = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8) (sizeof(ACPI_HID_DEVICE_PATH)),
        (UINT8) ((sizeof(ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID(0x0A08), // PCI Express
    0
  }, {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

STATIC PCI_ROOT_BRIDGE mRootBridgeTemplate = {
  0,                                              // Segment
  0,                                              // Supports
  0,                                              // Attributes
  TRUE,                                           // DmaAbove4G
  FALSE,                                          // NoExtendedConfigSpace
  FALSE,                                          // ResourceAssigned
  EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM |          // AllocationAttributes
  EFI_PCI_HOST_BRIDGE_MEM64_DECODE,
  {
    // Bus
    0,
    0xFF,
    0
  }, {
    // Io
    0,
    0,
    0
  }, {
    // Mem
    MAX_UINT64,
    0,
    0
  }, {
    // MemAbove4G
    MAX_UINT64,
    0,
    0
  }, {
    // PMem
    MAX_UINT64,
    0,
    0
  }, {
    // PMemAbove4G
    MAX_UINT64,
    0,
    0
  },
  (EFI_DEVICE_PATH_PROTOCOL *)&mEfiPciRootBridgeDevicePath
};

STATIC
EFI_STATUS
ConstructRootBridge (
    PCI_ROOT_BRIDGE      *Bridge,
    MANGO_PCI_RESOURCE   *Resource
    )
{
  EFI_PCI_ROOT_BRIDGE_DEVICE_PATH *DevicePath;
  CopyMem (Bridge, &mRootBridgeTemplate, sizeof *Bridge);
  Bridge->Segment = Resource->Segment;
  Bridge->Bus.Base = Resource->BusBase;
  Bridge->Bus.Limit = Resource->BusBase + Resource->BusSize - 1;
  Bridge->Io.Base = Resource->IoBase;
  Bridge->Io.Translation = Resource->IoTranslation;
  // IoLimit is actually an address in CPU view
  // Bridge->Io.Limit = Resource->IoBase + Bridge->Io.Translation;
  Bridge->Io.Limit = Resource->IoBase + Resource->IoSize - 1;

  Bridge->MemAbove4G.Base = Resource->Mmio64Base;
  Bridge->MemAbove4G.Limit = Resource->Mmio64Base + Resource->Mmio64Size - 1;;
  Bridge->MemAbove4G.Translation = Resource->Mmio64Translation;
  Bridge->Mem.Base = Resource->Mmio32Base;
  Bridge->Mem.Limit = Resource->Mmio32Base + Resource->Mmio32Size - 1;
  Bridge->Mem.Translation = Resource->Mmio32Translation;

  //
  // No separate ranges for prefetchable and non-prefetchable BARs
  //
  Bridge->PMem.Base           = MAX_UINT64;
  Bridge->PMem.Limit          = 0;
  Bridge->PMemAbove4G.Base    = MAX_UINT64;
  Bridge->PMemAbove4G.Limit   = 0;

  DevicePath = AllocateCopyPool (
                 sizeof (EFI_PCI_ROOT_BRIDGE_DEVICE_PATH), 
                 (VOID *)&mEfiPciRootBridgeDevicePath
                 );
  if (DevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a]:[%dL] AllocatePool failed!\n", __func__, __LINE__));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Embedded the Root Complex Index into the DevicePath
  // This will be used later by the platform NotifyPhase()
  //
  DevicePath->AcpiDevicePath.UID = Bridge->Segment;

  Bridge->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;
  return EFI_SUCCESS;
}

/**
  Return all the root bridge instances in an array.

  @param Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges()
          when it's not used.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  UINTN *Count
  )
{
  EFI_STATUS                  Status;
  UINTN                       PortIndex;
  UINTN                       LinkIndex;
  UINT8                       PcieEnableCount;
  PCI_ROOT_BRIDGE             *Bridges;

  //
  // Set default value to 0 in case we got any error
  //
  *Count = 0;
  PcieEnableCount = PcdGet8 (PcdMangoPcieEnableMask);

  Bridges = AllocatePool (PcieEnableCount * sizeof (PCI_ROOT_BRIDGE));
  if (Bridges == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a:%d] - AllocatePool failed!\n", __func__, __LINE__));
    return NULL;
  }

  for (PortIndex = 0; PortIndex < PCIE_MAX_PORT; PortIndex++) {
    for (LinkIndex = 0; LinkIndex < PCIE_MAX_LINK; LinkIndex++) {
      if (!((PcieEnableCount >> ((PCIE_MAX_PORT * PortIndex) + LinkIndex)) & 0x01)) {
        continue;
      }
      Status = ConstructRootBridge (&Bridges[*Count], &mPciResource[PortIndex][LinkIndex]);
      if (EFI_ERROR (Status)) {
        continue;
      }
      (*Count)++;
    }
  }

  if (*Count == 0) {
    FreePool (Bridges);
    return NULL;
  }
  return Bridges;
}

/**
  Free the root bridge instances array returned from PciHostBridgeGetRootBridges().

  @param Bridges The root bridge instances array.
  @param Count   The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  PCI_ROOT_BRIDGE *Bridges,
  UINTN           Count
  )
{
  //
  // Unsupported
  //
}

/**
  Inform the platform that the resource conflict happens.

  @param HostBridgeHandle Handle of the Host Bridge.
  @param Configuration    Pointer to PCI I/O and PCI memory resource
                          descriptors. The Configuration contains the resources
                          for all the root bridges. The resource for each root
                          bridge is terminated with END descriptor and an
                          additional END is appended indicating the end of the
                          entire resources. The resource descriptor field
                          values follow the description in
                          EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                          .SubmitResources().
**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  EFI_HANDLE                        HostBridgeHandle,
  VOID                              *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  UINTN                             RootBridgeIndex;
  DEBUG ((DEBUG_ERROR, "PciHostBridge: Resource conflict happens!\n"));

  RootBridgeIndex = 0;
  Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((DEBUG_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for (; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (Descriptor->ResType <
              ARRAY_SIZE (mPciHostBridgeLibAcpiAddressSpaceTypeStr)
              );
      DEBUG ((DEBUG_ERROR, " %s: Length/Alignment = 0x%lx / 0x%lx\n",
              mPciHostBridgeLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
              Descriptor->AddrLen, Descriptor->AddrRangeMax
              ));
      if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
        DEBUG ((DEBUG_ERROR, "     Granularity/SpecificFlag = %ld / %02x%s\n",
                Descriptor->AddrSpaceGranularity, Descriptor->SpecificFlag,
                ((Descriptor->SpecificFlag &
                  EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE
                  ) != 0) ? L" (Prefetchable)" : L""
                ));
      }
    }
    //
    // Skip the END descriptor for root bridge
    //
    ASSERT (Descriptor->Desc == ACPI_END_TAG_DESCRIPTOR);
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)(
                   (EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1
                   );
  }
}
