/** @file

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2025. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <SmbiosProcessorSpecificData.h>
#include <ProcessorSpecificHobData.h>
#include <Protocol/FdtClient.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseRiscVSbiLib.h>

#include "SmbiosPlatformDxe.h"

STATIC VOID SbiGetMachineVendorId (
  OUT UINTN  *MachineVendorId
  )
{
  SBI_RET  Ret;

  Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_MVENDORID, 0);

  *MachineVendorId = (UINTN)Ret.Value;
}

STATIC VOID SbiGetMachineArchId (
  OUT UINTN  *MachineArchId
  )
{
  SBI_RET  Ret;

  Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_MARCHID, 0);

  *MachineArchId = (UINTN)Ret.Value;
}

STATIC VOID SbiGetMachineImplId (
  OUT UINTN  *MachineImplId
  )
{
  SBI_RET  Ret;

  Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_MIMPID, 0);

  *MachineImplId = (UINTN)Ret.Value;
}

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformProcessorAdditional) {
  EFI_STATUS                  FindNodeStatus, Status;
  SMBIOS_TABLE_TYPE44         *Type44Ptr;
  FDT_CLIENT_PROTOCOL         *FdtClient;
  CONST CHAR8                 *CompatibleString;
  INT32                       Node;
  UINT32                      Index, PropSize;
  UINT64                      CoreReg;
  UINTN                       MachineVendorId, MachineArchId, MachineImplId;
  CONST VOID                  *Prop;
  EFI_RISCV_FIRMWARE_CONTEXT  *FirmwareContext;

  CompatibleString = "thead,c920";

  GetFirmwareContext (&FirmwareContext);
  SbiGetMachineVendorId (&MachineVendorId);
  SbiGetMachineArchId (&MachineArchId);
  SbiGetMachineImplId (&MachineImplId);

  DEBUG ((DEBUG_INFO, "BootHardId: %lu\n", FirmwareContext->BootHartId));
  DEBUG ((DEBUG_INFO, "MachineVendorId: %lx, MachineArchId: %lx, MachineImplId: %lx\n", MachineVendorId, MachineArchId, MachineImplId));

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL, (VOID **)&FdtClient);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "[%a] No FDT client service found\n", __func__));
    return EFI_NOT_FOUND;
  }

  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, CompatibleString, &Node), Index = 0;
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, CompatibleString, Node, &Node)) {
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "reg", &Prop, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] GetNodeProperty failed (Status == %r)\n", __func__, Status));
      continue;
    }
    CoreReg = SwapBytes32 (((CONST UINT32 *)Prop)[0]);

    Type44Ptr = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE44) + sizeof (SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA) + 2); // Two ending zero.
    if (Type44Ptr == NULL) {
      return EFI_NOT_FOUND;
    }

    Type44Ptr->Hdr.Type                                 = SMBIOS_TYPE_PROCESSOR_ADDITIONAL_INFORMATION;
    Type44Ptr->Hdr.Handle                               = SMBIOS_HANDLE_PROCESSOR_ADDITIONAL;
    Type44Ptr->Hdr.Length                               = sizeof (SMBIOS_TABLE_TYPE44) + sizeof (SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA);
    Type44Ptr->RefHandle                                = SMBIOS_HANDLE_PROCESSOR;
    Type44Ptr->ProcessorSpecificBlock.Length            = sizeof (SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA);
    Type44Ptr->ProcessorSpecificBlock.ProcessorArchType = 0x7;

    //
    // The SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA structure is compatible with SMBIOS Table Specification v3.3.0
    //
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->Revision = SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA_REVISION;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->Length = sizeof (SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA);
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->HartId.Value64_L = CoreReg;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->HartId.Value64_H = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->BootHartId = (CoreReg == FirmwareContext->BootHartId ? 1 : 0);
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineVendorId.Value64_L = MachineVendorId;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineVendorId.Value64_H = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineArchId.Value64_L = MachineArchId;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineArchId.Value64_H = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineImplId.Value64_L = MachineImplId;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineImplId.Value64_H = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->InstSetSupported = FixedPcdGet32(PcdMisa);
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->PrivilegeModeSupported = SMBIOS_RISC_V_PSD_MACHINE_MODE_SUPPORTED    |
                                                                                         SMBIOS_RISC_V_PSD_SUPERVISOR_MODE_SUPPORTED |
                                                                                         SMBIOS_RISC_V_PSD_USER_MODE_SUPPORTED       |
                                                                                         SMBIOS_RISC_V_PSD_DEBUG_MODE_SUPPORTED;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MModeExcepDelegation.Value64_L     = TO_BE_FILLED;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MModeExcepDelegation.Value64_H     = TO_BE_FILLED;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MModeInterruptDelegation.Value64_L = TO_BE_FILLED;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MModeInterruptDelegation.Value64_H = TO_BE_FILLED;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->HartXlen           = RegisterLen64;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineModeXlen    = RegisterLen64;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->SupervisorModeXlen = RegisterLen64;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->UserModeXlen       = RegisterLen64;

    Status = SmbiosPlatformDxeDirectAddRecord ((UINT8 *)Type44Ptr, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Fail to add SMBIOS Type 44\n"));
      FreePool (Type44Ptr);
      return Status;
    }

    FreePool (Type44Ptr);
    Index++;
  }

  return EFI_SUCCESS;
}
