/** @file
*  Differentiated System Description Table Fields (DSDT)
*
*  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
*  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

Scope(_SB)
{
  // DesignWare FUART
  Device(URT0) {
    Name(_HID, "SGPH0003")              // _HID: Hardware ID
    Name(_CID, "HISI0031")              // _CID: Compatible ID
    Name(_UID, 0)
    Name(_CCA, 1)                       // _CCA: Cache Coherency Attribute
    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "clock-frequency", 500000000 },
        Package () { "reg-shift", 2 },
        Package () { "reg-io-width", 4 },
//        Package () { "clock-names", Package () { "baudclk", "apb_pclk" } },
      }
    })
    Name (_CRS, ResourceTemplate () {   // _CRS: Current Resource Settings
      QWordMemory ( // 64-bit memory
            ResourceConsumer, PosDecode,
            MinFixed, MaxFixed,
            NonCacheable, ReadWrite,
            0x0,                       // Granularity
            0x7030000000,              // Min Base Address
            0x7030000FFF,              // Max Base Address
            0x00000000,                // Translate
            0x00001000                 // Length
          )
      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 41 }

      // ClockInput (500000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_UART_500M)
      // ClockInput (250000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_APB_UART)
    })
    Method (_STA) {
      Return (0xF)
    }
  }

  Device(URT1) {
    Name(_HID, "SGPH0003")              // _HID: Hardware ID
    Name(_CID, "HISI0031")              // _CID: Compatible ID
    Name(_UID, 1)
    Name(_CCA, 1)                       // _CCA: Cache Coherency Attribute
    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "clock-frequency", 500000000 },
        Package () { "reg-shift", 2 },
        Package () { "reg-io-width", 4 },
       //  Package () { "clock-names", Package () { "baudclk", "apb_pclk" } },
      }
    })
    Name (_CRS, ResourceTemplate () {   // _CRS: Current Resource Settings
      QWordMemory ( // 64-bit memory
            ResourceConsumer, PosDecode,
            MinFixed, MaxFixed,
            NonCacheable, ReadWrite,
            0x0,                       // Granularity
            0x7030001000,              // Min Base Address
            0x7030001FFF,              // Max Base Address
            0x00000000,                // Translate
            0x00001000                 // Length
          )
      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 42 }

     // ClockInput (500000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_UART_500M)
     //  ClockInput (250000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_APB_UART)
    })
    Method (_STA) {
      Return (0xF)
    }
  }

  Device(URT2) {
    Name(_HID, "SGPH0003")              // _HID: Hardware ID
    Name(_CID, "HISI0031")              // _CID: Compatible ID
    Name(_UID, 2)
    Name(_CCA, 1)                       // _CCA: Cache Coherency Attribute
    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "clock-frequency", 500000000 },
        Package () { "reg-shift", 2 },
        Package () { "reg-io-width", 4 },
      //  Package () { "clock-names", Package () { "baudclk", "apb_pclk" } },
      }
    })
    Name (_CRS, ResourceTemplate () {   // _CRS: Current Resource Settings
      QWordMemory ( // 64-bit memory
            ResourceConsumer, PosDecode,
            MinFixed, MaxFixed,
            NonCacheable, ReadWrite,
            0x0,                       // Granularity
            0x7030002000,              // Min Base Address
            0x7030002FFF,              // Max Base Address
            0x00000000,                // Translate
            0x00001000                 // Length
          )
      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 43 }

     // ClockInput (500000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_UART_500M)
     // ClockInput (250000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_APB_UART)
    })
    Method (_STA) {
      Return (0xF)
    }
  }

  Device(URT3) {
    Name(_HID, "SGPH0003")              // _HID: Hardware ID
    Name(_CID, "HISI0031")              // _CID: Compatible ID
    Name(_UID, 3)
    Name(_CCA, 1)                       // _CCA: Cache Coherency Attribute
    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "clock-frequency", 500000000 },
        Package () { "reg-shift", 2 },
        Package () { "reg-io-width", 4 },
       //  Package () { "clock-names", Package () { "baudclk", "apb_pclk" } },
      }
    })
    Name (_CRS, ResourceTemplate () {   // _CRS: Current Resource Settings
      QWordMemory ( // 64-bit memory
            ResourceConsumer, PosDecode,
            MinFixed, MaxFixed,
            NonCacheable, ReadWrite,
            0x0,                       // Granularity
            0x7030003000,              // Min Base Address
            0x7030003FFF,              // Max Base Address
            0x00000000,                // Translate
            0x00001000                 // Length
          )
      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 44 }

     // ClockInput (500000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_UART_500M)
     //  ClockInput (250000000, 1, Hz, Variable, "\\_SB.DIV0", GATE_CLK_APB_UART)
    })
    Method (_STA) {
      Return (0xF)
    }
  }
}

