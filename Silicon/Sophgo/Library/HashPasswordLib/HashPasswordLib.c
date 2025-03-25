/** @file
The header file of Hash Password.

Copyright (c) 2025, Sophgo. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/BaseCryptLib.h>
#include "Include/Library/HashPasswordLib.h"

/**
  Hashes the given password using SHA-256.

  @param[in]  Password      The password to be hashed.
  @param[out] PasswordHash  The buffer to store the resulting hash.

  @retval EFI_SUCCESS       The password was successfully hashed.
  @retval Other             An error occurred during the hashing process.
**/
EFI_STATUS
HashPassword(
  IN  CHAR16    *Password,
  OUT UINT8     *PasswordHash
  )
{
  EFI_STATUS Status;

  Status = Sha256HashAll(
             (UINT8 *)Password,
             StrLen(Password) * sizeof(CHAR16),
             PasswordHash
           );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to hash password: %r\n", Status));
    return Status;
  }
  return EFI_SUCCESS;
}
