/** @file
  SG2042 PEI platform resource initialization.

  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong University, China. All rights reserved.
  Copyright (c) 2026, SOPHGO Technologies Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResourcePublicationLib.h>

EFI_STATUS
EFIAPI
PeiPlatformInitialization (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = PublishSystemMemory (
             PcdGet64 (PcdPeiPermanentMemBase),
             PcdGet64 (PcdPeiPermanentMemSize)
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
