/** 
  Copyright (c) 2025, SOPHGO Technology Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __SECURE_BOOT_VARIABLE_VERIFY_PK_H__
#define __SECURE_BOOT_VARIABLE_VERIFY_PK_H__

/**
  Initializes RootKey with data from FFS section. This key is for verifying the PK

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
**/
EFI_STATUS
SecureBootInitRootKey (
  IN VOID
  );

/**
  Initializes Pk signature with data from FFS section. 

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
**/
EFI_STATUS
SecureBootInitPKSignature (
  IN VOID
  );

#endif
