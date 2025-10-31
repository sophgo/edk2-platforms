/** @file
  Build FV related hobs for platform.

  Copyright (c) 2025, SOPHGO Technologies Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiPei.h"
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PrePiLib.h>

/**
  Publish DXE (Decompressed) Memory based FVs to let PEI
  and DXE know about them.

  @retval EFI_SUCCESS   Platform PEI FVs were initialized successfully.

**/
EFI_STATUS
PeiFvInitialization (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS           Status;
  UINT32               FvSize;
  EFI_PEI_FV_HANDLE    VolumeHandle;

  DEBUG ((DEBUG_INFO, "%a: Parser Compressed Firmware Volume\n", __func__));

  Status = DecompressFirstFv ();
  GetNextVolume (1, &VolumeHandle);
  FvSize = ((EFI_FIRMWARE_VOLUME_HEADER *)VolumeHandle)->FvLength;

  //
  // Let DXE know about the DXE FV, Done in DecompressFirstFv
  //
  DEBUG ((
    DEBUG_INFO,
    "Platform builds DXE FV at 0x%x, size 0x%x\n",
    VolumeHandle,
    FvSize
    ));

  //
  // Let PEI know about the DXE FV so it can find the DXE Core
  //
  PeiServicesInstallFvInfoPpi (
    NULL,
    (VOID*)VolumeHandle,
    FvSize,
    NULL,
    NULL
    );

  return EFI_SUCCESS;
}