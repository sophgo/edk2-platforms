/** @file
 The library call to pass the device tree to DXE via HOB.

 Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
 Copyright (c) 2025, SOPHGO Technologies Inc. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiPei.h"
#include <Library/DebugLib.h>
#include <Library/HobLib.h>

/**
  Cpu Peim initialization.

**/
EFI_STATUS
PeiCpuInitialization (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  //
  // Publish the CPU Memory and IO spaces sizes.
  //
  BuildCpuHob (PcdGet8 (PcdPrePiCpuMemorySize), PcdGet8 (PcdPrePiCpuIoSize));
  DEBUG ((
    DEBUG_INFO,
    "RiscV cpu memorysize is 0x%x\n",
    PcdGet8 (PcdPrePiCpuMemorySize)
    ));

  return EFI_SUCCESS;
}