#ifndef Hash_PASSWD_LIB_H__
#define Hash_PASSWD_LIB_H__
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  Hashes the given password using SHA-256.

  @param[in]  Password      The password to be hashed.
  @param[out] PasswordHash  The buffer to store the resulting hash.

  @retval EFI_SUCCESS       The password was successfully hashed.
  @retval Other             An error occurred during the hashing process.
**/
EFI_STATUS
HashPassword(
    IN CHAR16 *Password,
    OUT UINT8 *PasswordHash
);

#endif