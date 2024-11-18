/** @file
 *
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
  // GPIO
  Device (GPI0) {
    Name(_HID, "SGPH0010")
    Name(_CID, "HISI0181")
    Name(_UID, 0)

    Name (_CRS, ResourceTemplate ()  {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x7040009000,
        0x7040009FFF,
        0x0,
        0x1000
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 26 }
    })

    Device (PRTA) {
      Name(_ADR, 0)
      Name(_UID, 0)
      Name(_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package () {"bank-name", "port0a"},
          Package () {"reg", 0},
          Package () {"snps,nr-gpios", 32},
        }
      })
    }
  }

  Device (GPI1) {
    Name(_HID, "SGPH0010")
    Name(_CID, "HISI0181")
    Name(_UID, 1)

    Name (_CRS, ResourceTemplate ()  {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x704000a000,
        0x704000aFFF,
        0x0,
        0x1000
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 27 }
    })

    Device (PRTA) {
      Name(_ADR, 0)
      Name(_UID, 0)
      Name(_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package () {"bank-name", "port0a"},
          Package () {"reg", 0},
          Package () {"snps,nr-gpios", 32},
        }
      })
    }
  }

  Device (GPI2) {
    Name(_HID, "SGPH0010")
    Name(_CID, "HISI0181")
    Name(_UID, 2)

    Name (_CRS, ResourceTemplate ()  {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x704000b000,
        0x704000bFFF,
        0x0,
        0x1000
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 28 }
    })

    Device (PRTA) {
      Name(_ADR, 0)
      Name(_UID, 0)
      Name(_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package () {"bank-name", "port0a"},
          Package () {"reg", 0},
          Package () {"snps,nr-gpios", 32},
        }
      })
    }
  }

  // system controller
  Device (SCTL) {
    Name(_HID, "SGPH0020")
    Name(_CRS, ResourceTemplate() {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x7050000000,
        0x7050007FFF,
        0x0,
        0x8000
      )
    })
  }

  //!! no size found
  Device (PCTL) {
    Name(_HID, "SGPH0021")
    Name(_CRS, ResourceTemplate() {
	  QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,        
        MaxFixed,       
        NonCacheable,  
        ReadWrite,    
        0x0,         
        0x7050001000,
        0x7050007FFF,
        0x0,
        0x7000
	  )
    })

    Name (_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "subctrl-syscon", Package() { \_SB.SCTL } },
        Package () { "top_pinctl_offset", 0x1000 },
      }
    })
  }

  // I2C for 100k
  Device (I2C0) {
    Name(_HID, "SGPH0011")
    Name(_CID, "HISI02A2")
    Name(_UID, 0)
    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () {"clock-frequency", 100000},
      }
    })
    Name (_CRS, ResourceTemplate ()  {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x7040005000,
        0x7040005FFF,
        0x0,
        0x1000
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 31 }
    })
    Name(SSCN, Package() { 0x3E2, 0x47D, 0 })  // !! not found
    Name(FMCN, Package() { 0xA4, 0x13F, 0 })   // !! not found
  }

  Device (I2C1) {
    Name(_HID, "SGPH0011")
    Name(_CID, "HISI02A2")
    Name(_UID, 1)
    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () {"clock-frequency", 100000},
      }
    })
    Name (_CRS, ResourceTemplate ()  {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x7040006000,
        0x7040006FFF,
        0x0,
        0x1000
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 32 }
    })
    Name(SSCN, Package() { 0x3E2, 0x47D, 0 })
    Name(FMCN, Package() { 0xA4, 0x13F, 0 })
  }

  Device (I2C2) {
    Name(_HID, "SGPH0011")
    Name(_CID, "HISI02A2")
    Name(_UID, 2)
    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () {"clock-frequency", 100000},
      }
    })
    Name (_CRS, ResourceTemplate ()  {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x7040007000,
        0x7040007FFF,
        0x0,
        0x1000
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 33 }
    })
    Name(SSCN, Package() { 0x3E2, 0x47D, 0 })
    Name(FMCN, Package() { 0xA4, 0x13F, 0 })
  }

  Device (I2C3) {
    Name(_HID, "SGPH0011")
    Name(_CID, "HISI02A2")
    Name(_UID, 3)
    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () {"clock-frequency", 100000},
      }
    })
    Name (_CRS, ResourceTemplate ()  {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x7040008000,
        0x7040008FFF,
        0x0,
        0x1000
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 34 }
    })
    Name(SSCN, Package() { 0x3E2, 0x47D, 0 })
    Name(FMCN, Package() { 0xA4, 0x13F, 0 })
  }

  // PWM
  Device (PWM0) {
    Name (_HID, "SGPH0005")
    Name (_UID, 0)
    Method (_STA)
    {
      Return (0xf)
    }
    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () {"base-clk", 100000000},
      }
    })
    Name (_CRS, ResourceTemplate () {   // _CRS: Current Resource Settings
      QWordMemory (
        ResourceConsumer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                       // Granularity
        0x704000C000,              // Min Base Address
        0x704000CFFF,              // Max Base Address
        0x0,                       // Translate
        0x0000001000               // Length
      )
    })

  }
}
// no spi device found, just spifmc, should put it here?
/*
  Device (SPI0) {
    Name (_HID, "SGPH0006")
    Name (_CID, "HISI0173")
    Name (_UID, 0x0)
    Method (_STA)
    {
      Return (0xf)
    }

    Name (_CRS, ResourceTemplate () {
      QWordMemory (
        ResourceConsumer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                       // Granularity
        0x7040004000,              // Min Base Address
        0x7040004FFF,              // Max Base Address
        0x0,                       // Translate
        0x0000001000               // Length
      )
      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 110 }
    })

    Name (_DSD, Package ()  // _DSD: Device-Specific Data
    {
      ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package (2) { "clock-frequency", 250000000 },
        Package (2) { "num-cs", 2 },
      }
    })
  }

  Device (SPI1) {
    Name (_HID, "SGPH0006")
    Name (_CID, "HISI0173")
    Name (_UID, 0x1)
    Method (_STA)
    {
      Return (0xf)
    }

    Name (_CRS, ResourceTemplate () {
      QWordMemory (
        ResourceConsumer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                       // Granularity
        0x7040005000,              // Min Base Address
        0x7040005FFF,              // Max Base Address
        0x0,                       // Translate
        0x0000001000               // Length
      )
      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 111 }
    })

    Name (_DSD, Package ()  // _DSD: Device-Specific Data
    {
      ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package (2) { "clock-frequency", 250000000 },
        Package (2) { "num-cs", 2 },
      }
    })
  }
}
*/
