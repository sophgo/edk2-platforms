/** @file
 *
 *  [DSDT] SD controller/card definition (SDHC)
 *
 *  Copyright (c) 2025, SOPHGO Inc. All rights reserved.
 *  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

Scope(_SB)
{
  Device (SDC0)
  {
    Name (_HID, "SOPH0005")
    Name (_UID, 0x1)
    Name (_CCA, 1)                      // _CCA: Cache Coherency Attribute
    Method (_STA)
    {
      Return(0xf)
    }
    Name (_CRS, ResourceTemplate () {   // _CRS: Current Resource Settings
      QWordMemory ( // 64-bit memory
        ResourceConsumer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                       // Granularity
        0x704002B000,              // Min Base Address
        0x704002BFFF,              // Max Base Address
        0x00000000,                // Translate
        0x00001000                 // Length
      )
      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 136 }
    })

    Name (_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
            Package () { "bus-width", 4 },
            Package () { "wp-inverted", 0x1 },
            Package () { "no-mmc", 0x1 },
            Package () { "no-sdio", 0x1  },
            Package () { "no-1-8-v", 0x1 },
            Package () { "core-clk", 100000000 }
      }
    })
  }
}
