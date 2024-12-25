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
#endif /* INI_PARSER_LIB_H_ */
