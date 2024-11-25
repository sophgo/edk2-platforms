/** @file
  Implement EFI RealTimeClock via RTC Lib for DS1307 RTC.

  Based on RTC implementation available in
  EmbeddedPkg/Library/TemplateRealTimeClockLib/RealTimeClockLib.c

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright 2017, 2020 NXP
  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/RealTimeClockLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Include/DwI2c.h>

#define I2C_BUS_NUM         2
#define I2C_SLAVE_RTC_ADDR  0X68

#define DS1307_SEC_BIT_CH   0x80  /* Clock Halt (in Register 0) */

//
// TIME MASKS
//
#define MASK_SEC            0x7F
#define MASK_MIN            0x7F
#define MASK_24_HOUR        0x3F
#define MASK_12_HOUR        0x1F
#define MASK_WEEK           0x07
#define MASK_DATE           0x3F
#define MASK_MONTH          0x1F
#define MASK_YEAR           0xFF

#define HOUR_MODE_BIT       (1 << 6)
#define PM_AM_BIT           (1 << 5)

#define START_YEAR          1970
#define END_YEAR            2070

//
// Make sure x is inside the range of [a..b]
//
#ifndef RANGE
#define RANGE(x, a, b) (MIN ((b), MAX ((x), (a))))
#endif

STATIC SOPHGO_I2C_MASTER_PROTOCOL  *mI2cMasterProtocol;

/**
  Read data from RTC.

  @param  Length       The length of data to be read.
  @param  Data         The data to be read

**/
STATIC
EFI_STATUS
RtcRead (
  IN   UINT32  Length,
  OUT  UINT8   *Data
  )
{
  EFI_STATUS   Status;

  Status = mI2cMasterProtocol->Read (mI2cMasterProtocol,
                                     I2C_BUS_NUM,
                                     I2C_SLAVE_RTC_ADDR,
                                     0, Length, Data);

  return Status;
}

/**
  Write data to RTC.

  @param  Length       The length of data to be writen.
  @param  Data         The data to be written

**/
STATIC
EFI_STATUS
RtcWrite (
  IN   UINT32  Length,
  OUT  UINT8   *Data
  )
{
  EFI_STATUS   Status;

  Status = mI2cMasterProtocol->Write (mI2cMasterProtocol,
                                      I2C_BUS_NUM,
                                      I2C_SLAVE_RTC_ADDR,
                                      0, Length, Data);

  return Status;
}

/**
  Returns the current time and date information, and the time-keeping capabilities
  of the hardware platform.

  @param  Time                  A pointer to storage to receive a snapshot of the current time.
  @param  Capabilities          An optional pointer to a buffer to receive the real time clock
                                device's capabilities.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER Time is NULL.
  @retval EFI_DEVICE_ERROR      The time could not be retrieved due to hardware error.

**/
EFI_STATUS
EFIAPI
LibGetTime (
  OUT  EFI_TIME               *Time,
  OUT  EFI_TIME_CAPABILITIES  *Capabilities
  )
{
  EFI_STATUS  Status;
  UINT8       TimeBcd[7] = {0};

  Status = RtcRead (7, TimeBcd);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: I2c smbus read error, Status: %r.\n", __func__, Status));
    return EFI_DEVICE_ERROR;
  } else if (TimeBcd[0] & DS1307_SEC_BIT_CH) {
    DEBUG ((DEBUG_ERROR, "%a: Warning, RTC oscillator has stopped\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  Time->Second = BcdToDecimal8 (TimeBcd[0] & MASK_SEC);
  Time->Minute = BcdToDecimal8 (TimeBcd[1] & MASK_MIN);

  if (TimeBcd[2] & HOUR_MODE_BIT) {
    /* 12-hour mode */
    Time->Hour = BcdToDecimal8 (TimeBcd[2] & MASK_12_HOUR);
    if (TimeBcd[2] & PM_AM_BIT)
      Time->Hour += 12;
  } else {
    /* 24-hour mode */
    Time->Hour = BcdToDecimal8 (TimeBcd[2] & MASK_24_HOUR);
  }

  /* Pad1 is used to indicate the day of the week */
  Time->Pad1   = BcdToDecimal8 (TimeBcd[3] & MASK_WEEK);
  Time->Day    = BcdToDecimal8 (TimeBcd[4] & MASK_DATE);
  Time->Month  = BcdToDecimal8 (TimeBcd[5] & MASK_MONTH);

  //
  // RTC can save year 1970 to 2069
  // On writing Year, save year % 100
  // On Reading reversing the operation e.g. 2012
  // write = 12 (2012 % 100)
  // read = 2012 (12 + 2000)
  //
  Time->Year  = BcdToDecimal8 (TimeBcd[6] & MASK_YEAR);
  Time->Year  = Time->Year +
                (Time->Year >= 70 ? START_YEAR - 70 : END_YEAR -70);

  return EFI_SUCCESS;
}

/**
  Sets the current local time and date information.

  @param  Time                  A pointer to the current time.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The time could not be set due to hardware error.

**/
EFI_STATUS
EFIAPI
LibSetTime (
  IN EFI_TIME  *Time
  )
{
  EFI_STATUS  Status;
  UINT8       Second = 0;
  UINT8       TimeBcd[7] = {0};

  Status = RtcRead (1, &Second);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: I2c smbus read error, Status: %r.\n", __func__, Status));
    return EFI_DEVICE_ERROR;
  } else if (Second & DS1307_SEC_BIT_CH) {
    DEBUG ((DEBUG_ERROR, "%a: Warning, RTC oscillator has stopped\n", __func__));
    return EFI_DEVICE_ERROR;
  } else if (Time->Year < START_YEAR || Time->Year >= END_YEAR) {
    DEBUG ((DEBUG_ERROR, "%a: WARNING, Year should be between %d and %d!\n",
            __func__, START_YEAR, (END_YEAR - 1)));
    return EFI_INVALID_PARAMETER;
  }

  TimeBcd[0] = DecimalToBcd8 (RANGE (Time->Second, 0, 59));
  TimeBcd[1] = DecimalToBcd8 (RANGE (Time->Minute, 0, 59));
  TimeBcd[2] = DecimalToBcd8 (RANGE (Time->Hour, 0, 23));
  TimeBcd[3] = DecimalToBcd8 (RANGE (Time->Pad1, 1, 7));
  TimeBcd[4] = DecimalToBcd8 (RANGE (Time->Day, 1, 31));
  TimeBcd[5] = DecimalToBcd8 (RANGE (Time->Month, 1, 12));
  TimeBcd[6] = DecimalToBcd8 (Time->Year % 100);

  Status = RtcWrite (7, TimeBcd);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: I2c smbus write error, Status: %r.\n", __func__, Status));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Returns the current wakeup alarm clock setting.

  @param  Enabled               Indicates if the alarm is currently enabled or disabled.
  @param  Pending               Indicates if the alarm signal is pending and requires acknowledgement.
  @param  Time                  The current alarm setting.

  @retval EFI_SUCCESS           The alarm settings were returned.
  @retval EFI_INVALID_PARAMETER Any parameter is NULL.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be retrieved due to a hardware error.

**/
EFI_STATUS
EFIAPI
LibGetWakeupTime (
  OUT BOOLEAN   *Enabled,
  OUT BOOLEAN   *Pending,
  OUT EFI_TIME  *Time
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Sets the system wakeup alarm clock time.

  @param  Enabled               Enable or disable the wakeup alarm.
  @param  Time                  If Enable is TRUE, the time to set the wakeup alarm for.

  @retval EFI_SUCCESS           If Enable is TRUE, then the wakeup alarm was enabled. If
                                Enable is FALSE, then the wakeup alarm was disabled.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be set due to a hardware error.
  @retval EFI_UNSUPPORTED       A wakeup timer is not supported on this platform.

**/
EFI_STATUS
EFIAPI
LibSetWakeupTime (
  IN BOOLEAN    Enabled,
  OUT EFI_TIME  *Time
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This is the declaration of an EFI image entry point. This can be the entry point to an application
  written to this specification, an EFI boot service driver, or an EFI runtime driver.

  @param  ImageHandle           Handle that identifies the loaded image.
  @param  SystemTable           System Table for this image.

  @retval EFI_SUCCESS           The operation completed successfully.

**/
EFI_STATUS
EFIAPI
LibRtcInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (
                  &gSophgoI2cMasterProtocolGuid,
                  NULL,
                  (VOID **)&mI2cMasterProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot locate I2c Master protocol\n",
            __func__));
    return Status;
  }

  return EFI_SUCCESS;
}
