/** @file
*  Differentiated System Description Table Fields (DSDT)
*
*  Copyright (c) 2024, Sophgo. All rights reserved.
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
    Method (_STA)
    {
      Return (0xF)
    }

    /** Clock Divider Control Register of Divider for clk_tx_eth0
    *   
    *   bit[0]: Divider Reset Control
    *   bit[3]: Select Divide Factor from Register
    *   bit[32-16]: Clock Divider Factor
    **/
    OperationRegion(CDDR, SystemMemory, 0x70500020FC, 4)
    Field(CDDR, DWordAcc, NoLock, Preserve) {
      TDIV, 32
    }

    Method (_CLK, 1, NotSerialized)
    {
      Divide(2000000000, Arg0,, Local0)
      ShiftLeft(Local0, 0x10, Local0)
      Or(Local0, 0x9, Local0)
      Store(Local0, TDIV)
    }

    Name (_DEP, Package () {
      \_SB.GPI0
    })

     Name (_CRS, ResourceTemplate () {   // _CRS: Current Resource Settings
      QWordMemory (
          ResourceConsumer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0,                       // Granularity
          0x7030006000,              // Min Base Address
          0x7030009FFF,              // Max Base Address
          0x0,                       // Translate
          0x0000004000               // Length
        )
      QWordMemory (
          ResourceConsumer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0,                       // Granularity
          0x7050000000,              // Min Base Address
          0x70500003FF,              // Max Base Address
          0x0,                       // Translate
          0x0000000400               // Length
        )
      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 296 }
      GpioIo (Exclusive, PullUp, 0, 0, IoRestrictionOutputOnly, "\\_SB.GPI0", 0, ResourceConsumer) { 28 }
    })
    
    Name (_DSD, Package ()  // _DSD: Device-Specific Data
    {
      ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package (2) { "interrupt-names", Package () { "macirq" } },
        Package (2) { "snps,multicast-filter-bins", 0 },
        Package (2) { "snps,perfect-filter-entries", 1 },
        Package (2) { "snps,txpbl", 32 },
        Package (2) { "snps,rxpbl", 32 },
        Package (2) { "snps,aal", 1 },
        Package (2) { "snps,tso", 1 },
        Package (2) { "sophgo,gmac", 1 },
        Package (2) { "sophgo,gmac-no-rxdelay", 1 },

        Package (2) { "snps,axi-config", \_SB.ETH0.AXIC },
        Package (2) { "snps,mtl-rx-config", \_SB.ETH0.MTRX },
        Package (2) { "snps,mtl-tx-config", \_SB.ETH0.MTTX },

        Package (2) { "phy-mode", "rgmii-id" },
        Package (2) { "phy-reset-gpios", Package () { ^ETH0, 0, 0, 0 } },
        Package (2) { "phy-handle", Package () { \_SB.ETH0.MDIO.PHY0 } },
      }
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
          Package() { "snps,rx-queues-to-use", 4 },
          Package() { "snps,rx-sched-wsp", 1 },
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
    }

    // MTL TX configuration
    Device (MTTX)
    {
      Name (_ADR, 0x0)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package() { "snps,tx-queues-to-use", 4 },
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
            Package (2) {"device_type", "ethernet-phy"},
            Package (2) {"rx-internal-delay-ps", 2050},
            Package (2) {"led0_config", 0x7},
            Package (2) {"led1_config", 0x19f0},
            Package (2) {"led2_config", 0x0},
          }
        })
      }
    }
  }
}