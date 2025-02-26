/** @file

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MOTORCOMM_PHY_DXE_H__
#define __MOTORCOMM_PHY_DXE_H__
#include <Include/Phy.h>
#include <Uefi.h>

#define GENMASK(end, start)  (((1ULL << ((end) - (start) + 1)) - 1) << (start))
#define __ffs(x) (__builtin_ffs(x) - 1)

/**
 * FIELD_PREP() - prepare a bitfield element
 * @_mask: shifted mask defining the field's length and position
 * @_val:  value to put in the field
 *
 * FIELD_PREP() masks and shifts up the value.  The result should
 * be combined with other fields of the bitfield using logical OR.
 */
#define FIELD_PREP(_mask, _val)   (((_val) << __ffs(_mask)) & (_mask))

/**
 * FIELD_GET() - extract a bitfield element
 * @_mask: shifted mask defining the field's length and position
 * @_val:  value of entire bitfield
 *
 * FIELD_GET() extracts the field specified by @_mask from the
 * bitfield passed in as @_val by masking and shifting it down.
 */
#define FIELD_GET(_mask, _val)    (((_val) & (_mask)) >> __ffs(_mask))

#define PHY_ID_YT8511           0x0000010a
#define PHY_ID_YT8521           0x0000011a
#define PHY_ID_YT8531           0x4f51e91b
#define PHY_ID_YT8531S          0x4f51e91a

/* YT8521/YT8531S Register Overview
 *      UTP Register space      |       FIBER Register space
 *  ------------------------------------------------------------
 * |    UTP MII                 |       FIBER MII               |
 * |    UTP MMD                 |                               |
 * |    UTP Extended            |       FIBER Extended          |
 *  ------------------------------------------------------------
 * |                    Common Extended                         |
 *  ------------------------------------------------------------
 */

/* 0x10 ~ 0x15 , 0x1E and 0x1F are common MII registers of yt phy */

/* Specific Function Control Register */
#define YTPHY_SPECIFIC_FUNCTION_CONTROL_REG     0x10

/* 2b00 Manual MDI configuration
 * 2b01 Manual MDIX configuration
 * 2b10 Reserved
 * 2b11 Enable automatic crossover for all modes  *default*
 */
#define YTPHY_SFCR_MDI_CROSSOVER_MODE_MASK      (BIT6 | BIT5)
#define YTPHY_SFCR_CROSSOVER_EN                 BIT3
#define YTPHY_SFCR_SQE_TEST_EN                  BIT2
#define YTPHY_SFCR_POLARITY_REVERSAL_EN         BIT1
#define YTPHY_SFCR_JABBER_DIS                   BIT0

/* Specific Status Register */
#define YTPHY_SPECIFIC_STATUS_REG               0x11
#define YTPHY_SSR_SPEED_MODE_OFFSET             14

#define YTPHY_SSR_SPEED_MODE_MASK               (BIT15 | BIT14)
#define YTPHY_SSR_SPEED_10M                     0x0
#define YTPHY_SSR_SPEED_100M                    0x1
#define YTPHY_SSR_SPEED_1000M                   0x2
#define YTPHY_SSR_DUPLEX_OFFSET                 13
#define YTPHY_SSR_DUPLEX                        BIT13
#define YTPHY_SSR_PAGE_RECEIVED                 BIT12
#define YTPHY_SSR_SPEED_DUPLEX_RESOLVED         BIT11
#define YTPHY_SSR_LINK                          BIT10
#define YTPHY_SSR_MDIX_CROSSOVER                BIT6
#define YTPHY_SSR_DOWNGRADE                     BIT5
#define YTPHY_SSR_TRANSMIT_PAUSE                BIT3
#define YTPHY_SSR_RECEIVE_PAUSE                 BIT2
#define YTPHY_SSR_POLARITY                      BIT1
#define YTPHY_SSR_JABBER                        BIT0

/* Interrupt enable Register */
#define YTPHY_INTERRUPT_ENABLE_REG              0x12
#define YTPHY_IER_WOL                           BIT6

/* Interrupt Status Register */
#define YTPHY_INTERRUPT_STATUS_REG              0x13
#define YTPHY_ISR_AUTONEG_ERR                   BIT15
#define YTPHY_ISR_SPEED_CHANGED                 BIT14
#define YTPHY_ISR_DUPLEX_CHANGED                BIT13
#define YTPHY_ISR_PAGE_RECEIVED                 BIT12
#define YTPHY_ISR_LINK_FAILED                   BIT11
#define YTPHY_ISR_LINK_SUCCESSED                BIT10
#define YTPHY_ISR_WOL                           BIT6
#define YTPHY_ISR_WIRESPEED_DOWNGRADE           BIT5
#define YTPHY_ISR_SERDES_LINK_FAILED            BIT3
#define YTPHY_ISR_SERDES_LINK_SUCCESSED         BIT2
#define YTPHY_ISR_POLARITY_CHANGED              BIT1
#define YTPHY_ISR_JABBER_HAPPENED               BIT0

/* Speed Auto Downgrade Control Register */
#define YTPHY_SPEED_AUTO_DOWNGRADE_CONTROL_REG  0x14
#define YTPHY_SADCR_SPEED_DOWNGRADE_EN          BIT5

/* If these bits are set to 3, the PHY attempts five times ( 3(set value) +
 * additional 2) before downgrading, default 0x3
 */
#define YTPHY_SADCR_SPEED_RETRY_LIMIT           (0x3 << 2)

/* Rx Error Counter Register */
#define YTPHY_RX_ERROR_COUNTER_REG              0x15

/* Extended Register's Address Offset Register */
#define YTPHY_PAGE_SELECT                       0x1E

/* Extended Register's Data Register */
#define YTPHY_PAGE_DATA                         0x1F

/* FIBER Auto-Negotiation link partner ability */
#define YTPHY_FLPA_PAUSE                        (0x3 << 7)
#define YTPHY_FLPA_ASYM_PAUSE                   (0x2 << 7)

#define YT8511_PAGE_SELECT      0x1e
#define YT8511_PAGE             0x1f
#define YT8511_EXT_CLK_GATE     0x0c
#define YT8511_EXT_DELAY_DRIVE  0x0d
#define YT8511_EXT_SLEEP_CTRL   0x27

/* 2b00 25m from pll
 * 2b01 25m from xtl *default*
 * 2b10 62.m from pll
 * 2b11 125m from pll
 */
#define YT8511_CLK_125M         (BIT2 | BIT1)
#define YT8511_PLLON_SLP        BIT14

/* RX Delay enabled = 1.8ns 1000T, 8ns 10/100T */
#define YT8511_DELAY_RX         BIT0

/* TX Gig-E Delay is bits 7:4, default 0x5
 * TX Fast-E Delay is bits 15:12, default 0xf
 * Delay = 150ps * N - 250ps
 * On = 2000ps, off = 50ps
 */
#define YT8511_DELAY_GE_TX_EN   (0xf << 4)
#define YT8511_DELAY_GE_TX_DIS  (0x2 << 4)
#define YT8511_DELAY_FE_TX_EN   (0xf << 12)
#define YT8511_DELAY_FE_TX_DIS  (0x2 << 12)

/* Extended register is different from MMD Register and MII Register.
 * We can use ytphy_read_ext/ytphy_write_ext/ytphy_modify_ext function to
 * operate extended register.
 * Extended Register  start
 */

/* Phy gmii clock gating Register */
#define YT8521_CLOCK_GATING_REG                 0xC
#define YT8521_CGR_RX_CLK_EN                    BIT12

#define YT8521_EXTREG_SLEEP_CONTROL1_REG        0x27
#define YT8521_ESC1R_SLEEP_SW                   BIT15
#define YT8521_ESC1R_PLLON_SLP                  BIT14

/* Phy fiber Link timer cfg2 Register */
#define YT8521_LINK_TIMER_CFG2_REG              0xA5
#define YT8521_LTCR_EN_AUTOSEN                  BIT15

/* 0xA000, 0xA001, 0xA003, 0xA006 ~ 0xA00A and 0xA012 are common ext registers
 * of yt8521 phy. There is no need to switch reg space when operating these
 * registers.
 */

#define YT8521_REG_SPACE_SELECT_REG             0xA000
#define YT8521_RSSR_SPACE_MASK                  BIT1
#define YT8521_RSSR_FIBER_SPACE                 (0x1 << 1)
#define YT8521_RSSR_UTP_SPACE                   (0x0 << 1)
#define YT8521_RSSR_TO_BE_ARBITRATED            (0xFF)

#define YT8521_CHIP_CONFIG_REG                  0xA001
#define YT8521_CCR_SW_RST                       BIT15
#define YT8531_RGMII_LDO_VOL_MASK               GENMASK(5, 4)
#define YT8531_LDO_VOL_3V3                      0x0
#define YT8531_LDO_VOL_1V8                      0x2

/* 1b0 disable 1.9ns rxc clock delay  *default*
 * 1b1 enable 1.9ns rxc clock delay
 */
#define YT8521_CCR_RXC_DLY_EN                   BIT8
#define YT8521_CCR_RXC_DLY_1_900_NS             1900

#define YT8521_CCR_MODE_SEL_MASK                (BIT2 | BIT1 | BIT0)
#define YT8521_CCR_MODE_UTP_TO_RGMII            0
#define YT8521_CCR_MODE_FIBER_TO_RGMII          1
#define YT8521_CCR_MODE_UTP_FIBER_TO_RGMII      2
#define YT8521_CCR_MODE_UTP_TO_SGMII            3
#define YT8521_CCR_MODE_SGPHY_TO_RGMAC          4
#define YT8521_CCR_MODE_SGMAC_TO_RGPHY          5
#define YT8521_CCR_MODE_UTP_TO_FIBER_AUTO       6
#define YT8521_CCR_MODE_UTP_TO_FIBER_FORCE      7

/* 3 phy polling modes,poll mode combines utp and fiber mode*/
#define YT8521_MODE_FIBER                       0x1
#define YT8521_MODE_UTP                         0x2
#define YT8521_MODE_POLL                        0x3

#define YT8521_RGMII_CONFIG1_REG                0xA003
/* 1b0 use original tx_clk_rgmii  *default*
 * 1b1 use inverted tx_clk_rgmii.
 */
#define YT8521_RC1R_TX_CLK_SEL_INVERTED         BIT14
#define YT8521_RC1R_RX_DELAY_MASK               GENMASK(13, 10)
#define YT8521_RC1R_FE_TX_DELAY_MASK            GENMASK(7, 4)
#define YT8521_RC1R_GE_TX_DELAY_MASK            GENMASK(3, 0)
#define YT8521_RC1R_RGMII_0_000_NS              0
#define YT8521_RC1R_RGMII_0_150_NS              1
#define YT8521_RC1R_RGMII_0_300_NS              2
#define YT8521_RC1R_RGMII_0_450_NS              3
#define YT8521_RC1R_RGMII_0_600_NS              4
#define YT8521_RC1R_RGMII_0_750_NS              5
#define YT8521_RC1R_RGMII_0_900_NS              6
#define YT8521_RC1R_RGMII_1_050_NS              7
#define YT8521_RC1R_RGMII_1_200_NS              8
#define YT8521_RC1R_RGMII_1_350_NS              9
#define YT8521_RC1R_RGMII_1_500_NS              10
#define YT8521_RC1R_RGMII_1_650_NS              11
#define YT8521_RC1R_RGMII_1_800_NS              12
#define YT8521_RC1R_RGMII_1_950_NS              13
#define YT8521_RC1R_RGMII_2_100_NS              14
#define YT8521_RC1R_RGMII_2_250_NS              15

#define YTPHY_MISC_CONFIG_REG                   0xA006
#define YTPHY_MCR_FIBER_SPEED_MASK              BIT0
#define YTPHY_MCR_FIBER_1000BX                  (0x1 << 0)
#define YTPHY_MCR_FIBER_100FX                   (0x0 << 0)

/* WOL MAC ADDR: MACADDR2(highest), MACADDR1(middle), MACADDR0(lowest) */
#define YTPHY_WOL_MACADDR2_REG                  0xA007
#define YTPHY_WOL_MACADDR1_REG                  0xA008
#define YTPHY_WOL_MACADDR0_REG                  0xA009

#define YTPHY_WOL_CONFIG_REG                    0xA00A
#define YTPHY_WCR_INTR_SEL                      BIT6
#define YTPHY_WCR_ENABLE                        BIT3

/* 2b00 84ms
 * 2b01 168ms  *default*
 * 2b10 336ms
 * 2b11 672ms
 */
#define YTPHY_WCR_PULSE_WIDTH_MASK              (BIT2 | BIT1)
#define YTPHY_WCR_PULSE_WIDTH_672MS             (BIT2 | BIT1)

/* 1b0 Interrupt and WOL events is level triggered and active LOW  *default*
 * 1b1 Interrupt and WOL events is pulse triggered and active LOW
 */
#define YTPHY_WCR_TYPE_PULSE                    BIT0

#define YTPHY_PAD_DRIVE_STRENGTH_REG            0xA010
#define YT8531_RGMII_RXC_DS_MASK                GENMASK(15, 13)
#define YT8531_RGMII_RXD_DS_HI_MASK             BIT12           /* Bit 2 of rxd_ds */
#define YT8531_RGMII_RXD_DS_LOW_MASK            GENMASK(5, 4)   /* Bit 1/0 of rxd_ds */
#define YT8531_RGMII_RX_DS_DEFAULT              0x3

#define YTPHY_SYNCE_CFG_REG                     0xA012
#define YT8521_SCR_SYNCE_ENABLE                 BIT5
/* 1b0 output 25m clock
 * 1b1 output 125m clock  *default*
 */
#define YT8521_SCR_CLK_FRE_SEL_125M             BIT3
#define YT8521_SCR_CLK_SRC_MASK                 GENMASK(2, 1)
#define YT8521_SCR_CLK_SRC_PLL_125M             0
#define YT8521_SCR_CLK_SRC_UTP_RX               1
#define YT8521_SCR_CLK_SRC_SDS_RX               2
#define YT8521_SCR_CLK_SRC_REF_25M              3
#define YT8531_SCR_SYNCE_ENABLE                 BIT6
/* 1b0 output 25m clock   *default*
 * 1b1 output 125m clock
 */
#define YT8531_SCR_CLK_FRE_SEL_125M             BIT4
#define YT8531_SCR_CLK_SRC_MASK                 GENMASK(3, 1)
#define YT8531_SCR_CLK_SRC_PLL_125M             0
#define YT8531_SCR_CLK_SRC_UTP_RX               1
#define YT8531_SCR_CLK_SRC_SDS_RX               2
#define YT8531_SCR_CLK_SRC_CLOCK_FROM_DIGITAL   3
#define YT8531_SCR_CLK_SRC_REF_25M              4
#define YT8531_SCR_CLK_SRC_SSC_25M              5

/* Extended Register  end */

#define YTPHY_DTS_OUTPUT_CLK_DIS                0
#define YTPHY_DTS_OUTPUT_CLK_25M                25000000
#define YTPHY_DTS_OUTPUT_CLK_125M               125000000

#endif /* __MOTORCOMM_PHY_DXE_H__ */
