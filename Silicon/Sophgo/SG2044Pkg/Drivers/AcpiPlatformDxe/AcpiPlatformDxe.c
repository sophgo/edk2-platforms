/** @file
  Sample ACPI Platform Driver

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

#include <IndustryStandard/Acpi.h>

#define EFI_ACPI_MAX_NUM_TABLES         20
#define DSDT_SIGNATURE                  0x54445344
#define TPU_NUM		1
#define PCIE_NUM	10

#pragma pack(1)
struct word_address_space_desc {
	UINT8 desc;
	UINT8 length_low;
	UINT8 length_high;
	UINT8 type;
	UINT8 flags;
	UINT8 type_flags;
	UINT8 granularity0;
	UINT8 granularity1;
	UINT8 min0;
	UINT8 min1;
	UINT8 max0;
	UINT8 max1;
	UINT8 transl0;
	UINT8 transl1;
	UINT8 length0;
	UINT8 length1;
};

struct qword_address_space_desc {
	UINT8 desc;
	UINT8 length_low;
	UINT8 length_high;
	UINT8 type;
	UINT8 flags;
	UINT8 type_flags;
	UINT64 granularity;
	UINT64 min;
	UINT64 max;
	UINT64 transl;
	UINT64 length;
};
#pragma pack()

static const char *ini_node_name[] = {
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

void debug_qword_resource(struct qword_address_space_desc *tmp)
{
	DEBUG ((DEBUG_VERBOSE, "min: %lx\n", tmp->min));
	DEBUG ((DEBUG_VERBOSE, "max: %lx\n", tmp->max));
	DEBUG ((DEBUG_VERBOSE, "transl: %lx\n", tmp->transl));
	DEBUG ((DEBUG_VERBOSE, "length: %lx\n", tmp->length));
}

STATIC EFI_STATUS
AcpiPatchPCIe(
  EFI_ACPI_SDT_PROTOCOL  *AcpiSdtProtocol,
  EFI_ACPI_HANDLE        TableHandle
  )
{
  struct qword_address_space_desc *pmem32;
  struct qword_address_space_desc *mem32;
  struct qword_address_space_desc *pmem64;
  struct qword_address_space_desc *mem64;
  struct qword_address_space_desc *io;
  EFI_STATUS          Status = 0;
  EFI_ACPI_HANDLE     ObjectHandle;
  EFI_ACPI_DATA_TYPE  DataType;
  CHAR8               *Buffer;
  UINTN               DataSize;
  EFI_ACPI_HANDLE     CrsHandle;
  EFI_ACPI_HANDLE     StaHandle;
  CHAR8		      NodePath[256];
  CHAR8		      SegName[64];
  CHAR8		      value[128];
  CHAR8		      *End;
  UINTN		      val;
  INT16		      Index;
  INT16		      loop;

  for (Index = 0; Index < PCIE_NUM; Index++) {
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.PCI%1X", Index);
    Status = AcpiSdtProtocol->FindPath (TableHandle, NodePath, &ObjectHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "can not found PCIe %d node in DSDT table\n", Index));
      Status = EFI_NOT_FOUND;
      continue;
    }

    Status = AcpiSdtProtocol->FindPath (ObjectHandle, "_CRS", &CrsHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "can not found PCIe _CRS node in DSDT table\n"));
      Status = EFI_NOT_FOUND;
      continue;
    }

    Status = AcpiSdtProtocol->GetOption (CrsHandle, 0, &DataType, (VOID *)&Buffer, &DataSize);
    if (EFI_ERROR (Status) ||  (Buffer == NULL)) {
      DEBUG ((DEBUG_INFO, "can not get PCIe _CRS node in DSDT table\n"));
      continue;
    }
    pmem32 = (struct qword_address_space_desc *)(Buffer + 10 + sizeof(struct word_address_space_desc));
    mem32 = (struct qword_address_space_desc *)(Buffer + 10 + sizeof(struct word_address_space_desc)
						            + (sizeof(struct qword_address_space_desc) * 1));
    pmem64 = (struct qword_address_space_desc *)(Buffer + 10 + sizeof(struct word_address_space_desc)
							     + (sizeof(struct qword_address_space_desc) * 2));
    mem64 = (struct qword_address_space_desc *)(Buffer + 10 + sizeof(struct word_address_space_desc)
						            + (sizeof(struct qword_address_space_desc) * 3));
    io = (struct qword_address_space_desc *)(Buffer + 10 + sizeof(struct word_address_space_desc)
						           + (sizeof(struct qword_address_space_desc) * 4));

    AsciiSPrint (SegName, sizeof (SegName), "pcie%1X", Index);
    for (loop = 0; loop < sizeof(ini_node_name) / sizeof(ini_node_name[0]); loop++) {
      if (IniGetValueBySectionAndName (SegName, ini_node_name[loop], value) == 0) {
        Status = AsciiStrHexToUint64S(value, &End, &val);
	switch (loop)
	{
	case 0:
		pmem32->min = val;
		break;
	case 1:
		pmem32->transl = 0;
		break;
	case 2:
		pmem32->length = val;
		pmem32->max = pmem32->min + val - 1;

		debug_qword_resource(pmem32);
		break;
	case 3:
		mem32->min = val;
		break;
	case 4:
		mem32->transl = 0;
		break;
	case 5:
		mem32->length = val;
		mem32->max = mem32->min + val - 1;

		debug_qword_resource(mem32);
		break;
	case 6:
		pmem64->min = val;
		break;
	case 7:
		pmem64->transl = 0;
		break;
	case 8:
		pmem64->length = val;
		pmem64->max = pmem64->min + val - 1;

		debug_qword_resource(pmem64);
		break;
	case 9:
		mem64->min = val;
		break;
	case 10:
		mem64->transl = 0;
		break;
	case 11:
		mem64->length = val;
		mem64->max = mem64->min + val - 1;

		debug_qword_resource(mem64);
		break;
	case 12:
		io->transl = val;
		break;
	case 13:
		io->min = val;
		break;
	case 14:
		io->length = val;
		io->max = io->min + val - 1;

		debug_qword_resource(io);
		break;
	}

      } else {
        Status = AcpiSdtProtocol->FindPath (ObjectHandle, "_STA", &StaHandle);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "can not found PCIe _STA node in DSDT table\n"));
          Status = EFI_NOT_FOUND;
	  break;
        }

        Status = AcpiSdtProtocol->GetOption (StaHandle, 2, &DataType, (VOID *)&Buffer, &DataSize);
        if (!EFI_ERROR (Status)) {
          Buffer[3] = 0;
          DEBUG ((DEBUG_INFO, "disable %s\n", SegName));
	  break;
        }
      }
    }
  }
  return Status;
}

STATIC VOID
AcpiPatchTpu (
  EFI_ACPI_SDT_PROTOCOL  *AcpiSdtProtocol,
  EFI_ACPI_HANDLE        TableHandle
  )
{
  CHAR8  NodePath[256];
  CHAR8  value[128];
  INT16  Index;
  CHAR8  soc_mode = 0;

  if (IniGetValueBySectionAndName ("sophgo-config", "work-mode", value) == 0) {
    if (!AsciiStrCmp(value, "soc"))
      soc_mode = 0x1;
  }

  for (Index = 0; Index < TPU_NUM; Index++) {
    AsciiSPrint (NodePath, sizeof (NodePath), "\\_SB.TPU%1X._STA", Index);
    if (soc_mode)
      UpdateStatusMethodObject (AcpiSdtProtocol, TableHandle, NodePath, 0xf);
    else
      UpdateStatusMethodObject (AcpiSdtProtocol, TableHandle, NodePath, 0x0);
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
  UINTN                   i;

  DEBUG ((DEBUG_ERROR, "Updating TPU and PCIe node in ACPI DSDT table\n"));

  //
  // Find the AcpiTable protocol
  Status = gBS->LocateProtocol(&gEfiAcpiSdtProtocolGuid, NULL, (VOID**) &AcpiTableProtocol);
  if (EFI_ERROR(Status)) {
    //DEBUG("Unable to locate ACPI table protocol\n");
    return EFI_SUCCESS;
  }

  Status = IniConfIniParse (NULL);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Config INI parse fail. %r\n", Status));
    return EFI_NOT_FOUND;
  }

  //
  // Search for DSDT Table
  for (i = 0; i < EFI_ACPI_MAX_NUM_TABLES; i++) {
    Status = AcpiTableProtocol->GetAcpiTable(i, &Table, &TableVersion, &TableKey);
    if (EFI_ERROR(Status))
      break;

    if (Table->Signature != DSDT_SIGNATURE)
      continue;

    Status = AcpiTableProtocol->OpenSdt(TableKey, &TableHandle);
    if (EFI_ERROR(Status)) {
      break;
    }

    AcpiPatchTpu (AcpiTableProtocol, TableHandle);
    AcpiPatchPCIe(AcpiTableProtocol, TableHandle);

    AcpiTableProtocol->Close(TableHandle);
    AcpiCheckSum (Table);
  }

  return EFI_SUCCESS;
}

/**
  Entrypoint of Acpi Platform driver.

  @param  ImageHandle
  @param  SystemTable

  @return EFI_SUCCESS
  @return EFI_LOAD_ERROR
  @return EFI_OUT_OF_RESOURCES

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
    return EFI_ABORTED;
  }

  //
  // Locate the firmware volume protocol
  //
  Status = LocateFvInstanceWithTables (&FwVol);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  //
  // Read tables from the storage file.
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

