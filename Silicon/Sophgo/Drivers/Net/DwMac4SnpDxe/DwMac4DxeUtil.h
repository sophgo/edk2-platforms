/** @file

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef STMMAC_DXE_UTIL_H__
#define STMMAC_DXE_UTIL_H__

#include <Protocol/SimpleNetwork.h>
#include <Library/UefiLib.h>
#include <Base.h>
#define BIT(nr)              (1UL << (nr))
#define GENMASK(end, start)  (((1ULL << ((end) - (start) + 1)) - 1) << (start))

#define UPPER_32_BITS(n)      ((UINT32)((n) >> 32))
#define LOWER_32_BITS(n)      ((UINT32)((n) & 0xffffffff))

#define GMAC4_VERSION           0x00000110      /* GMAC4+ CORE Version */

//
// MAC registers
//
#define GMAC_CONFIG			0x00000000
#define GMAC_EXT_CONFIG			0x00000004
#define GMAC_PACKET_FILTER		0x00000008
#define GMAC_HASH_TAB(x)		(0x10 + (x) * 4)
#define GMAC_VLAN_TAG			0x00000050
#define GMAC_VLAN_TAG_DATA		0x00000054
#define GMAC_VLAN_HASH_TABLE		0x00000058
#define GMAC_RX_FLOW_CTRL		0x00000090
#define GMAC_VLAN_INCL			0x00000060
#define GMAC_QX_TX_FLOW_CTRL(x)		(0x70 + x * 4)
#define GMAC_TXQ_PRTY_MAP0		0x98
#define GMAC_TXQ_PRTY_MAP1		0x9C
#define GMAC_RXQ_CTRL0			0x000000a0
#define GMAC_RXQ_CTRL1			0x000000a4
#define GMAC_RXQ_CTRL2			0x000000a8
#define GMAC_RXQ_CTRL3			0x000000ac
#define GMAC_INT_STATUS			0x000000b0
#define GMAC_INT_EN			0x000000b4
#define GMAC_1US_TIC_COUNTER		0x000000dc
#define GMAC_PCS_BASE			0x000000e0
#define GMAC_PHYIF_CONTROL_STATUS	0x000000f8
#define GMAC_PMT			0x000000c0
#define GMAC_DEBUG			0x00000114
#define GMAC_HW_FEATURE0		0x0000011c
#define GMAC_HW_FEATURE1		0x00000120
#define GMAC_HW_FEATURE2		0x00000124
#define GMAC_HW_FEATURE3		0x00000128
#define GMAC_MDIO_ADDR			0x00000200
#define GMAC_MDIO_DATA			0x00000204
#define GMAC_GPIO_STATUS		0x0000020C
#define GMAC_ARP_ADDR			0x00000210
#define GMAC_ADDR_HIGH(reg)		(0x300 + reg * 8)
#define GMAC_ADDR_LOW(reg)		(0x304 + reg * 8)
#define GMAC_L3L4_CTRL(reg)		(0x900 + (reg) * 0x30)
#define GMAC_L4_ADDR(reg)		(0x904 + (reg) * 0x30)
#define GMAC_L3_ADDR0(reg)		(0x910 + (reg) * 0x30)
#define GMAC_L3_ADDR1(reg)		(0x914 + (reg) * 0x30)
#define GMAC_TIMESTAMP_STATUS		0x00000b20

//
// RX Queues Routing
//
#define GMAC_RXQCTRL_AVCPQ_MASK		GENMASK(2, 0)
#define GMAC_RXQCTRL_AVCPQ_SHIFT	0
#define GMAC_RXQCTRL_PTPQ_MASK		GENMASK(6, 4)
#define GMAC_RXQCTRL_PTPQ_SHIFT		4
#define GMAC_RXQCTRL_DCBCPQ_MASK	GENMASK(10, 8)
#define GMAC_RXQCTRL_DCBCPQ_SHIFT	8
#define GMAC_RXQCTRL_UPQ_MASK		GENMASK(14, 12)
#define GMAC_RXQCTRL_UPQ_SHIFT		12
#define GMAC_RXQCTRL_MCBCQ_MASK		GENMASK(18, 16)
#define GMAC_RXQCTRL_MCBCQ_SHIFT	16
#define GMAC_RXQCTRL_MCBCQEN		BIT20
#define GMAC_RXQCTRL_MCBCQEN_SHIFT	20
#define GMAC_RXQCTRL_TACPQE		BIT21
#define GMAC_RXQCTRL_TACPQE_SHIFT	21
#define GMAC_RXQCTRL_FPRQ		GENMASK(26, 24)
#define GMAC_RXQCTRL_FPRQ_SHIFT		24

//
// MAC Packet Filtering
//
#define GMAC_PACKET_FILTER_PR		BIT0
#define GMAC_PACKET_FILTER_HMC		BIT2
#define GMAC_PACKET_FILTER_PM		BIT4
#define GMAC_PACKET_FILTER_DBF		BIT5
#define GMAC_PACKET_FILTER_PCF		BIT7
#define GMAC_PACKET_FILTER_HPF		BIT10
#define GMAC_PACKET_FILTER_VTFE		BIT16
#define GMAC_PACKET_FILTER_IPFE		BIT20
#define GMAC_PACKET_FILTER_RA		BIT31

#define GMAC_MAX_PERFECT_ADDRESSES	128

//
// MAC VLAN
//
#define GMAC_VLAN_EDVLP			BIT26
#define GMAC_VLAN_VTHM			BIT25
#define GMAC_VLAN_DOVLTC		BIT20
#define GMAC_VLAN_ESVL			BIT18
#define GMAC_VLAN_ETV			BIT16
#define GMAC_VLAN_VID			GENMASK(15, 0)
#define GMAC_VLAN_VLTI			BIT20
#define GMAC_VLAN_CSVL			BIT19
#define GMAC_VLAN_VLC			GENMASK(17, 16)
#define GMAC_VLAN_VLC_SHIFT		16
#define GMAC_VLAN_VLHT			GENMASK(15, 0)

//
// MAC VLAN Tag
//
#define GMAC_VLAN_TAG_VID		GENMASK(15, 0)
#define GMAC_VLAN_TAG_ETV		BIT16

//
// MAC VLAN Tag Control
//
#define GMAC_VLAN_TAG_CTRL_OB		BIT0
#define GMAC_VLAN_TAG_CTRL_CT		BIT1
#define GMAC_VLAN_TAG_CTRL_OFS_MASK	GENMASK(6, 2)
#define GMAC_VLAN_TAG_CTRL_OFS_SHIFT	2
#define GMAC_VLAN_TAG_CTRL_EVLS_MASK	GENMASK(22, 21)
#define GMAC_VLAN_TAG_CTRL_EVLS_SHIFT	21
#define GMAC_VLAN_TAG_CTRL_EVLRXS	BIT24

#define GMAC_VLAN_TAG_STRIP_NONE	(0x0 << GMAC_VLAN_TAG_CTRL_EVLS_SHIFT)
#define GMAC_VLAN_TAG_STRIP_PASS	(0x1 << GMAC_VLAN_TAG_CTRL_EVLS_SHIFT)
#define GMAC_VLAN_TAG_STRIP_FAIL	(0x2 << GMAC_VLAN_TAG_CTRL_EVLS_SHIFT)
#define GMAC_VLAN_TAG_STRIP_ALL		(0x3 << GMAC_VLAN_TAG_CTRL_EVLS_SHIFT)

//
// MAC VLAN Tag Data/Filter
//
#define GMAC_VLAN_TAG_DATA_VID		GENMASK(15, 0)
#define GMAC_VLAN_TAG_DATA_VEN		BIT16
#define GMAC_VLAN_TAG_DATA_ETV		BIT17

//
// MAC RX Queue Enable
//
#define GMAC_RX_QUEUE_CLEAR(queue)	~(GENMASK(1, 0) << ((queue) * 2))
#define GMAC_RX_AV_QUEUE_ENABLE(queue)	BIT((queue) * 2)
#define GMAC_RX_DCB_QUEUE_ENABLE(queue)	BIT(((queue) * 2) + 1)

//
// MAC Flow Control RX
//
#define GMAC_RX_FLOW_CTRL_RFE		BIT0

//
// RX Queues Priorities
//
#define GMAC_RXQCTRL_PSRQX_MASK(x)	GENMASK(7 + ((x) * 8), 0 + ((x) * 8))
#define GMAC_RXQCTRL_PSRQX_SHIFT(x)	((x) * 8)

//
// TX Queues Priorities
//
#define GMAC_TXQCTRL_PSTQX_MASK(x)	GENMASK(7 + ((x) * 8), 0 + ((x) * 8))
#define GMAC_TXQCTRL_PSTQX_SHIFT(x)	((x) * 8)

//
// MAC Flow Control TX
//
#define GMAC_TX_FLOW_CTRL_TFE		BIT1
#define GMAC_TX_FLOW_CTRL_PT_SHIFT	16

//
// MAC Interrupt bitmap
//
#define GMAC_INT_RGSMIIS		BIT0
#define GMAC_INT_PCS_LINK		BIT1
#define GMAC_INT_PCS_ANE		BIT2
#define GMAC_INT_PCS_PHYIS		BIT3
#define GMAC_INT_PMT_EN			BIT4
#define GMAC_INT_LPI_EN			BIT5
#define GMAC_INT_TSIE			BIT12
#define GMAC_INT_FPE_EN                 BIT17

#define	GMAC_PCS_IRQ_DEFAULT	(GMAC_INT_RGSMIIS | GMAC_INT_PCS_LINK |	\
				 GMAC_INT_PCS_ANE)

#define	GMAC_INT_DEFAULT_ENABLE	(GMAC_INT_PMT_EN | GMAC_INT_LPI_EN | \
				 GMAC_INT_TSIE)

/* Energy Efficient Ethernet (EEE) for GMAC4
 *
 * LPI status, timer and control register offset
 */
#define GMAC4_LPI_CTRL_STATUS	0xd0
#define GMAC4_LPI_TIMER_CTRL	0xd4
#define GMAC4_LPI_ENTRY_TIMER	0xd8
#define GMAC4_MAC_ONEUS_TIC_COUNTER	0xdc

//
// LPI control and status defines
//
#define GMAC4_LPI_CTRL_STATUS_LPITCSE	BIT21 /* LPI Tx Clock Stop Enable */
#define GMAC4_LPI_CTRL_STATUS_LPIATE	BIT20 /* LPI Timer Enable */
#define GMAC4_LPI_CTRL_STATUS_LPITXA	BIT19 /* Enable LPI TX Automate */
#define GMAC4_LPI_CTRL_STATUS_PLS	BIT17 /* PHY Link Status */
#define GMAC4_LPI_CTRL_STATUS_LPIEN	BIT16 /* LPI Enable */
#define GMAC4_LPI_CTRL_STATUS_RLPIEX	BIT3  /* Receive LPI Exit */
#define GMAC4_LPI_CTRL_STATUS_RLPIEN	BIT2  /* Receive LPI Entry */
#define GMAC4_LPI_CTRL_STATUS_TLPIEX	BIT1  /* Transmit LPI Exit */
#define GMAC4_LPI_CTRL_STATUS_TLPIEN	BIT0  /* Transmit LPI Entry */

//
// MAC Debug bitmap
//
#define GMAC_DEBUG_TFCSTS_MASK		GENMASK(18, 17)
#define GMAC_DEBUG_TFCSTS_SHIFT		17
#define GMAC_DEBUG_TFCSTS_IDLE		0
#define GMAC_DEBUG_TFCSTS_WAIT		1
#define GMAC_DEBUG_TFCSTS_GEN_PAUSE	2
#define GMAC_DEBUG_TFCSTS_XFER		3
#define GMAC_DEBUG_TPESTS		BIT16
#define GMAC_DEBUG_RFCFCSTS_MASK	GENMASK(2, 1)
#define GMAC_DEBUG_RFCFCSTS_SHIFT	1
#define GMAC_DEBUG_RPESTS		BIT0

//
// MAC config
//
#define GMAC_CONFIG_ARPEN		BIT31
#define GMAC_CONFIG_SARC		GENMASK(30, 28)
#define GMAC_CONFIG_SARC_SHIFT		28
#define GMAC_CONFIG_IPC			BIT27
#define GMAC_CONFIG_IPG			GENMASK(26, 24)
#define GMAC_CONFIG_IPG_SHIFT		24
#define GMAC_CONFIG_2K			BIT22
#define GMAC_CONFIG_ACS			BIT20
#define GMAC_CONFIG_BE			BIT18
#define GMAC_CONFIG_JD			BIT17
#define GMAC_CONFIG_JE			BIT16
#define GMAC_CONFIG_PS			BIT15
#define GMAC_CONFIG_FES			BIT14
#define GMAC_CONFIG_FES_SHIFT		14
#define GMAC_CONFIG_DM			BIT13
#define GMAC_CONFIG_LM			BIT12
#define GMAC_CONFIG_DCRS		BIT9
#define GMAC_CONFIG_TE			BIT1
#define GMAC_CONFIG_RE			BIT0

//
// MAC extended config
//
#define GMAC_CONFIG_EIPG		GENMASK(29, 25)
#define GMAC_CONFIG_EIPG_SHIFT		25
#define GMAC_CONFIG_EIPG_EN		BIT24
#define GMAC_CONFIG_HDSMS		GENMASK(22, 20)
#define GMAC_CONFIG_HDSMS_SHIFT		20
#define GMAC_CONFIG_HDSMS_256		(0x2 << GMAC_CONFIG_HDSMS_SHIFT)

//
// MAC HW features0 bitmap
//
#define GMAC_HW_FEAT_SAVLANINS		BIT27
#define GMAC_HW_FEAT_ADDMAC		BIT18
#define GMAC_HW_FEAT_RXCOESEL		BIT16
#define GMAC_HW_FEAT_TXCOSEL		BIT14
#define GMAC_HW_FEAT_EEESEL		BIT13
#define GMAC_HW_FEAT_TSSEL		BIT12
#define GMAC_HW_FEAT_ARPOFFSEL		BIT9
#define GMAC_HW_FEAT_MMCSEL		BIT8
#define GMAC_HW_FEAT_MGKSEL		BIT7
#define GMAC_HW_FEAT_RWKSEL		BIT6
#define GMAC_HW_FEAT_SMASEL		BIT5
#define GMAC_HW_FEAT_VLHASH		BIT4
#define GMAC_HW_FEAT_PCSSEL		BIT3
#define GMAC_HW_FEAT_HDSEL		BIT2
#define GMAC_HW_FEAT_GMIISEL		BIT1
#define GMAC_HW_FEAT_MIISEL		BIT0

//
// MAC HW features1 bitmap
//
#define GMAC_HW_FEAT_L3L4FNUM		GENMASK(30, 27)
#define GMAC_HW_HASH_TB_SZ		GENMASK(25, 24)
#define GMAC_HW_FEAT_AVSEL		BIT20
#define GMAC_HW_TSOEN			BIT18
#define GMAC_HW_FEAT_SPHEN		BIT17
#define GMAC_HW_ADDR64			GENMASK(15, 14)
#define GMAC_HW_TXFIFOSIZE		GENMASK(10, 6)
#define GMAC_HW_RXFIFOSIZE		GENMASK(4, 0)

//
// MAC HW features2 bitmap
//
#define GMAC_HW_FEAT_AUXSNAPNUM		GENMASK(30, 28)
#define GMAC_HW_FEAT_PPSOUTNUM		GENMASK(26, 24)
#define GMAC_HW_FEAT_TXCHCNT		GENMASK(21, 18)
#define GMAC_HW_FEAT_RXCHCNT		GENMASK(15, 12)
#define GMAC_HW_FEAT_TXQCNT		GENMASK(9, 6)
#define GMAC_HW_FEAT_RXQCNT		GENMASK(3, 0)

//
// MAC HW features3 bitmap
//
#define GMAC_HW_FEAT_ASP		GENMASK(29, 28)
#define GMAC_HW_FEAT_TBSSEL		BIT27
#define GMAC_HW_FEAT_FPESEL		BIT26
#define GMAC_HW_FEAT_ESTWID		GENMASK(21, 20)
#define GMAC_HW_FEAT_ESTDEP		GENMASK(19, 17)
#define GMAC_HW_FEAT_ESTSEL		BIT16
#define GMAC_HW_FEAT_FRPES		GENMASK(14, 13)
#define GMAC_HW_FEAT_FRPBS		GENMASK(12, 11)
#define GMAC_HW_FEAT_FRPSEL		BIT10
#define GMAC_HW_FEAT_DVLAN		BIT5
#define GMAC_HW_FEAT_NRVF		GENMASK(2, 0)

//
// GMAC GPIO Status reg
//
#define GMAC_GPO0			BIT16
#define GMAC_GPO1			BIT17
#define GMAC_GPO2			BIT18
#define GMAC_GPO3			BIT19

//
// MAC HW ADDR regs
//
#define GMAC_HI_DCS			GENMASK(18, 16)
#define GMAC_HI_DCS_SHIFT		16
#define GMAC_HI_REG_AE			BIT31

//
// L3/L4 Filters regs
//
#define GMAC_L4DPIM0			BIT21
#define GMAC_L4DPM0			BIT20
#define GMAC_L4SPIM0			BIT19
#define GMAC_L4SPM0			BIT18
#define GMAC_L4PEN0			BIT16
#define GMAC_L3DAIM0			BIT5
#define GMAC_L3DAM0			BIT4
#define GMAC_L3SAIM0			BIT3
#define GMAC_L3SAM0			BIT2
#define GMAC_L3PEN0			BIT0
#define GMAC_L4DP0			GENMASK(31, 16)
#define GMAC_L4DP0_SHIFT		16
#define GMAC_L4SP0			GENMASK(15, 0)

//
// MAC Timestamp Status
//
#define GMAC_TIMESTAMP_AUXTSTRIG	BIT2
#define GMAC_TIMESTAMP_ATSNS_MASK	GENMASK(29, 25)
#define GMAC_TIMESTAMP_ATSNS_SHIFT	25

//
// MTL registers
//
#define MTL_OPERATION_MODE		0x00000c00
#define MTL_FRPE			BIT15
#define MTL_OPERATION_SCHALG_MASK	GENMASK(6, 5)
#define MTL_OPERATION_SCHALG_WRR	(0x0 << 5)
#define MTL_OPERATION_SCHALG_WFQ	(0x1 << 5)
#define MTL_OPERATION_SCHALG_DWRR	(0x2 << 5)
#define MTL_OPERATION_SCHALG_SP		(0x3 << 5)
#define MTL_OPERATION_RAA		BIT2
#define MTL_OPERATION_RAA_SP		(0x0 << 2)
#define MTL_OPERATION_RAA_WSP		(0x1 << 2)

#define MTL_INT_STATUS			0x00000c20
#define MTL_INT_QX(x)			BIT(x)

#define MTL_RXQ_DMA_MAP0		0x00000c30 /* queue 0 to 3 */
#define MTL_RXQ_DMA_MAP1		0x00000c34 /* queue 4 to 7 */
#define MTL_RXQ_DMA_QXMDMACH_MASK(x)	(0xf << 8 * (x))
#define MTL_RXQ_DMA_QXMDMACH(chan, q)	((chan) << (8 * (q)))

#define MTL_CHAN_BASE_ADDR              0x00000d00
#define MTL_CHAN_BASE_OFFSET            0x40
#define MTL_CHANX_BASE_ADDR(x)          (MTL_CHAN_BASE_ADDR + \
                                        (x * MTL_CHAN_BASE_OFFSET))

#define MTL_CHAN_TX_OP_MODE(x)          MTL_CHANX_BASE_ADDR(x)
#define MTL_CHAN_TX_DEBUG(x)            (MTL_CHANX_BASE_ADDR(x) + 0x8)
#define MTL_CHAN_INT_CTRL(x)            (MTL_CHANX_BASE_ADDR(x) + 0x2c)
#define MTL_CHAN_RX_OP_MODE(x)          (MTL_CHANX_BASE_ADDR(x) + 0x30)
#define MTL_CHAN_RX_DEBUG(x)            (MTL_CHANX_BASE_ADDR(x) + 0x38)

#define MTL_OP_MODE_RSF			BIT5
#define MTL_OP_MODE_TXQEN_MASK		GENMASK(3, 2)
#define MTL_OP_MODE_TXQEN_AV		BIT2
#define MTL_OP_MODE_TXQEN		BIT3
#define MTL_OP_MODE_TSF			BIT1

#define MTL_OP_MODE_TQS_MASK		GENMASK(24, 16)
#define MTL_OP_MODE_TQS_SHIFT		16

#define MTL_OP_MODE_TTC_MASK		0x70
#define MTL_OP_MODE_TTC_SHIFT		4

#define MTL_OP_MODE_TTC_32		0
#define MTL_OP_MODE_TTC_64		(1 << MTL_OP_MODE_TTC_SHIFT)
#define MTL_OP_MODE_TTC_96		(2 << MTL_OP_MODE_TTC_SHIFT)
#define MTL_OP_MODE_TTC_128		(3 << MTL_OP_MODE_TTC_SHIFT)
#define MTL_OP_MODE_TTC_192		(4 << MTL_OP_MODE_TTC_SHIFT)
#define MTL_OP_MODE_TTC_256		(5 << MTL_OP_MODE_TTC_SHIFT)
#define MTL_OP_MODE_TTC_384		(6 << MTL_OP_MODE_TTC_SHIFT)
#define MTL_OP_MODE_TTC_512		(7 << MTL_OP_MODE_TTC_SHIFT)

#define MTL_OP_MODE_RQS_MASK		GENMASK(29, 20)
#define MTL_OP_MODE_RQS_SHIFT		20

#define MTL_OP_MODE_RFD_MASK		GENMASK(19, 14)
#define MTL_OP_MODE_RFD_SHIFT		14

#define MTL_OP_MODE_RFA_MASK		GENMASK(13, 8)
#define MTL_OP_MODE_RFA_SHIFT		8

#define MTL_OP_MODE_EHFC		BIT7

#define MTL_OP_MODE_RTC_MASK		0x18
#define MTL_OP_MODE_RTC_SHIFT		3

#define MTL_OP_MODE_RTC_32		(1 << MTL_OP_MODE_RTC_SHIFT)
#define MTL_OP_MODE_RTC_64		0
#define MTL_OP_MODE_RTC_96		(2 << MTL_OP_MODE_RTC_SHIFT)
#define MTL_OP_MODE_RTC_128		(3 << MTL_OP_MODE_RTC_SHIFT)

//
// MTL ETS Control register
//
#define MTL_ETS_CTRL_BASE_ADDR		0x00000d10
#define MTL_ETS_CTRL_BASE_OFFSET	0x40

#define MTL_ETS_CTRL_CC			BIT3
#define MTL_ETS_CTRL_AVALG		BIT2

//
// MTL Queue Quantum Weight
//
#define MTL_TXQ_WEIGHT_BASE_ADDR	0x00000d18
#define MTL_TXQ_WEIGHT_BASE_OFFSET	0x40

#define MTL_TXQ_WEIGHT_ISCQW_MASK	GENMASK(20, 0)

//
// MTL sendSlopeCredit register
//
#define MTL_SEND_SLP_CRED_BASE_ADDR	0x00000d1c
#define MTL_SEND_SLP_CRED_OFFSET	0x40

#define MTL_SEND_SLP_CRED_SSC_MASK	GENMASK(13, 0)

//
// MTL hiCredit register
//
#define MTL_HIGH_CRED_BASE_ADDR		0x00000d20
#define MTL_HIGH_CRED_OFFSET		0x40


#define MTL_HIGH_CRED_HC_MASK		GENMASK(28, 0)

//
// MTL loCredit register
//
#define MTL_LOW_CRED_BASE_ADDR		0x00000d24
#define MTL_LOW_CRED_OFFSET		0x40

#define MTL_HIGH_CRED_LC_MASK		GENMASK(28, 0)

//
// MTL debug
//
#define MTL_DEBUG_TXSTSFSTS		BIT5
#define MTL_DEBUG_TXFSTS		BIT4
#define MTL_DEBUG_TWCSTS		BIT3

//
// MTL debug: Tx FIFO Read Controller Status
//
#define MTL_DEBUG_TRCSTS_MASK		GENMASK(2, 1)
#define MTL_DEBUG_TRCSTS_SHIFT		1
#define MTL_DEBUG_TRCSTS_IDLE		0
#define MTL_DEBUG_TRCSTS_READ		1
#define MTL_DEBUG_TRCSTS_TXW		2
#define MTL_DEBUG_TRCSTS_WRITE		3
#define MTL_DEBUG_TXPAUSED		BIT0

//
// MAC debug: GMII or MII Transmit Protocol Engine Status
//
#define MTL_DEBUG_RXFSTS_MASK		GENMASK(5, 4)
#define MTL_DEBUG_RXFSTS_SHIFT		4
#define MTL_DEBUG_RXFSTS_EMPTY		0
#define MTL_DEBUG_RXFSTS_BT		1
#define MTL_DEBUG_RXFSTS_AT		2
#define MTL_DEBUG_RXFSTS_FULL		3
#define MTL_DEBUG_RRCSTS_MASK		GENMASK(2, 1)
#define MTL_DEBUG_RRCSTS_SHIFT		1
#define MTL_DEBUG_RRCSTS_IDLE		0
#define MTL_DEBUG_RRCSTS_RDATA		1
#define MTL_DEBUG_RRCSTS_RSTAT		2
#define MTL_DEBUG_RRCSTS_FLUSH		3
#define MTL_DEBUG_RWCSTS		BIT0

//
// MTL interrupt
//
#define MTL_RX_OVERFLOW_INT_EN		BIT24
#define MTL_RX_OVERFLOW_INT		BIT16

//
// Default operating mode of the MAC
//
#define GMAC_CORE_INIT (GMAC_CONFIG_JD | GMAC_CONFIG_PS | \
			GMAC_CONFIG_BE | GMAC_CONFIG_DCRS | \
			GMAC_CONFIG_JE)

//
// To dump the core regs excluding  the Address Registers
//
#define	GMAC_REG_NUM	132

//
// MTL debug
//
#define MTL_DEBUG_TXSTSFSTS		BIT5
#define MTL_DEBUG_TXFSTS		BIT4
#define MTL_DEBUG_TWCSTS		BIT3

//
// MTL debug: Tx FIFO Read Controller Status
//
#define MTL_DEBUG_TRCSTS_MASK		GENMASK(2, 1)
#define MTL_DEBUG_TRCSTS_SHIFT		1
#define MTL_DEBUG_TRCSTS_IDLE		0
#define MTL_DEBUG_TRCSTS_READ		1
#define MTL_DEBUG_TRCSTS_TXW		2
#define MTL_DEBUG_TRCSTS_WRITE		3
#define MTL_DEBUG_TXPAUSED		BIT0

//
// MAC debug: GMII or MII Transmit Protocol Engine Status
//
#define MTL_DEBUG_RXFSTS_MASK		GENMASK(5, 4)
#define MTL_DEBUG_RXFSTS_SHIFT		4
#define MTL_DEBUG_RXFSTS_EMPTY		0
#define MTL_DEBUG_RXFSTS_BT		1
#define MTL_DEBUG_RXFSTS_AT		2
#define MTL_DEBUG_RXFSTS_FULL		3
#define MTL_DEBUG_RRCSTS_MASK		GENMASK(2, 1)
#define MTL_DEBUG_RRCSTS_SHIFT		1
#define MTL_DEBUG_RRCSTS_IDLE		0
#define MTL_DEBUG_RRCSTS_RDATA		1
#define MTL_DEBUG_RRCSTS_RSTAT		2
#define MTL_DEBUG_RRCSTS_FLUSH		3
#define MTL_DEBUG_RWCSTS		BIT0

//
// SGMII/RGMII status register
//
#define GMAC_PHYIF_CTRLSTATUS_TC		BIT0
#define GMAC_PHYIF_CTRLSTATUS_LUD		BIT1
#define GMAC_PHYIF_CTRLSTATUS_SMIDRXS		BIT4
#define GMAC_PHYIF_CTRLSTATUS_LNKMOD		BIT16
#define GMAC_PHYIF_CTRLSTATUS_SPEED		GENMASK(18, 17)
#define GMAC_PHYIF_CTRLSTATUS_SPEED_SHIFT	17
#define GMAC_PHYIF_CTRLSTATUS_LNKSTS		BIT19
#define GMAC_PHYIF_CTRLSTATUS_JABTO		BIT20
#define GMAC_PHYIF_CTRLSTATUS_FALSECARDET	BIT21

//
// Most common CRC32 Polynomial for little endian machines
//
#define CRC_POLYNOMIAL                                            0xEDB88320

//
// DMA CRS Control and Status Register Mapping
//
#define DMA_BUS_MODE                    0x00001000
#define DMA_SYS_BUS_MODE                0x00001004
#define DMA_STATUS                      0x00001008
#define DMA_DEBUG_STATUS_0              0x0000100c
#define DMA_DEBUG_STATUS_1              0x00001010
#define DMA_DEBUG_STATUS_2              0x00001014
#define DMA_AXI_BUS_MODE                0x00001028
#define DMA_TBS_CTRL                    0x00001050

#define DMA_DEBUG_STATUS_0_AXWHSTS      BIT0
#define DMA_DEBUG_STATUS_0_AXRHSTS      BIT1
#define DMA_DEBUG_STATUS_0_RPS0_SHIFT   8
#define DMA_DEBUG_STATUS_0_RPS0_MASK    GENMASK(11, 8)
#define DMA_DEBUG_RPS0_STOP             0
#define DMA_DEBUG_RPS0_RUN_FRTD         1
#define DMA_DEBUG_RPS0_RSVD             2
#define DMA_DEBUG_RPS0_RUN_WRP          3
#define DMA_DEBUG_RPS0_SUSPND           4
#define DMA_DEBUG_RPS0_RUN_CRD          5
#define DMA_DEBUG_RPS0_TSTMP            6
#define DMA_DEBUG_RPS0_RUN_TRP          7
#define DMA_DEBUG_STATUS_0_TPS0_SHIFT   12
#define DMA_DEBUG_STATUS_0_TPS0_MASK    GENMASK(15, 12)
#define DMA_DEBUG_TPS0_STOP             0
#define DMA_DEBUG_TPS0_RUN_FTTD         1
#define DMA_DEBUG_TPS0_RUN_WS           2
#define DMA_DEBUG_TPS0_RUN_RDS          3
#define DMA_DEBUG_TPS0_TSTMP_WS         4
#define DMA_DEBUG_TPS0_RSVD             5
#define DMA_DEBUG_TPS0_SUSPND           6
#define DMA_DEBUG_TPS0_RUN_CTD          7

//
// DMA Bus Mode bitmap
//
#define DMA_BUS_MODE_DCHE               BIT19
#define DMA_BUS_MODE_INTM_MASK          GENMASK(17, 16)
#define DMA_BUS_MODE_INTM_SHIFT         16
#define DMA_BUS_MODE_INTM_MODE1         0x1
#define DMA_BUS_MODE_SFT_RESET          BIT0

//
// DMA SYS Bus Mode bitmap
//
#define DMA_BUS_MODE_SPH                BIT24
#define DMA_BUS_MODE_PBL                BIT16
#define DMA_BUS_MODE_PBL_SHIFT          16
#define DMA_BUS_MODE_RPBL_SHIFT         16
#define DMA_BUS_MODE_MB                 BIT14
#define DMA_BUS_MODE_FB                 BIT0

//
// Rx watchdog register
//
#define DMA_RX_WATCHDOG         0x00001024

/* DMA debug status bitmap */
#define DMA_DEBUG_STATUS_TS_MASK        0xf
#define DMA_DEBUG_STATUS_RS_MASK        0xf

/* DMA AXI bitmap */
#define DMA_AXI_EN_LPI                  BIT31
#define DMA_AXI_LPI_XIT_FRM             BIT30
#define DMA_AXI_WR_OSR_LMT              GENMASK(27, 24)
#define DMA_AXI_WR_OSR_LMT_SHIFT        24
#define DMA_AXI_RD_OSR_LMT              GENMASK(19, 16)
#define DMA_AXI_RD_OSR_LMT_SHIFT        16

#define DMA_AXI_OSR_MAX                 0xf
#define DMA_AXI_MAX_OSR_LIMIT ((DMA_AXI_OSR_MAX << DMA_AXI_WR_OSR_LMT_SHIFT) | \
                                (DMA_AXI_OSR_MAX << DMA_AXI_RD_OSR_LMT_SHIFT))

//
// AXI Master Bus Mode
//
#define DMA_AXI_BUS_MODE        0x00001028

#define DMA_AXI_EN_LPI          BIT31
#define DMA_AXI_LPI_XIT_FRM     BIT30
#define DMA_SYS_BUS_MB                  BIT14
#define DMA_AXI_1KBBE                   BIT13
#define DMA_SYS_BUS_AAL                 BIT12
#define DMA_SYS_BUS_EAME                BIT11
#define DMA_AXI_BLEN256                 BIT7
#define DMA_AXI_BLEN128                 BIT6
#define DMA_AXI_BLEN64                  BIT5
#define DMA_AXI_BLEN32                  BIT4
#define DMA_AXI_BLEN16                  BIT3
#define DMA_AXI_BLEN8                   BIT2
#define DMA_AXI_BLEN4                   BIT1
#define DMA_SYS_BUS_FB                  BIT0

#define DMA_BURST_LEN_DEFAULT           (DMA_AXI_BLEN256 | DMA_AXI_BLEN128 | \
                                        DMA_AXI_BLEN64 | DMA_AXI_BLEN32 | \
                                        DMA_AXI_BLEN16 | DMA_AXI_BLEN8 | \
                                        DMA_AXI_BLEN4)

#define DMA_AXI_BURST_LEN_MASK          0x000000FE

/* DMA TBS Control */
#define DMA_TBS_FTOS                    GENMASK(31, 8)
#define DMA_TBS_FTOV                    BIT0
#define DMA_TBS_DEF_FTOS                (DMA_TBS_FTOS | DMA_TBS_FTOV)

/* Following DMA defines are channel-oriented */
#define DMA_CHAN_BASE_ADDR              0x00001100
#define DMA_CHAN_BASE_OFFSET            0x80
//
// DMA: chanels oriented
//
#define DMA_CHAN_BASE_ADDR              0x00001100
#define DMA_CHAN_BASE_OFFSET            0x80
#define DMA_CHANX_BASE_ADDR(x)          (DMA_CHAN_BASE_ADDR + \
                                        (x * DMA_CHAN_BASE_OFFSET))
#define DMA_CHAN_REG_NUMBER             17

#define DMA_CHAN_CONTROL(x)             DMA_CHANX_BASE_ADDR(x)
#define DMA_CHAN_TX_CONTROL(x)          (DMA_CHANX_BASE_ADDR(x) + 0x4)
#define DMA_CHAN_RX_CONTROL(x)          (DMA_CHANX_BASE_ADDR(x) + 0x8)
#define DMA_CHAN_TX_BASE_ADDR_HI(x)     (DMA_CHANX_BASE_ADDR(x) + 0x10)
#define DMA_CHAN_TX_BASE_ADDR(x)        (DMA_CHANX_BASE_ADDR(x) + 0x14)
#define DMA_CHAN_RX_BASE_ADDR_HI(x)     (DMA_CHANX_BASE_ADDR(x) + 0x18)
#define DMA_CHAN_RX_BASE_ADDR(x)        (DMA_CHANX_BASE_ADDR(x) + 0x1c)
#define DMA_CHAN_TX_END_ADDR(x)         (DMA_CHANX_BASE_ADDR(x) + 0x20)
#define DMA_CHAN_RX_END_ADDR(x)         (DMA_CHANX_BASE_ADDR(x) + 0x28)
#define DMA_CHAN_TX_RING_LEN(x)         (DMA_CHANX_BASE_ADDR(x) + 0x2c)
#define DMA_CHAN_RX_RING_LEN(x)         (DMA_CHANX_BASE_ADDR(x) + 0x30)
#define DMA_CHAN_INTR_ENA(x)            (DMA_CHANX_BASE_ADDR(x) + 0x34)
#define DMA_CHAN_RX_WATCHDOG(x)         (DMA_CHANX_BASE_ADDR(x) + 0x38)
#define DMA_CHAN_SLOT_CTRL_STATUS(x)    (DMA_CHANX_BASE_ADDR(x) + 0x3c)
#define DMA_CHAN_CUR_TX_DESC(x)         (DMA_CHANX_BASE_ADDR(x) + 0x44)
#define DMA_CHAN_CUR_RX_DESC(x)         (DMA_CHANX_BASE_ADDR(x) + 0x4c)
#define DMA_CHAN_CUR_TX_BUF_ADDR_H(x)   (DMA_CHANX_BASE_ADDR(x) + 0x50)
#define DMA_CHAN_CUR_TX_BUF_ADDR(x)     (DMA_CHANX_BASE_ADDR(x) + 0x54)
#define DMA_CHAN_CUR_RX_BUF_ADDR_H(x)   (DMA_CHANX_BASE_ADDR(x) + 0x58)
#define DMA_CHAN_CUR_RX_BUF_ADDR(x)     (DMA_CHANX_BASE_ADDR(x) + 0x5c)
#define DMA_CHAN_STATUS(x)              (DMA_CHANX_BASE_ADDR(x) + 0x60)

//
// DMA Control X
//
#define DMA_CONTROL_SPH                 BIT24
#define DMA_CONTROL_MSS_MASK            GENMASK(13, 0)

//
// DMA Tx Channel X Control register defines
//
#define DMA_CONTROL_EDSE                BIT28
#define DMA_CONTROL_TSE                 BIT12
#define DMA_CONTROL_OSP                 BIT4
#define DMA_CONTROL_ST                  BIT0

//
// DMA Rx Channel X Control register defines
//
#define DMA_CONTROL_SR                  BIT0
#define DMA_RBSZ_MASK                   GENMASK(14, 1)
#define DMA_RBSZ_SHIFT                  1

//
// Interrupt enable bits per channel
//
#define DMA_CHAN_INTR_ENA_NIE           BIT16
#define DMA_CHAN_INTR_ENA_AIE           BIT15
#define DMA_CHAN_INTR_ENA_NIE_4_10      BIT15
#define DMA_CHAN_INTR_ENA_AIE_4_10      BIT14
#define DMA_CHAN_INTR_ENA_CDE           BIT13
#define DMA_CHAN_INTR_ENA_FBE           BIT12
#define DMA_CHAN_INTR_ENA_ERE           BIT11
#define DMA_CHAN_INTR_ENA_ETE           BIT10
#define DMA_CHAN_INTR_ENA_RWE           BIT9
#define DMA_CHAN_INTR_ENA_RSE           BIT8
#define DMA_CHAN_INTR_ENA_RBUE          BIT7
#define DMA_CHAN_INTR_ENA_RIE           BIT6
#define DMA_CHAN_INTR_ENA_TBUE          BIT2
#define DMA_CHAN_INTR_ENA_TSE           BIT1
#define DMA_CHAN_INTR_ENA_TIE           BIT0

#define DMA_CHAN_INTR_NORMAL            (DMA_CHAN_INTR_ENA_NIE | \
                                         DMA_CHAN_INTR_ENA_RIE | \
                                         DMA_CHAN_INTR_ENA_TIE)

#define DMA_CHAN_INTR_ABNORMAL          (DMA_CHAN_INTR_ENA_AIE | \
                                         DMA_CHAN_INTR_ENA_FBE)

//
// DMA default interrupt mask for 4.00
//
#define DMA_CHAN_INTR_DEFAULT_MASK      (DMA_CHAN_INTR_NORMAL | \
                                         DMA_CHAN_INTR_ABNORMAL)
#define DMA_CHAN_INTR_DEFAULT_RX        (DMA_CHAN_INTR_ENA_RIE)
#define DMA_CHAN_INTR_DEFAULT_TX        (DMA_CHAN_INTR_ENA_TIE)
#define DMA_CHAN_INTR_NORMAL_4_10       (DMA_CHAN_INTR_ENA_NIE_4_10 | \
                                         DMA_CHAN_INTR_ENA_RIE | \
                                         DMA_CHAN_INTR_ENA_TIE)

#define DMA_CHAN_INTR_ABNORMAL_4_10     (DMA_CHAN_INTR_ENA_AIE_4_10 | \
                                         DMA_CHAN_INTR_ENA_FBE)
//
// Interrupt status per channel
//
#define DMA_CHAN_STATUS_REB             GENMASK(21, 19)
#define DMA_CHAN_STATUS_REB_SHIFT       19
#define DMA_CHAN_STATUS_TEB             GENMASK(18, 16)
#define DMA_CHAN_STATUS_TEB_SHIFT       16
#define DMA_CHAN_STATUS_NIS             BIT15
#define DMA_CHAN_STATUS_AIS             BIT14
#define DMA_CHAN_STATUS_CDE             BIT13
#define DMA_CHAN_STATUS_FBE             BIT12
#define DMA_CHAN_STATUS_ERI             BIT11
#define DMA_CHAN_STATUS_ETI             BIT10
#define DMA_CHAN_STATUS_RWT             BIT9
#define DMA_CHAN_STATUS_RPS             BIT8
#define DMA_CHAN_STATUS_RBU             BIT7
#define DMA_CHAN_STATUS_RI              BIT6
#define DMA_CHAN_STATUS_TBU             BIT2
#define DMA_CHAN_STATUS_TPS             BIT1
#define DMA_CHAN_STATUS_TI              BIT0

#define DMA_CHAN_STATUS_MSK_COMMON      (DMA_CHAN_STATUS_NIS | \
		                         DMA_CHAN_STATUS_AIS | \
					 DMA_CHAN_STATUS_CDE | \
					 DMA_CHAN_STATUS_FBE)

#define DMA_CHAN_STATUS_MSK_RX          (DMA_CHAN_STATUS_REB | \
		                         DMA_CHAN_STATUS_ERI | \
					 DMA_CHAN_STATUS_RWT | \
					 DMA_CHAN_STATUS_RPS | \
					 DMA_CHAN_STATUS_RBU | \
					 DMA_CHAN_STATUS_RI  | \
					 DMA_CHAN_STATUS_MSK_COMMON)

#define DMA_CHAN_STATUS_MSK_TX          (DMA_CHAN_STATUS_ETI | \
		                         DMA_CHAN_STATUS_TBU | \
					 DMA_CHAN_STATUS_TPS | \
					 DMA_CHAN_STATUS_TI  | \
					 DMA_CHAN_STATUS_MSK_COMMON)

#define DMA_RX_NO_ERROR                   0x0
#define DMA_RX_WRITE_DATA_BUFFER_ERROR    0x1
#define DMA_RX_WRITE_DESCRIPTOR_ERROR     0x3
#define DMA_RX_READ_DATA_BUFFER_ERROR     0x5
#define DMA_RX_READ_DESCRIPTOR_ERROR      0x7
#define DMA_TX_NO_ERROR                   0x0
#define DMA_TX_WRITE_DATA_BUFFER_ERROR    0x1
#define DMA_TX_WRITE_DESCRIPTOR_ERROR     0x3
#define DMA_TX_READ_DATA_BUFFER_ERROR     0x5
#define DMA_TX_READ_DESCRIPTOR_ERROR      0x7

//
// MMC (MAC Management Counters)
//
#define MMC_TX_FRAMECOUNT_GB      0x700 + 0x18
#define MMC_TX_BROADCASTFRAME_G   0x700 + 0x1c
#define MMC_TX_MULTICASTFRAME_G   0x700 + 0x20
#define MMC_TX_UNICAST_GB         0x700 + 0x3c
#define MMC_TX_LATECOL            0x700 + 0x58
#define MMC_TX_EXESSCOL           0x700 + 0x5c
#define MMC_TX_OCTETCOUNT_GB      0x700 + 0x64
#define MMC_TX_FRAMECOUNT_G       0x700 + 0x68
#define MMC_TX_OVERSIZE_G         0x700 + 0x78

#define MMC_RX_FRAMECOUNT_GB      0x700 + 0x80
#define MMC_RX_OCTETCOUNT_GB      0x700 + 0x84
#define MMC_RX_BROADCASTFRAME_G   0x700 + 0x8c
#define MMC_RX_MULTICASTFRAME_G   0x700 + 0x90
#define MMC_RX_CRC_ERROR          0x700 + 0x94
#define MMC_RX_UNDERSIZE_G        0x700 + 0xa4
#define MMC_RX_OVERSIZE_G         0x700 + 0xa8
#define MMC_RX_UNICAST_G          0x700 + 0xc4

#define TX_DESC_NUM               4
#define RX_DESC_NUM               4
//#define ETH_BUFFER_SIZE           2048 // 2KiB
#define ETH_BUFFER_SIZE           1600 // 2KiB
#define TX_TOTAL_BUFFER_SIZE      (TX_DESC_NUM * ETH_BUFFER_SIZE)
#define RX_TOTAL_BUFFER_SIZE      (RX_DESC_NUM * ETH_BUFFER_SIZE)
#define RX_MAX_PACKET             1600 // ALIGN(1568, 64)

/* Normal transmit descriptor defines (without split feature) */

/* TDES2 (read format) */
#define TDES2_BUFFER1_SIZE_MASK         GENMASK(13, 0)
#define TDES2_VLAN_TAG_MASK             GENMASK(15, 14)
#define TDES2_VLAN_TAG_SHIFT            14
#define TDES2_BUFFER2_SIZE_MASK         GENMASK(29, 16)
#define TDES2_BUFFER2_SIZE_MASK_SHIFT   16
#define TDES3_IVTIR_MASK                GENMASK(19, 18)
#define TDES3_IVTIR_SHIFT               18
#define TDES3_IVLTV                     BIT17
#define TDES2_TIMESTAMP_ENABLE          BIT30
#define TDES2_IVT_MASK                  GENMASK(31, 16)
#define TDES2_IVT_SHIFT                 16
#define TDES2_INTERRUPT_ON_COMPLETION   BIT31

/* TDES3 (read format) */
#define TDES3_PACKET_SIZE_MASK          GENMASK(14, 0)
#define TDES3_VLAN_TAG                  GENMASK(15, 0)
#define TDES3_VLTV                      BIT16
#define TDES3_CHECKSUM_INSERTION_MASK   GENMASK(17, 16)
#define TDES3_CHECKSUM_INSERTION_SHIFT  16
#define TDES3_TCP_PKT_PAYLOAD_MASK      GENMASK(17, 0)
#define TDES3_TCP_SEGMENTATION_ENABLE   BIT18
#define TDES3_HDR_LEN_SHIFT             19
#define TDES3_SLOT_NUMBER_MASK          GENMASK(22, 19)
#define TDES3_SA_INSERT_CTRL_MASK       GENMASK(25, 23)
#define TDES3_SA_INSERT_CTRL_SHIFT      23
#define TDES3_CRC_PAD_CTRL_MASK         GENMASK(27, 26)

/* TDES3 (write back format) */
#define TDES3_IP_HDR_ERROR              BIT0
#define TDES3_DEFERRED                  BIT1
#define TDES3_UNDERFLOW_ERROR           BIT2
#define TDES3_EXCESSIVE_DEFERRAL        BIT3
#define TDES3_COLLISION_COUNT_MASK      GENMASK(7, 4)
#define TDES3_COLLISION_COUNT_SHIFT     4
#define TDES3_EXCESSIVE_COLLISION       BIT8
#define TDES3_LATE_COLLISION            BIT9
#define TDES3_NO_CARRIER                BIT10
#define TDES3_LOSS_CARRIER              BIT11
#define TDES3_PAYLOAD_ERROR             BIT12
#define TDES3_PACKET_FLUSHED            BIT13
#define TDES3_JABBER_TIMEOUT            BIT14
#define TDES3_ERROR_SUMMARY             BIT15
#define TDES3_TIMESTAMP_STATUS          BIT17
#define TDES3_TIMESTAMP_STATUS_SHIFT    17

/* TDES3 context */
#define TDES3_CTXT_TCMSSV               BIT26

/* TDES3 Common */
#define TDES3_RS1V                      BIT26
#define TDES3_RS1V_SHIFT                26
#define TDES3_LAST_DESCRIPTOR           BIT28
#define TDES3_LAST_DESCRIPTOR_SHIFT     28
#define TDES3_FIRST_DESCRIPTOR          BIT29
#define TDES3_CONTEXT_TYPE              BIT30
#define TDES3_CONTEXT_TYPE_SHIFT        30

//
// TDS3 use for both format (read and write back)
//
#define TDES3_OWN                       BIT31
#define TDES3_OWN_SHIFT                 31
/* Normal receive descriptor defines (without split feature) */

/* RDES0 (write back format) */
#define RDES0_VLAN_TAG_MASK             GENMASK(15, 0)

/* RDES1 (write back format) */
#define RDES1_IP_PAYLOAD_TYPE_MASK      GENMASK(2, 0)
#define RDES1_IP_HDR_ERROR              BIT3
#define RDES1_IPV4_HEADER               BIT4
#define RDES1_IPV6_HEADER               BIT5
#define RDES1_IP_CSUM_BYPASSED          BIT6
#define RDES1_IP_CSUM_ERROR             BIT7
#define RDES1_PTP_MSG_TYPE_MASK         GENMASK(11, 8)
#define RDES1_PTP_PACKET_TYPE           BIT12
#define RDES1_PTP_VER                   BIT13
#define RDES1_TIMESTAMP_AVAILABLE       BIT14
#define RDES1_TIMESTAMP_AVAILABLE_SHIFT 14
#define RDES1_TIMESTAMP_DROPPED         BIT15
#define RDES1_IP_TYPE1_CSUM_MASK        GENMASK(31, 16)

/* RDES2 (write back format) */
#define RDES2_L3_L4_HEADER_SIZE_MASK    GENMASK(9, 0)
#define RDES2_VLAN_FILTER_STATUS        BIT15
#define RDES2_SA_FILTER_FAIL            BIT16
#define RDES2_DA_FILTER_FAIL            BIT17
#define RDES2_HASH_FILTER_STATUS        BIT18
#define RDES2_MAC_ADDR_MATCH_MASK       GENMASK(26, 19)
#define RDES2_HASH_VALUE_MATCH_MASK     GENMASK(26, 19)
#define RDES2_L3_FILTER_MATCH           BIT27
#define RDES2_L4_FILTER_MATCH           BIT28
#define RDES2_L3_L4_FILT_NB_MATCH_MASK  GENMASK(27, 26)
#define RDES2_L3_L4_FILT_NB_MATCH_SHIFT 26
#define RDES2_HL                        GENMASK(9, 0)

/* RDES3 (write back format) */
#define RDES3_PACKET_SIZE_MASK          GENMASK(14, 0)
#define RDES3_ERROR_SUMMARY             BIT15
#define RDES3_PACKET_LEN_TYPE_MASK      GENMASK(18, 16)
#define RDES3_DRIBBLE_ERROR             BIT19
#define RDES3_RECEIVE_ERROR             BIT20
#define RDES3_OVERFLOW_ERROR            BIT21
#define RDES3_RECEIVE_WATCHDOG          BIT22
#define RDES3_GIANT_PACKET              BIT23
#define RDES3_CRC_ERROR                 BIT24
#define RDES3_RDES0_VALID               BIT25
#define RDES3_RDES1_VALID               BIT26
#define RDES3_RDES2_VALID               BIT27
#define RDES3_LAST_DESCRIPTOR           BIT28
#define RDES3_FIRST_DESCRIPTOR          BIT29
#define RDES3_CONTEXT_DESCRIPTOR        BIT30
#define RDES3_CONTEXT_DESCRIPTOR_SHIFT  30

/* RDES3 (read format) */
#define RDES3_BUFFER1_VALID_ADDR        BIT24
#define RDES3_BUFFER2_VALID_ADDR        BIT25
#define RDES3_INT_ON_COMPLETION_EN      BIT30

/* TDS3 use for both format (read and write back) */
#define RDES3_OWN                       BIT31

//
// MTL algorithms identifiers.
//
#define MTL_TX_ALGORITHM_WRR    0x0
#define MTL_TX_ALGORITHM_WFQ    0x1
#define MTL_TX_ALGORITHM_DWRR   0x2
#define MTL_TX_ALGORITHM_SP     0x3
#define MTL_RX_ALGORITHM_SP     0x4
#define MTL_RX_ALGORITHM_WSP    0x5

//
// RX/TX Queue Mode.
//
#define MTL_QUEUE_AVB           0x0
#define MTL_QUEUE_DCB           0x1

#define PAUSE_TIME      0xffff
//
// Flow Control defines
//
#define FLOW_OFF        0
#define FLOW_RX         1
#define FLOW_TX         2
#define FLOW_AUTO       (FLOW_TX | FLOW_RX)


#define SF_DMA_MODE 1           /* DMA STORE-AND-FORWARD Operation Mode */

/* Basic descriptor structure for normal and alternate descriptors */
typedef struct {
  UINT32 Des0;
  UINT32 Des1;
  UINT32 Des2;
  UINT32 Des3;
} DMA_DESCRIPTOR;

typedef struct {
  EFI_PHYSICAL_ADDRESS        PhysAddress;
  void                        *Mapping;
} MAP_INFO;

typedef struct {
  //DMA_DESCRIPTOR              *TxDescRing[TX_DESC_NUM];
  //DMA_DESCRIPTOR              *RxDescRing[RX_DESC_NUM];
  DMA_DESCRIPTOR              *TxDescRing;
  DMA_DESCRIPTOR              *RxDescRing;
  //EFI_PHYSICAL_ADDRESS        RxBuffer;
  UINT8                       *TxBuffer;
  UINT8                       *RxBuffer;

  // CHAR8                       TxBuffer[TX_TOTAL_BUFFER_SIZE];
  // CHAR8                       RxBuffer[RX_TOTAL_BUFFER_SIZE];
  MAP_INFO                    TxDescRingMap[TX_DESC_NUM];
  MAP_INFO                    RxDescRingMap[RX_DESC_NUM];
  MAP_INFO                    TxBufNum[TX_DESC_NUM];
  MAP_INFO                    RxBufNum[RX_DESC_NUM];
  UINT32                      TxCurrentDescriptorNum;
  UINT32                      TxNextDescriptorNum;
  UINT32                      RxCurrentDescriptorNum;
  UINT32                      RxNextDescriptorNum;
} STMMAC_DRIVER;

typedef struct {
  // Driver signature
  UINT32                                 Signature;
  EFI_HANDLE                             ControllerHandle;

  // EFI SNP protocol instances
  EFI_SIMPLE_NETWORK_PROTOCOL            Snp;
  EFI_SIMPLE_NETWORK_MODE                SnpMode;

  // EFI Snp statistics instance
  EFI_NETWORK_STATISTICS                 Stats;

  STMMAC_DRIVER                          MacDriver;
  PHY_DEVICE                             *PhyDev;
  SOPHGO_PHY_PROTOCOL                    *Phy;

  EFI_LOCK                               Lock;

  UINTN                                  RegBase;
#if 1
  // Array of the recycled transmit buffer address
  //UINT64                                 *RecycledTxBuf;
  UINT8                                  *RecycledTxBuf;

  // The maximum number of recycled buffer pointers in RecycledTxBuf
  UINT32                                 MaxRecycledTxBuf;

  // Current number of recycled buffer pointers in RecycledTxBuf
  UINT32                                 RecycledTxBufCount;

  // For TX buffer DmaUnmap
  VOID                                   *MappingTxbuf;
#endif
} SOPHGO_SIMPLE_NETWORK_DRIVER;

#define SNP_DRIVER_SIGNATURE             SIGNATURE_32('A', 'S', 'N', 'P')
#define INSTANCE_FROM_SNP_THIS(a)        CR(a, SOPHGO_SIMPLE_NETWORK_DRIVER, Snp, SNP_DRIVER_SIGNATURE)
#define SNP_TX_BUFFER_INCREASE           32
#define SNP_MAX_TX_BUFFER_NUM            65536

VOID
EFIAPI
StmmacSetUmacAddr (
  IN  EFI_MAC_ADDRESS               *MacAddress,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         RegN
  );

VOID
EFIAPI
StmmacGetMacAddr (
  OUT EFI_MAC_ADDRESS               *MacAddress,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         RegN
  );

VOID
EFIAPI
StmmacReadMacAddress (
  OUT EFI_MAC_ADDRESS               *MacAddress,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

EFI_STATUS
EFIAPI
StmmacDxeInitialization (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

EFI_STATUS
EFIAPI
StmmacDmaInit (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

EFI_STATUS
EFIAPI
StmmacSetupTxdesc (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
 );

EFI_STATUS
EFIAPI
StmmacSetupRxdesc (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

VOID
EFIAPI
StmmacStartTransmission (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

EFI_STATUS
EFIAPI
StmmacSetFilters (
  IN  UINT32                        ReceiveFilterSetting,
  IN  BOOLEAN                       Reset,
  IN  UINTN                         NumMfilter             OPTIONAL,
  IN  EFI_MAC_ADDRESS               *Mfilter               OPTIONAL,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

UINT32
EFIAPI
GenEtherCrc32 (
  IN  EFI_MAC_ADDRESS         *Mac,
  IN  UINT32                  AddrLen
  );

UINT8
EFIAPI
BitReverse (
  UINT8                       Value
  );

VOID
EFIAPI
StmmacStopTxRx (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

EFI_STATUS
EFIAPI
StmmacDmaStart (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );


VOID
EFIAPI
StmmacGetDmaStatus (
  OUT UINT32                        *IrqStat  OPTIONAL,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

VOID
EFIAPI
StmmacGetStatistic (
  IN  EFI_NETWORK_STATISTICS        *Stats,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

VOID
EFIAPI
StmmacStartAllDma (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

VOID
EFIAPI
StmmacStopAllDma (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

EFI_STATUS
EFIAPI
StmmacInitDmaEngine (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

VOID
EFIAPI
StmmacMacLinkUp (
  IN  UINT32                        Speed,
  IN  UINT32                        Duplex,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

VOID
EFIAPI
StmmacDebug (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

VOID
StmmacSetRxTailPtr (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         TailPtr,
  IN  UINT32                        Channel
  );

VOID
StmmacSetTxTailPtr (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         TailPtr,
  IN  UINT32                        Channel
  );

VOID
StmmacMtlConfiguration (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );

VOID
StmmacMacFlowControl (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        Duplex,
  IN  UINT32                        FlowCtrl
  );

EFI_STATUS
EFIAPI
PhyLinkAdjustGmacConfig (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  );
#endif // STMMAC_DXE_UTIL_H__
