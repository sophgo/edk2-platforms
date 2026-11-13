/** @file
  This driver installs SMBIOS information for Sophgo SG2042 platforms.

  The structures shared by all boards live here; the board specific strings,
  chassis type and memory-array limit come from the SmbiosData<Board>.c file
  selected by gSophgoTokenSpaceGuid.PcdSmbiosBoardType.

  Copyright (c) 2015-2020, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseRiscVSbiLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Smbios.h>

#include <IndustryStandard/SmBios.h>

#include "SmbiosPlatformDxe.h"

//
// The PCIe 32-bit BAR reservation carves a 1 GiB hole out of low memory, so the
// sum of the System Memory HOBs is 1 GiB smaller than the installed capacity.
// The memory device report is based on the installed capacity (see design doc).
//
#define SMBIOS_MEMORY_HOLE_SIZE  (0x40000000ULL)

///
/// RISC-V processor specific data block for SMBIOS Type 44.
/// Defined by RISC-V BRS v1.0, section 6.1.2.
///
#pragma pack(1)
typedef struct {
  UINT16    Revision;
  UINT64    HartId;
  UINT64    MachineVendorId;
  UINT64    MachineArchId;
  UINT64    MachineImplId;
} SMBIOS_RISCV_PSD;
#pragma pack()

#define SMBIOS_TYPE44_LENGTH         (sizeof (SMBIOS_TABLE_TYPE44) + sizeof (SMBIOS_RISCV_PSD))
#define SMBIOS_RISCV_PSD_REVISION    0x0100
#define SMBIOS_RISCV_PSD_ARCH_RV64   0x07

//
// Fallback Machine Vendor ID (T-Head) used when the SBI call is unavailable.
//
#define SMBIOS_FALLBACK_MVENDORID    0x5B7ULL

//
// SMBIOS tables often reference each other using fixed constants, define a
// list of these constants for the hardcoded tables.
//
enum SMBIOS_REFERENCE_HANDLES {
  SMBIOS_HANDLE_L1I = 0x1000,
  SMBIOS_HANDLE_L1D,
  SMBIOS_HANDLE_L2,
  SMBIOS_HANDLE_L3,
  SMBIOS_HANDLE_MOTHERBOARD,
  SMBIOS_HANDLE_CHASSIS,
  SMBIOS_HANDLE_PROCESSOR,      // 0x1006, socket 0
  SMBIOS_HANDLE_MEMORY,         // 0x1007
  SMBIOS_HANDLE_DIMM,           // 0x1008
  SMBIOS_HANDLE_PROCESSOR_1     // 0x1009, socket 1
};

//
// SG2042 is a 64-core SoC; a board may carry two of them.
//
#define SMBIOS_CORES_PER_SOCKET   64U
#define SMBIOS_MAX_SOCKETS        2U

/**
  SMBIOS handle of the processor structure for the given socket.
**/
STATIC
UINT16
SmbiosProcessorHandle (
  IN UINT32  Socket
  )
{
  return (Socket == 0) ? (UINT16)SMBIOS_HANDLE_PROCESSOR : (UINT16)SMBIOS_HANDLE_PROCESSOR_1;
}

STATIC SMBIOS_BOARD_DATA CONST  *mBoardData;
STATIC CHAR8 CONST               *mSystemStrings[5];
STATIC CHAR8 CONST               *mBoardStrings[4];
STATIC CHAR8 CONST               *mChassisStrings[3];
STATIC CHAR8 CONST               *mProcessorStrings[5];
STATIC UINT32                    mSocketCount;
STATIC CHAR8                     mOemBiosString[48];
STATIC UINT8                     mProcessorId[8];
STATIC UINT64                    mMachineVendorId;
STATIC UINT64                    mMachineArchId;
STATIC UINT64                    mMachineImplId;

// BIOS information (section 7.1)
STATIC SMBIOS_TABLE_TYPE0  mSmbiosType0 = {
  {                                    // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_BIOS_INFORMATION,  // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE0),       // UINT8 Length
    SMBIOS_HANDLE_PI_RESERVED,
  },
  1,                                   // Vendor
  2,                                   // BiosVersion (patched from PcdBiosVersionString)
  0x0000,                              // BiosSegment: UEFI has no real-mode entry point
  3,                                   // BiosReleaseDate (__DATE__)
  0xFF,                                // BiosSize: unknown, see ExtendedBiosSize
  {                                    // BiosCharacteristics
    0,                                 // Reserved                          :2
    0,                                 // Unknown                           :1
    0,                                 // BiosCharacteristicsNotSupported   :1
    0,                                 // IsaIsSupported                    :1
    0,                                 // McaIsSupported                    :1
    0,                                 // EisaIsSupported                   :1
    1,                                 // PciIsSupported                    :1
    0,                                 // PcmciaIsSupported                 :1
    0,                                 // PlugAndPlayIsSupported            :1
    0,                                 // ApmIsSupported                    :1
    1,                                 // BiosIsUpgradable                  :1
    0,                                 // BiosShadowingAllowed              :1
    0,                                 // VlVesaIsSupported                 :1
    0,                                 // EscdSupportIsAvailable            :1
    1,                                 // BootFromCdIsSupported             :1
    1,                                 // SelectableBootIsSupported         :1
    0,                                 // RomBiosIsSocketed                 :1
    0,                                 // BootFromPcmciaIsSupported         :1
    0,                                 // EDDSpecificationIsSupported       :1
    0,                                 // JapaneseNecFloppyIsSupported      :1
    0,                                 // JapaneseToshibaFloppyIsSupported  :1
    0,                                 // Floppy525_360IsSupported          :1
    0,                                 // Floppy525_12IsSupported           :1
    0,                                 // Floppy35_720IsSupported           :1
    0,                                 // Floppy35_288IsSupported           :1
    0,                                 // PrintScreenIsSupported            :1
    0,                                 // Keyboard8042IsSupported           :1
    0,                                 // SerialIsSupported                 :1
    0,                                 // PrinterIsSupported                :1
    0,                                 // CgaMonoIsSupported                :1
    0,                                 // NecPc98                           :1
    0                                  // ReservedForVendor                 :3
  },
  {
    0x03,                              // ACPI + USB legacy
    0x08                               // UEFI (no "targeted content distribution")
  },
  0xFF,                                // SystemBiosMajorRelease
  0xFF,                                // SystemBiosMinorRelease
  0xFF,                                // EmbeddedControllerFirmwareMajorRelease
  0xFF,                                // EmbeddedControllerFirmwareMinorRelease
  { 64, 0 }                            // ExtendedBiosSize: 64 MB
};

// System information (section 7.2)
STATIC SMBIOS_TABLE_TYPE1  mSmbiosType1 = {
  { EFI_SMBIOS_TYPE_SYSTEM_INFORMATION, sizeof (SMBIOS_TABLE_TYPE1), SMBIOS_HANDLE_PI_RESERVED },
  1,                                   // Manufacturer
  2,                                   // Product Name
  3,                                   // Version
  0,                                   // Serial Number: not available
  { 0x9987FD42, 0x907E, 0x5446, { 0x1D, 0x7D, 0x7D, 0xA0, 0x10, 0x9F, 0x60, 0xA1 }},
  6,                                   // Wake-up Type: power switch
  0,                                   // SKU Number
  0                                    // Family
};

// Baseboard (section 7.3)
STATIC SMBIOS_TABLE_TYPE2  mSmbiosType2 = {
  { EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION, sizeof (SMBIOS_TABLE_TYPE2), SMBIOS_HANDLE_MOTHERBOARD },
  1,                                   // Manufacturer
  2,                                   // Product Name
  3,                                   // Version
  0,                                   // Serial Number
  0,                                   // Asset Tag
  { 1 },                               // Board is a hosting board
  0,                                   // Location In Chassis
  SMBIOS_HANDLE_CHASSIS,
  BaseBoardTypeMotherBoard,
  1,
  { SMBIOS_HANDLE_PROCESSOR }
};

// Enclosure
STATIC SMBIOS_TABLE_TYPE3  mSmbiosType3 = {
  { EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE, sizeof (SMBIOS_TABLE_TYPE3), SMBIOS_HANDLE_CHASSIS },
  1,                                   // Manufacturer
  0,                                   // Enclosure type (patched per board)
  2,                                   // Version
  0,                                   // Serial Number
  0,                                   // Asset Tag
  ChassisStateUnknown,
  ChassisStateSafe,
  ChassisStateSafe,
  ChassisSecurityStatusNone,
  { 0, 0, 0, 0 },
  1,                                   // 1U
  1,                                   // power cords
  0,                                   // contained elements
  0                                    // SKU Number
};

// Processor
STATIC SMBIOS_TABLE_TYPE4  mSmbiosType4 = {
  { EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE4), SMBIOS_HANDLE_PROCESSOR },
  1,                                   // Socket Designation ("SOCKET 0"/"SOCKET 1")
  3,                                   // Processor Type: Central Processor
  ProcessorFamilyIndicatorFamily2,     // Family, see Family 2 below
  3,                                   // Manufacturer ("SOPHGO")
  {{ 0 }, { 0 }},                      // Processor ID (patched: BRS JEP-106 vendor code)
  4,                                   // Version ("SG2042")
  { 0, 1, 0, 1, 0, 1 },                // Voltage: 0x8A = 1.0 V
  25,                                  // External Clock: 25 MHz
  2000,                                // Max Speed
  2000,                                // Current Speed
  0x41,                                // Status: populated + enabled
  ProcessorUpgradeOther,
  SMBIOS_HANDLE_L1I,
  SMBIOS_HANDLE_L2,
  SMBIOS_HANDLE_L3,
  0,                                   // Serial Number
  0,                                   // Asset Tag
  0,                                   // Part Number
  64,                                  // Core Count
  64,                                  // Core Enabled
  64,                                  // Thread Count
  0xEC,                                // Characteristics
  ProcessorFamilyRiscVRV64,
  0, 0, 0
};

//
// Cache information.
// Sizes are the system-wide totals (same convention as SG2044):
//   L1I = 64 cores x 64 KB = 4 MB,  L1D = 64 cores x 64 KB = 4 MB,
//   L2  = 16 clusters x 1 MB = 16 MB, L3 = 64 MB per die.
//
STATIC SMBIOS_TABLE_TYPE7  mSmbiosType7L1I = {
  { EFI_SMBIOS_TYPE_CACHE_INFORMATION, sizeof (SMBIOS_TABLE_TYPE7), SMBIOS_HANDLE_L1I },
  1,
  0x380,                               // L1 enabled, operational mode unknown
  { 4096, 0 },                         // 4 MB
  { 4096, 0 },
  { 0, 1 },
  { 0, 1 },
  0,
  CacheErrorParity,
  CacheTypeInstruction,
  CacheAssociativity2Way,
  { 4096, 0 },
  { 4096, 0 }
};

STATIC SMBIOS_TABLE_TYPE7  mSmbiosType7L1D = {
  { EFI_SMBIOS_TYPE_CACHE_INFORMATION, sizeof (SMBIOS_TABLE_TYPE7), SMBIOS_HANDLE_L1D },
  2,
  0x380,                               // L1 enabled, operational mode unknown
  { 4096, 0 },                         // 4 MB
  { 4096, 0 },
  { 0, 1 },
  { 0, 1 },
  0,
  CacheErrorSingleBit,
  CacheTypeData,
  CacheAssociativity2Way,
  { 4096, 0 },
  { 4096, 0 }
};

STATIC SMBIOS_TABLE_TYPE7  mSmbiosType7L2 = {
  { EFI_SMBIOS_TYPE_CACHE_INFORMATION, sizeof (SMBIOS_TABLE_TYPE7), SMBIOS_HANDLE_L2 },
  3,
  0x181,                               // L2 enabled, write back
  { 16384, 0 },                        // 16 MB
  { 16384, 0 },
  { 0, 1 },
  { 0, 1 },
  0,
  CacheErrorSingleBit,
  CacheTypeUnified,
  CacheAssociativity16Way,
  { 16384, 0 },
  { 16384, 0 }
};

STATIC SMBIOS_TABLE_TYPE7  mSmbiosType7L3 = {
  { EFI_SMBIOS_TYPE_CACHE_INFORMATION, sizeof (SMBIOS_TABLE_TYPE7), SMBIOS_HANDLE_L3 },
  4,
  0x182,                               // L3 enabled, write back
  { 1024, 1 },                         // 64 MB, 64 KB granularity
  { 1024, 1 },
  { 0, 1 },
  { 0, 1 },
  0,
  CacheErrorSingleBit,
  CacheTypeUnified,
  CacheAssociativity8Way,
  { 65536, 0 },                        // 64 MB in KB
  { 65536, 0 }
};

// Physical memory array
STATIC SMBIOS_TABLE_TYPE16  mSmbiosType16 = {
  { EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, sizeof (SMBIOS_TABLE_TYPE16), SMBIOS_HANDLE_MEMORY },
  MemoryArrayLocationSystemBoard,
  MemoryArrayUseSystemMemory,
  MemoryErrorCorrectionSingleBitEcc,   // ECC supported
  0,                                   // Maximum Capacity (patched per board, KB)
  0xFFFE,                              // No error information structure
  1,                                   // Number of devices (one Type 17)
  0
};

// Memory device
STATIC SMBIOS_TABLE_TYPE17  mSmbiosType17 = {
  { EFI_SMBIOS_TYPE_MEMORY_DEVICE, sizeof (SMBIOS_TABLE_TYPE17), SMBIOS_HANDLE_DIMM },
  SMBIOS_HANDLE_MEMORY,                // Array handle
  0xFFFE,                              // No error information
  72,                                  // Total Width: 64 data + 8 ECC
  64,                                  // Data Width
  0x7FFF,                              // Size: see ExtendedSize
  MemoryFormFactorRowOfChips,
  0,                                   // Not part of a set
  1,                                   // Device Locator
  2,                                   // Bank Locator
  MemoryTypeDdr4,
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },   // Unbuffered
  0,                                   // Speed
  0, 0, 0, 0,                          // Manufacturer / Serial / Asset / Part Number
  0,                                   // Rank
  0,                                   // Extended Size (patched at runtime, MB)
  0,                                   // Configured Memory Speed
  0, 0, 0,                             // Min / Max / Configured Voltage
  MemoryTechnologyDram,
  {{ 0, 0, 0, 1, 0, 0, 0 }},           // Volatile memory
  0,                                   // Firmware Version
  0, 0, 0, 0,                          // Module / subsystem controller IDs
  0,                                   // Non-Volatile Size
  0,                                   // Volatile Size (patched at runtime)
  0,                                   // Cache Size
  0,                                   // Logical Size
  0,                                   // Extended Speed
  0                                    // Extended Configured Memory Speed
};

// OEM strings (Type 11)
STATIC SMBIOS_TABLE_TYPE11  mSmbiosType11 = {
  { EFI_SMBIOS_TYPE_OEM_STRINGS, sizeof (SMBIOS_TABLE_TYPE11), SMBIOS_HANDLE_PI_RESERVED },
  3
};

// BIOS language information (Type 13)
STATIC SMBIOS_TABLE_TYPE13  mSmbiosType13 = {
  { EFI_SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION, sizeof (SMBIOS_TABLE_TYPE13), SMBIOS_HANDLE_PI_RESERVED },
  1,                                   // Installable Languages
  1,                                   // Flags: bit0=1, abbreviated language format ("en-US")
  { 0 },
  1                                    // Current Languages
};

// System boot information
STATIC SMBIOS_TABLE_TYPE32  mSmbiosType32 = {
  { EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, sizeof (SMBIOS_TABLE_TYPE32), SMBIOS_HANDLE_PI_RESERVED },
  { 0, 0, 0, 0, 0, 0 },
  BootInformationStatusNoError
};

STATIC CHAR8 CONST *mSmbiosType0Strings[] = {
  "SOPHGO",
  "3.0.0",                             // patched from PcdBiosVersionString
  __DATE__,
  NULL
};

STATIC CHAR8 CONST *mSmbiosType7Strings[] = {
  "L1 Instruction",
  "L1 Data",
  "L2",
  "L3",
  NULL
};

STATIC CHAR8 CONST *mSmbiosType11Strings[] = {
  "SOPHGO",
  NULL,                                // board product, set at runtime
  NULL,                                // "BIOS <version>", set at runtime
  NULL
};

STATIC CHAR8 CONST *mSmbiosType13Strings[] = {
  "en-US",
  "en-US",
  NULL
};

STATIC CHAR8 CONST *mSmbiosType16Strings[] = {
  NULL
};

STATIC CHAR8 CONST *mSmbiosType17Strings[] = {
  "DIMM SLOT",
  "BANK 0",
  NULL
};

STATIC CHAR8 CONST *mSmbiosType32Strings[] = {
  NULL
};

/**
  Create a SMBIOS record and add it to the table.

  @param  Smbios       SMBIOS protocol.
  @param  Template     Fixed structure, required.
  @param  StringArray  NULL-terminated array of strings, may be NULL.
**/
STATIC
EFI_STATUS
EFIAPI
LogSmbiosData (
  IN  EFI_SMBIOS_PROTOCOL     *Smbios,
  IN  EFI_SMBIOS_TABLE_HEADER *Template,
  IN  CONST CHAR8 * CONST     *StringArray
  )
{
  EFI_STATUS               Status;
  EFI_SMBIOS_TABLE_HEADER  *Record;
  UINTN                    Index;
  UINTN                    Size;
  CHAR8                    *Str;

  //
  // Fixed part + strings + terminating double NUL.
  //
  Size = Template->Length;
  if (StringArray != NULL) {
    for (Index = 0; StringArray[Index] != NULL; Index++) {
      Size += AsciiStrSize (StringArray[Index]);
    }
  }

  Size += 2;

  Record = (EFI_SMBIOS_TABLE_HEADER *)AllocateZeroPool (Size);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (Record, Template, Template->Length);

  Str = (CHAR8 *)((UINTN)Record + Record->Length);
  if (StringArray != NULL) {
    for (Index = 0; StringArray[Index] != NULL; Index++) {
      UINTN  StringSize = AsciiStrSize (StringArray[Index]);
      CopyMem (Str, StringArray[Index], StringSize);
      Str += StringSize;
    }
  }

  Status = Smbios->Add (Smbios, NULL, &Record->Handle, Record);
  ASSERT_EFI_ERROR (Status);

  FreePool (Record);
  return Status;
}

/**
  Install a Type 19 record for the given address range.

  The record buffer is zeroed so that the string area is a clean double NUL;
  passing an uninitialized stack structure would leak stack bytes into the table.
**/
STATIC
EFI_STATUS
SmbiosInstallMemoryStructure (
  IN EFI_SMBIOS_PROTOCOL  *Smbios,
  IN UINT64               StartingAddress,
  IN UINT64               RegionLength
  )
{
  SMBIOS_TABLE_TYPE19  *Record;
  EFI_SMBIOS_HANDLE    Handle;
  EFI_STATUS           Status;

  Record = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE19) + 2);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Record->Hdr.Type             = EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS;
  Record->Hdr.Length           = sizeof (SMBIOS_TABLE_TYPE19);
  Record->Hdr.Handle           = SMBIOS_HANDLE_PI_RESERVED;
  Record->StartingAddress      = 0xFFFFFFFF;
  Record->EndingAddress        = 0xFFFFFFFF;
  Record->MemoryArrayHandle    = SMBIOS_HANDLE_MEMORY;
  Record->PartitionWidth       = 1;
  Record->ExtendedStartingAddress = StartingAddress;
  Record->ExtendedEndingAddress   = StartingAddress + RegionLength - 1;

  Handle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (Smbios, NULL, &Handle, (EFI_SMBIOS_TABLE_HEADER *)Record);

  FreePool (Record);
  return Status;
}

/**
  Walk the HOB list: publish one Type 19 per System Memory range and compute
  the installed memory size.
**/
STATIC
EFI_STATUS
SmbiosMemoryInstall (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINT64                MemorySize;
  UINT64                InstalledSize;

  MemorySize = 0;

  Hob.Raw = GetHobList ();
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
        MemorySize += Hob.ResourceDescriptor->ResourceLength;
        SmbiosInstallMemoryStructure (
          Smbios,
          Hob.ResourceDescriptor->PhysicalStart,
          Hob.ResourceDescriptor->ResourceLength
          );
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  //
  // Report the installed capacity: the mapped ranges plus the hole reserved for
  // the PCIe 32-bit window.
  //
  InstalledSize = MemorySize + SMBIOS_MEMORY_HOLE_SIZE;
  if ((InstalledSize >> 20) <= 0x7FFF) {
    mSmbiosType17.Size = (UINT16)(InstalledSize >> 20);
    mSmbiosType17.ExtendedSize = 0;
  } else {
    mSmbiosType17.Size = 0x7FFF;
    mSmbiosType17.ExtendedSize = (UINT32)(InstalledSize >> 20);
  }

  mSmbiosType17.VolatileSize = InstalledSize;

  return EFI_SUCCESS;
}

/**
  Publish the baseboard record, listing every populated socket as a contained object.
**/
STATIC
EFI_STATUS
SmbiosInstallBaseboard (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  SMBIOS_TABLE_TYPE2  *Record;
  EFI_STATUS          Status;
  UINTN               Length;
  UINT32              Index;

  Length = sizeof (SMBIOS_TABLE_TYPE2) + (mSocketCount - 1) * sizeof (UINT16);
  Record = AllocateZeroPool (Length + 2);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (Record, &mSmbiosType2, sizeof (SMBIOS_TABLE_TYPE2));
  Record->Hdr.Length                     = (UINT8)Length;
  Record->NumberOfContainedObjectHandles = (UINT8)mSocketCount;
  for (Index = 0; Index < mSocketCount; Index++) {
    Record->ContainedObjectHandles[Index] = SmbiosProcessorHandle (Index);
  }

  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)Record, mBoardStrings);
  FreePool (Record);
  return Status;
}

/**
  Publish one processor record per populated socket.
**/
STATIC
EFI_STATUS
SmbiosInstallProcessors (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  SMBIOS_TABLE_TYPE4  Record;
  EFI_STATUS          Status;
  UINT32              Index;

  for (Index = 0; Index < mSocketCount; Index++) {
    CopyMem (&Record, &mSmbiosType4, sizeof (Record));
    Record.Hdr.Handle = SmbiosProcessorHandle (Index);
    Record.Socket     = (UINT8)((Index == 0) ? 1 : 2);   // "SOCKET 0" / "SOCKET 1"

    Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&Record, mProcessorStrings);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Publish the SMBIOS structures that are shared by all boards.
**/
STATIC
EFI_STATUS
SmbiosInstallStructures (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS  Status;

  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType0, mSmbiosType0Strings);
  ASSERT_EFI_ERROR (Status);

  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType1, mSystemStrings);
  ASSERT_EFI_ERROR (Status);

  Status = SmbiosInstallBaseboard (Smbios);
  ASSERT_EFI_ERROR (Status);

  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType3, mChassisStrings);
  ASSERT_EFI_ERROR (Status);

  Status = SmbiosInstallProcessors (Smbios);
  ASSERT_EFI_ERROR (Status);

  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType7L1I, mSmbiosType7Strings);
  ASSERT_EFI_ERROR (Status);
  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType7L1D, mSmbiosType7Strings);
  ASSERT_EFI_ERROR (Status);
  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType7L2, mSmbiosType7Strings);
  ASSERT_EFI_ERROR (Status);
  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType7L3, mSmbiosType7Strings);
  ASSERT_EFI_ERROR (Status);

  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType11, mSmbiosType11Strings);
  ASSERT_EFI_ERROR (Status);

  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType13, mSmbiosType13Strings);
  ASSERT_EFI_ERROR (Status);

  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType16, mSmbiosType16Strings);
  ASSERT_EFI_ERROR (Status);

  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType17, mSmbiosType17Strings);
  ASSERT_EFI_ERROR (Status);

  Status = LogSmbiosData (Smbios, (EFI_SMBIOS_TABLE_HEADER *)&mSmbiosType32, mSmbiosType32Strings);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Publish one Type 44 per hart, as required by RISC-V BRS v1.0 (SMBIOS_100).
**/
STATIC
EFI_STATUS
SmbiosInstallProcessorAdditionalInformation (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS               Status;
  SMBIOS_TABLE_TYPE44      *Record;
  SMBIOS_RISCV_PSD         *Psd;
  UINT32                   HartCount;
  UINT32                   Index;
  EFI_SMBIOS_HANDLE        Handle;

  HartCount = PcdGet32 (PcdSmbiosHartCount);
  if (HartCount == 0) {
    return EFI_SUCCESS;
  }

  for (Index = 0; Index < HartCount; Index++) {
    Record = AllocateZeroPool (SMBIOS_TYPE44_LENGTH + 2);
    if (Record == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Record->Hdr.Type                  = EFI_SMBIOS_TYPE_PROCESSOR_ADDITIONAL_INFORMATION;
    Record->Hdr.Length                = (UINT8)SMBIOS_TYPE44_LENGTH;
    Record->Hdr.Handle                = SMBIOS_HANDLE_PI_RESERVED;
    Record->RefHandle                 = SmbiosProcessorHandle (Index / SMBIOS_CORES_PER_SOCKET);
    Record->ProcessorSpecificBlock.Length            = sizeof (SMBIOS_RISCV_PSD);
    Record->ProcessorSpecificBlock.ProcessorArchType = SMBIOS_RISCV_PSD_ARCH_RV64;

    Psd                  = (SMBIOS_RISCV_PSD *)(Record + 1);
    Psd->Revision        = SMBIOS_RISCV_PSD_REVISION;
    Psd->HartId          = Index;
    Psd->MachineVendorId = mMachineVendorId;
    Psd->MachineArchId   = mMachineArchId;
    Psd->MachineImplId   = mMachineImplId;

    Handle = SMBIOS_HANDLE_PI_RESERVED;
    Status = Smbios->Add (Smbios, NULL, &Handle, (EFI_SMBIOS_TABLE_HEADER *)Record);
    FreePool (Record);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Select the board data table.
**/
STATIC
EFI_STATUS
SmbiosGetBoardData (
  OUT SMBIOS_BOARD_DATA CONST  **BoardData
  )
{
  switch (PcdGet32 (PcdSmbiosBoardType)) {
    case SMBIOS_BOARD_SG2042_EVB:
      *BoardData = &gSmbiosBoardDataSG2042Evb;
      break;
    case SMBIOS_BOARD_MILKV_PIONEER:
      *BoardData = &gSmbiosBoardDataMilkVPioneer;
      break;
    case SMBIOS_BOARD_SRA1_20:
      *BoardData = &gSmbiosBoardDataSra120;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "SmbiosPlatformDxe: invalid PcdSmbiosBoardType %u\n", PcdGet32 (PcdSmbiosBoardType)));
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Read the machine identity CSRs through SBI.
**/
STATIC
VOID
SmbiosGetMachineIds (
  VOID
  )
{
  SBI_RET  Ret;

  Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_MVENDORID, 0);
  mMachineVendorId = (Ret.Error == SBI_SUCCESS) ? (UINT64)Ret.Value : SMBIOS_FALLBACK_MVENDORID;

  Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_MARCHID, 0);
  mMachineArchId = (Ret.Error == SBI_SUCCESS) ? (UINT64)Ret.Value : 0;

  Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_MIMPID, 0);
  mMachineImplId = (Ret.Error == SBI_SUCCESS) ? (UINT64)Ret.Value : 0;
}

/**
  Installs SMBIOS information for SG2042 platforms.

  @retval EFI_SUCCESS  SMBIOS data successfully installed.
  @retval Other        SMBIOS data was not installed.
**/
EFI_STATUS
EFIAPI
SmbiosPlatformDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  EFI_SMBIOS_PROTOCOL *Smbios;
  CHAR16              *BiosVersion;
  CHAR8               *AsciiVersion;
  UINTN               VersionLen;
  UINT32              VendorId32;

  Status = SmbiosGetBoardData (&mBoardData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Type 0 BIOS version from the PCD.
  //
  BiosVersion = (CHAR16 *)PcdGetPtr (PcdBiosVersionString);
  if ((BiosVersion != NULL) && (BiosVersion[0] != L'\0')) {
    VersionLen   = StrLen (BiosVersion);
    AsciiVersion = AllocatePool (VersionLen + 1);
    if (AsciiVersion != NULL) {
      UnicodeStrToAsciiStrS (BiosVersion, AsciiVersion, VersionLen + 1);
      mSmbiosType0Strings[1] = AsciiVersion;
    }
  }

  //
  // Board specific fields.
  //
  mSystemStrings[0]  = "SOPHGO";
  mSystemStrings[1]  = mBoardData->SystemProductName;
  mSystemStrings[2]  = mBoardData->ProductVersion;
  mSystemStrings[3]  = NULL;

  mBoardStrings[0]   = "SOPHGO";
  mBoardStrings[1]   = mBoardData->BoardProductName;
  mBoardStrings[2]   = mBoardData->ProductVersion;
  mBoardStrings[3]   = NULL;

  mChassisStrings[0] = "SOPHGO";
  mChassisStrings[1] = mBoardData->ProductVersion;
  mChassisStrings[2] = NULL;

  mProcessorStrings[0] = "SOCKET 0";
  mProcessorStrings[1] = "SOCKET 1";
  mProcessorStrings[2] = "SOPHGO";
  mProcessorStrings[3] = "SG2042";
  mProcessorStrings[4] = NULL;

  mSocketCount = PcdGet32 (PcdSmbiosSocketCount);
  if ((mSocketCount == 0) || (mSocketCount > SMBIOS_MAX_SOCKETS)) {
    DEBUG ((DEBUG_ERROR, "SmbiosPlatformDxe: invalid PcdSmbiosSocketCount %u\n", mSocketCount));
    return EFI_INVALID_PARAMETER;
  }

  mSmbiosType3.Type = mBoardData->ChassisType;
  mSmbiosType16.MaximumCapacity = mBoardData->MaxMemoryCapacityKb;

  if (AsciiVersion != NULL) {
    AsciiSPrint (mOemBiosString, sizeof (mOemBiosString), "BIOS %a", AsciiVersion);
  } else {
    AsciiStrCpyS (mOemBiosString, sizeof (mOemBiosString), "BIOS");
  }
  mSmbiosType11Strings[1] = mBoardData->SystemProductName;
  mSmbiosType11Strings[2] = mOemBiosString;

  //
  // Processor ID per RISC-V BRS v1.0 section 6.1.1: DWORD0 = JEP-106 vendor
  // code (mvendorid as reported by SBI), DWORD1 = vendor part version (0).
  //
  SmbiosGetMachineIds ();
  VendorId32 = (UINT32)mMachineVendorId;
  ZeroMem (mProcessorId, sizeof (mProcessorId));
  CopyMem (mProcessorId, &VendorId32, sizeof (VendorId32));
  CopyMem (&mSmbiosType4.ProcessorId, mProcessorId, sizeof (mProcessorId));

  //
  // Find the SMBIOS protocol.
  //
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SmbiosMemoryInstall (Smbios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SmbiosInstallStructures (Smbios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SmbiosInstallProcessorAdditionalInformation (Smbios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return gBS->InstallProtocolInterface (
                &ImageHandle,
                &gSophgoSmbiosPlatformReadyProtocolGuid,
                EFI_NATIVE_INTERFACE,
                NULL
                );
}
