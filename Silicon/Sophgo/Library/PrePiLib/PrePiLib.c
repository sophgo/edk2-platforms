/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2025, SOPHGO Technologies Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PrePi.h"

EFI_STATUS
EFIAPI
DecompressFirstFv (
  VOID
  )
{
  EFI_STATUS           Status;
  EFI_PEI_FV_HANDLE    VolumeHandle;
  EFI_PEI_FILE_HANDLE  FileHandle;

  Status = FfsAnyFvFindFirstFile (EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE, &VolumeHandle, &FileHandle);
  if (!EFI_ERROR (Status)) {
    Status = FfsProcessFvFile (FileHandle);
  }
  return Status;
}

RETURN_STATUS
EFIAPI
PeCoffLoaderGetEntryPoint (
  IN  VOID  *Pe32Data,
  OUT VOID  **EntryPoint
  )
{
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;

  ASSERT (Pe32Data   != NULL);
  ASSERT (EntryPoint != NULL);

  DosHdr = (EFI_IMAGE_DOS_HEADER *)Pe32Data;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN)Pe32Data + (UINTN)((DosHdr->e_lfanew) & 0x0ffff));
  } else {
    //
    // DOS image header is not present, so PE header is at the image base.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)Pe32Data;
  }

  //
  // Calculate the entry point relative to the start of the image.
  // AddressOfEntryPoint is common for PE32 & PE32+
  //
  if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    *EntryPoint = (VOID *)((UINTN)Pe32Data + (UINTN)(Hdr.Te->AddressOfEntryPoint & 0x0ffffffff) + sizeof (EFI_TE_IMAGE_HEADER) - Hdr.Te->StrippedSize);
    return RETURN_SUCCESS;
  } else if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE) {
    *EntryPoint = (VOID *)((UINTN)Pe32Data + (UINTN)(Hdr.Pe32->OptionalHeader.AddressOfEntryPoint & 0x0ffffffff));
    return RETURN_SUCCESS;
  }

  return RETURN_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LoadPeiEntryPointFromFfsFile (
  IN EFI_PEI_FILE_HANDLE          FileHandle,
  OUT EFI_PHYSICAL_ADDRESS        *EntryPoint
  )
{
  EFI_STATUS                      Status;
  VOID                            *PeCoffImage;

  Status = FfsFindSectionDataWithHook (EFI_SECTION_PE32, NULL, FileHandle, &PeCoffImage);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to found vaild section\n"));
    return Status;
  }

  Status = PeCoffLoaderGetEntryPoint ((VOID *)(UINTN)PeCoffImage, (VOID **)EntryPoint);
  if (EFI_ERROR (Status))
    DEBUG ((DEBUG_INFO, "%a: Failed to find PEI Core EntryPoint\n", __func__));

  return Status;
}

EFI_STATUS
EFIAPI
GetNextVolume (
  IN UINTN                     Instance,
  OUT EFI_PEI_FV_HANDLE        *VolumeHandle
  )
{
  EFI_STATUS           Status;

  Status = FfsFindNextVolume (Instance, VolumeHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to find the target volume!\n"));
  }
  return Status;
}

EFI_STATUS
EFIAPI
LoadPeiEntryPointFromFv (
  OUT EFI_PEI_CORE_ENTRY_POINT    *PeiCoreEntryPoint
  )
{
  EFI_STATUS                Status;
  EFI_PEI_FV_HANDLE         VolumeHandle = NULL;
  EFI_PEI_FILE_HANDLE       FileHandle = NULL;

  Status = FfsAnyFvFindFirstFile (EFI_FV_FILETYPE_PEI_CORE, &VolumeHandle, &FileHandle);
  if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to find the pei core!\n"));
      return Status;
  }
  DEBUG ((DEBUG_INFO, "%a: Fv: 0x%lX\n", __func__, (UINTN)VolumeHandle));

  LoadPeiEntryPointFromFfsFile (FileHandle, (EFI_PHYSICAL_ADDRESS *)PeiCoreEntryPoint);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to find the PeiCoreEntryPoint!\n"));
    return Status;
  }

  return Status;
}