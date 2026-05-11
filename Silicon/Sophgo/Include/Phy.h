/** @file
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __ETH_PHY_H__
#define __ETH_PHY_H__

#define SOPHGO_PHY_PROTOCOL_GUID { 0x9A94120C, 0xE250, 0x48B5, { 0x8B, 0x53, 0x1B, 0x7E, 0x2F, 0x64, 0x13, 0x3F } }

typedef struct _SOPHGO_PHY_PROTOCOL SOPHGO_PHY_PROTOCOL;

//
// Generic MII registers.
//
#define MII_BMCR                0x00    /* Basic mode control register */
#define MII_BMSR                0x01    /* Basic mode status register  */
#define MII_PHYSID1             0x02    /* PHYS ID 1                   */
#define MII_PHYSID2             0x03    /* PHYS ID 2                   */
#define MII_ADVERTISE           0x04    /* Advertisement control reg   */
#define MII_LPA                 0x05    /* Link partner ability reg    */
#define MII_EXPANSION           0x06    /* Expansion register          */
#define MII_CTRL1000            0x09    /* 1000BASE-T control          */
#define MII_STAT1000            0x0a    /* 1000BASE-T status           */
#define MII_MMD_CTRL            0x0d    /* MMD Access Control Register */
#define MII_MMD_DATA            0x0e    /* MMD Access Data Register */
#define MII_ESTATUS             0x0f    /* Extended Status             */
#define MII_DCOUNTER            0x12    /* Disconnect counter          */
#define MII_FCSCOUNTER          0x13    /* False carrier counter       */
#define MII_NWAYTEST            0x14    /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER         0x15    /* Receive error counter       */
#define MII_SREVISION           0x16    /* Silicon revision            */
#define MII_RESV1               0x17    /* Reserved...                 */
#define MII_LBRERROR            0x18    /* Lpback, rx, bypass error    */
#define MII_PHYADDR             0x19    /* PHY address                 */
#define MII_RESV2               0x1a    /* Reserved...                 */
#define MII_TPISTATUS           0x1b    /* TPI status for 10mbps       */
#define MII_NCONFIG             0x1c    /* Network interface config    */

//
// Basic mode control register.
//
#define BMCR_RESV               0x003f  /* Unused...                   */
#define BMCR_SPEED1000          0x0040  /* MSB of Speed (1000)         */
#define BMCR_CTST               0x0080  /* Collision test              */
#define BMCR_FULLDPLX           0x0100  /* Full duplex                 */
#define BMCR_ANRESTART          0x0200  /* Auto negotiation restart    */
#define BMCR_ISOLATE            0x0400  /* Isolate data paths from MII */
#define BMCR_PDOWN              0x0800  /* Enable low power state      */
#define BMCR_ANENABLE           0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100           0x2000  /* Select 100Mbps              */
#define BMCR_LOOPBACK           0x4000  /* TXD loopback bits           */
#define BMCR_RESET              0x8000  /* Reset to default state      */
#define BMCR_SPEED10            0x0000  /* Select 10Mbps               */

//
// Basic mode status register.
//
#define BMSR_ERCAP              0x0001  /* Ext-reg capability          */
#define BMSR_JCD                0x0002  /* Jabber detected             */
#define BMSR_LSTATUS            0x0004  /* Link status                 */
#define BMSR_ANEGCAPABLE        0x0008  /* Able to do auto-negotiation */
#define BMSR_RFAULT             0x0010  /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE       0x0020  /* Auto-negotiation complete   */
#define BMSR_RESV               0x00c0  /* Unused...                   */
#define BMSR_ESTATEN            0x0100  /* Extended Status in R15      */
#define BMSR_100HALF2           0x0200  /* Can do 100BASE-T2 HDX       */
#define BMSR_100FULL2           0x0400  /* Can do 100BASE-T2 FDX       */
#define BMSR_10HALF             0x0800  /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL             0x1000  /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF            0x2000  /* Can do 100mbps, half-duplex */
#define BMSR_100FULL            0x4000  /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4           0x8000  /* Can do 100mbps, 4k packets  */

//
// Advertisement control register.
//
#define ADVERTISE_SLCT          0x001f  /* Selector bits               */
#define ADVERTISE_CSMA          0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL     0x0020  /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040  /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080  /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100  /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400  /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800  /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */

#define ADVERTISE_FULL          (ADVERTISE_100FULL | ADVERTISE_10FULL | \
                                 ADVERTISE_CSMA)
#define ADVERTISE_ALL           (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                                 ADVERTISE_100HALF | ADVERTISE_100FULL)

//
// Link partner ability register.
//
#define LPA_SLCT                0x001f  /* Same as advertise selector  */
#define LPA_10HALF              0x0020  /* Can do 10mbps half-duplex   */
#define LPA_1000XFULL           0x0020  /* Can do 1000BASE-X full-duplex */
#define LPA_10FULL              0x0040  /* Can do 10mbps full-duplex   */
#define LPA_1000XHALF           0x0040  /* Can do 1000BASE-X half-duplex */
#define LPA_100HALF             0x0080  /* Can do 100mbps half-duplex  */
#define LPA_1000XPAUSE          0x0080  /* Can do 1000BASE-X pause     */
#define LPA_100FULL             0x0100  /* Can do 100mbps full-duplex  */
#define LPA_1000XPAUSE_ASYM     0x0100  /* Can do 1000BASE-X pause asym*/
#define LPA_100BASE4            0x0200  /* Can do 100mbps 4k packets   */
#define LPA_PAUSE_CAP           0x0400  /* Can pause                   */
#define LPA_PAUSE_ASYM          0x0800  /* Can pause asymetrically     */
#define LPA_RESV                0x1000  /* Unused...                   */
#define LPA_RFAULT              0x2000  /* Link partner faulted        */
#define LPA_LPACK               0x4000  /* Link partner acked us       */
#define LPA_NPAGE               0x8000  /* Next page bit               */

#define LPA_DUPLEX              (LPA_10FULL | LPA_100FULL)
#define LPA_100                 (LPA_100FULL | LPA_100HALF | LPA_100BASE4)

//
// Expansion register for auto-negotiation.
//
#define EXPANSION_NWAY          0x0001  /* Can do N-way auto-nego      */
#define EXPANSION_LCWP          0x0002  /* Got new RX page code word   */
#define EXPANSION_ENABLENPAGE   0x0004  /* This enables npage words    */
#define EXPANSION_NPCAPABLE     0x0008  /* Link partner supports npage */
#define EXPANSION_MFAULTS       0x0010  /* Multiple faults detected    */
#define EXPANSION_RESV          0xffe0  /* Unused...                   */

#define ESTATUS_1000_XFULL      0x8000  /* Can do 1000BX Full */
#define ESTATUS_1000_XHALF      0x4000  /* Can do 1000BX Half */
#define ESTATUS_1000_TFULL      0x2000  /* Can do 1000BT Full          */
#define ESTATUS_1000_THALF      0x1000  /* Can do 1000BT Half          */

//
// N-way test register.
//
#define NWAYTEST_RESV1          0x00ff  /* Unused...                   */
#define NWAYTEST_LOOPBACK       0x0100  /* Enable loopback for N-way   */
#define NWAYTEST_RESV2          0xfe00  /* Unused...                   */

//
// 1000BASE-T Control register
//
#define ADVERTISE_1000FULL      0x0200  /* Advertise 1000BASE-T full duplex */
#define ADVERTISE_1000HALF      0x0100  /* Advertise 1000BASE-T half duplex */
#define CTL1000_AS_MASTER       0x0800
#define CTL1000_ENABLE_MASTER   0x1000

//
// 1000BASE-T Status register
//
#define LPA_1000LOCALRXOK       0x2000  /* Link partner local receiver status */
#define LPA_1000REMRXOK         0x1000  /* Link partner remote receiver status */
#define LPA_1000FULL            0x0800  /* Link partner 1000BASE-T full duplex */
#define LPA_1000HALF            0x0400  /* Link partner 1000BASE-T half duplex */

//
// Flow control flags
//
#define FLOW_CTRL_TX            0x01
#define FLOW_CTRL_RX            0x02

//
// MMD Access Control register fields
//
#define MII_MMD_CTRL_DEVAD_MASK 0x1f    /* Mask MMD DEVAD*/
#define MII_MMD_CTRL_ADDR       0x0000  /* Address */
#define MII_MMD_CTRL_NOINCR     0x4000  /* no post increment */
#define MII_MMD_CTRL_INCR_RDWT  0x8000  /* post increment on reads & writes */
#define MII_MMD_CTRL_INCR_ON_WT 0xC000  /* post increment on writes only */

#define PHY_TIMEOUT             500

/* The forced speed, 10Mb, 100Mb, gigabit, 2.5Gb, 10GbE. */
#define SPEED_10                10
#define SPEED_100               100
#define SPEED_1000              1000
#define SPEED_2500              2500
#define SPEED_5000              5000
#define SPEED_10000             10000
#define SPEED_14000             14000
#define SPEED_20000             20000
#define SPEED_25000             25000
#define SPEED_40000             40000
#define SPEED_50000             50000
#define SPEED_56000             56000
#define SPEED_100000            100000
#define SPEED_200000            200000
#define SPEED_400000            400000
#define SPEED_800000            800000

#define SPEED_UNKNOWN           -1

/* Duplex, half or full. */
#define DUPLEX_HALF             0x00
#define DUPLEX_FULL             0x01
#define DUPLEX_UNKNOWN          0xff

/**
 * enum PHY_INTERFACE - Interface Mode definitions
 *
 * @PHY_INTERFACE_MODE_NA: Not Applicable - don't touch
 * @PHY_INTERFACE_MODE_INTERNAL: No interface, MAC and PHY combined
 * @PHY_INTERFACE_MODE_MII: Media-independent interface
 * @PHY_INTERFACE_MODE_GMII: Gigabit media-independent interface
 * @PHY_INTERFACE_MODE_SGMII: Serial gigabit media-independent interface
 * @PHY_INTERFACE_MODE_TBI: Ten Bit Interface
 * @PHY_INTERFACE_MODE_REVMII: Reverse Media Independent Interface
 * @PHY_INTERFACE_MODE_RMII: Reduced Media Independent Interface
 * @PHY_INTERFACE_MODE_REVRMII: Reduced Media Independent Interface in PHY role
 * @PHY_INTERFACE_MODE_RGMII: Reduced gigabit media-independent interface
 * @PHY_INTERFACE_MODE_RGMII_ID: RGMII with Internal RX+TX delay
 * @PHY_INTERFACE_MODE_RGMII_RXID: RGMII with Internal RX delay
 * @PHY_INTERFACE_MODE_RGMII_TXID: RGMII with Internal RX delay
 * @PHY_INTERFACE_MODE_RTBI: Reduced TBI
 * @PHY_INTERFACE_MODE_SMII: Serial MII
 * @PHY_INTERFACE_MODE_XGMII: 10 gigabit media-independent interface
 * @PHY_INTERFACE_MODE_XLGMII:40 gigabit media-independent interface
 * @PHY_INTERFACE_MODE_MOCA: Multimedia over Coax
 * @PHY_INTERFACE_MODE_PSGMII: Penta SGMII
 * @PHY_INTERFACE_MODE_QSGMII: Quad SGMII
 * @PHY_INTERFACE_MODE_TRGMII: Turbo RGMII
 * @PHY_INTERFACE_MODE_100BASEX: 100 BaseX
 * @PHY_INTERFACE_MODE_1000BASEX: 1000 BaseX
 * @PHY_INTERFACE_MODE_2500BASEX: 2500 BaseX
 * @PHY_INTERFACE_MODE_5GBASER: 5G BaseR
 * @PHY_INTERFACE_MODE_RXAUI: Reduced XAUI
 * @PHY_INTERFACE_MODE_XAUI: 10 Gigabit Attachment Unit Interface
 * @PHY_INTERFACE_MODE_10GBASER: 10G BaseR
 * @PHY_INTERFACE_MODE_25GBASER: 25G BaseR
 * @PHY_INTERFACE_MODE_USXGMII:  Universal Serial 10GE MII
 * @PHY_INTERFACE_MODE_10GKR: 10GBASE-KR - with Clause 73 AN
 * @PHY_INTERFACE_MODE_QUSGMII: Quad Universal SGMII
 * @PHY_INTERFACE_MODE_1000BASEKX: 1000Base-KX - with Clause 73 AN
 * @PHY_INTERFACE_MODE_10G_QXGMII: 10G-QXGMII - 4 ports over 10G USXGMII
 * @PHY_INTERFACE_MODE_MAX: Book keeping
 *
 * Describes the interface between the MAC and PHY.
 */
typedef enum {
  PHY_INTERFACE_MODE_NA,
  PHY_INTERFACE_MODE_INTERNAL,
  PHY_INTERFACE_MODE_MII,
  PHY_INTERFACE_MODE_GMII,
  PHY_INTERFACE_MODE_SGMII,
  PHY_INTERFACE_MODE_TBI,
  PHY_INTERFACE_MODE_REVMII,
  PHY_INTERFACE_MODE_RMII,
  PHY_INTERFACE_MODE_REVRMII,
  PHY_INTERFACE_MODE_RGMII,
  PHY_INTERFACE_MODE_RGMII_ID,
  PHY_INTERFACE_MODE_RGMII_RXID,
  PHY_INTERFACE_MODE_RGMII_TXID,
  PHY_INTERFACE_MODE_RTBI,
  PHY_INTERFACE_MODE_SMII,
  PHY_INTERFACE_MODE_XGMII,
  PHY_INTERFACE_MODE_XLGMII,
  PHY_INTERFACE_MODE_MOCA,
  PHY_INTERFACE_MODE_PSGMII,
  PHY_INTERFACE_MODE_QSGMII,
  PHY_INTERFACE_MODE_TRGMII,
  PHY_INTERFACE_MODE_100BASEX,
  PHY_INTERFACE_MODE_1000BASEX,
  PHY_INTERFACE_MODE_2500BASEX,
  PHY_INTERFACE_MODE_5GBASER,
  PHY_INTERFACE_MODE_RXAUI,
  PHY_INTERFACE_MODE_XAUI,
  /* 10GBASE-R, XFI, SFI - single lane 10G Serdes */
  PHY_INTERFACE_MODE_10GBASER,
  PHY_INTERFACE_MODE_25GBASER,
  PHY_INTERFACE_MODE_USXGMII,
  /* 10GBASE-KR - with Clause 73 AN */
  PHY_INTERFACE_MODE_10GKR,
  PHY_INTERFACE_MODE_QUSGMII,
  PHY_INTERFACE_MODE_1000BASEKX,
  PHY_INTERFACE_MODE_10G_QXGMII,
  PHY_INTERFACE_MODE_MAX,
} PHY_INTERFACE;

/*
 * struct PHY_DEVICE - An instance of a PHY
 * @State: State of the PHY for management purposes
 * @Speed: Current link speed
 * @Duplex: Current duplex
 * @Interface: enum PHY_INTERFACE value
 */
typedef struct {
  UINT32          PhyAddr;
  UINT32          CurrentLink;
  UINT32          PhyOldLink;
  BOOLEAN         LinkUp;
  UINT32          Duplex;
  BOOLEAN         AutoNegotiation;
  UINT32          Speed;
  PHY_INTERFACE   Interface;
  UINT32          RxInternalDelayPs;
  UINT32          TxInternalDelayPs;
  //PHY_STATE       State;
} PHY_DEVICE;

/*
 * Before calling SOPHGO_PHY_STATUS driver should request PHY_DEVICE structure by
 * calling SOPHGO_PHY_INIT. Pointer to that needs to be provided as an argument to
 * SOPHGO_PHY_STATUS.
 */
typedef
EFI_STATUS
(EFIAPI *SOPHGO_PHY_STATUS) (
  IN CONST SOPHGO_PHY_PROTOCOL  *This,
  IN OUT PHY_DEVICE             *PhyDev
  );

/*
 * SOPHGO_PHY_INIT allocates PhyDev and provides driver with pointer via **PhyDev.
 * After it becomes unnecessary, PhyDev should be freed by a driver (or it will
 * get freed at ExitBootServices).
 */
typedef
EFI_STATUS
(EFIAPI *SOPHGO_PHY_INIT) (
  IN CONST SOPHGO_PHY_PROTOCOL  *This,
  IN PHY_INTERFACE              PhyInterface,
  IN OUT PHY_DEVICE             **PhyDev
  );

struct _SOPHGO_PHY_PROTOCOL {
  SOPHGO_PHY_STATUS             Status;
  SOPHGO_PHY_INIT               Init;
};

extern EFI_GUID gSophgoPhyProtocolGuid;

#endif // __ETH_PHY_H__
