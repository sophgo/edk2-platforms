/** @file
  This file is used to implement DesignWare SPI communication.

  Copyright (c) 2009, Intel Corporation.
  Copyright (C) 2014 Stefan Roese <sr@denx.de>
  Copyright (C) 2020 Sean Anderson <seanga2@gmail.com>
  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Pi/PiDxeCis.h>
#include <Library/TimerLib.h>
#include <Protocol/FdtClient.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include "Tpm2Transfer.h"

extern EFI_DXE_SERVICES     *gDS;

STATIC DW_GPIO              *mDwGpioInsatce;
STATIC DW_SPI               *mDwSpiInsatce;
STATIC SUB_CTRL             *mSubCtrl;
STATIC SPI_DEVICE           *Tpm2SpiSlave;
STATIC DW_SPI_CHIP_DATA     mChip;

STATIC
VOID
DwWriteL (
  IN  DW_SPI  *Dws,
  IN  UINT32  OffSet,
  IN  UINT32  Value
  )
{
  volatile UINTN Regs = Dws->Regs + OffSet;
  MmioWrite32 (Regs, Value);
}

STATIC
UINT32
DwReadL (
  IN  DW_SPI  *Dws,
  IN  UINT32  OffSet
  )
{
  volatile UINTN Regs = Dws->Regs + OffSet;
  return MmioRead32 (Regs);
}

/**
  Set chip select line electric level.

  @param[in]  Spi    The pointer to SPI_DEVICE.
  @param[in]  Level  TRUE for high electric level, FALSE for low electric level.

**/
VOID
DwSpiSetCs (
  IN  SPI_DEVICE  *Spi,
  IN  BOOLEAN     Level
  )
{
  DW_SPI   *Dws;
  BOOLEAN  IsCsActiveHigh;

  Dws = mDwSpiInsatce;
  IsCsActiveHigh = !!(Spi->Mode & SPI_CS_HIGH);

  //
  // DW SPI controller demands any native CS being set in order to
  // proceed with data transfer. So in order to activate the SPI
  // communications we must set a corresponding bit in the Slave
  // Enable register no matter whether the SPI core is configured to
  // support active-high or active-low CS level.
  //
  if (IsCsActiveHigh == Level)
    DwWriteL (Dws, DW_SPI_SER, BIT(Spi->Cs));
  else
    DwWriteL (Dws, DW_SPI_SER, 0);
}

EFI_STATUS
GetGpioInstanceByPcd (
  VOID
  )
{
  UINTN       mGpioBase;

  mGpioBase = (UINTN)PcdGet64 (PcdTpm2GpioBaseAddress);
  mDwGpioInsatce = AllocateZeroPool (sizeof (DW_GPIO));
  if (!mDwGpioInsatce) {
    DEBUG ((DEBUG_ERROR, "allocate gpio memory failed\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  mDwGpioInsatce->Regs            = mGpioBase;
  mDwGpioInsatce->NrGpios         = GPIO_PINS_PER_CONTROLLER;

  return EFI_SUCCESS;
}

EFI_STATUS
GetSpiInstanceByPcd (
  VOID
  )
{
  UINTN       mSpiBase;

  mSpiBase = (UINTN)PcdGet64 (PcdTpm2SpiBaseAddress);
  mDwSpiInsatce = AllocateZeroPool (sizeof (DW_SPI));
  if (!mDwSpiInsatce) {
    DEBUG ((DEBUG_ERROR, "allocate spi memory failed\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  mDwSpiInsatce->Regs             = mSpiBase;
  mDwSpiInsatce->MaxFreq          = CLOCK_FREQURNCY;
  mDwSpiInsatce->MaxMemFreq       = mDwSpiInsatce->MaxFreq;
  mDwSpiInsatce->SetCs            = DwSpiSetCs;
  mDwSpiInsatce->BusNum           = 0;
  mDwSpiInsatce->DefRxSampleDlyNs = 0;

  return EFI_SUCCESS;
}

EFI_STATUS
GetTopInstanceByPcd (
  VOID
  )
{
  UINTN       mTopBase;

  mTopBase = (UINTN)PcdGet64 (PcdTopBaseAddress);
  mSubCtrl = AllocateZeroPool (sizeof (SUB_CTRL));
  if (!mSubCtrl) {
    DEBUG ((DEBUG_ERROR, "allocate gpio memory failed\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  mSubCtrl->Regs    = mTopBase;
  mSubCtrl->Offset  = TOP_PIN_MUX_OFFSET;

  return EFI_SUCCESS;
}

EFI_STATUS
SetMemory (
  IN UINTN  Reg
  )
{
  EFI_STATUS                      Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR Desc;

  Status = gDS->GetMemorySpaceDescriptor (Reg, &Desc);
  if (EFI_ERROR (Status) || Desc.GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
    Status = gDS->AddMemorySpace(
                    EfiGcdMemoryTypeMemoryMappedIo,
                    Reg,
                    SIZE_4KB,
                    EFI_MEMORY_UC | EFI_MEMORY_XP
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a:%d] Add memory space failed: %r\n",
            __func__, __LINE__, Status));
      return Status;
    }
  }

  Status = gDS->SetMemorySpaceAttributes (
                  Reg,
                  SIZE_4KB,
                  EFI_MEMORY_UC | EFI_MEMORY_XP
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a:%d] Set memory attributes failed: %r\n",
            __func__, __LINE__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Find last (most-significant) bit set (1-based index).
  Note GenericFls (0) = 0, GenericFls (1) = 1, GenericFls (0x80000000) = 32.

  @param[in]   Num         The word to search.

**/
STATIC
INT32
GenericFls (
  IN UINT32 Number
  )
{
  int Result = 32;

  if (!Number)
    return 0;
  if (!(Number & 0xffff0000u)) {
    Number <<= 16;
    Result -=  16;
  }
  if (!(Number & 0xff000000u)) {
    Number <<= 8;
    Result -=  8;
  }
  if (!(Number & 0xf0000000u)) {
    Number <<= 4;
    Result -=  4;
  }
  if (!(Number & 0xc0000000u)) {
    Number <<= 2;
    Result -=  2;
  }
  if (!(Number & 0x80000000u)) {
    Number <<= 1;
    Result -=  1;
  }

  return Result;
}

STATIC
UINT32
RoundupPowOfTwo (
  IN  UINT32  Num
  )
{
  if (Num == 0) {
      return 1;
  }

  return 1U << GenericFls (Num - 1);;
}

STATIC
UINT32
Hweight16 (
  UINT32 Val
  )
{
  UINT32 Res;

  Res = Val - ((Val >> 1) & 0x5555);
  Res = (Res & 0x3333) + ((Res >> 2) & 0x3333);
  Res = (Res + (Res >> 4)) & 0x0F0F;

  return (Res + (Res >> 8)) & 0x00FF;
}

STATIC
UINT32
FieldPrep (
  IN UINT32 Mask,
  IN UINT32 Val
  )
{
  INT32  Shift, Tmp;

  Shift = 0;
  Tmp   = Mask;
  while ((Tmp & 1) == 0) {
    Tmp >>= 1;
    Shift++;
  }

  return (Val << Shift) & Mask;
}

STATIC
VOID *
SpiGetCtldata (
  IN CONST SPI_DEVICE *Spi
  )
{
  //
  // Ctldata is for the bus_controller driver's runtime state
  //
  return Spi->ControllerState;
}

STATIC
VOID
SpiSetCtldata (
  IN OUT SPI_DEVICE *Spi,
  IN     VOID       *State
  )
{
  Spi->ControllerState = State;
}

STATIC
VOID
DwSpiSetClk (
  IN  DW_SPI *Dws,
  IN  UINT16 Div
  )
{
  DwWriteL (Dws, DW_SPI_BAUDR, Div);
}

STATIC
VOID
DwSpiEnableChip (
  IN DW_SPI *Dws,
  IN INT32  Enable
  )
{
  DwWriteL (Dws, DW_SPI_SSIENR, (Enable ? 1 : 0));
}

STATIC
VOID
DwSpiMaskIntr (
  IN DW_SPI *Dws,
  IN UINT32 Mask
  )
{
  UINT32 NewMask;

  NewMask = DwReadL (Dws, DW_SPI_IMR) & ~Mask;
  DwWriteL (Dws, DW_SPI_IMR, NewMask);
}

STATIC
VOID
DwSpiResetChip (
  IN DW_SPI *Dws
  )
{
  DwSpiEnableChip (Dws, 0);
  DwSpiMaskIntr (Dws, 0xff);
  DwReadL (Dws, DW_SPI_ICR);
  DwWriteL (Dws, DW_SPI_SER, 0);
  DwSpiEnableChip (Dws, 1);
}

STATIC
UINT32
DwSpiPrepareCr0 (
  IN SPI_DEVICE *Spi
  )
{
  UINT32 Cr0 = 0;

  //
  // CTRLR0[ 5: 4] Frame Format
  //
  Cr0 |= FieldPrep (CTRLR0_FRF_MASK, CTRLR0_FRF_SPI);

  //
  // SPI mode (SCPOL|SCPH)
  // CTRLR0[ 6] Serial Clock Phase
  // CTRLR0[ 7] Serial Clock Polarity
  //
  if (Spi->Mode & CTRLR0_SPI_CPOL)
    Cr0 |= DW_CTRLR0_SCPOL;
  if (Spi->Mode & CTRLR0_SPI_CPHA)
    Cr0 |= DW_CTRLR0_SCPHA;

  //
  // CTRLR0[11] Shift Register Loop
  //
  if (Spi->Mode & CTRLR0_SPI_LOOP)
    Cr0 |= DW_CTRLR0_SRL;

  return Cr0;
}

STATIC
VOID
AssertCs (
  VOID
  )
{
  volatile UINTN     TopRegs;
  UINT32             Val;

  TopRegs = mDwGpioInsatce->Regs + GPIO_SWPORTA_DR;
  Val = MmioRead32 (TopRegs);
  Val = Val & 0xfd;
  MmioWrite32 (TopRegs, Val);
}

STATIC
VOID
DeAssertCs (
  VOID
  )
{
  volatile UINTN     TopRegs;
  UINT32             Val;

  TopRegs = mDwGpioInsatce->Regs + GPIO_SWPORTA_DR;
  Val = MmioRead32 (TopRegs);
  Val = Val | 0x02;
  MmioWrite32 (TopRegs, Val);
}

/**
  Setup a spi slave using the given parameters.

  @param[in]       This           The pointer to SOPHGO_SPI_PROTOCOL.
  @param[in, out]  Spi            The pointer to SPI_DEVICE.
  @param[in]       SpiBus         The spi bus number used by this spi slave.
  @param[in]       Cs             The chip select lines used by this spi slave.
  @param[in]       Mode           Defines how data is clocked out and in.

  @retval  EFI_SUCCESS            The operation completed successfully.
  @retval  EFI_INVALID_PARAMETER  The SpiBus exceeds the number of spi controllers
  @retval  EFI_OUT_OF_RESOURCES   Cat not allocate memory.

**/
EFI_STATUS
EFIAPI
DwSpiSetup (
  IN OUT SPI_DEVICE          *Spi,
  IN     UINT32              SpiBus,
  IN     UINT8               Cs,
  IN     UINT8               Mode
  )
{
  DW_SPI_CHIP_DATA *Chip;
  UINT32           RxSampleDlyNs;
  DW_SPI           *Dws;

  Dws          = mDwSpiInsatce;
  Spi->SpiBus = SpiBus;
  Spi->Cs     = Cs;
  Spi->Mode   = Mode;
  Chip        = SpiGetCtldata (Spi);

  //
  // Only alloc on first setup
  //
  if (!Chip) {
    Chip =  &mChip;
    SpiSetCtldata (Spi, Chip);

    RxSampleDlyNs = Dws->DefRxSampleDlyNs;

    Chip->RxSampleDly = DIV_ROUND_CLOSEST (RxSampleDlyNs, NSEC_PER_SEC / Dws->MaxFreq);
  }

  //
  // Update CR0 data each time the setup callback is invoked since
  // the device parameters could have been changed, for instance, by
  // the MMC SPI driver or something else.
  //
  Chip->Cr0 = DwSpiPrepareCr0 (Spi);

  return EFI_SUCCESS;
}

/**
  Clean a spi slave.

  @param[in]       This  The pointer to SOPHGO_SPI_PROTOCOL.
  @param[in, out]  Spi   The pointer to SPI_DEVICE.

  @retval  EFI_SUCCESS   The operation completed successfully.

**/
EFI_STATUS
EFIAPI
DwSpiCleanup (
  IN OUT SPI_DEVICE          *Spi
  )
{
  DW_SPI_CHIP_DATA *Chip;

  Chip = SpiGetCtldata (Spi);

  SpiSetCtldata (Spi, NULL);

  return EFI_SUCCESS;
}

STATIC
VOID
SpiHwInit (
  IN OUT  DW_SPI  *Dws
  )
{
  UINT32  Cr0;
  UINT32  Tmp;
  UINT32  Ser;

  DwSpiResetChip (Dws);

  if (!Dws->Version)
    Dws->Version = DwReadL (Dws, DW_SPI_VERSION);

  //
  // Try to detect the number of native chip-selects if the platform
  // driver didn't set it up. There can be up to 16 lines configured.
  //
  if (!Dws->NumCs) {

    DwWriteL (Dws, DW_SPI_SER, 0xffff);
    Ser = DwReadL (Dws, DW_SPI_SER);
    DwWriteL (Dws, DW_SPI_SER, 0);

    Dws->NumCs = Hweight16 (Ser);
  }

  //
  // Try to detect the FIFO depth if not set by interface driver,
  // the depth could be from 2 to 256 from HW spec
  //
  if (!Dws->FifoLen) {
    UINT32 Fifo;

    for (Fifo = 1; Fifo < 256; Fifo++) {
      DwWriteL (Dws, DW_SPI_TXFTLR, Fifo);
      if (Fifo != DwReadL (Dws, DW_SPI_TXFTLR))
        break;
    }
    DwWriteL (Dws, DW_SPI_TXFTLR, 0);

    Dws->FifoLen = (Fifo == 1) ? 0 : Fifo;
  }

  //
  // Detect CTRLR0.DFS field size and offset by testing the lowest bits
  // writability. Note DWC SSI controller also has the extended DFS, but
  // with zero offset.
  //
  Cr0 = DwReadL (Dws, DW_SPI_CTRLR0);
  Tmp = Cr0;
  DwSpiEnableChip (Dws, 0);
  DwWriteL (Dws, DW_SPI_CTRLR0, 0xffffffff);
  Cr0 = DwReadL (Dws, DW_SPI_CTRLR0);
  DwWriteL (Dws, DW_SPI_CTRLR0, Tmp);
  DwSpiEnableChip (Dws, 1);

  if (!(Cr0 & CTRLR0_DFS_MASK)) {
    Dws->DfsOffset = CTRLR0_DFS_32_OFFSET;
  } else {
    Dws->DfsOffset = CTRLR0_DFS_OFFSET;
  }

  DEBUG ((DEBUG_INFO,"[Spi%u base: 0x%lx, MaxFreq: %u, CS num: %u, FIFO depth/width: %u/%u, Version: %c.%c%c%c]\n",
          Dws->BusNum, Dws->Regs, Dws->MaxFreq, Dws->NumCs,
          Dws->FifoLen, (Dws->DfsOffset == CTRLR0_DFS_OFFSET) ? 16 : 32,
          Dws->Version >> 24, Dws->Version >> 16,
          Dws->Version >> 8, Dws->Version));
}

STATIC
VOID
Tpm2PreInitialize (
  VOID
  )
{
  volatile UINTN      TopRegs;
  UINT32              Val;

  TopRegs = mSubCtrl->Regs + mSubCtrl->Offset + TOP_SPI0_CS_OFFSET;
  Val = MmioRead32 (TopRegs);
  Val = Val | 0x14;
  MmioWrite32 (TopRegs, Val);
  Val = MmioRead32 (TopRegs);

  TopRegs = mDwGpioInsatce->Regs + GPIO_SWPORTA_DDR;
  Val = MmioRead32 (TopRegs);
  Val = Val | 0x02;
  MmioWrite32 (TopRegs, Val);
  Val = MmioRead32 (TopRegs);

  TopRegs = mDwGpioInsatce->Regs + GPIO_SWPORTA_DR;
  Val = MmioRead32 (TopRegs);
  Val = Val | 0x02;
  MmioWrite32 (TopRegs, Val);
}

EFI_STATUS
EFIAPI
Tpm2SpiInitialize (
  VOID
  )
{
  EFI_STATUS                      Status;
  UINT32                          SpiBus = SPI_BUS_NUM;
  UINT8                           Cs     = SPI_SLAVE_NUM;
  UINT8                           Mode   = SPI_POLARITY;

  Status = GetTopInstanceByPcd ();
  Status = SetMemory (mSubCtrl->Regs + mSubCtrl->Offset);
  if (EFI_ERROR (Status))
    return Status;

  Status = GetGpioInstanceByPcd ();
  Status = SetMemory (mDwGpioInsatce->Regs);
  if (EFI_ERROR (Status))
    return Status;

  Status = GetSpiInstanceByPcd ();
  Status = SetMemory (mDwSpiInsatce->Regs);
  if (EFI_ERROR (Status))
    return Status;

  Tpm2PreInitialize ();
  SpiHwInit (mDwSpiInsatce);

  Tpm2SpiSlave = AllocateZeroPool (sizeof (SPI_DEVICE));
  if (!Tpm2SpiSlave)
    return EFI_OUT_OF_RESOURCES;

  Status = DwSpiSetup (Tpm2SpiSlave, SpiBus, Cs, Mode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,"Cannot setup spi slave\n"));
    return Status;
  }

  return Status;
}

STATIC
EFI_STATUS
DwSpiCheckStatus (
  IN OUT DW_SPI   *Dws,
  IN     BOOLEAN  IsReadFromRisr
  )
{
  UINT32     IrqStatus;
  EFI_STATUS Status;

  Status = EFI_SUCCESS;

  if (IsReadFromRisr)
    IrqStatus = DwReadL (Dws, DW_SPI_RISR);
  else
    IrqStatus = DwReadL (Dws, DW_SPI_ISR);

  if (IrqStatus & DW_SPI_INT_RXOI) {
    DEBUG ((DEBUG_ERROR, "RX FIFO overflow detected\n"));
    Status = EFI_DEVICE_ERROR;
  }

  if (IrqStatus & DW_SPI_INT_RXUI) {
    DEBUG ((DEBUG_ERROR, "RX FIFO underflow detected\n"));
    Status = EFI_DEVICE_ERROR;
  }

  if (IrqStatus & DW_SPI_INT_TXOI) {
    DEBUG ((DEBUG_ERROR, "TX FIFO overflow detected\n"));
    Status = EFI_DEVICE_ERROR;
  }

  //
  // Generically handle the erroneous situation
  //
  if (EFI_ERROR (Status))
    DwSpiResetChip (Dws);

  return Status;
}

STATIC
UINT32
DwSpiTxMax (
  IN DW_SPI *Dws
  )
{
  UINT32 TxRoom, RxTxGap;

  TxRoom = Dws->FifoLen - DwReadL (Dws, DW_SPI_TXFLR);

  //
  // Another concern is about the tx/rx mismatch, we
  // though to use (Dws->FifoLen - rxflr - txflr) as
  // one maximum value for tx, but it doesn't cover the
  // data which is out of tx/rx fifo and inside the
  // shift registers. So a control from sw point of
  // view is taken.
  //
  RxTxGap = Dws->FifoLen - (Dws->RxLen - Dws->TxLen);

  //
  // Return the max entries we can fill into tx fifo
  //
  return MIN3((UINT32)Dws->TxLen, TxRoom, RxTxGap);
}

STATIC
UINT32
DwSpiRxMax (
  IN DW_SPI *Dws
  )
{
  //
  // Return the max entries we should read out of rx fifo
  //
  return MIN_T(UINT32, Dws->RxLen, DwReadL (Dws, DW_SPI_RXFLR));
}

VOID
DwSpiUpdateConfig (
  IN OUT DW_SPI      *Dws,
  IN     SPI_DEVICE  *Spi,
  IN     DW_SPI_CFG  *Cfg
  )
{
  DW_SPI_CHIP_DATA *Chip;
  UINT32           Cr0;
  UINT32           SpeedHz;
  UINT16           ClkDiv;

  Chip = SpiGetCtldata (Spi);
  Cr0  = Chip->Cr0;

  //
  // CTRLR0[ 4/3: 0] or CTRLR0[ 20: 16] Data Frame Size
  //
  Cr0 |= (Cfg->Dfs - 1) << Dws->DfsOffset;
  Cr0 |= FieldPrep (CTRLR0_TMOD_MASK, Cfg->Tmode);

  DwWriteL (Dws, DW_SPI_CTRLR0, Cr0);

  if (Cfg->Tmode == CTRLR0_TMOD_EPROMREAD ||
      Cfg->Tmode == CTRLR0_TMOD_RO)
    DwWriteL (Dws, DW_SPI_CTRLR1, Cfg->Ndf ? Cfg->Ndf - 1 : 0);

  //
  // Note DW APB SSI clock divider doesn't support odd numbers
  //
  ClkDiv = (DIV_ROUND_UP(Dws->MaxFreq, Cfg->Freq) + 1) & 0xfffe;
  SpeedHz = Dws->MaxFreq / ClkDiv;

  if (Dws->CurrentFreq != SpeedHz) {
    DwSpiSetClk (Dws, ClkDiv);
    Dws->CurrentFreq = SpeedHz;
  }

  //
  // Update RX sample delay if required
  //
  if (Dws->CurRxSampleDly != Chip->RxSampleDly) {
    DwWriteL (Dws, DW_SPI_RX_SAMPLE_DLY, Chip->RxSampleDly);
    Dws->CurRxSampleDly = Chip->RxSampleDly;
  }
}

EFI_STATUS
DwSpiTransferOneConfig (
  IN  SPI_DEVICE          *Spi,
  IN  UINT32              BitsPerWord,
  IN  UINT32              SpeedHz
  )
{
  DW_SPI_CFG  Cfg;
  DW_SPI      *Dws = mDwSpiInsatce;

  if (SpeedHz == 0)
    SpeedHz = Dws->MaxFreq;
  if (BitsPerWord == 0)
    BitsPerWord = 8;

  Cfg.Tmode = CTRLR0_TMOD_TR;
  Cfg.Dfs   = BitsPerWord;
  Cfg.Freq  = SpeedHz;

  DwSpiEnableChip (Dws, 0);

  DwSpiUpdateConfig (Dws, Spi, &Cfg);

  //
  // For poll mode just disable all interrupts
  //
  DwSpiMaskIntr (Dws, 0xff);

  Dws->SetCs (Tpm2SpiSlave, 0);

  DwSpiEnableChip (Dws, 1);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
DwSpiTransferOne (
  DW_SPI     *Dws
  )
{
  UINT32      TxMax;
  UINT32      RxMax;
  UINT32      RxW;
  UINT32      TxW;
  EFI_STATUS  Status;

  do {
    TxMax     = DwSpiTxMax (Dws);
    while (TxMax--) {
      TxW = *(UINT8 *)(Dws->Tx);
      DwWriteL (Dws, DW_SPI_DR, TxW);
      Dws->Tx += Dws->NBytes;
      --Dws->TxLen;
    }
    RxMax     = DwSpiRxMax (Dws);
    while (RxMax--) {
      RxW = DwReadL (Dws, DW_SPI_DR);
      *(UINT8 *)(Dws->Rx) = RxW;
      Dws->Rx += Dws->NBytes;
      --Dws->RxLen;
    }
    Status = DwSpiCheckStatus (Dws, TRUE);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "check status error\n"));
      return Status;
    }
  } while (Dws->RxLen);  

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
DwSpiFlowControl (
  DW_SPI      *Dws
  )
{
  UINT32      Index;
  UINT32      TxMax;
  UINT32      RxMax;
  UINT32      RxW;
  EFI_STATUS  Status;

  for (Index = 0; Index < TRY_TIMES; Index++) {
    Dws->TxLen  = 1;
    Dws->RxLen  = Dws->TxLen;
    do {
      TxMax     = DwSpiTxMax (Dws);
      while (TxMax--) {
        DwWriteL (Dws, DW_SPI_DR, 0);
        --Dws->TxLen;
      }
      RxMax     = DwSpiRxMax (Dws);
      while (RxMax--) {
        RxW = DwReadL (Dws, DW_SPI_DR);
        --Dws->RxLen;
      }
      Status = DwSpiCheckStatus (Dws, TRUE);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "check status error\n"));
        return Status;
      }
    } while (Dws->RxLen);

    if (RxW & 0x01)
      break;
  }
  if (Index == TRY_TIMES)
    return EFI_TIMEOUT;

  return EFI_SUCCESS;
}

EFI_STATUS 
Tpm2WriteNBytes (
  IN UINTN  regs,
  IN UINT8* Data,
  IN UINT16  Size
  )
{
  DW_SPI      *Dws;
  UINT8       TransferLen;
  UINT8       mTxBuffer[68];
  UINT8       mRxBuffer[68];
  EFI_STATUS  Status;
  UINT32      BitsPerWord;
  UINT32      SpeedHz;

  BitsPerWord = 8;
  SpeedHz     = SPI_SPEED_HZ;
  Status      = DwSpiTransferOneConfig (Tpm2SpiSlave, BitsPerWord, SpeedHz);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot config spi controller\n"));
    return Status;
  }

  Dws = mDwSpiInsatce;
  Dws->NBytes = RoundupPowOfTwo (BITS_TO_BYTES (8));

  while (Size) {
    TransferLen  = MIN_T(UINT16, Size, 64);
    mTxBuffer[0] = 0x00 | ((TransferLen - 1) & 0x3f);
    mTxBuffer[1] = 0xD4;
    mTxBuffer[2] = (UINT8)((regs >> 8) & 0xff);
    mTxBuffer[3] = (UINT8)(regs & 0xff);
    CopyMem (&mTxBuffer[4], Data, TransferLen);

    Dws->Tx      = mTxBuffer;
    Dws->TxLen   = 4;
    Dws->Rx      = mRxBuffer;
    Dws->RxLen   = Dws->TxLen;

    AssertCs ();
    Status = DwSpiTransferOne (Dws);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Tpm: command transfer failed\n"));
      return Status;
    }

    if ((mRxBuffer[3] & 0x01) == 0) {
      Status = DwSpiFlowControl (Dws);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Tpm: flow control failed\n"));
        return Status;
      }
    }

    Dws->TxLen  = TransferLen;
    Dws->RxLen  = Dws->TxLen;
    Status = DwSpiTransferOne (Dws);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Tpm: data transfer failed\n"));
      return Status;
    }
    DeAssertCs ();
    MicroSecondDelay (5);
  
    Data += TransferLen;
    Size -= TransferLen;
  }

  AssertCs ();
  return Status;
}

EFI_STATUS 
Tpm2readNBytes (
  IN UINTN   regs,
  IN UINT16  Size,
  OUT UINT8  *OutBuffer
  )
{
  DW_SPI      *Dws;
  UINT8       TransferLen;
  UINT8       mTxBuffer[68];
  UINT8       mRxBuffer[68];
  EFI_STATUS  Status;
  UINT32      BitsPerWord;
  UINT32      SpeedHz;
  UINT16      OriSize;

  BitsPerWord = 8;
  SpeedHz     = SPI_SPEED_HZ;
  Status      = DwSpiTransferOneConfig (Tpm2SpiSlave, BitsPerWord, SpeedHz);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot config spi controller\n"));
    return Status;
  }

  OriSize     = Size;
  Dws         = mDwSpiInsatce;
  Dws->NBytes = RoundupPowOfTwo (BITS_TO_BYTES (8));

  while (Size) {
    TransferLen  = MIN_T(UINT16, Size, 64);
    mTxBuffer[0] = 0x80 | ((TransferLen - 1) & 0x3f);
    mTxBuffer[1] = 0xD4;
    mTxBuffer[2] = (UINT8)((regs >> 8) & 0xff);
    mTxBuffer[3] = (UINT8)(regs & 0xff);

    Dws->Tx      = mTxBuffer;
    Dws->TxLen   = 4;
    Dws->Rx      = mRxBuffer;
    Dws->RxLen   = Dws->TxLen;

    AssertCs ();
    Status = DwSpiTransferOne (Dws);

    if ((mRxBuffer[3] & 0x01) == 0) {
      Status = DwSpiFlowControl (Dws);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Tpm: flow control failed\n"));
        return Status;
      }
    }

    Dws->TxLen  = TransferLen;
    Dws->RxLen  = Dws->TxLen;
    Status = DwSpiTransferOne (Dws);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Tpm: data transfer failed\n"));
      return Status;
    }

    DeAssertCs ();
    MicroSecondDelay (5);

    Size -= TransferLen;
  }
  CopyMem (OutBuffer, &mRxBuffer[4], OriSize);
  AssertCs ();

  return Status;
}

UINT8
Tpm2Read8 (
  IN UINTN  regs
  )
{
  UINT8       ReadByte;
  EFI_STATUS  Status;

  Status = Tpm2readNBytes (regs, sizeof (UINT8), &ReadByte);
  if (EFI_ERROR (Status))
    return 0xff;

  return ReadByte;
}

UINT16
Tpm2Read16 (
  IN UINTN  regs
  )
{
  UINT8       RxBuffer[2];
  UINT16      ReadByte;
  EFI_STATUS  Status;

  Status = Tpm2readNBytes (regs, sizeof (UINT16), RxBuffer);
  if (EFI_ERROR (Status))
    return 0xffff;

  ReadByte = ((RxBuffer[0]) << 8) | RxBuffer[1];

  return ReadByte;
}

UINT32
Tpm2Read32 (
  IN UINTN regs
  )
{
  UINT8       RxBuffer[4];
  UINT32      ReadByte;
  EFI_STATUS  Status;
  
  Status = Tpm2readNBytes (regs, sizeof (UINT32), RxBuffer);
  if (EFI_ERROR (Status))
    return 0xffffffff;

  ReadByte = ((RxBuffer[0]) << 24) | ((RxBuffer[1]) << 16)
              | ((RxBuffer[2]) << 8) | RxBuffer[3];

  return ReadByte;
}

VOID
Tpm2Write8 (
  IN UINTN regs,
  IN UINT8 Data
  )
{
  Tpm2WriteNBytes (regs, &Data, sizeof (UINT8));
}

VOID
Tpm2Write32 (
  IN UINTN  regs,
  IN UINT32 Data
  )
{
  UINT8       TxBuffer[4];

  TxBuffer[0] = (UINT8)((Data >> 24) | 0xff);
  TxBuffer[1] = (UINT8)((Data >> 16) | 0xff);
  TxBuffer[2] = (UINT8)((Data >> 8) | 0xff);
  TxBuffer[4] = (UINT8)(Data | 0xff);

  Tpm2WriteNBytes (regs, TxBuffer, sizeof (UINT8));
}
