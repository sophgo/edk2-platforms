/** @file
 *  Library implementation for the INI File Parser.
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef INI_PARSER_LIB_H_
#define INI_PARSER_LIB_H_

typedef struct {
  CHAR8 *Name;
  UINTN Addr;
  // .....
} TEST_CONFIG;

typedef struct {
  UINTN Mac0Addr;
  UINTN Mac1Addr;
} MAC_CONFIG;

extern TEST_CONFIG Config;
extern MAC_CONFIG  MacConfig;

extern CHAR8 MemoryData[];

BOOLEAN
EFIAPI
IsIniFileExist (
  VOID
  );

EFI_STATUS
EFIAPI
TestIniParser (
  VOID
  );

EFI_STATUS
EFIAPI
MacAddrIniParser (
  VOID
  );

/**
  Get the value associated with a section and name.

  @param[in]  Section   The section name.
  @param[in]  Name      The key name.
  @param[out] Value     The value associated with the key.

  @retval 0             Successfully retrieved the value.
  @retval -1            The key was not found.
**/
INT32
IniGetValueBySectionAndName (
  CONST CHAR8 *Section,
  CONST CHAR8 *Name,
  CHAR8 *Value
  );

/**
  Handler function for processing INI data.

  @param[in] User       User-defined context data.
  @param[in] Section    The current section name.
  @param[in] Name       The current key name.
  @param[in] Value      The current key value.

  @retval 1             Successfully processed.
  @retval 0             Parsing complete or failed.
**/
INT32
IniHandler (
  VOID       *User,
  CONST CHAR8 *Section,
  CONST CHAR8 *Name,
  CONST CHAR8 *Value
  );

/**
  Parse INI-format data.

  @param[in] User       User-defined context data.

  @retval >=0           Number of successfully parsed key-value pairs.
  @retval -1            Parsing failed.
**/
INT32
IniConfIniParse (
  IN VOID  *User
  );

#endif /* INI_PARSER_LIB_H_ */
