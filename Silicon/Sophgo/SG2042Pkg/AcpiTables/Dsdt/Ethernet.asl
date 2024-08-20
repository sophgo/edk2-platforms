/** @file
*  Differentiated System Description Table Fields (DSDT)
*
*  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

Scope(_SB)
{
  Device (ETH0) {
    Name (_HID, "SGPH0007")
    Name (_UID, Zero)
    Name (_CCA, 0)
    Method (_STA)                                       // _STA: Device status
    {
      Return (0xF)
    }
    Method (_CRS, 0x0, NotSerialized) 
    {
      Name (RBUF, ResourceTemplate()
      {
        QWordMemory (
          ResourceConsumer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0,                       // Granularity
          0x7040026000,              // Min Base Address
          0x7040029FFF,              // Max Base Address
          0x0,                       // Translate
          0x0000004000               // Length
        )

        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 132 }

        GpioIo (Exclusive, PullUp, 0, 0, IoRestrictionOutputOnly, "\\_SB.GPI0", 0, ResourceConsumer) {27}

        ClockInput (125000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_TX_ETH0)
        ClockInput (125000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_TX_ETH0)
        ClockInput (250000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_AXI_ETH0)
        ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_PTP_REF_I_ETH0)
        ClockInput (25000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_REF_ETH0)
      })
      Return(RBUF)
    }

    Name (_DSD, Package ()  // _DSD: Device-Specific Data
    {
      ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package (2) { "compatible", Package () { "bitmain,ethernet" } },
        Package (2) { "interrupt-names", Package () { "macirq" } },
        Package (2) { "snps,multicast-filter-bins", 0 },
        Package (2) { "snps,perfect-filter-entries", 1 },
        Package (2) { "snps,txpbl", 32 },
        Package (2) { "snps,rxpbl", 32 },
        Package (2) { "snps,aal", 1 },
        Package (2) { "max-speed", 1000 },

        Package (2) { "snps,axi-config", \_SB.ETH0.AXIC },
        Package (2) { "snps,mtl-rx-config", \_SB.ETH0.MTRX },
        Package (2) { "snps,mtl-tx-config", \_SB.ETH0.MTTX },

        Package (2) { "phy-mode", "rgmii-txid" },
        Package (2) { "phy-reset-gpios", Package () { ^ETH0, 0, 0, 0 } },
        Package (2) { "phy-handle", Package () { \_SB.ETH0.MDIO.PHY0 } },

        Package (2) { "clock-names", Package () { "clk_tx", "gate_clk_tx", "stmmaceth", "ptp_ref", "gate_clk_ref" } },
      }
    })

    Name (_DMA, ResourceTemplate() {
      QWordMemory (
        ResourceProducer, ,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0,
        0x0,               // MIN
        0x1effffffff,      // MAX
        0x0000000000,      // TRA
        0x1f00000000,      // LEN
        , ,)
    })

    // AXI configuration
    Device (AXIC)
    {
      Name (_ADR, 0x0)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package () { "snps,wr_osr_lmt", 1},
          Package () { "snps,rd_osr_lmt", 2},
          Package () { "snps,blen", Package () { 4, 8, 16, 0, 0, 0, 0 } },
        }
      })
    }

    // MTL RX configuration
    Device (MTRX)
    {
      Name (_ADR, 0x0)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package() { "snps,rx-queues-to-use", 8 },
        }
      })

      Device (QUE0)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE1)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE2)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE3)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE4)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE5)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE6)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE7)
      {
        Name (_ADR, 0x0)
      }
    }

    // MTL TX configuration
    Device (MTTX)
    {
      Name (_ADR, 0x0)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package() { "snps,tx-queues-to-use", 8 },
        }
      })

      Device (QUE0)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE1)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE2)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE3)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE4)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE5)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE6)
      {
        Name (_ADR, 0x0)
      }

      Device (QUE7)
      {
        Name (_ADR, 0x0)
      }
    }

    Device (MDIO)
    {
      Name (_ADR, 0x0)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package (2) { "compatible", Package () { "snps,dwmac-mdio" } },
        }
      })

      Device (PHY0)
      {
        Name (_ADR, 0x0)
        Name (_DSD, Package () {
          ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
          Package () {
            Package (2) { "compatible", Package () { "ethernet-phy-ieee802.3-c22" } },
            Package (2) {"reg", 0},
            Package (2) {"device_type", "ethernet-phy"},
          }
        })
      }
    }
  }
}