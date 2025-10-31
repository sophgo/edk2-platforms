/** @file
 The library call to pass the device tree to DXE via HOB.

 Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
 Copyright (c) 2024, SOPHGO Inc, All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Include/Library/PrePiLib.h>
#include <libfdt.h>
#include <Guid/FdtHob.h>
#include <Library/BaseRiscVSbiLib.h>
#include <Library/ResourcePublicationLib.h>


/**
  Publish PEI core memory.
  @return EFI_SUCCESS     The PEIM initialized successfully.
**/
EFI_STATUS
PublishPeiMemory (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  MemoryBase;
  UINT64                MemorySize;

  //
  // TODO: This value should come from platform
  // configuration or the memory sizing code.
  //
  // MemoryBase = 0x85000000UL;
  // MemorySize = 0x4000000; // 64MB
  MemoryBase = PcdGet64 (PcdPeiPermanentMemBase);
  MemorySize = PcdGet64 (PcdPeiPermanentMemSize);
  DEBUG ((DEBUG_INFO, "%a: PEI MemoryBase is 0x%x and MemorySize is 0x%x\n", __func__, MemoryBase, MemorySize));

  //
  // Publish this memory to the PEI Core
  //

  Status = PublishSystemMemory (MemoryBase, MemorySize);
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Build memory map I/O range resource HOB using the
  base address and size.

  @param  MemoryBase     Memory map I/O base.
  @param  MemorySize     Memory map I/O size.

**/
STATIC
VOID
AddIoMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  )
{
  /* Align to EFI_PAGE_SIZE */
  MemorySize = ALIGN_VALUE (MemorySize, EFI_PAGE_SIZE);
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

STATIC
VOID
PopulateSdIoResources (
  VOID
  )
{
  AddIoMemoryBaseSizeHob (PcdGet64 (PcdSdBaseAddress), PcdGet64 (PcdSdRegSize));
   DEBUG ((
        DEBUG_INFO,
        "%a(): SD card MemoryBase=0x%lx\tMemorySize=0x%lx\n",
        __func__,
        PcdGet64 (PcdSdBaseAddress),
        PcdGet64 (PcdSdRegSize)
        ));
}

/**
  @param  FileHandle      Handle of the file being invoked.
  @param  PeiServices     Describes the list of possible PEI Services.
  @retval EFI_SUCCESS            The address of FDT is passed in HOB.
          EFI_UNSUPPORTED        Can't locate FDT.
**/
EFI_STATUS
EFIAPI
PeiPlatformInitialization (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  PopulateSdIoResources ();
  PublishPeiMemory ();

  return EFI_SUCCESS;
}
