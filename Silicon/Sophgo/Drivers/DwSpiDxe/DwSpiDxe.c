/** @file
  This file is used to implement DesignWare SPI communication.

  Copyright (c) 2009, Intel Corporation.
  Copyright (C) 2014 Stefan Roese <sr@denx.de>
  Copyright (C) 2020 Sean Anderson <seanga2@gmail.com>
  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DwSpiDxe.h"

STATIC DW_SPI               *mSpiMasterInstances;
STATIC UINT32               mSpiBusCount;
STATIC SOPHGO_SPI_PROTOCOL  *mSpiProtocol;

STATIC
VOID
DwWriteL (
  IN  DW_SPI  *Dws,
  IN  UINT32  OffSet,
  IN  UINT32  Value
  )
{
  MmioWrite32 ((UINTN)(Dws->Regs + OffSet), Value);
}

STATIC
UINT32
DwReadL (
  IN  DW_SPI  *Dws,
  IN  UINT32  OffSet
  )
{
  return MmioRead32 ((UINTN)(Dws->Regs + OffSet));
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
Clamp (
  IN UINT32  Value,
  IN UINT32  Low,
  IN UINT32  High
  )
{
  return Value < Low ? Low : (Value > High ? High : Value);
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
EFI_STATUS
DevicePropertyReadU32 (
  IN       DW_SPI  *Dws,
  IN CONST CHAR8   *PropertyName,
  OUT      UINT32  *Val
  )
{
  FDT_CLIENT_PROTOCOL  *FdtClient;
  EFI_STATUS           FindNodeStatus, Status;
  INT32                Node;
  CONST VOID           *Prop;
  UINT32               PropSize;
  CHAR8                *SpiCompatibleString;
  UINTN                BaseReg;

  SpiCompatibleString = "snps,dw-apb-ssi";
  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL, (VOID **)&FdtClient);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "No FDT client service found\n"));
    return EFI_NOT_FOUND;
  }

  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, SpiCompatibleString, &Node);
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, SpiCompatibleString, Node, &Node)) {
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "reg", &Prop, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: GetNodeProperty (reg) failed (Status == %r)\n", __func__, Status));
      continue;
    }
    BaseReg = SwapBytes64 (((CONST UINT64 *)Prop)[0]);
    if (Dws->Regs == BaseReg) {
      Status = FdtClient->GetNodeProperty (FdtClient, Node, PropertyName, &Prop, &PropSize);
      if (EFI_ERROR (Status)) {
        return EFI_NOT_FOUND;
      }
      *Val  = SwapBytes32 (((CONST UINT32 *)Prop)[0]);
      return EFI_SUCCESS;
    } else {
      continue;
    }
  }

  return EFI_NOT_FOUND;
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
  IN     SOPHGO_SPI_PROTOCOL *This,
  IN OUT SPI_DEVICE          *Spi,
  IN     UINT32              SpiBus,
  IN     UINT8               Cs,
  IN     UINT8               Mode
  )
{
  DW_SPI_CHIP_DATA *Chip;
  DW_SPI           *Dws;
  UINT32           RxSampleDlyNs;
  EFI_STATUS       Status;

  if (SpiBus >= mSpiBusCount) {
    DEBUG ((DEBUG_ERROR, "SpiBus should be less than %u\n", mSpiBusCount));
    return EFI_INVALID_PARAMETER;
  }

  Dws         = &mSpiMasterInstances[Spi->SpiBus];
  Spi->SpiBus = SpiBus;
  Spi->Cs     = Cs;
  Spi->Mode   = Mode;
  Chip        = SpiGetCtldata (Spi);

  //
  // Only alloc on first setup
  //
  if (!Chip) {
    Chip = AllocateZeroPool (sizeof (DW_SPI_CHIP_DATA));
    if (!Chip)
      return EFI_OUT_OF_RESOURCES;
    SpiSetCtldata (Spi, Chip);

    Status = DevicePropertyReadU32 (Dws, "rx-sample-delay-ns", &RxSampleDlyNs);
    if (EFI_ERROR (Status))
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
  IN     SOPHGO_SPI_PROTOCOL *This,
  IN OUT SPI_DEVICE          *Spi
  )
{
  DW_SPI_CHIP_DATA *Chip;

  Chip = SpiGetCtldata (Spi);

  FreePool (Chip);
  SpiSetCtldata (Spi, NULL);

  return EFI_SUCCESS;
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

  Dws = &mSpiMasterInstances[Spi->SpiBus];
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

STATIC
VOID
DwWriter (
  IN OUT DW_SPI  *Dws
  )
{
  UINT32 Max = DwSpiTxMax (Dws);
  UINT32 TxW = 0;

  while (Max--) {
    if (Dws->Tx) {
      if (Dws->NBytes == 1)
        TxW = *(UINT8 *)(Dws->Tx);
      else if (Dws->NBytes == 2)
        TxW = *(UINT16 *)(Dws->Tx);
      else
        TxW = *(UINT32 *)(Dws->Tx);

      Dws->Tx += Dws->NBytes;
    }
    DwWriteL (Dws, DW_SPI_DR, TxW);
    --Dws->TxLen;
  }
}

STATIC
VOID
DwReader (
  IN OUT DW_SPI  *Dws
  )
{
  UINT32 Max = DwSpiRxMax (Dws);
  UINT32 RxW;

  while (Max--) {
    RxW = DwReadL (Dws, DW_SPI_DR);
    if (Dws->Rx) {
      if (Dws->NBytes == 1)
        *(UINT8 *)(Dws->Rx) = RxW;
      else if (Dws->NBytes == 2)
        *(UINT16 *)(Dws->Rx) = RxW;
      else
        *(UINT32 *)(Dws->Rx) = RxW;

      Dws->Rx += Dws->NBytes;
    }
    --Dws->RxLen;
  }
}

STATIC
EFI_STATUS
DwSpiPollTransfer (
  IN OUT DW_SPI        *Dws,
  IN     SPI_TRANSFER  *Transfer
  )
{
  EFI_STATUS  Status;

  do {
    DwWriter (Dws);

    //
    // The delay code is deleted here to ensure that
    // CS line keeps assert state as much as possible
    // during SPI transmission.
    //

    DwReader (Dws);

    Status = DwSpiCheckStatus (Dws, TRUE);
    if (EFI_ERROR (Status))
      return Status;
  } while (Dws->RxLen);

  return EFI_SUCCESS;
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

/**
  Transfer data in poll-based mode.

  @param[in]   This               The pointer to SOPHGO_SPI_PROTOCOL.
  @param[in]   Spi                The pointer to SPI_DEVICE.
  @param[in]   Transfer           The pointer to SPI_TRANSFER.

  @retval  EFI_SUCCESS            The operation completed successfully.
  @retval  EFI_INVALID_PARAMETER  The parameter in SPI_TRANSFER is invalid.
  @retval  EFI_DEVICE_ERROR       Abnormal interruption occurred during transmission.

**/
EFI_STATUS
EFIAPI
DwSpiTransferOne (
  IN  SOPHGO_SPI_PROTOCOL *This,
  IN  SPI_DEVICE          *Spi,
  IN  SPI_TRANSFER        *Transfer
  )
{
  EFI_STATUS  Status;
  DW_SPI      *Dws;
  DW_SPI_CFG  Cfg;

  if (Spi->SpiBus >= mSpiBusCount) {
    return EFI_INVALID_PARAMETER;
  } else if ((Transfer->TxBuf == NULL) ||
             (Transfer->RxBuf == NULL) ||
             (Transfer->Len   == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Dws = &mSpiMasterInstances[Spi->SpiBus];
  if (Transfer->SpeedHz == 0)
    Transfer->SpeedHz = Dws->MaxFreq;
  if (Transfer->BitsPerWord == 0)
    Transfer->BitsPerWord = 8;

  Cfg.Tmode = CTRLR0_TMOD_TR;
  Cfg.Dfs   = Transfer->BitsPerWord;
  Cfg.Freq  = Transfer->SpeedHz;

  Dws->NBytes = RoundupPowOfTwo (BITS_TO_BYTES (Transfer->BitsPerWord));
  Dws->Tx     = (VOID *)Transfer->TxBuf;
  Dws->TxLen  = Transfer->Len / Dws->NBytes;
  Dws->Rx     = Transfer->RxBuf;
  Dws->RxLen  = Dws->TxLen;

  DwSpiEnableChip (Dws, 0);

  DwSpiUpdateConfig (Dws, Spi, &Cfg);

  Transfer->EffectiveSpeedHz = Dws->CurrentFreq;

  //
  // For poll mode just disable all interrupts
  //
  DwSpiMaskIntr (Dws, 0xff);

  Dws->SetCs(Spi, 0);

  DwSpiEnableChip (Dws, 1);

  Status = DwSpiPollTransfer (Dws, Transfer);

  return Status;
}

STATIC
EFI_STATUS
DwSpiInitMemBuf (
  IN OUT   DW_SPI      *Dws,
  IN CONST SPI_MEM_OP  *Op
  )
{
  UINT32 Index, Loop, Len;
  UINT8  *Out;

  //
  // Calculate the total length of the EEPROM command transfer and
  // either use the pre-allocated buffer or create a temporary one.
  //
  Len = Op->Cmd.NBytes + Op->Addr.NBytes + Op->Dummy.NBytes;
  if (Op->Data.Dir == SPI_MEM_DATA_OUT)
    Len += Op->Data.NBytes;

  Out = AllocateZeroPool (Len);
  if (Out == NULL)
    return EFI_OUT_OF_RESOURCES;

  //
  // Collect the operation code, address and dummy bytes into the single
  // buffer. If it's a transfer with data to be sent, also copy it into the
  // single buffer in order to speed the data transmission up.
  //
  for (Index = 0; Index < Op->Cmd.NBytes; ++Index)
    Out[Index] = DW_SPI_GET_BYTE (Op->Cmd.OpCode, Op->Cmd.NBytes - Index - 1);
  for (Loop = 0; Loop < Op->Addr.NBytes; ++Index, ++Loop)
    Out[Index] = DW_SPI_GET_BYTE (Op->Addr.Val, Op->Addr.NBytes - Loop - 1);
  for (Loop = 0; Loop < Op->Dummy.NBytes; ++Index, ++Loop)
    Out[Index] = 0x0;

  if (Op->Data.Dir == SPI_MEM_DATA_OUT)
    CopyMem (&Out[Index], Op->Data.Buf.Out, Op->Data.NBytes);

  Dws->NBytes = 1;
  Dws->Tx     = Out;
  Dws->TxLen  = Len;
  if (Op->Data.Dir == SPI_MEM_DATA_IN) {
    Dws->Rx    = Op->Data.Buf.In;
    Dws->RxLen = Op->Data.NBytes;
  } else {
    Dws->Rx    = NULL;
    Dws->RxLen = 0;
  }

  return EFI_SUCCESS;
}

STATIC
VOID
DwSpiFreeMemBuf (
  IN OUT  DW_SPI  *Dws
  )
{
  FreePool (Dws->Tx);
}

STATIC
EFI_STATUS
DwSpiWriteThenRead (
  IN OUT DW_SPI      *Dws,
  IN     SPI_DEVICE  *Spi
  )
{
  UINT32 Room, Entries, Sts;
  UINT32 Len;
  UINT8  *Buf;

  //
  // At initial stage we just pre-fill the Tx FIFO in with no rush,
  // since native CS hasn't been enabled yet and the automatic data
  // transmission won't start til we do that.
  //
  Len = MIN (Dws->FifoLen, Dws->TxLen);
  Buf = Dws->Tx;
  while (Len--)
    DwWriteL (Dws, DW_SPI_DR, *Buf++);

  //
  // After setting any bit in the SER register the transmission will
  // start automatically. We have to keep up with that procedure
  // otherwise the CS de-assertion will happen whereupon the memory
  // operation will be pre-terminated.
  //
  Len = Dws->TxLen - ((VOID *)Buf - Dws->Tx);
  DwSpiSetCs (Spi, FALSE);
  while (Len) {
    Entries = DwReadL (Dws, DW_SPI_TXFLR);
    if (!Entries) {
      DEBUG ((DEBUG_ERROR, "CS de-assertion on Tx\n"));
      return EFI_DEVICE_ERROR;
    }
    Room = MIN (Dws->FifoLen - Entries, Len);
    for (; Room; --Room, --Len)
      DwWriteL (Dws, DW_SPI_DR, *Buf++);
  }

  //
  // Data fetching will start automatically if the EEPROM-read mode is
  // activated. We have to keep up with the incoming data pace to
  // prevent the Rx FIFO overflow causing the inbound data loss.
  //
  Len = Dws->RxLen;
  Buf = Dws->Rx;
  while (Len) {
    Entries = DwReadL (Dws, DW_SPI_RXFLR);
    if (!Entries) {
      Sts = DwReadL(Dws, DW_SPI_RISR);
      if (Sts & DW_SPI_INT_RXOI) {
        DEBUG ((DEBUG_ERROR, "FIFO overflow on Rx\n"));
        return EFI_DEVICE_ERROR;
      }
      continue;
    }
    Entries = MIN (Entries, Len);
    for (; Entries; --Entries, --Len)
      *Buf++ = DwReadL (Dws, DW_SPI_DR);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
DwSpiWaitMemOpDone (
  IN  DW_SPI      *Dws
  )
{
  INT32  Retry = DW_SPI_WAIT_RETRIES;
  UINT64 DelayNs;
  UINT64 Ns;
  UINT32 NEnts;

  NEnts = DwReadL (Dws, DW_SPI_TXFLR);
  Ns = NSEC_PER_SEC / Dws->CurrentFreq * NEnts;
  Ns *= Dws->NBytes * BITS_PER_BYTE;

  DelayNs = Ns;

  while ((DwReadL (Dws, DW_SPI_SR) & DW_SPI_SR_BUSY) && Retry--)
    NanoSecondDelay (DelayNs);

  if (Retry < 0) {
    DEBUG ((DEBUG_ERROR, "Mem op hanged up\n"));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
VOID
DwSpiStopMemOp (
  IN  DW_SPI      *Dws,
  IN  SPI_DEVICE  *Spi
  )
{
  DwSpiEnableChip(Dws, 0);
  DwSpiSetCs(Spi, TRUE);
  DwSpiEnableChip(Dws, 1);
}

EFI_STATUS
EFIAPI
DwSpiExecMemOp (
  IN  SOPHGO_SPI_PROTOCOL *This,
  IN  SPI_DEVICE          *Spi,
  IN  SPI_MEM_OP          *Op
  )
{
  EFI_STATUS  Status;
  DW_SPI      *Dws;
  DW_SPI_CFG  Cfg;

  if (Spi->SpiBus >= mSpiBusCount)
    return EFI_INVALID_PARAMETER;

  Dws = &mSpiMasterInstances[Spi->SpiBus];

  //
  // Collect the outbound data into a single buffer to speed the
  // transmission up at least on the initial stage.
  //
  Status = DwSpiInitMemBuf (Dws, Op);
  if (EFI_ERROR (Status))
    return Status;

  //
  // DW SPI EEPROM-read mode is required only for the SPI memory Data-IN
  // operation. Transmit-only mode is suitable for the rest of them.
  //
  Cfg.Dfs  = 8;
  Cfg.Freq = Clamp (Op->SpeedHz, 0U, Dws->MaxMemFreq);
  if (Op->Data.Dir == SPI_MEM_DATA_IN) {
    Cfg.Tmode = CTRLR0_TMOD_EPROMREAD;
    Cfg.Ndf   = Op->Data.NBytes;
  } else {
    Cfg.Tmode = CTRLR0_TMOD_TO;
  }

  DwSpiEnableChip (Dws, 0);

  DwSpiUpdateConfig (Dws, Spi, &Cfg);

  DwSpiMaskIntr (Dws, 0xff);

  DwSpiEnableChip (Dws, 1);

  Status = DwSpiWriteThenRead (Dws, Spi);

  //
  // Wait for the operation being finished and check the controller
  // status only if there hasn't been any run-time error detected. In the
  // former case it's just pointless. In the later one to prevent an
  // additional error message printing since any hw error flag being set
  // would be due to an error detected on the data transfer.
  //
  if (!EFI_ERROR (Status)) {
    Status = DwSpiWaitMemOpDone (Dws);
    if (!EFI_ERROR (Status))
      Status = DwSpiCheckStatus (Dws, TRUE);
  }

  DwSpiStopMemOp (Dws, Spi);

  DwSpiFreeMemBuf (Dws);

  return Status;
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

  DEBUG ((DEBUG_ERROR,"[Spi%u base: 0x%lx, MaxFreq: %u, CS num: %u, FIFO depth/width: %u/%u, Version: %c.%c%c%c]\n",
          Dws->BusNum, Dws->Regs, Dws->MaxFreq, Dws->NumCs,
          Dws->FifoLen, (Dws->DfsOffset == CTRLR0_DFS_OFFSET) ? 16 : 32,
          Dws->Version >> 24, Dws->Version >> 16,
          Dws->Version >> 8, Dws->Version));
}

EFI_STATUS
GetSpiInfoByFdt (
  IN  CONST CHAR8     *CompatibleString
  )
{
  FDT_CLIENT_PROTOCOL  *FdtClient;
  EFI_STATUS           FindNodeStatus, Status;
  INT32                Node;
  UINT32               Index;
  CONST VOID           *Prop;
  UINT32               PropSize;
  DW_SPI               *Dws;

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
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "clock-frequency", &Prop, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: GetNodeProperty (clock-frequency) failed (Status == %r)\n", __func__, Status));
      continue;
    }
    ++Index;
  }

  if (Index == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot get Spi node from DTS (Status == %r)\n", __func__, Status));
    return EFI_NOT_FOUND;
  }

  mSpiBusCount        = Index;
  mSpiMasterInstances = AllocateZeroPool ((mSpiBusCount * sizeof (DW_SPI)));
  if (mSpiMasterInstances == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Dws = mSpiMasterInstances;
  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, CompatibleString, &Node), Index = 0;
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, CompatibleString, Node, &Node)) {
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "reg", &Prop, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: GetNodeProperty (reg) failed (Status == %r)\n", __func__, Status));
      continue;
    } else {
      Dws->Regs = SwapBytes64 (((CONST UINT64 *)Prop)[0]);
    }
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "clock-frequency", &Prop, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: GetNodeProperty (clock-frequency) failed (Status == %r)\n", __func__, Status));
      continue;
    } else {
      Dws->MaxFreq    = SwapBytes32 (((CONST UINT32 *)Prop)[0]);
      Dws->MaxMemFreq = Dws->MaxFreq;
      Dws->SetCs      = DwSpiSetCs;
      Dws->BusNum     = Index;

      //
      // Get default rx sample delay
      //
      Status = FdtClient->GetNodeProperty (FdtClient, Node, "rx-sample-delay-ns", &Prop, &PropSize);
      if (EFI_ERROR (Status)) {
        Dws->DefRxSampleDlyNs = 0;
      } else {
        Dws->DefRxSampleDlyNs = SwapBytes32 (((CONST UINT32 *)Prop)[0]);
      }
    }
    ++Dws;
    ++Index;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
ConfigControllerMemory (
  VOID
  )
{
  EFI_STATUS             Status;
  UINT32                 Index;
  EFI_CPU_ARCH_PROTOCOL  *Cpu;

  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid,
                                NULL, (VOID **)&Cpu );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a:%d] Locate CpuArchProtocol failed: %r\n",
            __func__, __LINE__, Status));
    return Status;
  }

  for (Index = 0; Index < mSpiBusCount; Index++) {
    Status = Cpu->SetMemoryAttributes (
                    Cpu,
                    mSpiMasterInstances[Index].Regs,
                    SIZE_4KB,
                    EFI_MEMORY_UC
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
EFIAPI
DwSpiEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  CHAR8       *CompatibleString;
  UINT32      Index;
  DW_SPI      *Dws;

  CompatibleString = "snps,dw-apb-ssi";
  Status = GetSpiInfoByFdt (CompatibleString);
  if (EFI_ERROR (Status))
    return Status;

  Status = ConfigControllerMemory ();
  if (EFI_ERROR (Status)) {
    goto ErrorConfigMemory;
  }

  for (Index = 0; Index < mSpiBusCount; ++Index) {
    Dws = &mSpiMasterInstances[Index];
    SpiHwInit (Dws);
  }

  mSpiProtocol = AllocateZeroPool (sizeof (SOPHGO_SPI_PROTOCOL));
  if (mSpiProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory for mSpiProtocol\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorConfigMemory;
  }

  mSpiProtocol->SpiSetupDevice   = DwSpiSetup;
  mSpiProtocol->SpiCleanupDevice = DwSpiCleanup;
  mSpiProtocol->SpiTransferOne   = DwSpiTransferOne;
  mSpiProtocol->SpiExecMemOp     = DwSpiExecMemOp;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gSophgoSpiProtocolGuid,
                  mSpiProtocol,
                  NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,"%a: Failed to Install mSpiProtocol: %r\n",
            __func__, Status));
    goto ErrorInstallProtocol;
  }

  return EFI_SUCCESS;

ErrorInstallProtocol:
  FreePool (mSpiProtocol);

ErrorConfigMemory:
  FreePool (mSpiMasterInstances);

  return Status;
}

EFI_STATUS
EFIAPI
DwSpiUnload (
  IN  EFI_HANDLE  ImageHandle
  )
{
  if (mSpiMasterInstances != NULL) {
    FreePool (mSpiMasterInstances);
  }
  if (mSpiProtocol != NULL) {
    FreePool (mSpiProtocol);
  }

  gBS->UninstallProtocolInterface (
         &ImageHandle,
         &gSophgoSpiProtocolGuid,
         NULL);

  return EFI_SUCCESS;
}
