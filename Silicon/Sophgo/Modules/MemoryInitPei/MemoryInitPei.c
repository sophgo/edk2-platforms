/** @file
  Platform PEI driver

  Copyright (c) 2019-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>
  Copyright (c) 2025, SOPHGO Technologies Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

#include <Ppi/ReadOnlyVariable2.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Guid/MemoryTypeInformation.h>
#include <Ppi/MasterBootMode.h>
#include <IndustryStandard/Pci22.h>

#include <Guid/VendorGlobalVariables.h>


typedef struct {
  UINT32 Value;
} RESERVE_MEMORY_DATA;

EFI_PEI_PPI_DESCRIPTOR  mPpiBootMode[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiMasterBootModePpiGuid,
    NULL
  }
};

STATIC EFI_BOOT_MODE  mBootMode = BOOT_WITH_FULL_CONFIGURATION;

EFI_STATUS
EFIAPI
GetMemSizeFromEfusePei (
  OUT  UINT64    *MemSize
  );

STATIC
VOID
ShowMemValue(
    IN  CHAR8         *Name,
    IN  UINT64        Value
    )
{
  DEBUG ((DEBUG_INFO, "%a: [0x%010lx]\n", Name, Value));
}

EFI_STATUS
EFIAPI
GetReservedMemorySize (
  OUT  UINT64        *ReservedMemSize
  )
{
  EFI_STATUS                            Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI      *VariableServices;
  UINTN                                 VarSize;
  RESERVE_MEMORY_DATA                   ReservedMem;

  VarSize = sizeof (RESERVE_MEMORY_DATA);
  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **)&VariableServices
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate ReadOnlyVariable2 PPI\n", __func__));
    return Status;
  }

  Status = VariableServices->GetVariable (
                                VariableServices,
                                EFI_RESERVE_MEMORYSIZE_VARIABLE_NAME,
                                &gEfiSophgoGlobalVariableGuid,
                                NULL,
                                &VarSize,
                                &ReservedMem
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Reserved Memory Size\n", __func__));
    return Status;
  }

  *ReservedMemSize = (UINT64)ReservedMem.Value * 1024 * 1024 * 1024; // Convert GB to Bytes
  DEBUG ((DEBUG_VERBOSE, "%a: Reserved Memory Size is 0x%016lx\n", __func__, *ReservedMemSize));

  return Status;
}

VOID
BuildMemoryTypeInformationHob (
  VOID
  )
{
  EFI_MEMORY_TYPE_INFORMATION  Info[6];

  Info[0].Type          = EfiACPIReclaimMemory;
  Info[0].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIReclaimMemory);
  Info[1].Type          = EfiACPIMemoryNVS;
  Info[1].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIMemoryNVS);
  Info[2].Type          = EfiReservedMemoryType;
  Info[2].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiReservedMemoryType);
  Info[3].Type          = EfiRuntimeServicesData;
  Info[3].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesData);
  Info[4].Type          = EfiRuntimeServicesCode;
  Info[4].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesCode);
  // Terminator for the list
  Info[5].Type          = EfiMaxMemoryType;
  Info[5].NumberOfPages = 0;

  BuildGuidDataHob (&gEfiMemoryTypeInformationGuid, &Info, sizeof (Info));
}

/**
  Build memory map I/O range resource HOB using the
  base address and size.

  @param  MemoryBase     Memory map I/O base.
  @param  MemorySize     Memory map I/O size.

**/
VOID
AddIoMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    EFI_RESOURCE_ATTRIBUTE_PRESENT     |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

/**
  Build reserved memory range resource HOB.

  @param  MemoryBase     Reserved memory range base address.
  @param  MemorySize     Reserved memory range size.

**/
VOID
AddReservedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_RESERVED,
    EFI_RESOURCE_ATTRIBUTE_PRESENT     |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

/**
  Build memory map I/O resource using the base address
  and the top address of memory range.

  @param  MemoryBase     Memory map I/O range base address.
  @param  MemoryLimit    The top address of memory map I/O range

**/
VOID
AddIoMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit
  )
{
  AddIoMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}

/**
  Create memory range resource HOB using the memory base
  address and size.

  @param  MemoryBase     Memory range base address.
  @param  MemorySize     Memory range size.

**/
VOID
AddMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

/**
  Create memory range resource HOB using memory base
  address and top address of the memory range.

  @param  MemoryBase     Memory range base address.
  @param  MemoryLimit    Memory range size.

**/
VOID
AddMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit
  )
{
  AddMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}

/**
  Publish system RAM and reserve memory regions.

**/
VOID
InitializeRamRegions (
  VOID
  )
{
  EFI_STATUS            Status;
  UINT64                MemSize;
  UINT64                MemBaseAddress;
  UINT64                MemSizeEfuse;
  UINT64                ReservedMemSize;

  Status = GetMemSizeFromEfusePei (&MemSizeEfuse);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get memory size from efuse\n", __func__));
    return;
  }

  Status = GetReservedMemorySize (&ReservedMemSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get reserved memory size\n", __func__));
    ReservedMemSize = 0;
  }

  if (MemSizeEfuse <= ReservedMemSize) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid memory size!\n", __func__));
    return;
  } else {
    MemSize = MemSizeEfuse - ReservedMemSize;
  }

  MemSize = FixedPcdGet64 (PcdEfiMemoryBottom) > FixedPcdGet64 (PcdMemoryBaseAddress) ?
        (MemSize - (FixedPcdGet64 (PcdEfiMemoryBottom) - FixedPcdGet64 (PcdMemoryBaseAddress))) : MemSize;

  MemBaseAddress = FixedPcdGet64 (PcdEfiMemoryBottom) > FixedPcdGet64 (PcdMemoryBaseAddress) ?
        FixedPcdGet64 (PcdEfiMemoryBottom) : FixedPcdGet64 (PcdMemoryBaseAddress);

  DEBUG ((DEBUG_INFO, "Publish System RAM:\n"));
  ShowMemValue("MemBaseAddr     ", MemBaseAddress);
  ShowMemValue("MemSize         ", MemSize);
  ShowMemValue("MemSizeTotal    ", MemSizeEfuse);
  ShowMemValue("MemSizeReserved ", ReservedMemSize);

  AddMemoryRangeHob (MemBaseAddress, MemSize);
}


/**
  Add PCI resource.

**/
VOID
AddPciResource (
  VOID
  )
{
  //
  // Platform-specific
  //
}

/**
  Platform memory map initialization.

**/
VOID
MemMapInitialization (
  VOID
  )
{
  //
  // Create Memory Type Information HOB
  //
  BuildMemoryTypeInformationHob ();

  //
  // Add PCI IO Port space available for PCI resource allocations.
  //
  AddPciResource ();
}

/**
  Check if system returns from S3.

  @return BOOLEAN   TRUE, system returned from S3
                    FALSE, system is not returned from S3

**/
BOOLEAN
CheckResumeFromS3 (
  VOID
  )
{
  //
  // Platform implementation-specific
  //
  return FALSE;
}

/**
  Platform boot mode initialization.

**/
VOID
BootModeInitialization (
  VOID
  )
{
  EFI_STATUS  Status;

  if (!CheckResumeFromS3) {
    DEBUG ((DEBUG_INFO, "This is wake from S3\n"));
  } else {
    DEBUG ((DEBUG_INFO, "This is normal boot\n"));
  }

  Status = PeiServicesSetBootMode (mBootMode);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesInstallPpi (mPpiBootMode);
  ASSERT_EFI_ERROR (Status);
}


/**
  Perform Platform PEI initialization.

  @param  FileHandle      Handle of the file being invoked.
  @param  PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
EFIAPI
PeiMemoryInitialization (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  DEBUG ((DEBUG_INFO, "%a: Initialize Platform Memory Regions\n", __func__));

  BootModeInitialization ();
  InitializeRamRegions ();

  if (mBootMode != BOOT_ON_S3_RESUME) {
    MemMapInitialization ();
  }

  return EFI_SUCCESS;
}