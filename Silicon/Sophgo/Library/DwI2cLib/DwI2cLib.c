/** @file
  This file is used to implement DesignWare I2C communication.

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/FdtClient.h>

#include "DwI2cLib.h"

STATIC I2C_INFO *mI2cInfo;

typedef struct {
  UINT32          SsHcnt;
  UINT32          FsHcnt;
  UINT32          SsLcnt;
  UINT32          FsLcnt;
  UINT32          SdaHold;
} DW_SCL_SDA_CFG;

typedef struct {
  UINT32          I2cClkFrq;
  I2C_REGS        *Regs;
  DW_SCL_SDA_CFG  *SclSdaCfg;
} DW_I2C;

STATIC
VOID
DwI2cEnable (
  IN I2C_REGS  *I2cBase,
  IN BOOLEAN   Enable
  )
{
  UINT32  Ena     = Enable ? IC_ENABLE_0B : 0;
  INT16   TimeOut = 100;

  do {
    MmioWrite32 ((UINTN)(&I2cBase->IC_ENABLE), Ena);
    if ((MmioRead32 ((UINTN)(&I2cBase->IC_ENABLE_STATUS)) & IC_ENABLE_0B) == Ena)
      return;

    //
    // Wait 10 times the signaling period of the highest I2C
    // transfer supported by the driver (for 400KHz this is
    // 25us) as described in the DesignWare I2C databook.
    //
    gBS->Stall (25);
  } while (TimeOut--);

  DEBUG ((DEBUG_ERROR,
    "timeout in %sabling I2C adapter\n", Enable ? "en" : "dis"
  ));
}

STATIC
EFI_STATUS
__DwI2cSetBusSpeed (
  IN DW_I2C  *I2c,
  IN UINT32  Speed
  )
{
  INT32           I2cSpd;
  UINT32          ClkFreq;
  UINT32          Cntl;
  UINT32          Hcnt, Lcnt;
  I2C_REGS        *I2cBase   = I2c->Regs;
  DW_SCL_SDA_CFG  *SclSdaCfg = I2c->SclSdaCfg;

  ClkFreq = I2c->I2cClkFrq ? (I2c->I2cClkFrq / 1000000) : IC_CLK;

  if (Speed >= I2C_MAX_SPEED)
    I2cSpd = IC_SPEED_MODE_MAX;
  else if (Speed >= I2C_FAST_SPEED)
    I2cSpd = IC_SPEED_MODE_FAST;
  else
    I2cSpd = IC_SPEED_MODE_STANDARD;

  //
  // to set speed cltr must be disabled
  //
  DwI2cEnable (I2cBase, FALSE);

  Cntl = (MmioRead32 ((UINTN)(&I2cBase->IC_CON)) & (~IC_CON_SPD_MSK));

  switch (I2cSpd) {
  case IC_SPEED_MODE_MAX:
    Cntl |= IC_CON_SPD_SS | IC_CON_RE;
    if (SclSdaCfg) {
      Hcnt = SclSdaCfg->FsHcnt;
      Lcnt = SclSdaCfg->FsLcnt;
    } else {
      Hcnt = (ClkFreq * MIN_HS_SCL_HIGHTIME) / NANO_TO_MICRO;
      Lcnt = (ClkFreq * MIN_HS_SCL_LOWTIME) / NANO_TO_MICRO;
    }
    MmioWrite32 ((UINTN)(&I2cBase->IC_HS_SCL_HCNT), Hcnt);
    MmioWrite32 ((UINTN)(&I2cBase->IC_HS_SCL_LCNT), Lcnt);
    break;

  case IC_SPEED_MODE_STANDARD:
    Cntl |= IC_CON_SPD_SS;
    if (SclSdaCfg) {
      Hcnt = SclSdaCfg->SsHcnt;
      Lcnt = SclSdaCfg->SsLcnt;
    } else {
      Hcnt = (ClkFreq * MIN_SS_SCL_HIGHTIME) / NANO_TO_MICRO;
      Lcnt = (ClkFreq * MIN_SS_SCL_LOWTIME) / NANO_TO_MICRO;
    }
    MmioWrite32 ((UINTN)(&I2cBase->IC_SS_SCL_HCNT), Hcnt);
    MmioWrite32 ((UINTN)(&I2cBase->IC_SS_SCL_LCNT), Lcnt);
    break;

  case IC_SPEED_MODE_FAST:
  default:
    Cntl |= IC_CON_SPD_FS;
    if (SclSdaCfg) {
      Hcnt = SclSdaCfg->FsHcnt;
      Lcnt = SclSdaCfg->FsLcnt;
    } else {
      Hcnt = (ClkFreq * MIN_FS_SCL_HIGHTIME) / NANO_TO_MICRO;
      Lcnt = (ClkFreq * MIN_FS_SCL_LOWTIME) / NANO_TO_MICRO;
    }
    MmioWrite32 ((UINTN)(&I2cBase->IC_FS_SCL_HCNT), Hcnt);
    MmioWrite32 ((UINTN)(&I2cBase->IC_FS_SCL_LCNT), Lcnt);
    break;
  }

  //
  // always working in master mode and enable restart
  //
  MmioWrite32 ((UINTN)(&I2cBase->IC_CON), Cntl | IC_CON_RE | IC_CON_MM);

  //
  // Configure SDA Hold Time if required
  //
  if (SclSdaCfg)
    MmioWrite32 ((UINTN)(&I2cBase->IC_SDA_HOLD), SclSdaCfg->SdaHold);

  //
  // Enable back i2c now speed set
  //
  DwI2cEnable (I2cBase, TRUE);

  return EFI_SUCCESS;
}

STATIC
VOID
I2cSetAddress (
  IN I2C_REGS  *I2cBase,
  IN UINT32    i2c_addr
  )
{
  DwI2cEnable (I2cBase, FALSE);

  MmioWrite32 ((UINTN)(&I2cBase->IC_TAR), i2c_addr);

  DwI2cEnable (I2cBase, TRUE);
}

STATIC
VOID
I2cFlushRxFifo (
  IN I2C_REGS *I2cBase
  )
{
  while (MmioRead32 ((UINTN)(&I2cBase->IC_STATUS)) & IC_STATUS_RFNE)
    MmioRead32 ((UINTN)(&I2cBase->IC_CMD_DATA));
}

STATIC
EFI_STATUS
I2cWaitForBusBusy (
  IN I2C_REGS  *I2cBase
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   TimeoutEvt;

  TimeoutEvt = NULL;
  Status = gBS->CreateEvent (EVT_TIMER, TPL_NOTIFY,
                             NULL, NULL, &TimeoutEvt);
  if (EFI_ERROR (Status))
    return Status;

  Status = gBS->SetTimer (TimeoutEvt, TimerRelative,
                          EFI_TIMER_PERIOD_MICROSECONDS (I2C_BYTE_TO_BB));
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (TimeoutEvt);
    return Status;
  }

  while ((MmioRead32 ((UINTN)(&I2cBase->IC_STATUS)) & IC_STATUS_MA) ||
         !(MmioRead32 ((UINTN)(&I2cBase->IC_STATUS)) & IC_STATUS_TFE)) {

    if (gBS->CheckEvent (TimeoutEvt) == EFI_SUCCESS) {
      gBS->CloseEvent (TimeoutEvt);
      return EFI_TIMEOUT;
    }
  }
  gBS->CloseEvent (TimeoutEvt);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
I2cXferInit (
  IN I2C_REGS  *I2cBase,
  IN UINT8     Chip,
  IN UINT32    Addr,
  IN INT32     ALen
  )
{
  EFI_STATUS  Status;

  Status = I2cWaitForBusBusy (I2cBase);
  if (EFI_ERROR (Status))
    return Status;

  I2cSetAddress (I2cBase, Chip);
  while (ALen) {
    ALen--;

    //
    // high byte address going out first
    //
    MmioWrite32 ((UINTN)(&I2cBase->IC_CMD_DATA),
                 (Addr >> (ALen * 8)) & 0xff);
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
I2cXferFinish (
  IN I2C_REGS *I2cBase
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   TimeoutEvt;

  TimeoutEvt = NULL;
  Status = gBS->CreateEvent (EVT_TIMER, TPL_NOTIFY, NULL, NULL,
                             &TimeoutEvt);
  if (EFI_ERROR (Status))
    return Status;

  //
  // send stop bit
  //
  MmioWrite32 ((UINTN)(&I2cBase->IC_CMD_DATA), 1 << 9);

  Status = gBS->SetTimer (TimeoutEvt, TimerRelative,
                          EFI_TIMER_PERIOD_MICROSECONDS (I2C_STOPDET_TO));
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (TimeoutEvt);
    return Status;
  }

  while (1) {
    if (MmioRead32 ((UINTN)(&I2cBase->IC_RAW_INTR_STAT)) & IC_STOP_DET) {
      MmioRead32 ((UINTN)(&I2cBase->IC_CLR_STOP_DET));
      break;
    } else if (gBS->CheckEvent (TimeoutEvt) == EFI_SUCCESS) {
      break;
    }
  }
  gBS->CloseEvent (TimeoutEvt);

  Status = I2cWaitForBusBusy (I2cBase);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Timed out waiting for bus\n"));
    return Status;
  }

  I2cFlushRxFifo (I2cBase);

  return EFI_SUCCESS;
}

/**
  DesignWare i2c initialization function.

  @param[in]  I2c        DesignWare i2c struct.
  @param[in]  Speed      Required i2c speed.
  @param[in]  SlaveAddr  Slave address for the device.

**/
STATIC
VOID
__DwI2cInit (
  IN DW_I2C  *I2c,
  IN UINT32  Speed,
  IN UINT32  SlaveAddr
  )
{
  I2C_REGS *I2cBase = I2c->Regs;

  DwI2cEnable (I2cBase, FALSE);

  MmioWrite32 ((UINTN)(&I2cBase->IC_CON), (IC_CON_SD | IC_CON_SPD_FS | IC_CON_MM));
  MmioWrite32 ((UINTN)(&I2cBase->IC_RX_TL), IC_RX_TL_VALUE);
  MmioWrite32 ((UINTN)(&I2cBase->IC_TX_TL), IC_TX_TL_VALUE);
  MmioWrite32 ((UINTN)(&I2cBase->IC_INTR_MASK), IC_STOP_DET);
  __DwI2cSetBusSpeed (I2c, Speed);
  MmioWrite32 ((UINTN)(&I2cBase->IC_SAR), SlaveAddr);

  DwI2cEnable (I2cBase, TRUE);
}

STATIC
EFI_STATUS
DesignwareI2cXfer (
  IN DW_I2C   *I2c,
  IN I2C_MSG  *Msg,
  IN INT32    NMsgs
  )
{
  EFI_STATUS  Err;
  UINT16      CmdBuf[CMD_BUF_MAX];
  UINT8       DataBuf[CMD_BUF_MAX];
  INT32       Index, SubIndex;
  INT32       BufIndex, SubBufIndex;
  INT32       Wait;
  UINT32      I2cStatus;
  I2C_REGS    *Regs;

  if (NMsgs <= 0)
    return EFI_SUCCESS;

  Err = I2cXferInit (I2c->Regs, Msg[0].Addr, 0, 0);
  if (EFI_ERROR (Err))
    return Err;

  Regs = I2c->Regs;

  //
  // stream messages
  //
  BufIndex = 0, SubBufIndex = 0;
  for (Index = 0; Index < NMsgs; ++Index) {
    for (SubIndex = 0; SubIndex < Msg[Index].Len; ++SubIndex) {
      if (Msg[Index].Flags & I2C_M_RD) {
        CmdBuf[BufIndex] = IC_CMD;
        ++SubBufIndex;
      } else {
        CmdBuf[BufIndex] = Msg[Index].Buf[SubIndex];
      }
      ++BufIndex;
      if (BufIndex > CMD_BUF_MAX) {
        DEBUG ((DEBUG_ERROR, "too many commands\n"));
        return EFI_OUT_OF_RESOURCES;
      }
    }
  }

  SubIndex = 0;
  Wait     = 0;
  for (Index = 0; Index < BufIndex || SubIndex < SubBufIndex;) {

    //
    // check tx abort
    //
    if (MmioRead32 ((UINTN)(&Regs->IC_RAW_INTR_STAT)) & IC_TX_ABRT) {
      Err = (MmioRead32 ((UINTN)(&Regs->IC_TX_ABRT_SOURCE)) & IC_ABRT_7B_ADDR_NOACK)
                ? EFI_NOT_FOUND : EFI_DEVICE_ERROR;

      //
      // clear abort source
      //
      MmioRead32 ((UINTN)(&Regs->IC_CLR_TX_ABRT));
      goto Finish;
    }

    I2cStatus = MmioRead32 ((UINTN)(&Regs->IC_STATUS));

    //
    // transmit fifo not full, push one command if we have not sent
    // all commands out
    //
    if ((I2cStatus & IC_STATUS_TFNF) && (Index < BufIndex)) {
      MmioWrite32 ((UINTN)(&Regs->IC_CMD_DATA), CmdBuf[Index]);
      ++Index;
    }

    //
    // receive data if receive fifo not empty and if we have not
    // receive enough data
    //
    if ((I2cStatus & IC_STATUS_RFNE) && (SubIndex < SubBufIndex)) {
      DataBuf[SubIndex] = MmioRead32 ((UINTN)(&Regs->IC_CMD_DATA));
      ++SubIndex;
    }
    Wait++;
    gBS->Stall (10);
    if (Wait > 5000) {
      DEBUG ((DEBUG_ERROR, "i2c xfer timeout %d\n", __LINE__));
      return EFI_TIMEOUT;
    }
  }

  BufIndex = 0;
  for (Index = 0; Index < NMsgs; ++Index) {
    if ((Msg[Index].Flags & I2C_M_RD) == 0)
      continue;

    for (SubIndex = 0; SubIndex < Msg[Index].Len; ++SubIndex) {
      Msg[Index].Buf[SubIndex] = DataBuf[BufIndex];
      ++BufIndex;
      if (BufIndex > SubBufIndex) {
        DEBUG ((DEBUG_ERROR, "software logic error\n"));
        Err = EFI_DEVICE_ERROR;
        goto Finish;
      }
    }
  }

  Err = EFI_SUCCESS;

Finish:
  I2cXferFinish (Regs);

  return Err;
}

STATIC
EFI_STATUS
DesignwareI2cInit (
  IN OUT  DW_I2C  *I2c,
  IN      VOID    *Reg,
  IN      UINTN   Freq,
  IN      UINTN   Speed
  )
{
  I2c->Regs = Reg;
  I2c->I2cClkFrq = Freq;

  __DwI2cInit (I2c, Speed, 0);

  return EFI_SUCCESS;
}

EFI_STATUS
I2cInit (
  IN I2C_INFO  *Info,
  IN INT32     I2cNum
  )
{
  EFI_STATUS  Err;
  INT32       Loop;

  mI2cInfo = Info;

  for (Loop = 0; Loop < I2cNum; ++Loop) {
    Err = DesignwareI2cInit ((VOID *)&mI2cInfo[Loop].Dev,
                             (VOID *)mI2cInfo[Loop].Base,
                             mI2cInfo[Loop].Freq,
                             mI2cInfo[Loop].Speed);
    if (EFI_ERROR (Err))
      return Err;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
I2cXfer (
  IN INT32    I2c,
  IN I2C_MSG  *Msg,
  IN INT32    NMsgs
  )
{
  return DesignwareI2cXfer ((VOID *)&mI2cInfo[I2c].Dev, Msg, NMsgs);
}

/**
  I2c smbus read 1 byte data command.

  @param[in]   I2c   I2c bus number.
  @param[in]   Addr  I2c slave address.
  @param[in]   Cmd   Smbus command.
  @param[out]  Data  Buffer used to store the read data.

  @retval  EFI_SUCCESS              Read data success.
  @retval  EFI_NOT_FOUND            Unable to find i2c slave with the given address.
  @retval  EFI_DEVICE_ERROR         There was an error during the transmission.
  @retval  EFI_TIMEOUT              Waiting for bus busy timedout or transfer timeout.

**/
EFI_STATUS
EFIAPI
I2cSmbusReadByte (
  IN  INT32  I2c,
  IN  UINT8  Addr,
  IN  UINT8  Cmd,
  OUT UINT8  *Data
  )
{
  I2C_MSG  Msg[2];

  SetMem ((VOID *)Msg, sizeof (Msg), 0);

  Msg[0].Addr  = Addr;
  Msg[0].Flags = 0;
  Msg[0].Len   = 1;
  Msg[0].Buf   = &Cmd;

  Msg[1].Addr  = Addr;
  Msg[1].Flags = I2C_M_RD;
  Msg[1].Len   = 1;
  Msg[1].Buf   = Data;

  return I2cXfer (I2c, Msg, 2);
}

/**
  I2c smbus read specified byte length data command.

  @param[in]   I2c   I2c bus number.
  @param[in]   Addr  I2c slave address.
  @param[in]   Len   Byte size of the data to be written.
  @param[in]   Cmd   Smbus command.
  @param[out]  Data  Buffer used to store the read data.

  @retval  EFI_SUCCESS              Read data success.
  @retval  EFI_NOT_FOUND            Unable to find i2c slave with the given address.
  @retval  EFI_DEVICE_ERROR         There was an error during the transmission.
  @retval  EFI_TIMEOUT              Waiting for bus busy timedout or transfer timeout.

**/
EFI_STATUS
EFIAPI
I2cSmbusRead (
  IN  INT32   I2c,
  IN  UINT8   Addr,
  IN  UINT32  Len,
  IN  UINT8   Cmd,
  OUT UINT8   *Data
  )
{
  I2C_MSG  Msg[2];

  SetMem ((VOID *)Msg, sizeof (Msg), 0);

  Msg[0].Addr  = Addr;
  Msg[0].Flags = 0;
  Msg[0].Len   = 1;
  Msg[0].Buf   = &Cmd;

  Msg[1].Addr  = Addr;
  Msg[1].Flags = I2C_M_RD;
  Msg[1].Len   = Len;
  Msg[1].Buf   = Data;

  return I2cXfer (I2c, Msg, 2);
}

/**
  I2c smbus write 1 byte data command.

  @param[in]  I2c   I2c bus number.
  @param[in]  Addr  I2c slave address.
  @param[in]  Cmd   Smbus command.
  @param[in]  Data  Data to be writen.

  @retval  EFI_SUCCESS         Write data success.
  @retval  EFI_NOT_FOUND       Unable to find i2c slave with the given address.
  @retval  EFI_DEVICE_ERROR    There was an error during the transmission.
  @retval  EFI_TIMEOUT         Waiting for bus busy timedout or transfer timeout.

**/
EFI_STATUS
EFIAPI
I2cSmbusWriteByte (
  IN  INT32  I2c,
  IN  UINT8  Addr,
  IN  UINT8  Cmd,
  IN  UINT8  Data
  )
{
  I2C_MSG  Msg;
  UINT8    Buf[2];

  SetMem ((VOID *)(&Msg), sizeof (Msg), 0);

  Buf[0]    = Cmd;
  Buf[1]    = Data;

  Msg.Addr  = Addr;
  Msg.Flags = 0;
  Msg.Len   = 2;
  Msg.Buf   = Buf;

  return I2cXfer (I2c, &Msg, 1);
}

/**
  I2c smbus write multiple bytes to the device,
  maximum 16 bytes in a single operation.

  @param[in]  I2c         I2c bus number.
  @param[in]  Addr        I2c slave address.
  @param[in]  EepromAddr  The address to write data in.
  @param[in]  DataLen     The bytes of data to be writen.
  @param[in]  Data        Data to be writen.

  @retval  EFI_SUCCESS              Write data success.
  @retval  EFI_NOT_FOUND            Unable to find i2c slave with the given address.
  @retval  EFI_DEVICE_ERROR         There was an error during the transmission.
  @retval  EFI_TIMEOUT              Waiting for bus busy timedout or transfer timeout.
  @retval  EFI_INVALID_PARAMETER    Invalid function parameter.

**/
EFI_STATUS
EFIAPI
I2cSmbusWriteBlockData (
  IN        INT32   I2c,
  IN        UINT8   Addr,
  IN        UINT32  EepromAddr,
  IN        UINT32  DataLen,
  IN CONST  UINT8   *Data)
{
  I2C_MSG  Msg;
  UINT8    Buf[18];

  if (DataLen > 16) {
    DEBUG ((DEBUG_ERROR, "%s: write %d bytes exceeds 16!\n", __func__, DataLen));
    return EFI_INVALID_PARAMETER;
  }

  SetMem ((VOID *)(&Msg), sizeof (Msg), 0);
  CopyMem ((VOID *)(Buf + 2), (CONST VOID *)Data, DataLen);
  Buf[0] = (EepromAddr >> 8) & 0xff;
  Buf[1] = EepromAddr & 0xff;

  Msg.Addr  = Addr;
  Msg.Flags = 0;
  Msg.Len   = DataLen + 2;
  Msg.Buf   = Buf;

  return I2cXfer (I2c, &Msg, 1);
}

/**
  I2c smbus write specified byte length data command.

  @param[in]  I2c   I2c bus number.
  @param[in]  Addr  I2c slave address.
  @param[in]  Cmd   Smbus command.
  @param[in]  Len   Byte size of the data to be written.
  @param[in]  Data  Buffer used to store the written data.

  @retval  EFI_SUCCESS              Write data success.
  @retval  EFI_NOT_FOUND            Unable to find i2c slave with the given address.
  @retval  EFI_DEVICE_ERROR         There was an error during the transmission.
  @retval  EFI_TIMEOUT              Waiting for bus busy timedout or transfer timeout.
  @retval  EFI_INVALID_PARAMETER    Invalid function parameter.

**/
EFI_STATUS
EFIAPI
I2cSmbusWrite (
  IN        INT32   I2c,
  IN        UINT8   Addr,
  IN        UINT8   Cmd,
  IN        UINT32  Len,
  IN CONST  UINT8   *Data
  )
{
  I2C_MSG  Msg;
  UINT8    Buf[18];

  if (Len > 17) {
    DEBUG ((DEBUG_ERROR, "%s: write %d bytes exceeds 17!\n", __func__, Len));
    return EFI_INVALID_PARAMETER;
  }

  SetMem ((VOID *)(&Msg), sizeof (Msg), 0);
  CopyMem ((VOID *)(Buf + 1), (CONST VOID *)Data, Len);
  Buf[0] = Cmd;

  Msg.Addr  = Addr;
  Msg.Flags = 0;
  Msg.Len   = Len + 1;
  Msg.Buf   = Buf;

  return I2cXfer (I2c, &Msg, 1);
}

EFI_STATUS
EFIAPI
I2cLibConstructor (
  VOID
  )
{
  EFI_STATUS           Status;
  EFI_STATUS           FindNodeStatus;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  INT32                Node;
  CONST VOID           *Prop;
  UINT32               PropSize;
  UINT32               I2cNum;
  CONST CHAR8          *CompatibleString;
  I2C_INFO             *I2cInformation, *I2cInfoPointer;

  CompatibleString = "snps,designware-i2c";

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL, (VOID **)&FdtClient);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "No FDT client service found\n"));
    DEBUG ((DEBUG_ERROR, "Cannot init PCIe controllers\n"));
    return EFI_NOT_FOUND;
  }

  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, CompatibleString, &Node), I2cNum = 0;
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, CompatibleString, Node, &Node)) {
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "reg", &Prop, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: GetNodeProperty () failed (Status == %r)\n", __func__, Status));
      continue;
    } else {
      ++I2cNum;
    }
  }
  if (I2cNum == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot get I2c node from DTS (Status == %r)\n", __func__, Status));
    return EFI_NOT_FOUND;
  }

  I2cInformation = AllocateZeroPool ((I2cNum * sizeof (I2C_INFO)));
  if (I2cInformation == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  I2cInfoPointer = I2cInformation;
  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient, CompatibleString, &Node);
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient, CompatibleString, Node, &Node)) {
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "reg", &Prop, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: GetNodeProperty () failed (Status == %r)\n", __func__, Status));
      continue;
    }
    I2cInfoPointer->Base  = SwapBytes64 (((CONST UINT64 *)Prop)[0]);
    I2cInfoPointer->Freq  = 100 * 1000 * 1000;
    I2cInfoPointer->Speed = 100 * 1000;
    ++I2cInfoPointer;
  }

  for (int i = 0; i < I2cNum; i++) {
    DEBUG ((DEBUG_ERROR,
      "[I2c%d base: 0x%lx, freq: %lu, speed: %lu ]\n",
      i,
      I2cInformation[i].Base,
      I2cInformation[i].Freq,
      I2cInformation[i].Speed));
  }

  Status = I2cInit (I2cInformation, I2cNum);
  if (EFI_ERROR (Status)) {
    FreePool (I2cInformation);
  }
  return Status;
}

EFI_STATUS
EFIAPI
I2cLibDestructor (
  VOID
  )
{
  if (mI2cInfo != NULL) {
    FreePool (mI2cInfo);
  }

  return EFI_SUCCESS;
}
