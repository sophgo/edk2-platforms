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

#endif /* INI_PARSER_LIB_H_ */
