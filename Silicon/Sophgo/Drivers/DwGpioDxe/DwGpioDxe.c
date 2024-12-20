/** @file
  The Designware GPIO controller driver.

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Protocol/FdtClient.h>
#include <Include/DwGpio.h>

#define GPIO_MUX_VAL(Gpio)        (0x00000001 << (Gpio))

//
// Address GPIO_REG Registers
//
#define GPIO_SWPORTA_DR           0x00000000
#define GPIO_SWPORTA_DDR          0x00000004
#define GPIO_EXT_PORTA            0x00000050

#define GPIO_PINS_PER_CONTROLLER  32

typedef struct {
  UINTN    Regs;
  UINT32   NrGpios;
} DW_GPIO;

STATIC  DW_GPIO                   *mDwGpios;
STATIC  UINT32                    mNumberOfControllers;
STATIC  SOPHGO_GPIO_PROTOCOL      *mGpioProtocol;

VOID
GpioWrite (
  IN UINTN   Reg,
  IN UINT32  Val
  )
{
  MmioWrite32 (Reg, Val);
}

VOID
GpioRead (
  IN  UINTN   Reg,
  OUT UINT32  *Val
  )
{
  ASSERT (Val != NULL);
  *Val = MmioRead32 (Reg);
}

EFI_STATUS
GetGpioBase (
  IN  UINT32  Bus,
  IN  UINT32  Pin,
  OUT UINTN   *Base
  )
{
  if (Bus >= mNumberOfControllers)
    return EFI_INVALID_PARAMETER;

  if (Pin >= mDwGpios[Bus].NrGpios)
    return EFI_INVALID_PARAMETER;

  *Base = mDwGpios[Bus].Regs;

  return EFI_SUCCESS;
}

/**
  Get the direction of the given GPIO pin.

  @param[in]  This               Protocol instance structure
  @param[in]  Bus                GPIO bus number.
  @param[in]  Pin                GPIO pin number.
  @param[out] Direction          GPIO pin direction.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Parameter error.

**/
EFI_STATUS
EFIAPI
GpioGetDirection (
  IN  SOPHGO_GPIO_PROTOCOL  *This,
  IN  UINT32                Bus,
  IN  UINT32                Pin,
  OUT UINT8                 *Direction
  )
{
  UINTN       Base;
  UINT32      ReadVal;
  EFI_STATUS  Status;

  Status = GetGpioBase (Bus, Pin, &Base);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Get GPIO addr error: %r\n",Status));
    return Status;
  }

  GpioRead (Base + GPIO_SWPORTA_DDR, &ReadVal);

  *Direction = ReadVal & GPIO_MUX_VAL (Pin) ? GPIO_OUT : GPIO_IN;

  return EFI_SUCCESS;
}

/**
  Used to set/clear GPIO pin value.

  @param[in]  This               Protocol instance structure
  @param[in]  Bus                GPIO bus number.
  @param[in]  Pin                GPIO pin number.
  @param[in]  Val                1 to set, 0 to clear.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  pin number is out of range.

**/
EFI_STATUS
EFIAPI
GpioWriteBit (
  IN SOPHGO_GPIO_PROTOCOL  *This,
  IN UINT32                Bus,
  IN UINT32                Pin,
  IN UINT32                Val
  )
{
  UINTN       Base;
  UINT32      ReadVal;
  EFI_STATUS  Status;

  Status = GetGpioBase (Bus, Pin, &Base);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Get GPIO addr error: %r\n",Status));
    return Status;
  }

  GpioRead (Base + GPIO_SWPORTA_DR, &ReadVal);

  if (Val) {
    GpioWrite (Base + GPIO_SWPORTA_DR, ReadVal | GPIO_MUX_VAL (Pin));
  } else {
    GpioWrite (Base + GPIO_SWPORTA_DR, ReadVal & ~GPIO_MUX_VAL (Pin));
  }

  return EFI_SUCCESS;
}

/**
  When the pin is configured as an input, obtain its voltage level.
  And when it is configured as output, retrieve the value written
  to the data register last time.

  @param[in]   This              Protocol instance structure
  @param[in]   Bus               GPIO bus number.
  @param[in]   Pin               GPIO pin number.
  @param[out]  Level             The read pin level (1 : On/High, 0 : Off/Low) when input.
                                 The value written to the data register last time when output.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  pin number is out of range.

**/
EFI_STATUS
EFIAPI
GpioReadBit (
  IN  SOPHGO_GPIO_PROTOCOL  *This,
  IN  UINT32                Bus,
  IN  UINT32                Pin,
  OUT UINT32                *Level
  )
{
  UINTN       Base;
  UINT32      Val;
  UINT8       Direction;
  EFI_STATUS  Status;

  Status = GetGpioBase (Bus, Pin, &Base);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Get GPIO addr error: %r\n",Status));
    return Status;
  }

  Status = GpioGetDirection (This, Bus, Pin, &Direction);
  if (EFI_ERROR (Status))
    return Status;

  if (Direction == GPIO_OUT)
    GpioRead (Base + GPIO_SWPORTA_DR, &Val);
  else
    GpioRead (Base + GPIO_EXT_PORTA, &Val);

  *Level = Val & GPIO_MUX_VAL (Pin) ? 1 : 0;

  return EFI_SUCCESS;
}

EFI_STATUS
GpioConfig (
  IN UINT32  Bus,
  IN UINT32  Pin,
  IN UINT32  InOut
  )
{
  UINT32      Val;
  UINTN       Base;
  EFI_STATUS  Status;

  Status = GetGpioBase (Bus, Pin, &Base);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Get GPIO addr error: %r\n",Status));
    return Status;
  }

  GpioRead (Base + GPIO_SWPORTA_DDR, &Val);

  if (InOut == GPIO_OUT) {
    GpioWrite (Base + GPIO_SWPORTA_DDR, Val | GPIO_MUX_VAL (Pin));
  } else {
    GpioWrite (Base + GPIO_SWPORTA_DDR, Val & ~GPIO_MUX_VAL (Pin));
  }

  return EFI_SUCCESS;
}

/**
  Used to configure GPIO with a given Mode.

  @param[in]  This               Protocol instance structure
  @param[in]  Bus                GPIO bus number.
  @param[in]  Pin                GPIO pin number.
  @param[in]  Mode               GPIO configure mode.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Parameter error.

**/
EFI_STATUS
EFIAPI
GpioModeConfig (
  IN SOPHGO_GPIO_PROTOCOL  *This,
  IN UINT32                Bus,
  IN UINT32                Pin,
  IN GPIO_CONFIG_MODE      Mode
  )
{
  EFI_STATUS  Status;

  switch (Mode) {
    case GpioConfigOutLow:
      Status = GpioConfig (Bus, Pin, GPIO_OUT);
      if (EFI_ERROR (Status))
        return Status;

      Status = GpioWriteBit (This, Bus, Pin, 0);
      if (EFI_ERROR (Status))
        return Status;
      break;

    case GpioConfigOutHigh:
      Status = GpioConfig (Bus, Pin, GPIO_OUT);
      if (EFI_ERROR (Status))
        return Status;

      Status = GpioWriteBit (This, Bus, Pin, 1);
      if (EFI_ERROR (Status))
        return Status;
      break;

    case GpioConfigIn:
      Status = GpioConfig (Bus, Pin, GPIO_IN);
      if (EFI_ERROR (Status))
        return Status;
      break;

    default:
      DEBUG ((DEBUG_ERROR, "Invalid GPIO mode\n"));
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SetMemory (
  VOID
)
{
  EFI_STATUS  Status;
  UINT32      Index;

  for (Index = 0; Index < mNumberOfControllers; Index++) {
    Status = gDS->AddMemorySpace(
                    EfiGcdMemoryTypeMemoryMappedIo,
                    mDwGpios[Index].Regs,
                    SIZE_4KB,
                    EFI_MEMORY_UC | EFI_MEMORY_XP
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a:%d] Add memory space failed: %r\n",
            __func__, __LINE__, Status));
      return Status;
    }

    Status = gDS->SetMemorySpaceAttributes (
                    mDwGpios[Index].Regs,
                    SIZE_4KB,
                    EFI_MEMORY_UC | EFI_MEMORY_XP
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a:%d] Set memory attributes failed: %r\n",
              __func__, __LINE__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetControllerInfoByFdt (
  IN  CONST CHAR8     *CompatibleString
  )
{
  FDT_CLIENT_PROTOCOL  *FdtClient;
  EFI_STATUS           FindNodeStatus, Status;
  INT32                Node;
  UINT32               Index;
  CONST VOID           *Prop;
  UINT32               PropSize;

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL, (VOID **)&FdtClient);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "No FDT client service found\n"));
    return EFI_NOT_FOUND;
  }

  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, CompatibleString, &Node), Index = 0;
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, CompatibleString, Node, &Node)) {
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "reg", &Prop, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: GetNodeProperty (reg) failed (Status == %r)\n", __func__, Status));
      continue;
    }
    ++Index;
  }

  if (Index == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot get GPIO node from DTS (Status == %r)\n", __func__, Status));
    return EFI_NOT_FOUND;
  }

  mNumberOfControllers = Index;
  mDwGpios = AllocateZeroPool (mNumberOfControllers * sizeof (DW_GPIO));
  if (mDwGpios == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, CompatibleString, &Node), Index = 0;
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, CompatibleString, Node, &Node)) {
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "reg", &Prop, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: GetNodeProperty (reg) failed (Status == %r)\n", __func__, Status));
      continue;
    }
    mDwGpios[Index].Regs    = SwapBytes64 (((CONST UINT64 *)Prop)[0]);
    mDwGpios[Index].NrGpios = GPIO_PINS_PER_CONTROLLER;
    ++Index;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DwGpioEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS   Status;
  CONST CHAR8  *CompatibleString;

  CompatibleString = "snps,dw-apb-gpio";
  Status = GetControllerInfoByFdt (CompatibleString);
  if (EFI_ERROR (Status))
    return Status;

  Status = SetMemory ();
  if (EFI_ERROR (Status)) {
    goto ErrorSetMemory;
  }

  mGpioProtocol = AllocateZeroPool (sizeof (SOPHGO_GPIO_PROTOCOL));
  if (mGpioProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory for mGpioProtocol\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorSetMemory;
  }

  mGpioProtocol->SetValue     = GpioWriteBit;
  mGpioProtocol->GetValue     = GpioReadBit;
  mGpioProtocol->ModeConfig   = GpioModeConfig;
  mGpioProtocol->GetDirection = GpioGetDirection;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gSophgoGpioProtocolGuid,
                  mGpioProtocol,
                  NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,"%a: InstallProtocolInterface(): %r\n",
            __func__, Status));
    goto ErrorInstallProtocol;
  }

  for (UINT32 Index = 0; Index < mNumberOfControllers; ++Index) {
    DEBUG ((DEBUG_INFO, "[Gpio%u base: 0x%lx, nr-gpios: %u]\n",
                         Index,
                         mDwGpios[Index].Regs,
                         mDwGpios[Index].NrGpios));
  }

  return EFI_SUCCESS;

ErrorInstallProtocol:
  FreePool (mGpioProtocol);

ErrorSetMemory:
  FreePool (mDwGpios);

  return Status;
}

EFI_STATUS
EFIAPI
DwGpioUnload (
  IN  EFI_HANDLE  ImageHandle
  )
{
  if (mDwGpios != NULL)
    FreePool (mDwGpios);

  if (mGpioProtocol != NULL)
    FreePool (mGpioProtocol);

  gBS->UninstallProtocolInterface (
         &ImageHandle,
         &gSophgoGpioProtocolGuid,
         NULL);

  return EFI_SUCCESS;
}
