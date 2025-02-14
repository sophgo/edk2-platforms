/**
  @file
  This header file defines the SMBIOS_PARSED_DATA structure and the
  AllocSmbiosData function, which extracts information from SMBIOS tables.

  Copyright (c) Sophgo Inc. All rights reserved.
**/

#ifndef SMBIOS_INFORMATION_LIB_H
#define SMBIOS_INFORMATION_LIB_H

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Protocol/Smbios.h>

#define MAX_STRING_LENGTH 64

typedef struct {
  CHAR16 *BiosVendor;
  CHAR16 *BiosVersion;
  CHAR16 *BiosReleaseDate;
  CHAR16 *SystemManufacturer;
  CHAR16 *SystemProductName;
  CHAR16 *SystemSerialNumber;
  CHAR16 *BaseboardManufacturer;
  CHAR16 *BaseboardProductName;
  CHAR16 *ChassisManufacturer;
  CHAR16 *ProcessorVersion;
  UINT16 ProcessorCurrentSpeed;
  UINT8  ProcessorCoreCount;
  UINT8  ProcessorThreadCount;
  UINT16 L1ICacheSize;
  UINT16 L1DCacheSize;
  UINT16 L2CacheSize;
  UINT32 L3CacheSize;
  UINT8 InstallableLanguages;
  UINT8 BiosLanguageFlags;
  UINT8  MemoryArrayLocation;
  UINT8  MemoryArrayUse;
  CHAR16 *MemoryManufacturer;
  UINT32 MemorySize;
  UINT32 ExtendSize;
  UINT16 ExtendedSpeed;
  UINT8  MemoryRank;
  CHAR16 MemoryType[MAX_STRING_LENGTH];
  UINT64 MemoryArrayMappedStartingAddress;
  UINT64 MemoryArrayMappedEndingAddress;
  UINT8 BootStatus;
  UINT8 IPMIInterfaceType;
  CHAR16 *SystemVersion;
} SMBIOS_PARSED_DATA;

SMBIOS_PARSED_DATA *
AllocSmbiosData (
  VOID
);

INT32
FreeSmbiosData (
  SMBIOS_PARSED_DATA *ParsedData
) ;

BOOLEAN
IsServerProduct(
    VOID
  );

#endif
