/** @file

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2024. SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <SmbiosProcessorSpecificData.h>
#include <ProcessorSpecificHobData.h>

#include "SmbiosPlatformDxe.h"

SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformProcessorAdditional) {
  EFI_STATUS           Status;
  SMBIOS_TABLE_TYPE44                 *Type44Ptr;

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

    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->Revision = SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA_REVISION;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->Length = sizeof (SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA);
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->HartId.Value64_L = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->HartId.Value64_H = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->BootHartId = 1;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineVendorId.Value64_L = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineVendorId.Value64_H = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineArchId.Value64_L = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineArchId.Value64_H = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineImplId.Value64_L = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineImplId.Value64_H = 0;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->InstSetSupported = FixedPcdGet32(PcdMisa);
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->PrivilegeModeSupported = SMBIOS_RISC_V_PSD_MACHINE_MODE_SUPPORTED |
											 SMBIOS_RISC_V_PSD_SUPERVISOR_MODE_SUPPORTED |
											 SMBIOS_RISC_V_PSD_USER_MODE_SUPPORTED |
											 SMBIOS_RISC_V_PSD_DEBUG_MODE_SUPPORTED;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MModeExcepDelegation.Value64_L = TO_BE_FILLED;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MModeExcepDelegation.Value64_H = TO_BE_FILLED;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MModeInterruptDelegation.Value64_L = TO_BE_FILLED;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MModeInterruptDelegation.Value64_H = TO_BE_FILLED;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->HartXlen = 0x2;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->MachineModeXlen = 0x2;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->SupervisorModeXlen = 0x2;
    ((SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA *)(Type44Ptr + 1))->UserModeXlen = 0x2;

    Status = SmbiosPlatformDxeDirectAddRecord ((UINT8 *)Type44Ptr, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Fail to add SMBIOS Type 44\n"));
      return Status;
    }
    FreePool(Type44Ptr);

  return Status;
}
