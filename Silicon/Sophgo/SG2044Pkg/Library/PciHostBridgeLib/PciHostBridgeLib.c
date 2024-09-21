/** @file
  PCI Host Bridge Library instance for SOPHGO SG2044 platforms.

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Include/PciPlatformLib.h>

EFI_STATUS
EFIAPI
PciHostBridgeLibConstructor (
    IN  EFI_HANDLE        ImageHandle,
    IN  EFI_SYSTEM_TABLE  *SystemTable
    )
{
  return PciPlatformInit ();
}

/**
  Return all the root bridge instances in an array.

  @param Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges ()
          when it's not used.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  OUT UINTN *Count
  )
{
  return PciPlatformGetRoot (Count);
}

/**
  Free the root bridge instances array returned from PciHostBridgeGetRootBridges ().

  @param Bridges The root bridge instances array.
  @param Count   The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  IN PCI_ROOT_BRIDGE *Bridges,
  IN UINTN           Count
  )
{
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
                          EFI_PCI_HOST_BRIDGE_RESOUrce_ALLOCATION_PROTOCOL
                          .SubmitResources ().
**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  IN EFI_HANDLE                        HostBridgeHandle,
  IN VOID                              *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  UINTN                             RootBridgeIndex;

#ifndef MDEPKG_NDEBUG
  CONST CHAR16  *PciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
    L"Mem", L"I/O", L"Bus"
  };
#endif

  DEBUG ((DEBUG_ERROR, "PciHostBridge: Resource conflict happens!\n"));

  RootBridgeIndex = 0;
  Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((DEBUG_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for (; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (Descriptor->ResType <
          (sizeof (PciHostBridgeLibAcpiAddressSpaceTypeStr) /
           sizeof (PciHostBridgeLibAcpiAddressSpaceTypeStr[0])
          )
          );
      DEBUG ((DEBUG_ERROR, " %s: Length/Alignment = 0x%lx / 0x%lx\n",
            PciHostBridgeLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
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
