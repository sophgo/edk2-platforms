/** @file
  SG2042 PEI memory discovery.

  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong University, China. All rights reserved.
  Copyright (c) 2026, SOPHGO Technologies Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Guid/FdtHob.h>
#include <Guid/MemoryTypeInformation.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/MasterBootMode.h>

STATIC EFI_PEI_PPI_DESCRIPTOR  mBootModePpi = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEfiPeiMasterBootModePpiGuid,
  NULL
};

STATIC
VOID
AddMemoryRegion (
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN UINT64                Size
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
    Base,
    Size
    );
}

STATIC
VOID
AddReservedAllocation (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN UINT64                Size,
  IN EFI_MEMORY_TYPE       MemoryType
  )
{
  EFI_PHYSICAL_ADDRESS  Base;
  UINT64                End;

  Base = Address & ~((EFI_PHYSICAL_ADDRESS)EFI_PAGE_SIZE - 1);
  End  = ALIGN_VALUE (Address + Size, EFI_PAGE_SIZE);
  DEBUG ((
    DEBUG_INFO,
    "SG2042 reserved: raw 0x%lx/0x%lx, aligned 0x%lx/0x%lx\n",
    Address,
    Size,
    Base,
    End - Base
    ));
  if (End > Base) {
    BuildMemoryAllocationHob (Base, End - Base, MemoryType);
  }
}

STATIC
INT32
GetCellCount (
  IN CONST VOID   *Fdt,
  IN INT32        Node,
  IN CONST CHAR8  *Name
  )
{
  CONST INT32  *Property;
  INT32        Length;
  UINT32       Value;

  Property = FdtGetProp (Fdt, Node, Name, &Length);
  if (Property == NULL) {
    return Length;
  }

  if (Length != sizeof (*Property)) {
    return -FDT_ERR_BADNCELLS;
  }

  Value = Fdt32ToCpu (*Property);
  if (Value > FDT_MAX_NCELLS) {
    return -FDT_ERR_BADNCELLS;
  }

  return (INT32)Value;
}

STATIC
VOID
AddReservedMemoryRegions (
  IN CONST VOID  *Fdt
  )
{
  CONST FDT_RESERVE_ENTRY  *ReserveEntry;
  CONST INT32              *Reg;
  INT32                    Node;
  INT32                 SubNode;
  INT32                 Length;
  INT32                 AddressCells;
  INT32                 SizeCells;
  INTN                  Index;
  EFI_PHYSICAL_ADDRESS  Address;
  UINT64                Size;

  UINTN                   ReserveMapSize;

  ReserveMapSize = FdtOffsetDtStruct (Fdt) - FdtOffsetMemRsvmap (Fdt);
  for (Index = 0; Index * sizeof (*ReserveEntry) < ReserveMapSize; Index++) {
    ReserveEntry = (CONST FDT_RESERVE_ENTRY *)((CONST UINT8 *)Fdt +
                    FdtOffsetMemRsvmap (Fdt) + Index * sizeof (*ReserveEntry));
    if ((ReserveEntry->Address == 0) && (ReserveEntry->Size == 0)) {
      break;
    }

    AddReservedAllocation (
      Fdt64ToCpu (ReserveEntry->Address),
      Fdt64ToCpu (ReserveEntry->Size),
      EfiReservedMemoryType
      );
  }

  Node = FdtSubnodeOffset (Fdt, 0, "reserved-memory");
  if (Node < 0) {
    return;
  }

  AddressCells = GetCellCount (Fdt, Node, "#address-cells");
  SizeCells    = GetCellCount (Fdt, Node, "#size-cells");
  if ((AddressCells <= 0) || (SizeCells <= 0)) {
    return;
  }

  FdtForEachSubnode (SubNode, Fdt, Node) {
    Reg = FdtGetProp (Fdt, SubNode, "reg", &Length);
    if ((Reg == NULL) || (Length != (AddressCells + SizeCells) * sizeof (INT32))) {
      continue;
    }

    Address = Fdt32ToCpu (Reg[0]);
    if (AddressCells > 1) {
      Address = (Address << 32) | Fdt32ToCpu (Reg[1]);
    }

    Reg += AddressCells;
    Size   = Fdt32ToCpu (Reg[0]);
    if (SizeCells > 1) {
      Size = (Size << 32) | Fdt32ToCpu (Reg[1]);
    }

    AddReservedAllocation (
      Address,
      Size,
      FdtGetProp (Fdt, SubNode, "no-map", &Length) != NULL
      ? EfiReservedMemoryType : EfiBootServicesData
      );
  }
}

STATIC
VOID
AddMemoryTypeInformationHob (
  VOID
  )
{
  EFI_MEMORY_TYPE_INFORMATION  Info[9];
  STATIC CONST EFI_MEMORY_TYPE  Types[] = {
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiReservedMemoryType,
    EfiRuntimeServicesData,
    EfiRuntimeServicesCode,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiLoaderCode
  };
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (Types); Index++) {
    Info[Index].Type = Types[Index];
  }

  Info[0].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIReclaimMemory);
  Info[1].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIMemoryNVS);
  Info[2].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiReservedMemoryType);
  Info[3].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesData);
  Info[4].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesCode);
  Info[5].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiBootServicesCode);
  Info[6].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiBootServicesData);
  Info[7].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiLoaderCode);

  Info[Index].Type          = EfiMaxMemoryType;
  Info[Index].NumberOfPages = 0;
  BuildGuidDataHob (&gEfiMemoryTypeInformationGuid, &Info, sizeof (Info));
}

STATIC
EFI_STATUS
DiscoverMemory (
  IN CONST VOID  *Fdt
  )
{
  CONST CHAR8           *DeviceType;
  CONST UINT64          *Reg;
  INT32                 Node;
  INT32                 Previous;
  INT32                 Length;
  INT32                 MemoryNodes;
  EFI_PHYSICAL_ADDRESS  Base;
  UINT64                Size;

  MemoryNodes = 0;
  for (Previous = 0; ; Previous = Node) {
    Node = FdtNextNode (Fdt, Previous, NULL);
    if (Node < 0) {
      break;
    }

    DeviceType = FdtGetProp (Fdt, Node, "device_type", &Length);
    if ((DeviceType == NULL) || (Length != sizeof ("memory")) ||
        (AsciiStrCmp (DeviceType, "memory") != 0))
    {
      continue;
    }

    Reg = FdtGetProp (Fdt, Node, "reg", &Length);
    if ((Reg == NULL) || (Length != 2 * sizeof (UINT64))) {
      DEBUG ((DEBUG_ERROR, "%a: malformed memory node %d\n", __func__, Node));
      return EFI_INVALID_PARAMETER;
    }

    Base = Fdt64ToCpu (ReadUnaligned64 (Reg));
    Size = Fdt64ToCpu (ReadUnaligned64 (Reg + 1));
    if (Size == 0) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "SG2042 RAM: 0x%lx - 0x%lx\n", Base, Base + Size - 1));
    AddMemoryRegion (Base, Size);
    MemoryNodes++;
  }

  if (MemoryNodes == 0) {
    return EFI_NOT_FOUND;
  }

  AddReservedMemoryRegions (Fdt);
  AddMemoryTypeInformationHob ();
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeiMemoryInitialization (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS              Status;
  EFI_PEI_HOB_POINTERS    Hob;
  EFI_PHYSICAL_ADDRESS    *FdtAddress;
  CONST VOID              *Fdt;

  Hob.Raw = GetHobList ();
  Hob.Raw = GetNextGuidHob (&gFdtHobGuid, Hob.Raw);
  if (Hob.Raw == NULL) {
    return EFI_NOT_FOUND;
  }

  FdtAddress = GET_GUID_HOB_DATA (Hob);
  Fdt        = (CONST VOID *)(UINTN)*FdtAddress;
  if (FdtCheckHeader (Fdt) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PeiServicesSetBootMode (BOOT_WITH_FULL_CONFIGURATION);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PeiServicesInstallPpi (&mBootModePpi);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return DiscoverMemory (Fdt);
}
