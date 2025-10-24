/** @file
 *
 *  Differentiated System Description Table Fields (DSDT)
 *
 *  Copyright (c) 2025, SOPHGO Inc. All rights reserved.
 *  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

Scope(_SB)
{
  Device (I2C0) {
    Name(_HID, "SOPH0003")
    Name(_UID, 0)
    Method (_STA)
    {
      Return (0xF)
    }

    Name (_CRS, ResourceTemplate ()  {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x7030005000,
        0x7030005FFF,
        0x0,
        0x1000
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 101 }
    })
    Name(SSCN, Package() { 0x3E2, 0x47D, 0 })
    Name(FMCN, Package() { 0xA4, 0x13F, 0 })
  }

  Device (I2C1) {
    Name(_HID, "SOPH0003")
    Name(_UID, 1)
    Method (_STA)
    {
      Return (0xF)
    }

    Name (_CRS, ResourceTemplate ()  {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x7030006000,
        0x7030006FFF,
        0x0,
        0x1000
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 102 }
    })
    Name(SSCN, Package() { 0x3E2, 0x47D, 0 })
    Name(FMCN, Package() { 0xA4, 0x13F, 0 })
  }

  Device (I2C2) {
    Name(_HID, "SOPH0003")
    Name(_UID, 2)
    Method (_STA)
    {
      Return (0xF)
    }

    Name (_CRS, ResourceTemplate ()  {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x7030007000,
        0x7030007FFF,
        0x0,
        0x1000
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 103 }
    })
    Name(SSCN, Package() { 0x3E2, 0x47D, 0 })
    Name(FMCN, Package() { 0xA4, 0x13F, 0 })
  }

  Device (I2C3) {
    Name(_HID, "SOPH0003")
    Name(_UID, 3)
    Method (_STA)
    {
      Return (0xF)
    }

    Name (_CRS, ResourceTemplate ()  {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x7030008000,
        0x7030008FFF,
        0x0,
        0x1000
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 104 }
    })
    Name(SSCN, Package() { 0x3E2, 0x47D, 0 })
    Name(FMCN, Package() { 0xA4, 0x13F, 0 })
  }

  // GPIO
  Device (GPI0) {
    Name(_HID, "SOPH0010")
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
        0x7030009000,
        0x70300093FF,
        0x0,
        0x400
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 96 }
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
    Name(_HID, "SOPH0010")
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
        0x703000a000,
        0x703000a3FF,
        0x0,
        0x400
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 97 }
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
    Name(_HID, "SOPH0010")
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
        0x703000b000,
        0x703000b3FF,
        0x0,
        0x400
      )
      Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive,,,) { 98 }
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
    Name(_HID, "SOPH0011")
    Name(_CRS, ResourceTemplate() {
      QWordMemory (
        ResourceConsumer,
        ,
        MinFixed,
        MaxFixed,
        NonCacheable,
        ReadWrite,
        0x0,
        0x7030010000,
        0x7030017FFF,
        0x0,
        0x8000
      )
    })
  }

}