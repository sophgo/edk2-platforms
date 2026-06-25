/** @file
  Read BMC Builtin FRU via IPMI, cache serial numbers, and publish protocol.

  Copyright (c) 2026, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Protocol/IpmiFruInfo.h>

#include <Library/DebugLib.h>
#include <Library/IpmiFruInfoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

STATIC IPMI_FRU_INFO_PROTOCOL  mIpmiFruInfoProtocol = {
  IPMI_FRU_INFO_PROTOCOL_REVISION,
  IpmiFruInfoGet,
};

EFI_STATUS
EFIAPI
IpmiFruInfoDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "IpmiFruInfoDxe: load Builtin FRU at DXE init\n"));
  IpmiFruInfoGet (FruChassisSerialNumber);

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gIpmiFruInfoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mIpmiFruInfoProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "IpmiFruInfoDxe: failed to install protocol %r\n", Status));
  }

  return Status;
}
