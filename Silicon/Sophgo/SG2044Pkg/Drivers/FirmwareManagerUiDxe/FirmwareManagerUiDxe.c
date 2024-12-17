/** @file
  The entry point for Firmware Manager module.

  Copyright (c) 2024, SOPHGO Technologies Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "FirmwareManager.h"

/**
  The entry point for Firmware Manager configuration driver.

  @param[in]  ImageHandle          The image handle of the driver.
  @param[in]  SystemTable          The system table.

  @retval     EFI_ALREADY_STARTED  The driver already exists in system.
  @retval     EFI_OUT_OF_RESOURCES Fail to execute entry point due to lack of resources.
  @retval     EFI_SUCCES           All the related protocols are installed on the driver.
  @retval     Others               Fail to get the SecureBootEnable variable.

**/
EFI_STATUS
EFIAPI
FirmwareManagerDriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                       Status;
  FIRMWARE_MANAGER_CALLBACK_DATA   *PrivateData;

  //
  // If already started, return.
  //
  Status = gBS->OpenProtocol (
		  ImageHandle,
                  &gEfiCallerIdGuid,
                  NULL,
                  ImageHandle,
                  ImageHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
		  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Create a private data structure.
  //
  PrivateData = AllocateCopyPool (sizeof (FIRMWARE_MANAGER_CALLBACK_DATA), &gFirmwareManagerPrivate);
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Install Firmware Manager configuration form
  //
  Status = InstallFirmwareManagerForm (PrivateData);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Install private GUID.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
		  &ImageHandle,
                  &gEfiCallerIdGuid,
                  PrivateData,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  return EFI_SUCCESS;

ErrorExit:
  if (PrivateData != NULL) {
    UninstallFirmwareManagerForm (PrivateData);
  }

  return Status;
}

/**
  Unload the Firmware Manager configuration form.

  @param[in]  ImageHandle         The driver's image handle.

  @retval     EFI_SUCCESS         The SecureBoot configuration form is unloaded.
  @retval     Others              Failed to unload the form.

**/
EFI_STATUS
EFIAPI
FirmwareManagerDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                       Status;
  FIRMWARE_MANAGER_CALLBACK_DATA   *PrivateData;

  Status = gBS->HandleProtocol (
		  ImageHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &PrivateData
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (PrivateData->Signature == FIRMWARE_MANAGER_CALLBACK_DATA_SIGNATURE);

  gBS->UninstallMultipleProtocolInterfaces (
         &ImageHandle,
         &gEfiCallerIdGuid,
         PrivateData,
         NULL
         );

  UninstallFirmwareManagerForm (PrivateData);

  return EFI_SUCCESS;
}
