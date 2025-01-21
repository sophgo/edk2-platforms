#ifndef __CONFIG_UTILS_H__
#define __CONFIG_UTILS_H__

#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>
#include <Library/IniParserLib.h>
#include <Library/IniParserLib/IniParserUtil.h>
#include "Spifmc.h"
#include "SpiNorFlash.h"
#include "EfuseLib.h"

#define MAX_SECTION_LENGTH      128
#define MAX_NAME_LENGTH         128
#define MAX_VALUE_LENGTH        128
#define MAX_ENTRIES             500
#define VERSION_OFFSET          0x0
#define VERSION_SIZE            7
#define DATE_OFFSET             0x100
#define DATE_SIZE               10

typedef struct {
  CHAR8 Section[MAX_SECTION_LENGTH];
  CHAR8 Name[MAX_NAME_LENGTH];
  CHAR8 Value[MAX_VALUE_LENGTH];
} INI_ENTRY;
/**
  Read version and date from storage.

  @param[out] Version   The version string.
  @param[out] Date      The date string.

  @retval EFI_SUCCESS   Successfully retrieved version and date.
  @retval Others        Error status.
**/
INT32
ReadVersionAndDateFromFlash (
  CHAR8 *Version,
  CHAR8 *Date,
  UINTN StartAddress,
  UINTN EndAddress
 );

INT32
UpdateSmbiosFromEfuse (
  UINT32    BusNum,
  UINT32    Offset,
  UINT32    Count,
  UINT32    *Size
 );
#endif // __CONFIG_UTILS_H__
