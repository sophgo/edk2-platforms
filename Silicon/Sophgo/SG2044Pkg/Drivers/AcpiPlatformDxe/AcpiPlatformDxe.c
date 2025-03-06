/** @file
  ACPI Platform Driver for SOPHGO SG2044 platform

  Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/FirmwareVolume2.h>

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IniParserLib.h>
#include <Library/AcpiLib.h>
#include <Library/PrintLib.h>
#include <Library/SmbiosInformationLib.h>

#include <IndustryStandard/Acpi.h>
#include <Guid/Acpi.h>
#include <Guid/VendorGlobalVariables.h>
#include <Library/UefiRuntimeServicesTableLib.h>

//
// Constants and definitions
//
#define EFI_ACPI_MAX_NUM_TABLES  20
#define DSDT_SIGNATURE           0x54445344  // 'DSDT'
#define TPU_NUM                  1
#define PCIE_NUM                 10

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

//
// INI configuration node names
//
STATIC CONST CHAR8 *mIniNodeNames[] = {
  "pmem32-addr",
  "pmem32-transl",
  "pmem32-length",
  "mem32-addr",
  "mem32-transl",
  "mem32-length",
  "pmem64-addr",
  "pmem64-transl",
  "pmem64-length",
  "mem64-addr",
  "mem64-transl",
  "mem64-length",
  "io-addr",
  "io-transl",
  "io-length",
};

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
  EFI_STATUS                     Status;
  EFI_ACPI_HANDLE                ObjectHandle;
  EFI_ACPI_DATA_TYPE             DataType;
  CHAR8                          *Buffer;
  UINTN                          DataSize;
  EFI_ACPI_HANDLE                CrsHandle;
  EFI_ACPI_HANDLE                StaHandle;
  QWORD_ADDRESS_SPACE_DESCRIPTOR *Pmem32;
  QWORD_ADDRESS_SPACE_DESCRIPTOR *Mem32;
  QWORD_ADDRESS_SPACE_DESCRIPTOR *Pmem64;
  QWORD_ADDRESS_SPACE_DESCRIPTOR *Mem64;
  QWORD_ADDRESS_SPACE_DESCRIPTOR *Io;
  CHAR8                          NodePath[256];
  CHAR8                          SegName[64];
  CHAR8                          Value[64];
  CHAR8                          *End;
  UINT64                         Val;
  UINTN                          Index;
  UINTN                          Loop;

  for (Index = 0; Index < PCIE_NUM; Index++) {
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.PCI%1X", Index);
    Status = AcpiSdtProtocol->FindPath (TableHandle, NodePath, &ObjectHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "can not found PCIe %d node in DSDT table\n", Index));
      continue;
    }

    Status = AcpiSdtProtocol->FindPath (ObjectHandle, "_CRS", &CrsHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "can not found PCIe _CRS node in DSDT table\n"));
      continue;
    }

    Status = AcpiSdtProtocol->GetOption (CrsHandle, 0, &DataType, (VOID *)&Buffer, &DataSize);
    if (EFI_ERROR (Status) || (Buffer == NULL)) {
      DEBUG ((DEBUG_INFO, "can not get PCIe _CRS node in DSDT table\n"));
      continue;
    }

    Pmem32 = (QWORD_ADDRESS_SPACE_DESCRIPTOR *)(Buffer + 10 + sizeof(WORD_ADDRESS_SPACE_DESCRIPTOR));
    Mem32 = (QWORD_ADDRESS_SPACE_DESCRIPTOR *)(Buffer + 10 + sizeof(WORD_ADDRESS_SPACE_DESCRIPTOR) +
                                             sizeof(QWORD_ADDRESS_SPACE_DESCRIPTOR));
    Pmem64 = (QWORD_ADDRESS_SPACE_DESCRIPTOR *)(Buffer + 10 + sizeof(WORD_ADDRESS_SPACE_DESCRIPTOR) +
                                              (sizeof(QWORD_ADDRESS_SPACE_DESCRIPTOR) * 2));
    Mem64 = (QWORD_ADDRESS_SPACE_DESCRIPTOR *)(Buffer + 10 + sizeof(WORD_ADDRESS_SPACE_DESCRIPTOR) +
                                             (sizeof(QWORD_ADDRESS_SPACE_DESCRIPTOR) * 3));
    Io = (QWORD_ADDRESS_SPACE_DESCRIPTOR *)(Buffer + 10 + sizeof(WORD_ADDRESS_SPACE_DESCRIPTOR) +
                                          (sizeof(QWORD_ADDRESS_SPACE_DESCRIPTOR) * 4));

    AsciiSPrint (SegName, sizeof (SegName), "pcie%1X", Index);
    for (Loop = 0; Loop < sizeof(mIniNodeNames) / sizeof(mIniNodeNames[0]); Loop++) {
      if (IniGetValueBySectionAndName (SegName, mIniNodeNames[Loop], Value) == 0) {
        Status = AsciiStrHexToUint64S (Value, &End, &Val);
        if (EFI_ERROR (Status)) {
          continue;
        }

        switch (Loop) {
        case 0:
          Pmem32->Minimum = Val;
          break;
        case 1:
          Pmem32->Translation = 0;
          break;
        case 2:
          Pmem32->Length = Val;
          Pmem32->Maximum = Pmem32->Minimum + Val - 1;
          DebugPrintQwordResource (Pmem32);
          break;
        case 3:
          Mem32->Minimum = Val;
          break;
        case 4:
          Mem32->Translation = 0;
          break;
        case 5:
          Mem32->Length = Val;
          Mem32->Maximum = Mem32->Minimum + Val - 1;
          DebugPrintQwordResource (Mem32);
          break;
        case 6:
          Pmem64->Minimum = Val;
          break;
        case 7:
          Pmem64->Translation = 0;
          break;
        case 8:
          Pmem64->Length = Val;
          Pmem64->Maximum = Pmem64->Minimum + Val - 1;
          DebugPrintQwordResource (Pmem64);
          break;
        case 9:
          Mem64->Minimum = Val;
          break;
        case 10:
          Mem64->Translation = 0;
          break;
        case 11:
          Mem64->Length = Val;
          Mem64->Maximum = Mem64->Minimum + Val - 1;
          DebugPrintQwordResource (Mem64);
          break;
        case 12:
          Io->Translation = Val;
          break;
        case 13:
          Io->Minimum = Val;
          break;
        case 14:
          Io->Length = Val;
          Io->Maximum = Io->Minimum + Val - 1;
          DebugPrintQwordResource (Io);
          break;
        }
      } else {
        Status = AcpiSdtProtocol->FindPath (ObjectHandle, "_STA", &StaHandle);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "can not found PCIe _STA node in DSDT table\n"));
          break;
        }

        if (Index <= 4) {
          continue;
        }

        Status = AcpiSdtProtocol->GetOption (StaHandle, 2, &DataType, (VOID *)&Buffer, &DataSize);
        if (!EFI_ERROR (Status)) {
          Buffer[3] = 0;
          DEBUG ((DEBUG_INFO, "Disable %a\n", SegName));
          break;
        }
      }
    }
  }

  return Status;
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

  Status = IniConfIniParse (NULL);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Config INI parse fail. %r\n", Status));
    return EFI_NOT_FOUND;
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

    AcpiPatchTpu (AcpiTableProtocol, TableHandle);
    Status = AcpiPatchPCIe (AcpiTableProtocol, TableHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    AcpiPatchDeviceStatus (AcpiTableProtocol, TableHandle);

    AcpiTableProtocol->Close (TableHandle);
    AcpiCheckSum (Table);
  }

  return EFI_SUCCESS;
}

/**
  Entry point of the ACPI platform driver.

  @param[in] ImageHandle    Image handle of this driver.
  @param[in] SystemTable    Global system service table.

  @retval EFI_SUCCESS           The function completed successfully.
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
    } else {
      //
      // Add the table
      //
      TableHeader = (EFI_ACPI_DESCRIPTION_HEADER*) (CurrentTable);
      TableHandle = 0;

      TableSize = ((EFI_ACPI_DESCRIPTION_HEADER *) CurrentTable)->Length;
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

      //
      // Increment the instance
      //
      Instance++;
      CurrentTable = NULL;
    }
  }

  Status = UpdateAcpiDsdtTable ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, " UpdateAcpiDsdtTable Failed, Status = %r\n", Status));
  }

  return EFI_SUCCESS;
}

