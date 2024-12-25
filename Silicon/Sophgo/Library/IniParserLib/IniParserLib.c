/** @file
*  IniParserLib.c
*
*  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IniParserLib.h>
#include "IniParserUtil.h"

#define INI_FILE_MAX_SIZE   (FixedPcdGet32(PcdIniFileMaxSize))
CHAR8 MemoryData[INI_FILE_MAX_SIZE];

TEST_CONFIG Config;
MAC_CONFIG MacConfig;

STATIC
CHAR8 *
IniStrDup (
  IN CONST CHAR8  *String
  )
{
  return AllocateCopyPool (AsciiStrSize (String), String);
}

STATIC
INT32
TestHandler (
  IN       VOID  *User,
  IN CONST CHAR8 *Section,
  IN CONST CHAR8 *Name,
  IN CONST CHAR8 *Value
  )
{
  TEST_CONFIG *Config = (TEST_CONFIG *)User;

  #define MATCH(S, N) (AsciiStrCmp (Section, S) == 0 && AsciiStrCmp (Name, N) == 0)

  if (MATCH ("Processor", "Name")) {
    Config->Name = IniStrDup (Value);
  } else {
    return 0;
  }

  return -1;
}

STATIC
INT32
HandlerMacAddr (
  IN       VOID  *User,
  IN CONST CHAR8 *Section,
  IN CONST CHAR8 *Name,
  IN CONST CHAR8 *Value
  )
{
  MAC_CONFIG *Config = (MAC_CONFIG *)User;

  #define MATCH(S, N) (AsciiStrCmp (Section, S) == 0 && AsciiStrCmp (Name, N) == 0)

  if (MATCH("mac-address", "mac0")) {
    Config->Mac0Addr = AsciiStrHexToUintn (Value);
  } else if (MATCH("mac-address", "mac1")) {
    Config->Mac1Addr = AsciiStrHexToUintn (Value);
  } else {
    return 0;
  }

  return -1;
}

BOOLEAN
EFIAPI
IsIniFileExist (
  VOID
  )
{
  VOID *Address = (VOID *)PcdGet64 (PcdIniFileRamAddress);
  CHAR8 *IniHeader = "[sophgo-config]";
  CHAR8 *IniFooter = "[eof]";

  CopyMem (MemoryData, Address, INI_FILE_MAX_SIZE);
  MemoryData[INI_FILE_MAX_SIZE - 1] = '\0';

  UINTN Length = AsciiStrLen (IniHeader);

  if (AsciiStrnCmp (IniHeader, MemoryData, Length)) {
    DEBUG ((
      DEBUG_ERROR,
      "Not found conf.ini file, no header: \"%a\"\n",
      IniHeader
      ));
    return FALSE;
  } else if (!(AsciiStrStr (MemoryData, IniFooter))) {
     DEBUG ((
      DEBUG_INFO,
      "Found conf.ini header: \"%a\", but footer not found: \"%a\"\n",
      IniHeader,
      IniFooter
      ));
     return FALSE;
  } else {
     DEBUG ((
      DEBUG_VERBOSE,
      "conf.ini has been found!\n"
      ));
     return TRUE;
  }
}

EFI_STATUS
EFIAPI
TestIniParser (
  VOID
  )
{
  if (IniParseString ((CONST CHAR8 *)MemoryData, TestHandler, &Config) < 0) {
     DEBUG ((
      DEBUG_INFO,
      "%a: conf.ini parse failed!\n",
      __func__
      ));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "Config.Name: %a\n",
    Config.Name
    ));

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MacAddrIniParser (
  VOID
  )
{
  if (IniParseString ((CONST CHAR8 *)MemoryData, HandlerMacAddr, &MacConfig) < 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: conf.ini parse failed!\n",
      __func__
      ));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "Mac0Addr: 0x%lx\tMac1Addr: 0x%lx\n",
    MacConfig.Mac0Addr,
    MacConfig.Mac1Addr
    ));

  return EFI_SUCCESS;
}
