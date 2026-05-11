/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2024, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TrngLibUtil.h"

STATIC
UINT32
TrngMmioRead (
  IN SOPHGO_TRNG_DRIVER  *TrngDriver,
  IN UINT32              Offset
  )
{
  ASSERT ((Offset & 3) == 0);

  return MmioRead32 ((UINTN)(TrngDriver->RegBase + Offset));
}

STATIC
VOID
TrngMmioWrite (
  IN SOPHGO_TRNG_DRIVER  *TrngDriver,
  IN UINT32              Offset,
  IN UINT32              Data
  )
{
  ASSERT ((Offset & 3) == 0);

  MmioWrite32 ((UINTN)(TrngDriver->RegBase + Offset), Data);
}

EFI_STATUS
EFIAPI
Elpclp890WaitOnBusy (
  IN SOPHGO_TRNG_DRIVER *TrngDriver
  )
{
  UINT32 Value;
  UINT32 Time;

  Time = CLP890_RETRY_MAX;
  do {
    Value = TrngMmioRead (TrngDriver, CLP890_REG_STAT);
  } while ((Value & CLP890_REG_STAT_BUSY) && --Time);

  if (Time) {
    return EFI_SUCCESS;
  } else {
    return EFI_TIMEOUT;
  }
}

EFI_STATUS
EFIAPI
Elpclp890GetAlarms (
  IN SOPHGO_TRNG_DRIVER *TrngDriver
  )
{
  UINT32     Value;
  EFI_STATUS Status;

  Value = TrngMmioRead (TrngDriver, CLP890_REG_ISTAT);
  if (Value & CLP890_REG_ISTAT_ALARMS) {
    //
    // alarm happened
    //
    Value = TrngMmioRead (TrngDriver, CLP890_REG_ALARM);
    DEBUG ((DEBUG_INFO, "%a: Received alarm: %x\n", __func__, Value));

    //
    // clear istat
    //
    TrngMmioWrite (TrngDriver, CLP890_REG_ISTAT, CLP890_REG_ISTAT_ALARMS);
    TrngMmioWrite (TrngDriver, CLP890_REG_ALARM, 0x1F);
    TrngDriver->Status.AlarmCode = Value & 0x1F;

    if (TrngDriver->Status.AlarmCode != CLP890_REG_ALARM_FAILED_TEST_ID_OK)
      Status = Elpclp890CmdZeroize (TrngDriver);
      Status = EFI_DEVICE_ERROR;
    } else {
      Status = EFI_SUCCESS;
    }

    return Status;
}

EFI_STATUS
EFIAPI
Elpclp890SetNonce (
  IN SOPHGO_TRNG_DRIVER *TrngDriver,
  IN UINT32             Nonce
  )
{
  UINT32 Value;

  //
  // enable NONCE mode
  //
  Value = TrngMmioRead (TrngDriver, CLP890_REG_SMODE);
  Value = CLP890_REG_SMODE_SET_NONCE (Value, Nonce);

  TrngMmioWrite (TrngDriver, CLP890_REG_SMODE, Value);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
Elpclp890WaitOn_ (
  IN SOPHGO_TRNG_DRIVER *TrngDriver,
  IN UINT32             Mask
  )
{
  UINT32 Value;
  UINT32 Time;

  Time = CLP890_RETRY_MAX;
resume:
  do {
    Value = TrngMmioRead (TrngDriver, CLP890_REG_ISTAT);
  } while (!(Value & (Mask | CLP890_REG_ISTAT_ALARMS)) && --Time);

  if (Value & CLP890_REG_ISTAT_ALARMS) {
    return Elpclp890GetAlarms (TrngDriver);
  }

  if (Time && !(Value & Mask)) {
    goto resume;
  }

  if (Time) {
    TrngMmioWrite (TrngDriver, CLP890_REG_ISTAT, CLP890_REG_ISTAT_DONE);

    return EFI_SUCCESS;
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "%a: timeout occurs %lx!\n",
      __func__,
      Value
      ));

    return EFI_TIMEOUT;
  }
}

EFI_STATUS
EFIAPI
Elpclp890WaitOnDone (
  IN SOPHGO_TRNG_DRIVER *TrngDriver
  )
{
  return Elpclp890WaitOn_ (TrngDriver, CLP890_REG_ISTAT_DONE);
}

EFI_STATUS
EFIAPI
Elpclp890WaitOnZeroize (
  IN SOPHGO_TRNG_DRIVER *TrngDriver
  )
{
  return Elpclp890WaitOn_ (TrngDriver, CLP890_REG_ISTAT_ZEROIZE);
}

EFI_STATUS
EFIAPI
Elpclp890CmdZeroize (
  IN SOPHGO_TRNG_DRIVER *TrngDriver
  )
{
  DEBUG ((DEBUG_VERBOSE, "cmd: zeroize!\n"));

  if (Elpclp890GetAlarms (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // wait for core to not be busy
  //
  if (Elpclp890WaitOnBusy (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // issue zeroize command
  //
  TrngMmioWrite (TrngDriver, CLP890_REG_CTRL, CLP890_REG_CTRL_CMD_ZEROIZE);

  //
  // wait for done
  //
  if (Elpclp890WaitOnZeroize (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  TrngDriver->Status.CurrentState = CLP890_STATE_ZEROIZE;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Elpclp890CmdAdvanceState (
  IN SOPHGO_TRNG_DRIVER *TrngDriver
  )
{
  DEBUG ((DEBUG_VERBOSE, "cmd: advance state!\n"));

  //
  // valid state?
  //
  if (TrngDriver->Status.CurrentState != CLP890_STATE_GEN_RANDOM) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid state!\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (Elpclp890GetAlarms (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // wait for core to not be busy
  //
  if (Elpclp890WaitOnBusy (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  TrngMmioWrite (TrngDriver, CLP890_REG_CTRL, CLP890_REG_CTRL_CMD_ADVANCE_STATE);

  if (Elpclp890WaitOnDone (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  TrngDriver->Status.CurrentState = CLP890_STATE_ADVANCE_STATE;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Elpclp890CmdRenewState (
  IN SOPHGO_TRNG_DRIVER *TrngDriver
  )
{
  DEBUG ((DEBUG_VERBOSE, "cmd: renew state!\n"));

  //
  // valid state?
  //
  if (TrngDriver->Status.CurrentState != CLP890_STATE_SEEDING) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid state!\n",
      __func__
      ));

    return EFI_INVALID_PARAMETER;
  }

  if (Elpclp890GetAlarms (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  if (Elpclp890WaitOnBusy (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  TrngMmioWrite (TrngDriver, CLP890_REG_CTRL, CLP890_REG_CTRL_CMD_RENEW_STATE);

  if (Elpclp890WaitOnDone (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  TrngDriver->Status.CurrentState = CLP890_STATE_RENEW_STATE;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Elpclp890CmdSeed (
  IN SOPHGO_TRNG_DRIVER *TrngDriver,
  IN UINT32             *Seed
  )
{
  DEBUG ((DEBUG_VERBOSE, "cmd: seed!\n"));
  UINT32 Index;

  //
  // valid state?
  //
  if (TrngDriver->Status.CurrentState != CLP890_STATE_ZEROIZE &&
	TrngDriver->Status.CurrentState != CLP890_STATE_ADVANCE_STATE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid state!\n",
      __func__
      ));

    return EFI_INVALID_PARAMETER;
  }

  if (Elpclp890GetAlarms (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  if (Elpclp890WaitOnBusy (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  if (Seed == NULL) {
    //
    // noise reseed
    //
    Elpclp890SetNonce (TrngDriver, 0);

    TrngMmioWrite (TrngDriver, CLP890_REG_CTRL, CLP890_REG_CTRL_CMD_GEN_NOISE);

    //
    // wait on done
    //
    if (Elpclp890WaitOnDone (TrngDriver)) {
      return EFI_DEVICE_ERROR;
    }
  } else {
    //
    // nonce reseed
    //
    Elpclp890SetNonce (TrngDriver, 1);

    //
    // write seed
    //
    for (Index = 0; Index < 16; Index++) {
      TrngMmioWrite (TrngDriver, CLP890_REG_SEED0 + Index, Seed[Index]);
    }
  }

  TrngDriver->Status.CurrentState = CLP890_STATE_SEEDING;
  TrngDriver->Status.ReadsLeft = TrngDriver->Status.MaxReads;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Elpclp890CmdCreateState (
  IN SOPHGO_TRNG_DRIVER *TrngDriver,
  IN UINT32             *Ps
  )
{
  UINT32 ZeroPs[12] = { 0 };
  UINT32 Index;
  UINT32 KeyLength;
  UINT32 PsPresent;

  DEBUG ((DEBUG_VERBOSE, "cmd: create state!\n"));

  //
  // valid state?
  //
  if (TrngDriver->Status.CurrentState != CLP890_STATE_SEEDING) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid state!\n",
      __func__
      ));

    return EFI_INVALID_PARAMETER;
  }

  if (Elpclp890GetAlarms (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  if (Elpclp890WaitOnBusy (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // write PS if necessary
  //
  PsPresent = CLP890_REG_FEATURES_EXTRA_PS_PRESENT(
		  TrngMmioRead (TrngDriver, CLP890_REG_FEATURES));
  if (PsPresent) {
    if (!Ps) {
      Ps = &ZeroPs[0];
    }

    KeyLength = TrngDriver->Status.KeyLength ? 12 : 8;

    for (Index = 0; Index < KeyLength; Index++) {
      TrngMmioWrite (TrngDriver, CLP890_REG_NPA_DATA0 + Index, Ps[Index]);
    }
  }

  TrngMmioWrite (TrngDriver, CLP890_REG_CTRL, CLP890_REG_CTRL_CMD_CREATE_STATE);

  //
  // wait on done
  //
  if (Elpclp890WaitOnDone (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  TrngDriver->Status.CurrentState = CLP890_STATE_CREATE_STATE;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Elpclp890CmdGenRandom (
  IN SOPHGO_TRNG_DRIVER *TrngDriver,
  IN UINT32             NumReads,
  IN UINT32             *Rand
  )
{
  UINT32 Index, Loop;

  DEBUG ((DEBUG_VERBOSE, "cmd: generate random!\n"));

  //
  // valid state?
  //
  if (TrngDriver->Status.CurrentState != CLP890_STATE_RENEW_STATE &&
           TrngDriver->Status.CurrentState != CLP890_STATE_CREATE_STATE &&
	   TrngDriver->Status.CurrentState != CLP890_STATE_GEN_RANDOM &&
	   TrngDriver->Status.CurrentState != CLP890_STATE_ADVANCE_STATE &&
	   TrngDriver->Status.CurrentState != CLP890_STATE_REFRESH_ADDIN) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid state!\n",
      __func__
      ));

    return EFI_INVALID_PARAMETER;
  }

  if (Elpclp890GetAlarms (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  if (Elpclp890WaitOnBusy (TrngDriver)) {
    return EFI_DEVICE_ERROR;
  }

  TrngDriver->Status.CurrentState = CLP890_STATE_GEN_RANDOM;

  for (Loop = 0; Loop < NumReads; Loop++) {
    //
    // issue gen_random
    //
    TrngMmioWrite (TrngDriver, CLP890_REG_CTRL, CLP890_REG_CTRL_CMD_GEN_RANDOM);
    if (Elpclp890WaitOnDone (TrngDriver)) {
      return EFI_DEVICE_ERROR;
    }

    for (Index = 0; Index < 4; Index++) {
      Rand[Index] = TrngMmioRead (TrngDriver, CLP890_REG_RAND0 + (Index * 4));
    }

    Rand += 4;
    if (TrngDriver->Status.MaxReads && --(TrngDriver->Status.ReadsLeft) == 0) {
      //
      // reseed the device based on the noise source
      //
      if (Elpclp890CmdAdvanceState (TrngDriver)) {
        return EFI_DEVICE_ERROR;
      }

      if (Elpclp890CmdSeed (TrngDriver, NULL)) {
        return EFI_DEVICE_ERROR;
      }

      if (Elpclp890CmdRenewState (TrngDriver)) {
        return EFI_DEVICE_ERROR;
      }

      //
      // reset us back into GEN_RANDOM state
      //
      TrngDriver->Status.CurrentState = CLP890_STATE_GEN_RANDOM;
      TrngDriver->Status.ReadsLeft = TrngDriver->Status.MaxReads;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Elpclp890Init (
  IN SOPHGO_TRNG_DRIVER *TrngDriver,
  IN UINT32             MaxReads
  )
{
  UINT32 Value;

  Value = 0;
  TrngDriver->Status.MaxReads = MaxReads;

  //
  // zero regs
  //
  TrngMmioWrite (TrngDriver, CLP890_REG_ALARM,
		  TrngMmioRead (TrngDriver, CLP890_REG_ALARM));
  TrngMmioWrite (TrngDriver, CLP890_REG_STAT,
		  TrngMmioRead (TrngDriver, CLP890_REG_STAT));
  TrngMmioWrite (TrngDriver, CLP890_REG_ISTAT,
		  TrngMmioRead (TrngDriver, CLP890_REG_ISTAT));

  //
  // default mode
  //
  Value = CLP890_REG_SMODE_SET_MAX_REJECTS (Value, 10);
  Value = CLP890_REG_SMODE_SET_SECURE_EN (Value, 1);

  TrngMmioWrite (TrngDriver, CLP890_REG_SMODE, Value);
  TrngMmioWrite (TrngDriver, CLP890_REG_MODE, 0);

  //
  // Switch to zeroize mode
  //
  return Elpclp890CmdZeroize (TrngDriver);
}
