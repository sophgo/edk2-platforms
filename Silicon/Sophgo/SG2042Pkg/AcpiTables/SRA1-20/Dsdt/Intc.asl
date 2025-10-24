/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2025, SOPHGO Inc. All rights reserved.
  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>

**/

Scope(_SB)
{
  Device (PLIC) {        // thead,c900-plic
    Name(_HID, "RSCV0001")
    Name(_CRS, ResourceTemplate() {
      QWordMemory (
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                         // Granularity
        0x7090000000,                // Min Base Address
        0x7093FFFFFF,                // Max Base Address
        0x0000000000,                // Translate
        0x0004000000                 // Length
      )
    })

    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package ()
      {
        Package () { "reg-names", "control" },
        Package () { "riscv,max-priority", 7 },
        Package () { "riscv,ndev", 448 },
      }
    })

    Method(_GSB) {
      Return (0x0) // Global System Interrupt Base for I/O APIC starts at 0
    }
  }

  Device (INT0) {
    Name(_HID, "SOPH0001")

    Name (_DEP, Package () {
      \_SB.PLIC
    })

    Name(_CRS, ResourceTemplate() {
      QWordMemory ( //sta
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                         // Granularity
        0x70300102E0,                // Min Base Address
        0x70300102E3,                // Max Base Address
        0x0000000000,                // Translate
        0x0000000004                 // Length
      )

      QWordMemory ( //set
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                         // Granularity
        0x7030010300,                // Min Base Address
        0x7030010303,                // Max Base Address
        0x0000000000,                // Translate
        0x0000000004                 // Length
      )

      QWordMemory ( // clr
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                         // Granularity
        0x7030010304,                // Min Base Address
        0x7030010307,                // Max Base Address
        0x0000000000,                // Translate
        0x0000000004                 // Length
      )

      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive,,,) {
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
        76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87,
        88, 89, 90, 91, 92, 93, 94, 95,
      }
    })

    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package ()
      {
        Package () { "top-intc-id", 0 },
        Package () { "for-msi", 1 },
        Package () { "reg-bitwidth", 32 },
      }
    })
  }

  Device (INT1) {
    Name(_HID, "SOPH0001")

    Name (_DEP, Package () {
      \_SB.PLIC
    })

    Name(_CRS, ResourceTemplate() {
      QWordMemory ( //sta
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                         // Granularity
        0xF0300102E0,                // Min Base Address
        0xF0300102E3,                // Max Base Address
        0x0000000000,                // Translate
        0x0000000004                 // Length
      )

      QWordMemory ( //set
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                         // Granularity
        0xF030010300,                // Min Base Address
        0xF030010303,                // Max Base Address
        0x0000000000,                // Translate
        0x0000000004                 // Length
      )

      QWordMemory ( // clr
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        NonCacheable, ReadWrite,
        0x0,                         // Granularity
        0xF030010304,                // Min Base Address
        0xF030010307,                // Max Base Address
        0x0000000000,                // Translate
        0x0000000004                 // Length
      )

      Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive,,,) {
        288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299,
        300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311,
        312, 313, 314, 315, 316, 317, 318, 319,
      }
    })

    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package ()
      {
        Package () { "top-intc-id", 1 },
        Package () { "for-msi", 1 },
        Package () { "reg-bitwidth", 32 },
      }
    })
  }
}
