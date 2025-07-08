/** @file
  RISC-V SEC phase module for SOPHGO Platform.

  Copyright (c) 2008 - 2023, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All rights reserved.<BR>
  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
  Copyright (c) 2025, SOPHGO Technologies Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeilessSec.h"

EFI_PEI_FIRMWARE_VOLUME_INFO_PPI mDxeAddtionFVPpi = {
  EFI_FIRMWARE_FILE_SYSTEM2_GUID,
  NULL,
  0,
  NULL,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR mPrivateDispatchTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiPeiMemoryDiscoveredPpiGuid,
    NULL
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiFirmwareVolumeInfoPpiGuid,
    &mDxeAddtionFVPpi
  }
};

/**
  Initialize the memory and CPU, setting the boot mode, and platform
  initialization. It also builds the core information HOB.

  @return EFI_SUCCESS     Status.
**/
STATIC
EFI_STATUS
EFIAPI
SecInitializePlatform (
  VOID
  )
{
  EFI_STATUS  Status;
  FIRMWARE_SEC_PERFORMANCE      Performance;
  UINT64                        StartTimeStamp;

  MemoryPeimInitialization ();

  CpuPeimInitialization ();
  // Store timer value logged at the beginning of firmware image execution
  StartTimeStamp = GetPerformanceCounter();
  Performance.ResetEnd = GetTimeInNanoSecond (StartTimeStamp);

  // Build SEC Performance Data Hob
  BuildGuidDataHob (&gEfiFirmwarePerformanceGuid, &Performance, sizeof (Performance));

  // Set the Boot Mode
  SetBootMode (BOOT_WITH_FULL_CONFIGURATION);

  Status = PlatformPeimInitialization ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/** Transion from SEC phase to PEI phase.

  This function transits to S-mode PEI phase from M-mode SEC phase.

  @param[in]  SecCoreData     SecCore Infomation transfered to PEI core.

**/
VOID
EFIAPI
PeiCore (
  EFI_SEC_PEI_HAND_OFF            *SecCoreData
  )
{
  EFI_PEI_CORE_ENTRY_POINT        PeiCoreEntryPoint;
  EFI_FIRMWARE_VOLUME_HEADER      *BootFv;
  EFI_PEI_PPI_DESCRIPTOR          *EfiPeiPpiDescriptor;

  BootFv = (EFI_FIRMWARE_VOLUME_HEADER *)FixedPcdGet32 (PcdRiscVPeiFvBase);
  LoadPeiEntryPointFromFv (&PeiCoreEntryPoint);

  SecCoreData->DataSize               = sizeof (EFI_SEC_PEI_HAND_OFF);
  SecCoreData->BootFirmwareVolumeBase = BootFv;
  SecCoreData->BootFirmwareVolumeSize = (UINTN)BootFv->FvLength;

  EfiPeiPpiDescriptor = (EFI_PEI_PPI_DESCRIPTOR *)&mPrivateDispatchTable;
  //
  // Transfer the control to the PEI core
  //
  (*PeiCoreEntryPoint)(SecCoreData, EfiPeiPpiDescriptor);
}

/**

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.


  @param[in]  BootHartId         Hardware thread ID of boot hart.
  @param[in]  DeviceTreeAddress  Pointer to Device Tree (DTB)
**/
VOID
NORETURN
EFIAPI
SecStartup (
  IN  UINTN  BootHartId,
  IN  VOID   *DeviceTreeAddress
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *HobList;
  EFI_RISCV_FIRMWARE_CONTEXT  FirmwareContext;
  EFI_STATUS                  Status;
  UINT64                      UefiMemoryBase;
  UINT64                      StackBase;
  UINT32                      StackSize;
  EFI_PEI_FV_HANDLE           VolumeHandle;
  EFI_SEC_PEI_HAND_OFF        SecCoreData;

  SerialPortInitialize ();
  //
  // Report Status Code to indicate entering SEC core
  //
  
  DEBUG ((
    DEBUG_INFO,
    "%a() SecStartup: 0x%lx BootHartId: 0x%x, DeviceTreeAddress=0x%lx\n",
    __func__,
    SecStartup,
    BootHartId,
    DeviceTreeAddress
    ));

  FirmwareContext.BootHartId          = BootHartId;
  FirmwareContext.FlattenedDeviceTree = (UINT64)DeviceTreeAddress;
  SetFirmwareContextPointer (&FirmwareContext);

  StackBase      = (UINT64)FixedPcdGet32 (PcdTemporaryRamBase);
  StackSize      = FixedPcdGet32 (PcdTemporaryRamSize);
  UefiMemoryBase = FixedPcdGet64 (PcdEfiMemoryBottom);

  /*
   *  --------  --> Stack Top, EfiMemoryTop, EfiFreeMemoryTop (PcdTemporaryRamBase)
   * |        |
   * | Stack  |
   * |        |
   *  --------  --> Stack Base (PcdTemporaryRamSize)
   * |        |
   * |        |
   * | EfiMem |
   * |        |
   * |        |
   *  --------  --> EfiMemoryBottom, EfiFreeMemoryBottom (PcdEfiMemoryBottom)
   * |        |
   * |   FW   |
   * |        |
   *  --------  --> FW_BASE_ADDRESS, typically after opensbi
   */

  // Declare the PI/UEFI memory region
  HobList = HobConstructor (
              (VOID *)UefiMemoryBase,
              StackBase + StackSize - UefiMemoryBase,
              (VOID *)UefiMemoryBase,
              (VOID *)StackBase // The top of the UEFI Memory is reserved for the stacks
              );
  PrePeiSetHobList (HobList);

  SecInitializePlatform ();

  BuildStackHob (StackBase, StackSize);

  //
  // Process all libraries constructor function linked to SecMain.
  //
  ProcessLibraryConstructorList ();

  // Assume the FV that contains the SEC (our code) also contains a compressed FV.
  Status = DecompressFirstFv ();
  ASSERT_EFI_ERROR (Status);

  // transfer the second FV info to PEI phase
  GetNextVolume (1, &VolumeHandle);
  mDxeAddtionFVPpi.FvInfo = VolumeHandle;
  mDxeAddtionFVPpi.FvInfoSize = ((EFI_FIRMWARE_VOLUME_HEADER *)VolumeHandle)->FvLength;

  //transfer the memory info from SEC to PEI phase
  SecCoreData.StackBase = (VOID *)StackBase;
  SecCoreData.StackSize = StackSize;
  SecCoreData.TemporaryRamBase = (VOID *)UefiMemoryBase;
  SecCoreData.TemporaryRamSize = (StackBase + StackSize - UefiMemoryBase) >> 1;
  SecCoreData.PeiTemporaryRamBase = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize = SecCoreData.TemporaryRamSize;

  PeiCore (&SecCoreData);

  //
  // Should not come here.
  //
  UNREACHABLE ();
}
