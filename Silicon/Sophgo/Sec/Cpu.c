/** @file
 The library call to pass the device tree to DXE via HOB.

 Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
 Copyright (c) 2024, SOPHGO Inc. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>

/**
  Cpu Peim initialization.

**/
EFI_STATUS
CpuPeimInitialization (
  VOID
  )
{
  //
  // Publish the CPU Memory and IO spaces sizes.
  //
  BuildCpuHob (PcdGet8 (PcdPrePiCpuMemorySize), PcdGet8 (PcdPrePiCpuIoSize));

  return EFI_SUCCESS;
}
