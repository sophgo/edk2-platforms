/** @file
Some functions definition about print password input interface.

Copyright (c) 2024, Sophgo. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef PASSWORDREAD_H_
#define PASSWORDREAD_H_

/**
  Get string or password input from user.

  @param  Prompt            The prompt string shown on popup window.
  @param  StringPtr         Old user input and destination for use input string.

  @retval EFI_SUCCESS       If string input is read successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
ReadString (
  IN     CHAR16                      *Prompt,
  IN OUT CHAR16                      *StringPtr
  );

VOID
EFIAPI
CreateDialog (
  OUT EFI_INPUT_KEY  *Key,        OPTIONAL
  ...
);
#endif
