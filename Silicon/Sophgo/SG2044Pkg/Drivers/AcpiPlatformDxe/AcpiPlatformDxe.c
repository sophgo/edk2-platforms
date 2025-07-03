/** @file
  ACPI Platform Driver for SOPHGO SG2044 platform

  Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2024, Sophgo Technologies Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/FdtClient.h>

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IniParserLib.h>
#include <Library/AcpiLib.h>
#include <Library/PrintLib.h>
#include <Library/SmbiosInformationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <IndustryStandard/Acpi.h>
#include <Guid/Acpi.h>
#include <Guid/VendorGlobalVariables.h>
#include "SG2044AcpiHeader.h"

//
// Constants and definitions
//
#define EFI_ACPI_MAX_NUM_TABLES  20
#define DSDT_SIGNATURE           0x54445344  // 'DSDT'
#define TPU_NUM                  1
#define PCIE_NUM                 10
#define CLUSTER_NUM		 16
#define CPU_NUM_PER_CLUSTER	 4
#define HIGHEST_PERF_OFFSET	 14
#define NOMINAL_PERF_OFFSET	 16
#define LOWEST_NONLINEAR_PERF_OFFSET	 18
#define LOWEST_PERF_OFFSET	 20
#define LOWEST_FREQ_OFFSET_L	 317
#define LOWEST_FREQ_OFFSET_H	 318
#define NOMINAL_FREQ_OFFSET_L	 320
#define NOMINAL_FREQ_OFFSET_H	 321

#define KHz(f)	(f * 1000ULL)
#define MHz(f)	(KHz(f) * 1000)

//
// Resource descriptor structures
//
#pragma pack(1)
typedef struct {
  UINT8   Desc;
  UINT8   LengthLow;
  UINT8   LengthHigh;
  UINT8   Type;
  UINT8   Flags;
  UINT8   TypeFlags;
  UINT8   Granularity0;
  UINT8   Granularity1;
  UINT8   Min0;
  UINT8   Min1;
  UINT8   Max0;
  UINT8   Max1;
  UINT8   Translation0;
  UINT8   Translation1;
  UINT8   Length0;
  UINT8   Length1;
} WORD_ADDRESS_SPACE_DESCRIPTOR;

typedef struct {
  UINT8   Desc;
  UINT8   LengthLow;
  UINT8   LengthHigh;
  UINT8   Type;
  UINT8   Flags;
  UINT8   TypeFlags;
  UINT64  Granularity;
  UINT64  Minimum;
  UINT64  Maximum;
  UINT64  Translation;
  UINT64  Length;
} QWORD_ADDRESS_SPACE_DESCRIPTOR;
#pragma pack()

typedef struct {
  CHAR8   *Path;
  UINT8   ServerStatus;
  UINT8   NonServerStatus;
} DEVICE_STATUS_MAP;

/**
  Locate the first instance of a protocol.  If the protocol requested is an
  FV protocol, then it will return the first FV that contains the ACPI table
  storage file.

  @param  Instance      Return pointer to the first instance of the protocol

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateFvInstanceWithTables (
  OUT EFI_FIRMWARE_VOLUME2_PROTOCOL **Instance
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         NumberOfHandles;
  EFI_FV_FILETYPE               FileType;
  UINT32                        FvStatus;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINTN                         Size;
  UINTN                         Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FvInstance;

  FvStatus = 0;

  //
  // Locate protocol.
  //
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiFirmwareVolume2ProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );
  if (EFI_ERROR (Status)) {
    //
    // Defined errors at this time are not found and out of resources.
    //
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Locate Firmware Volume2 protocol failed (Status = %r)!\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Looking for FV with ACPI storage file
  //
  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //
    Status = gBS->HandleProtocol (
                     HandleBuffer[Index],
                     &gEfiFirmwareVolume2ProtocolGuid,
                     (VOID**) &FvInstance
                     );
    ASSERT_EFI_ERROR (Status);

    //
    // See if it has the ACPI storage file
    //
    Status = FvInstance->ReadFile (
                           FvInstance,
                           (EFI_GUID*)PcdGetPtr (PcdAcpiTableStorageFile),
                           NULL,
                           &Size,
                           &FileType,
                           &Attributes,
                           &FvStatus
                           );

    //
    // If we found it, then we are done
    //
    if (Status == EFI_SUCCESS) {
      *Instance = FvInstance;
      break;
    }
  }

  //
  // Our exit status is determined by the success of the previous operations
  // If the protocol was found, Instance already points to it.
  //

  //
  // Free any allocated buffers
  //
  gBS->FreePool (HandleBuffer);

  return Status;
}

/**
  This function calculates and updates an UINT8 checksum.

  @param  Buffer          Pointer to buffer to checksum
  @param  Size            Number of bytes to checksum

**/
VOID
AcpiPlatformChecksum (
  IN UINT8      *Buffer,
  IN UINTN      Size
  )
{
  UINTN ChecksumOffset;

  ChecksumOffset = OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Checksum);

  //
  // Set checksum to 0 first
  //
  Buffer[ChecksumOffset] = 0;

  //
  // Update checksum value
  //
  Buffer[ChecksumOffset] = CalculateCheckSum8 (Buffer, Size);
}

STATIC
VOID
AcpiCheckSum (
  IN OUT  EFI_ACPI_SDT_HEADER *Table
  )
{
  UINTN ChecksumOffset;
  UINT8 *Buffer;

  ChecksumOffset = OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Checksum);
  Buffer = (UINT8 *)Table;

  //
  // set checksum to 0 first
  //
  Buffer[ChecksumOffset] = 0;

  //
  // Update checksum value
  //
  Buffer[ChecksumOffset] = CalculateCheckSum8 (Buffer, Table->Length);
}

EFI_STATUS
UpdateStatusMethodObject (
  EFI_ACPI_SDT_PROTOCOL  *AcpiSdtProtocol,
  EFI_ACPI_HANDLE        TableHandle,
  CHAR8                  *AsciiObjectPath,
  CHAR8                  ReturnValue
  )
{
  EFI_STATUS          Status = 0;
  EFI_ACPI_HANDLE     ObjectHandle;
  EFI_ACPI_DATA_TYPE  DataType;
  CHAR8               *Buffer;
  UINTN               DataSize;

  Status = AcpiSdtProtocol->FindPath (TableHandle, AsciiObjectPath, &ObjectHandle);
  if (EFI_ERROR (Status) || (ObjectHandle == NULL)) {
    return EFI_SUCCESS;
  }

  ASSERT (ObjectHandle != NULL);

  Status = AcpiSdtProtocol->GetOption (ObjectHandle, 2, &DataType, (VOID *)&Buffer, &DataSize);
  if (!EFI_ERROR (Status) && (Buffer[2] == AML_BYTE_PREFIX)) {
    //
    // Only patch when the initial value is byte object.
    //
    Buffer[3] = ReturnValue;
  }

  AcpiSdtProtocol->Close (ObjectHandle);
  return Status;
}

/**
  Debug helper function to print QWORD resource descriptor details.

  @param[in] Resource    Pointer to QWORD_ADDRESS_SPACE_DESCRIPTOR structure

**/
STATIC
VOID
DebugPrintQwordResource (
  IN CONST QWORD_ADDRESS_SPACE_DESCRIPTOR  *Resource
  )
{
  DEBUG ((
    DEBUG_VERBOSE,
    "Resource: Min=0x%lx, Max=0x%lx, Trans=0x%lx, Len=0x%lx\n",
    Resource->Minimum,
    Resource->Maximum,
    Resource->Translation,
    Resource->Length
    ));
}

#define FDT_PCI_PARENT_ADDRESS_CELLS  2
#define FDT_PCI_PARENT_SIZE_CELLS     2
#define FDT_PCI_ADDRESS_CELLS         3
#define FDT_PCI_SIZE_CELLS            2
#define FDT_PCI_RANGE_SIZE            \
  ((FDT_PCI_PARENT_ADDRESS_CELLS + FDT_PCI_ADDRESS_CELLS + FDT_PCI_SIZE_CELLS) * 4)

#define FDT_PCI_MEM_TYPE_SHIFT  (24)
#define FDT_PCI_MEM_TYPE_MASK   (0x03 << FDT_PCI_MEM_TYPE_SHIFT)
#define FDT_PCI_MEM_TYPE_IO     (1 << FDT_PCI_MEM_TYPE_SHIFT)
#define FDT_PCI_MEM_TYPE_MEM32  (2 << FDT_PCI_MEM_TYPE_SHIFT)
#define FDT_PCI_MEM_TYPE_MEM64  (3 << FDT_PCI_MEM_TYPE_SHIFT)

#define FDT_PCI_MEM_PREFETCH_SHIFT    (30)
#define FDT_PCI_MEM_PREFETCH_MASK     (1 << FDT_PCI_MEM_PREFETCH_SHIFT)
#define FDT_PCI_MEM_PREFETCH          (1 << FDT_PCI_MEM_PREFETCH_SHIFT)

typedef struct {
  UINT32    Flag;
  UINT64    PciAddr;
  UINT64    CpuAddr;
  UINT64    Size;
} FDT_PCI_RANGE;

/* Include Start and End */
typedef struct {
  UINT32    Start;
  UINT32    End;
} FDT_PCI_BUS_RANGE;

typedef struct {
  FDT_PCI_RANGE     Io;
  FDT_PCI_RANGE     Mem32;
  FDT_PCI_RANGE     PMem32;
  FDT_PCI_RANGE     Mem64;
  FDT_PCI_RANGE     PMem64;
  FDT_PCI_BUS_RANGE BusRange;
  UINT32            Segment;
} PCI_INFO;

STATIC
VOID
ShowFdtPciRange(
    IN  CHAR8         *Name,
    IN  FDT_PCI_RANGE *Range
    )
{
  DEBUG ((DEBUG_INFO, "%a: [0x%010lx : 0x%010lx : 0x%010lx]\n",
        Name, Range->PciAddr, Range->CpuAddr, Range->Size));
}

STATIC
VOID
ShowPciRoot(
    IN  PCI_INFO          *PciRoot
    )
{
  DEBUG ((DEBUG_INFO, "Segment %u [%u - %u]\n",
        PciRoot->Segment, PciRoot->BusRange.Start, PciRoot->BusRange.End));
  DEBUG ((DEBUG_INFO, "Outbound:\n"));
  ShowFdtPciRange("IO", &PciRoot->Io);
  ShowFdtPciRange("Mem32", &PciRoot->Mem32);
  ShowFdtPciRange("PMem32", &PciRoot->PMem32);
  ShowFdtPciRange("Mem64", &PciRoot->Mem64);
  ShowFdtPciRange("PMem64", &PciRoot->PMem64);
}

STATIC
VOID
GetPciRootInfoFromFdt(
    IN  FDT_CLIENT_PROTOCOL *FdtClient,
    IN  INT32               Node,
    OUT PCI_INFO           *PciRoot
    )
{
  CONST VOID                *Prop;
  UINT32                    PropSize;
  EFI_STATUS                Status;
  FDT_PCI_RANGE             Range[5];
  UINT32                    RangeIndex;
  FDT_PCI_RANGE             *Aperture;

  /* get segment */
  Status = FdtClient->GetNodeProperty (FdtClient, Node, "linux,pci-domain", &Prop, &PropSize);
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "No segment property\n"));
    ASSERT(FALSE);
  }

  PciRoot->Segment = SwapBytes32 (*(UINT32 *)Prop);

  /* parse bus range */
  Status = FdtClient->GetNodeProperty (FdtClient, Node, "bus-range", &Prop, &PropSize);
  if (Status != EFI_SUCCESS)
    DEBUG ((DEBUG_WARN, "Cannot found ranges from dt, assume 0-255\n"));

  /* bus number always 0 for root port */
  PciRoot->BusRange.Start   = 0;
  PciRoot->BusRange.End     = 255;

  Status = FdtClient->GetNodeProperty (FdtClient, Node, "ranges", &Prop, &PropSize);

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Cannot found ranges from dt\n"));
    return;
  }

  if (PropSize > ARRAY_SIZE (Range) * FDT_PCI_RANGE_SIZE) {
    DEBUG ((DEBUG_WARN, "Too many range in dt, maybe a wrong config\n"));
    DEBUG ((DEBUG_WARN, "Only range[0] - range[%d] effect on\n", ARRAY_SIZE (Range)));
    PropSize = sizeof (Range);
  }

  /* get flag */
  for (RangeIndex = 0; RangeIndex < ARRAY_SIZE (Range); ++RangeIndex, Prop += FDT_PCI_RANGE_SIZE) {
    Range[RangeIndex].Flag = SwapBytes32 (*(UINT32 *)Prop);
    /* platform must support unaligned access */
    Range[RangeIndex].PciAddr =
      SwapBytes64 (*(UINT64 *)(Prop + 4));
    Range[RangeIndex].CpuAddr =
      SwapBytes64 (*(UINT64 *)(Prop + FDT_PCI_ADDRESS_CELLS * 4));
    Range[RangeIndex].Size =
      SwapBytes64 (*(UINT64 *)(Prop +  (FDT_PCI_ADDRESS_CELLS + FDT_PCI_PARENT_ADDRESS_CELLS) * 4));
  }

  for (RangeIndex = 0; RangeIndex < ARRAY_SIZE (Range); ++RangeIndex) {
    switch (Range[RangeIndex].Flag & (FDT_PCI_MEM_TYPE_MASK | FDT_PCI_MEM_PREFETCH_MASK)) {
      case FDT_PCI_MEM_TYPE_IO:
        Aperture = &PciRoot->Io;
        break;
      case FDT_PCI_MEM_TYPE_MEM32:
        Aperture = &PciRoot->Mem32;
        break;
      case FDT_PCI_MEM_TYPE_MEM32 | FDT_PCI_MEM_PREFETCH:
        Aperture = &PciRoot->PMem32;
        break;
      case FDT_PCI_MEM_TYPE_MEM64:
        Aperture = &PciRoot->Mem64;
        break;
      case FDT_PCI_MEM_TYPE_MEM64 | FDT_PCI_MEM_PREFETCH:
        Aperture = &PciRoot->PMem64;
        break;
      default:
        DEBUG ((DEBUG_ERROR, "Undefined PCI memory type\n"));
        continue;
    }
    CopyMem(Aperture, &Range[RangeIndex], sizeof(*Aperture));
  }
}

STATIC
VOID
SetDsdtPcieCrs (
    OUT QWORD_ADDRESS_SPACE_DESCRIPTOR  *Mem,
    IN  FDT_PCI_RANGE                   *FdtRegion
    )
{
  Mem->Minimum = FdtRegion->PciAddr;
  Mem->Length = FdtRegion->Size;
  Mem->Maximum = FdtRegion->PciAddr + FdtRegion->Size - 1;
  Mem->Translation = FdtRegion->PciAddr - FdtRegion->CpuAddr;
}

/**
  Update PCIe resource allocation in ACPI table.

  @param[in]  AcpiSdtProtocol  Pointer to ACPI SDT protocol
  @param[in]  TableHandle      Handle to ACPI table

**/
STATIC
EFI_STATUS
AcpiPatchPCIe (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdtProtocol,
  IN EFI_ACPI_HANDLE        TableHandle
  )
{
  RETURN_STATUS                 Status;
  UINT32                        Index;
  CHAR8                         NodePath[256];
  EFI_ACPI_HANDLE               ObjectHandle;
  EFI_ACPI_HANDLE               StaHandle;
  EFI_ACPI_HANDLE               CrsHandle;
  EFI_ACPI_DATA_TYPE            DataType;
  CHAR8                         *Buffer;
  UINTN                         DataSize;
  FDT_CLIENT_PROTOCOL           *FdtClient;
  RETURN_STATUS                 FindNodeStatus;
  PCI_INFO                      PciRoot;
  INT32                         Node;
  CONST CHAR8                   *Compatible = "sophgo,sg2044-pcie-host";
  QWORD_ADDRESS_SPACE_DESCRIPTOR  *PMem32;
  QWORD_ADDRESS_SPACE_DESCRIPTOR  *Mem32;
  QWORD_ADDRESS_SPACE_DESCRIPTOR  *PMem64;
  QWORD_ADDRESS_SPACE_DESCRIPTOR  *Mem64;
  QWORD_ADDRESS_SPACE_DESCRIPTOR  *Io;

  /* Init all PCIe nodes to disabled */
  for (Index = 0; Index < PCIE_NUM; Index++) {
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.PCI%1X", Index);
    Status = AcpiSdtProtocol->FindPath (TableHandle, NodePath, &ObjectHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Can not found PCIe %d node in DSDT table\n", Index));
      continue;
    }

    Status = AcpiSdtProtocol->FindPath (ObjectHandle, "_STA", &StaHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "can not found PCIe _STA node in DSDT table\n"));
      break;
    }

    Status = AcpiSdtProtocol->GetOption (StaHandle, 2, &DataType, (VOID *)&Buffer, &DataSize);
    if (!EFI_ERROR (Status)) {
      Buffer[3] = 0;
      DEBUG ((DEBUG_VERBOSE, "Disable PCIe%u\n", Index));
    }
  }

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL, (VOID **)&FdtClient);

  if (Status) {
    DEBUG ((DEBUG_ERROR, "No FDT client service found\n"));
    DEBUG ((DEBUG_ERROR, "Cannot init PCIe controllers\n"));
    return EFI_SUCCESS;
  }

  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, Compatible, &Node);
      FindNodeStatus == EFI_SUCCESS;
      FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, Compatible, Node, &Node)) {

    GetPciRootInfoFromFdt(FdtClient, Node, &PciRoot);
    ShowPciRoot(&PciRoot);

    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.PCI%1X", PciRoot.Segment);
    Status = AcpiSdtProtocol->FindPath (TableHandle, NodePath, &ObjectHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "can not found PCIe %d node in DSDT table\n", PciRoot.Segment));
      continue;
    }

    Status = AcpiSdtProtocol->FindPath (ObjectHandle, "_CRS", &CrsHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "can not found PCIe _CRS node in DSDT table\n"));
      continue;
    }

    Status = AcpiSdtProtocol->GetOption (CrsHandle, 0, &DataType, (VOID *)&Buffer, &DataSize);
    if (EFI_ERROR (Status) || (Buffer == NULL)) {
      DEBUG ((DEBUG_ERROR, "can not get PCIe _CRS node in DSDT table\n"));
      continue;
    }

    PMem32 = (QWORD_ADDRESS_SPACE_DESCRIPTOR *)(Buffer + 10 + sizeof(WORD_ADDRESS_SPACE_DESCRIPTOR));
    Mem32 = (QWORD_ADDRESS_SPACE_DESCRIPTOR *)(Buffer + 10 + sizeof(WORD_ADDRESS_SPACE_DESCRIPTOR) +
        sizeof(QWORD_ADDRESS_SPACE_DESCRIPTOR));
    PMem64 = (QWORD_ADDRESS_SPACE_DESCRIPTOR *)(Buffer + 10 + sizeof(WORD_ADDRESS_SPACE_DESCRIPTOR) +
        (sizeof(QWORD_ADDRESS_SPACE_DESCRIPTOR) * 2));
    Mem64 = (QWORD_ADDRESS_SPACE_DESCRIPTOR *)(Buffer + 10 + sizeof(WORD_ADDRESS_SPACE_DESCRIPTOR) +
        (sizeof(QWORD_ADDRESS_SPACE_DESCRIPTOR) * 3));
    Io = (QWORD_ADDRESS_SPACE_DESCRIPTOR *)(Buffer + 10 + sizeof(WORD_ADDRESS_SPACE_DESCRIPTOR) +
        (sizeof(QWORD_ADDRESS_SPACE_DESCRIPTOR) * 4));
    /* change outbound windows */
    SetDsdtPcieCrs (Mem32, &PciRoot.Mem32);
    SetDsdtPcieCrs (PMem32, &PciRoot.PMem32);
    SetDsdtPcieCrs (Mem64, &PciRoot.Mem64);
    SetDsdtPcieCrs (PMem64, &PciRoot.PMem64);
    SetDsdtPcieCrs (Io, &PciRoot.Io);
    DebugPrintQwordResource(Mem32);
    DebugPrintQwordResource(PMem32);
    DebugPrintQwordResource(Mem64);
    DebugPrintQwordResource(PMem64);
    DebugPrintQwordResource(Io);

    Status = AcpiSdtProtocol->FindPath (ObjectHandle, "_STA", &StaHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "can not found PCIe _STA node in DSDT table\n"));
      break;
    }

    Status = AcpiSdtProtocol->GetOption (StaHandle, 2, &DataType, (VOID *)&Buffer, &DataSize);
    if (!EFI_ERROR (Status)) {
      Buffer[3] = 0xf;
      DEBUG ((DEBUG_VERBOSE, "Enable PCIe%d\n", PciRoot.Segment));
    }
  }

  return Status;
}

/**
  Get resource in fdt by name.

  @param[in]  FdtClient     Handle to device tree parsing client
  @param[in]  Node          Handle to node
  @param[in]  ResourceName  Pointer to the resource name
  @param[in]  Data          Pointer to the resource

**/
STATIC
RETURN_STATUS
FdtGetResourceByName (
    IN  FDT_CLIENT_PROTOCOL *FdtClient,
    IN  INT32               Node,
    IN  CONST CHAR8         *ResourceName,
    OUT UINTN               *Data
    )
{
  EFI_STATUS  Status;
  CONST VOID  *ResourceProp;
  UINT32      ResourcePropSize;

  Status = FdtClient->GetNodeProperty(FdtClient, Node, ResourceName, &ResourceProp, &ResourcePropSize);

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "No resource name property\n"));
    return EFI_NOT_FOUND;
  }

  if (Data != NULL)
    *Data = SwapBytes64(*(UINT64 *)ResourceProp);

  return EFI_SUCCESS;
}

/**
  Update CPU status in ACPI table based on configuration.

  @param[in]  AcpiSdtProtocol  Pointer to ACPI SDT protocol
  @param[in]  TableHandle      Handle to ACPI table

**/
STATIC
VOID
AcpiPatchCpu (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdtProtocol,
  IN EFI_ACPI_HANDLE        TableHandle
  )
{
  EFI_STATUS            Status;
  RETURN_STATUS         FindNodeStatus;
  EFI_ACPI_HANDLE       ObjectHandle;
  EFI_ACPI_DATA_TYPE    DataType;
  FDT_CLIENT_PROTOCOL	*FdtClient;
  CONST CHAR8           *Compatible = "sophgo,sg2044-cppc";
  INT32                 Node;
  CHAR8                 *Buffer;
  UINTN                 DataSize;
  CHAR8                 CpcPath[256];
  UINT8			ClusterIndex;
  UINT8			CpuIndex;
  UINT8			MaxCpuIndex;
  UINT64		MaxFrequency;
  UINT64		MinFrequency;
  UINT32		HighestPerf;
  UINT32		LowestPerf;
  UINT64		Step;

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL, (VOID **)&FdtClient);

  if (Status) {
    DEBUG ((DEBUG_ERROR, "No FDT client service found\n"));
    return;
  }

  FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, Compatible, &Node);

  if (FindNodeStatus != EFI_SUCCESS) {
      DEBUG ((DEBUG_ERROR, "Cannot find device %a\n", Compatible));
      return;
  }

  FdtGetResourceByName (FdtClient, Node, "min-frequency", &MinFrequency);
  FdtGetResourceByName (FdtClient, Node, "max-frequency", &MaxFrequency);
  FdtGetResourceByName (FdtClient, Node, "step", &Step);

  HighestPerf = MaxFrequency / Step;
  LowestPerf = MinFrequency / Step;
  Step = Step / MHz(1);

  for (ClusterIndex = 0; ClusterIndex < CLUSTER_NUM; ClusterIndex++) {
    MaxCpuIndex = ClusterIndex * CPU_NUM_PER_CLUSTER + CPU_NUM_PER_CLUSTER;
    for (CpuIndex = ClusterIndex * CPU_NUM_PER_CLUSTER; CpuIndex < MaxCpuIndex; CpuIndex++) {
      AsciiSPrint (CpcPath, sizeof (CpcPath), "\\_SB.CL%02d.CP%02d._CPC", ClusterIndex, CpuIndex);

      Status = AcpiSdtProtocol->FindPath (TableHandle, CpcPath, &ObjectHandle);

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "can not found _CPC node in DSDT table\n"));
	break;
      }

      Status = AcpiSdtProtocol->GetOption (ObjectHandle, 0, &DataType, (VOID *)&Buffer, &DataSize);

      if (EFI_ERROR (Status) || (Buffer == NULL)) {
        DEBUG ((DEBUG_INFO, "can not get _CPC data in DSDT table\n"));
        continue;
      }

      *(Buffer + HIGHEST_PERF_OFFSET) = HighestPerf;
      *(Buffer + NOMINAL_PERF_OFFSET) = HighestPerf;
      *(Buffer + LOWEST_NONLINEAR_PERF_OFFSET) = LowestPerf;
      *(Buffer + LOWEST_PERF_OFFSET) = LowestPerf;
      *(Buffer + LOWEST_FREQ_OFFSET_L) = (LowestPerf * Step) & 0xff;
      *(Buffer + LOWEST_FREQ_OFFSET_H) = ((LowestPerf * Step) >> 8) & 0xff;
      *(Buffer + NOMINAL_FREQ_OFFSET_L) = (HighestPerf * Step) & 0xff;
      *(Buffer + NOMINAL_FREQ_OFFSET_H) = ((HighestPerf * Step) >> 8) & 0xff;
    }
  }

  return;
}

/**
  Update TPU status in ACPI table based on configuration.

  @param[in]  AcpiSdtProtocol  Pointer to ACPI SDT protocol
  @param[in]  TableHandle      Handle to ACPI table

**/
STATIC
VOID
AcpiPatchTpu (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdtProtocol,
  IN EFI_ACPI_HANDLE        TableHandle
  )
{
  CHAR8       NodePath[256];
  INT16       Index;
  BOOLEAN     SocMode = FALSE;
  UINTN       VarSize;
  UINT32      ReservedMemData;
  EFI_STATUS  Status;

  VarSize = sizeof (ReservedMemData);

  Status = gRT->GetVariable (
    EFI_RESERVE_MEMORYSIZE_VARIABLE_NAME,
    &gEfiSophgoGlobalVariableGuid,
    NULL,
    &VarSize,
    &ReservedMemData
    );

  if ( Status == EFI_SUCCESS && ReservedMemData != 0) {
    SocMode = TRUE;
  }

  for (Index = 0; Index < TPU_NUM; Index++) {
    AsciiSPrint (
      NodePath,
      sizeof (NodePath),
      "\\_SB.TPU%1X._STA",
      Index
      );

    UpdateStatusMethodObject (
      AcpiSdtProtocol,
      TableHandle,
      NodePath,
      SocMode ? 0xF : 0x0
      );
  }
}

/**
  Update device status in ACPI DSDT table based on product type.

  @param[in]  AcpiSdtProtocol  Pointer to ACPI SDT protocol
  @param[in]  TableHandle      Handle to ACPI table
**/
STATIC
VOID
AcpiPatchDeviceStatus (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdtProtocol,
  IN EFI_ACPI_HANDLE        TableHandle
  )
{
  STATIC CONST DEVICE_STATUS_MAP DeviceMap[] = {
    // Power button device: enabled(0x0B) for server, disabled(0x00) for non-server
    // Generic event device: enabled(0x0F) for server, disabled(0x00) for non-server
    {"\\_SB.PWRB._STA", 0x0B, 0x00},
    {"\\_SB.GED0._STA", 0x0F, 0x00},
    {"\\_SB.PWRB._STA", 0x0B, 0x00},
    {"\\_SB.GED1._STA", 0x0F, 0x00},

    // Thermal and fan devices: disabled(0x00) for server, enabled(0x0F) for non-server
    {"\\_SB.I2C1.FAN0._STA", 0x00, 0x0F},
    {"\\_SB.I2C1.FAN1._STA", 0x00, 0x0F},
    {"\\_SB.I2C1.TZ00._STA", 0x00, 0x0F},
    {"\\_SB.I2C1.TZ01._STA", 0x00, 0x0F},

    // Network device: disabled(0x00) for server, enabled(0x0F) for non-server
    {"\\_SB.ETH0._STA", 0x00, 0x0F},

    // GPIO devices: disabled(0x00) for server, enabled(0x0F) for non-server
    {"\\_SB.GPI0._STA", 0x00, 0x0F},
    {"\\_SB.GPI1._STA", 0x00, 0x0F},
    {"\\_SB.GPI2._STA", 0x00, 0x0F}
  };

  BOOLEAN IsServer;
  UINTN   Index;
  UINT8   Status;

  IsServer = IsServerProduct();

  for (Index = 0; Index < sizeof(DeviceMap) / sizeof(DeviceMap[0]); Index++) {
    Status = IsServer ? DeviceMap[Index].ServerStatus : DeviceMap[Index].NonServerStatus;

    UpdateStatusMethodObject (AcpiSdtProtocol, TableHandle, DeviceMap[Index].Path, Status);

    DEBUG ((DEBUG_INFO, "%a device %a: status = 0x%x\n",
            Status ? "Enable" : "Disable",
            DeviceMap[Index].Path,
            Status));
  }
}

/**
  Update ACPI DSDT table

  @return EFI_SUCCESS if ACPI DSDT table is updated successfully
*/
EFI_STATUS
UpdateAcpiDsdtTable (
  VOID
  )
{
  EFI_STATUS              Status;
  EFI_ACPI_SDT_PROTOCOL   *AcpiTableProtocol;
  EFI_ACPI_SDT_HEADER     *Table;
  EFI_ACPI_TABLE_VERSION  TableVersion;
  UINTN                   TableKey;
  EFI_ACPI_HANDLE         TableHandle;
  UINTN                   Index;

  DEBUG ((DEBUG_INFO, "Updating device node status in ACPI DSDT table\n"));

  //
  // Find the AcpiTable protocol
  //
  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID**) &AcpiTableProtocol);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to locate ACPI table protocol!\n"));
    return EFI_SUCCESS;
  }

  //
  // Search for DSDT Table
  //
  for (Index = 0; Index < EFI_ACPI_MAX_NUM_TABLES; Index ++) {
    Status = AcpiTableProtocol->GetAcpiTable (Index, &Table, &TableVersion, &TableKey);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Table->Signature != DSDT_SIGNATURE) {
      continue;
    }

    Status = AcpiTableProtocol->OpenSdt (TableKey, &TableHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    AcpiPatchCpu (AcpiTableProtocol, TableHandle);
    AcpiPatchTpu (AcpiTableProtocol, TableHandle);
    AcpiPatchPCIe (AcpiTableProtocol, TableHandle);
    AcpiPatchDeviceStatus (AcpiTableProtocol, TableHandle);

    AcpiTableProtocol->Close (TableHandle);
    AcpiCheckSum (Table);
  }

  return EFI_SUCCESS;
}

STATIC
INT64
GetCacheSize(
  IN CHAR8           *IniField
  )
{
  EFI_STATUS Status;
  CHAR8      value[128];
  CHAR8      *End;
  UINT64     Uint;

  if (IniGetValueBySectionAndName("CPU", IniField, value))
    return -1;

  Status = AsciiStrDecimalToUint64S(value, &End, &Uint);
  if (RETURN_ERROR(Status))
    return -1;

  return Uint;
}

/**
  Update ACPI PPTT table

  @return EFI_SUCCESS if ACPI PPTT table is updated successfully
*/
EFI_STATUS
UpdateAcpiPpttTable (
  VOID
  )
{
  EFI_STATUS               Status;
  INT64                    CacheSize;
  UINT16                   ClusterIndex, ClusterCoreIndex;
  UINT32                   L1IcacheSize, L1DcacheSize;
  EFI_ACPI_SDT_PROTOCOL    *AcpiTableProtocol;
  UINTN                    Index;
  EFI_ACPI_SDT_HEADER      *Table;
  UINT8                    *PackageBuffer;
  TH_PPTT_PACKAGE          *RootPackage;
  TH_PPTT_CLUSTER          *Cluster;
  EFI_ACPI_HANDLE          TableHandle;
  EFI_ACPI_TABLE_VERSION   TableVersion;
  UINTN                    TableKey;

  //
  // Find the AcpiTable protocol
  //
  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID**) &AcpiTableProtocol);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to locate ACPI table protocol!\n"));
    return Status;
  }

  //
  // Search for PPTT Table
  //
  for (Index = 0; Index < EFI_ACPI_MAX_NUM_TABLES; Index ++) {
    Status = AcpiTableProtocol->GetAcpiTable (Index, &Table, &TableVersion, &TableKey);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Table->Signature != EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE) {
      continue;
    }

    Status = AcpiTableProtocol->OpenSdt (TableKey, &TableHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    PackageBuffer = (UINT8 *)Table + sizeof(EFI_ACPI_6_5_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER);
    RootPackage   = (TH_PPTT_PACKAGE *)PackageBuffer;
    Cluster       = (TH_PPTT_CLUSTER *)(PackageBuffer + sizeof(TH_PPTT_PACKAGE));

    CacheSize = GetCacheSize("l3-cache-size");
    if (CacheSize >= 0) {
      RootPackage->L3Cache.Size = CacheSize;
    }
    L1IcacheSize = GetCacheSize("l1-i-cache-size");
    L1DcacheSize = GetCacheSize("l1-d-cache-size");

    CacheSize = GetCacheSize("l2-cache-size");
    if (CacheSize >= 0) {
      for (ClusterIndex = 0; ClusterIndex < CLUSTER_COUNT; ClusterIndex++) {
        Cluster[ClusterIndex].L2Cache.Size = CacheSize;
        for (ClusterCoreIndex = 0; ClusterCoreIndex < CORE_COUNT; ClusterCoreIndex++) {
          if (L1IcacheSize >= 0) {
            Cluster[ClusterIndex].Core[ClusterCoreIndex].ICache.Size = L1IcacheSize;
          }

          if (L1DcacheSize >= 0) {
            Cluster[ClusterIndex].Core[ClusterCoreIndex].DCache.Size = L1DcacheSize;
          }
        }
      }
    }

    AcpiTableProtocol->Close (TableHandle);
    AcpiCheckSum (Table);
  }

  return EFI_SUCCESS;
}

/**
  Entry point of the ACPI platform driver.

  @param[in] ImageHandle    Image handle of this driver.
  @param[in] SystemTable    Global system service table.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function failed to complete.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory for tables.
**/
EFI_STATUS
EFIAPI
AcpiPlatformDxeEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_ACPI_TABLE_PROTOCOL        *AcpiTable;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FwVol;
  INTN                           Instance;
  EFI_ACPI_COMMON_HEADER         *CurrentTable;
  UINTN                          TableHandle;
  UINT32                         FvStatus;
  UINTN                          TableSize;
  UINTN                          Size;
  EFI_ACPI_DESCRIPTION_HEADER    *TableHeader;

  Instance     = 0;
  CurrentTable = NULL;
  TableHandle  = 0;

  //
  // Find the AcpiTable protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID**)&AcpiTable
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate ACPI table protocol. %r\n", Status));
    return Status;
  }

  //
  // Locate the firmware volume protocol
  //
  Status = LocateFvInstanceWithTables (&FwVol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate firmware volume with ACPI tables. %r\n", Status));
    return Status;
  }

  //
  // Read and install all ACPI tables from the storage file
  //
  while (Status == EFI_SUCCESS) {
    Status = FwVol->ReadSection (
                      FwVol,
                      (EFI_GUID*)PcdGetPtr (PcdAcpiTableStorageFile),
                      EFI_SECTION_RAW,
                      Instance,
                      (VOID**) &CurrentTable,
                      &Size,
                      &FvStatus
                      );

    if (EFI_ERROR (Status)) {
      break;
    }

    TableHeader = (EFI_ACPI_DESCRIPTION_HEADER*) CurrentTable;
    TableSize = TableHeader->Length;
    ASSERT (Size >= TableSize);

    //
    // Checksum ACPI table
    //
    AcpiPlatformChecksum ((UINT8*)CurrentTable, TableSize);

    //
    // Install ACPI table
    //
    Status = AcpiTable->InstallAcpiTable (
                          AcpiTable,
                          CurrentTable,
                          TableSize,
                          &TableHandle
                          );

    //
    // Free memory allocated by ReadSection
    //
    gBS->FreePool (CurrentTable);

    if (EFI_ERROR(Status)) {
      return EFI_ABORTED;
    }

    Instance++;
    CurrentTable = NULL;
  }

  if (IniConfIniParse (NULL) < 0) {
    DEBUG ((DEBUG_ERROR, "Config INI parse fail.\n"));
  }

  Status = UpdateAcpiDsdtTable ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UpdateAcpiDsdtTable Failed, Status = %r\n", Status));
  }

  Status = UpdateAcpiPpttTable ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UpdateAcpiPpttTable Failed, Status = %r\n", Status));
  }

  return EFI_SUCCESS;
}
