/** @file
 *
 *  Motorcomm YT8531 PHY driver.
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/FdtClient.h>

#include <Include/Phy.h>
#include <Include/Mdio.h>
#include <Include/DwGpio.h>
#include "Motorcomm8531PhyDxe.h"

STATIC SOPHGO_MDIO_PROTOCOL *Mdio;
STATIC SOPHGO_GPIO_PROTOCOL *Gpio;

/**
 * struct YT_PHY_CFG_REG_MAP - map a config value to a register value
 * @Cfg: value in device configuration
 * @Reg: value in the register
 */
typedef struct {
  UINT32 Cfg;
  UINT32 Reg;
} YT_PHY_CFG_REG_MAP;

STATIC CONST YT_PHY_CFG_REG_MAP YtPhyRgmiiDelays[] = {
  /* for tx delay / rx delay with YT8521_CCR_RXC_DLY_EN is not set. */
  { 0,    YT8521_RC1R_RGMII_0_000_NS },
  { 150,  YT8521_RC1R_RGMII_0_150_NS },
  { 300,  YT8521_RC1R_RGMII_0_300_NS },
  { 450,  YT8521_RC1R_RGMII_0_450_NS },
  { 600,  YT8521_RC1R_RGMII_0_600_NS },
  { 750,  YT8521_RC1R_RGMII_0_750_NS },
  { 900,  YT8521_RC1R_RGMII_0_900_NS },
  { 1050, YT8521_RC1R_RGMII_1_050_NS },
  { 1200, YT8521_RC1R_RGMII_1_200_NS },
  { 1350, YT8521_RC1R_RGMII_1_350_NS },
  { 1500, YT8521_RC1R_RGMII_1_500_NS },
  { 1650, YT8521_RC1R_RGMII_1_650_NS },
  { 1800, YT8521_RC1R_RGMII_1_800_NS },
  { 1950, YT8521_RC1R_RGMII_1_950_NS },   /* default tx/rx delay */
  { 2100, YT8521_RC1R_RGMII_2_100_NS },
  { 2250, YT8521_RC1R_RGMII_2_250_NS },

  /* only for rx delay with YT8521_CCR_RXC_DLY_EN is set. */
  { 0    + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_0_000_NS },
  { 150  + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_0_150_NS },
  { 300  + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_0_300_NS },
  { 450  + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_0_450_NS },
  { 600  + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_0_600_NS },
  { 750  + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_0_750_NS },
  { 900  + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_0_900_NS },
  { 1050 + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_1_050_NS },
  { 1200 + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_1_200_NS },
  { 1350 + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_1_350_NS },
  { 1500 + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_1_500_NS },
  { 1650 + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_1_650_NS },
  { 1800 + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_1_800_NS },
  { 1950 + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_1_950_NS },
  { 2100 + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_2_100_NS },
  { 2250 + YT8521_CCR_RXC_DLY_1_900_NS,   YT8521_RC1R_RGMII_2_250_NS }
};

STATIC
UINT32
YtPhyGetDelayRegValue (
  IN PHY_DEVICE                *PhyDev,
  IN CONST YT_PHY_CFG_REG_MAP  *Tbl,
  IN UINT32                    TbSize,
  IN UINT16                    *RxcDelayEnable,
  IN UINT32                    DefaultValue,
  IN UINT32                    InternalDelayPs
  )
{
  UINT32 TbSizeHalf;
  UINT32 Index;

  TbSizeHalf = TbSize / 2;

  if (!InternalDelayPs) {
    goto Error_Value;
  }

  //
  // When RxDelayEnable is NULL, it is get the delay for tx, only half of
  // TbSize is valid.
  //
  if (!RxcDelayEnable) {
    TbSize = TbSizeHalf;
  }

  for (Index = 0; Index < TbSize; Index++) {
    if (Tbl[Index].Cfg == InternalDelayPs) {
      if (RxcDelayEnable && Index < TbSizeHalf) {
        *RxcDelayEnable = 0;
      }

      return Tbl[Index].Reg;
    }
  }

  DEBUG ((
    DEBUG_WARN,
    "%a() Unsupported value %d using default (%u)\n",
    __func__,
    InternalDelayPs,
    DefaultValue
    ));

Error_Value:
  //
  // When RxcDelayEnable is not NULL, it is get the delay for rx.
  // The rx default in dts and YtPhyRgmiiClkDelayConfig is 1950 ps,
  // so YT8521_CCR_RXC_DLY_EN should not be set.
  //
  if (RxcDelayEnable) {
    *RxcDelayEnable = 0;
  }

  return DefaultValue;
}

/**
 * YtPhyModifyExtendedRegister() - bits modify a PHY's extended register
 * @PhyDev: a pointer to a &struct PHY_DEVICE
 * @RegNum: register number to write
 * @Mask: bit mask of bits to clear
 * @Set: bit mask of bits to set
 *
 * NOTE: Convenience function which allows a PHY's extended register to be
 * modified as new register value = (old register value & ~mask) | set.
 * The caller must have taken the MDIO bus lock.
 *
 * returns 0 or negative error code
 */
STATIC
EFI_STATUS
YtPhyModifyExtendedRegister (
  IN PHY_DEVICE  *PhyDev,
  IN UINT16      RegNum,
  IN UINT16      Mask,
  IN UINT16      Set
  )
{
  EFI_STATUS Status;
  UINT32     OldValue;
  UINT32     NewValue;

  Status = Mdio->Write (Mdio, PhyDev->PhyAddr, YTPHY_PAGE_SELECT, RegNum);
  if (EFI_ERROR(Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Write PAGE_SELECT failed!\n",
      __func__
      ));
    return Status;
  }

  Status = Mdio->Read (Mdio, PhyDev->PhyAddr, YTPHY_PAGE_DATA, &OldValue);
  if (EFI_ERROR(Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Read PAGE_DATA failed!\n",
      __func__
      ));
    return Status;
  }

  NewValue = (OldValue & ~Mask) | Set;
  if (NewValue == OldValue) {
    return EFI_SUCCESS;
  }

  Status = Mdio->Write (Mdio, PhyDev->PhyAddr, YTPHY_PAGE_DATA, NewValue);
  if (EFI_ERROR(Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Write PAGE_DATA failed!\n",
      __func__
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
 * YtPhyModifyExtendedRegisterWithLock() - bits modify a PHY's extended register
 * @PhyDev: a pointer to a &struct PHY_DEVICE
 * @RegMum: register number to write
 * @Mask: bit mask of bits to clear
 * @Set: bit mask of bits to set
 *
 * NOTE: Convenience function which allows a PHY's extended register to be
 * modified as new register value = (old register value & ~mask) | set.
 *
 * returns 0 or negative error code
 */
STATIC
EFI_STATUS
YtPhyModifyExtendedRegisterWithLock (
  IN PHY_DEVICE  *PhyDev,
  IN UINT16      RegNum,
  IN UINT16      Mask,
  IN UINT16      Set
  )
{
  EFI_TPL    SavedTpl;
  EFI_STATUS Status;

  SavedTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  Status = YtPhyModifyExtendedRegister (PhyDev, RegNum, Mask, Set);

  gBS->RestoreTPL (SavedTpl);

  return Status;
}

STATIC
EFI_STATUS
YtPhyRgmiiClkDelayConfigWithLock (
  IN PHY_DEVICE *PhyDev
  )
{
  EFI_STATUS Status;
  EFI_TPL    SavedTpl;
  UINT32     TbSize;
  UINT16     RxcDelayEnable;
  UINT32     RxReg;
  UINT32     TxReg;
  UINT16     Mask;
  UINT16     Value;

  SavedTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  Value = 0;
  TbSize = ARRAY_SIZE (YtPhyRgmiiDelays);
  RxcDelayEnable = YT8521_CCR_RXC_DLY_EN;

  RxReg = YtPhyGetDelayRegValue (PhyDev, YtPhyRgmiiDelays, TbSize,
                                           &RxcDelayEnable,
                                           YT8521_RC1R_RGMII_1_950_NS,
					   PhyDev->RxInternalDelayPs);
  TxReg = YtPhyGetDelayRegValue (PhyDev, YtPhyRgmiiDelays, TbSize, NULL,
                                           YT8521_RC1R_RGMII_1_950_NS,
					   PhyDev->TxInternalDelayPs);

  switch (PhyDev->Interface) {
  case PHY_INTERFACE_MODE_RGMII:
    RxcDelayEnable = 0;
    break;
  case PHY_INTERFACE_MODE_RGMII_RXID:
    Value |= FIELD_PREP (YT8521_RC1R_RX_DELAY_MASK, RxReg);
    break;
  case PHY_INTERFACE_MODE_RGMII_TXID:
    RxcDelayEnable = 0;
    Value |= FIELD_PREP (YT8521_RC1R_GE_TX_DELAY_MASK, TxReg);
    break;
  case PHY_INTERFACE_MODE_RGMII_ID:
    Value |= FIELD_PREP (YT8521_RC1R_RX_DELAY_MASK, RxReg) |
             FIELD_PREP (YT8521_RC1R_GE_TX_DELAY_MASK, TxReg);
    break;
  default: /* do not support other modes */
    return EFI_UNSUPPORTED;
  }

  Status = YtPhyModifyExtendedRegister (PhyDev, YT8521_CHIP_CONFIG_REG,
                               YT8521_CCR_RXC_DLY_EN, RxcDelayEnable);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Enable RGMII clk 2ns delay failed!\n",
      __func__
      ));
    return Status;
  }

  //
  // Generally, it is not necessary to adjust YT8521_RC1R_FE_TX_DELAY
  //
  Mask = YT8521_RC1R_RX_DELAY_MASK | YT8521_RC1R_GE_TX_DELAY_MASK;

  Status = YtPhyModifyExtendedRegister (PhyDev, YT8521_RGMII_CONFIG1_REG, Mask, Value);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): RGMII rx and tx clk delay train confiuration failed!\n",
      __func__
      ));
    return Status;
  }

  gBS->RestoreTPL (SavedTpl);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
Yt8531SetDriveStrength (
  IN PHY_DEVICE *PhyDev
  )
{
  UINT32     DriveStrengthFieldLow;
  UINT32     DriveStrengthFieldHi;
  UINT32     DriveStrength;
  EFI_STATUS Status;

  //
  // Set rgmii rx clk drive strength.
  //
  DriveStrength = YT8531_RGMII_RX_DS_DEFAULT;

  Status = YtPhyModifyExtendedRegisterWithLock (PhyDev,
		                        YTPHY_PAD_DRIVE_STRENGTH_REG,
                                        YT8531_RGMII_RXC_DS_MASK,
                                        FIELD_PREP (YT8531_RGMII_RXC_DS_MASK, DriveStrength)
					);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Set RGMII rx clk drive strength failed!\n",
      __func__
      ));
    return Status;
  }

  //
  // Set rgmii rx data drive strength.
  //
  DriveStrength = YT8531_RGMII_RX_DS_DEFAULT;

  DriveStrengthFieldHi = FIELD_GET (BIT2, DriveStrength);
  DriveStrengthFieldHi = FIELD_PREP (YT8531_RGMII_RXD_DS_HI_MASK, DriveStrengthFieldHi);

  DriveStrengthFieldLow = FIELD_GET (GENMASK(1, 0), DriveStrength);
  DriveStrengthFieldLow = FIELD_PREP (YT8531_RGMII_RXD_DS_LOW_MASK, DriveStrengthFieldLow);

  Status = YtPhyModifyExtendedRegisterWithLock (PhyDev,
                                         YTPHY_PAD_DRIVE_STRENGTH_REG,
                                         YT8531_RGMII_RXD_DS_LOW_MASK | YT8531_RGMII_RXD_DS_HI_MASK,
                                         DriveStrengthFieldLow | DriveStrengthFieldHi
					 );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a[%d]: Set RGMII rx data drive strength failed!\n",
      __func__,
      __LINE__
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
Yt8531ConfigInit (
  IN PHY_DEVICE *PhyDev
  )
{
  EFI_STATUS Status;

  Status = YtPhyRgmiiClkDelayConfigWithLock (PhyDev);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): RGMII clock delay config failed!\n",
      __func__
      ));
    return Status;
  }

  //
  // Disable auto sleep.
  //
  Status = YtPhyModifyExtendedRegisterWithLock (PhyDev,
                                                YT8521_EXTREG_SLEEP_CONTROL1_REG,
                                                YT8521_ESC1R_SLEEP_SW,
					        0
						);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Disable auto sleep failed!\n",
      __func__
      ));
    return Status;
  }

  //
  // Enable RXC clock when no wire plug
  //
  Status = YtPhyModifyExtendedRegisterWithLock (PhyDev,
                                                YT8521_CLOCK_GATING_REG,
                                                YT8521_CGR_RX_CLK_EN,
						0
						);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Enable RXC clock when no wire plug failed!\n",
      __func__
      ));
    return Status;
  }

  Status = Yt8531SetDriveStrength (PhyDev);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Set drive strength failed!\n",
      __func__
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
Yt8531PhyParseStatus (
  IN CONST SOPHGO_PHY_PROTOCOL *This,
  IN PHY_DEVICE                *PhyDev
  )
{
  UINT32      Value;
  UINT32      Speed;
  UINT32      SpeedMode;
  EFI_STATUS  Status;

  Status = Mdio->Read (Mdio, PhyDev->PhyAddr, YTPHY_SPECIFIC_STATUS_REG, &Value);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Read SPECIFIC_STATUS_REG(0x11) failed!\n",
      __func__
      ));
     return Status;
  }

  SpeedMode = (Value & YTPHY_SSR_SPEED_MODE_MASK) >> YTPHY_SSR_SPEED_MODE_OFFSET;

  switch (SpeedMode) {
  case 2:
    Speed = SPEED_1000;
    break;
  case 1:
    Speed = SPEED_100;
    break;
  default:
    Speed = SPEED_10;
    break;
  }

  PhyDev->Speed = Speed;
  PhyDev->Duplex = (Value & YTPHY_SSR_DUPLEX) >> YTPHY_SSR_DUPLEX_OFFSET;
  PhyDev->LinkUp = !!(Value & YTPHY_SSR_LINK);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a(): Speed=%d Mbps, %s-duplex, Link %s!\n",
    __func__,
    PhyDev->Speed,
    PhyDev->Duplex == DUPLEX_HALF ? L"Half" : L"Full",
    PhyDev->LinkUp == 0 ? L"Down" : L"Up"
    ));

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PhyResetGpio (
  IN UINT32 Bus,
  IN UINT32 Pin
  )
{
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (
               &gSophgoGpioProtocolGuid,
               NULL,
               (VOID **) &Gpio
               );
  if (EFI_ERROR(Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Locate SOPHGO_GPIO_PROTOCOL failed (Status=%r)\n",
      __func__,
      Status
      ));
    return Status;
  }

  Gpio->ModeConfig (Gpio, Bus, Pin, GpioConfigOutLow);

  //
  // wait 100ms
  //
  gBS->Stall (100000);

  Gpio->ModeConfig (Gpio, Bus, Pin, GpioConfigOutHigh);

  //
  // wait 100ms
  //
  gBS->Stall (100000);

  return EFI_SUCCESS;
}

/*
 * Probe and init a PHY device.
 */
STATIC
EFI_STATUS
Yt8531PhyInitialize (
  IN CONST SOPHGO_PHY_PROTOCOL *This,
  IN PHY_INTERFACE             PhyInterface,
  IN PHY_DEVICE                **OutPhyDev
  )
{
  UINT16                      Mask;
  UINT16                      Value;
  EFI_STATUS                  Status;
  PHY_DEVICE                  *PhyDev;

  Status = gBS->LocateProtocol (
               &gSophgoMdioProtocolGuid,
               NULL,
               (VOID **) &Mdio
               );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Locate SOPHGO_MDIO_PROTOCOL failed (Status=%r)\n",
      __func__,
      Status
      ));
    return Status;
  }

  PhyDev = AllocateZeroPool (sizeof (PHY_DEVICE));
  PhyDev->Interface = PhyInterface;
  PhyDev->PhyAddr = 0;
  *OutPhyDev = PhyDev;
  DEBUG ((
    DEBUG_INFO,
    "%a(): PhyAddr is %d, interface %d\n",
    __func__,
    PhyDev->PhyAddr,
    PhyInterface
    ));

  if (PcdGetBool (PcdPhyResetGpio)) {
    Status = PhyResetGpio (0, PcdGet8(PcdPhyResetGpioPin));
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Enable sync e clock output.
  //
  Mask = YT8531_SCR_SYNCE_ENABLE;
  Value = 0;

  Status = YtPhyModifyExtendedRegisterWithLock (PhyDev,
		                                YTPHY_SYNCE_CFG_REG,
						Mask,
					        Value
						);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Enable sync e clock output failed!\n",
      __func__
      ));
    return Status;
  }

  //
  // Initialize the PHY
  //
  Status = Yt8531ConfigInit (PhyDev);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Initialize the PHY failed!\n",
      __func__
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Yt8531PhyDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SOPHGO_PHY_PROTOCOL *Phy;
  EFI_STATUS          Status;
  EFI_HANDLE          Handle;

  Handle = NULL;

  Phy = AllocateZeroPool (sizeof (SOPHGO_PHY_PROTOCOL));
  if (Phy == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Phy->Status = Yt8531PhyParseStatus;
  Phy->Init   = Yt8531PhyInitialize;

  Status = gBS->InstallMultipleProtocolInterfaces (
		  &Handle,
		  &gSophgoPhyProtocolGuid,
		  Phy,
		  NULL
		  );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Failed to install interfaces.\n",
      __func__
      ));

    FreePool (Phy);

    return Status;
  }

  return EFI_SUCCESS;
}
