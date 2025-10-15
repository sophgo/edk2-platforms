/** @file
*
*  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
*  Copyright (c) 2025, SOPHGO Inc. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

/*
  See ACPI 6.5 Spec, 6.2.11, PCI Firmware Spec 3.0, 4.5
*/
#define LNK_DEVICE(Unique_Id, Link_Name, irq)                                  \
  Device(Link_Name) {                                                          \
      Name(_HID, EISAID("PNP0C0F"))                                            \
      Name(_UID, Unique_Id)                                                    \
      Name(_PRS, ResourceTemplate() {                                          \
          Interrupt(ResourceProducer, Level, ActiveHigh, Exclusive) { irq }    \
      })                                                                       \
      Method (_CRS, 0) { Return (_PRS) }                                       \
      Method (_SRS, 1) { }                                                     \
      Method (_DIS) { }                                                        \
  }

#define PRT_ENTRY(Address, Pin, Link)                                                             \
        Package (4) {                                                                             \
            Address,    /* uses the same format as _ADR */                                        \
            Pin,        /* The PCI pin number of the device (0-INTA, 1-INTB, 2-INTC, 3-INTD). */  \
            Link,       /* Interrupt allocated via Link device. */                                \
            Zero        /* global system interrupt number (no used) */                            \
          }

#define ROOT_PRT_ENTRY(Pin, Link)   PRT_ENTRY(0x0000FFFF, Pin, Link)
                                                    // Device 0 for Bridge.

#define PCI_OSC_SUPPORT() \
  Name(SUPP, Zero) /* PCI _OSC Support Field value */ \
  Name(CTRL, Zero) /* PCI _OSC Control Field value */ \
  Method(_OSC,4) { \
    If(LEqual(Arg0,ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766"))) { \
      /* Create DWord-adressable fields from the Capabilities Buffer */ \
      CreateDWordField(Arg3,0,CDW1) \
      CreateDWordField(Arg3,4,CDW2) \
      CreateDWordField(Arg3,8,CDW3) \
      /* Save Capabilities DWord2 & 3 */ \
      Store(CDW2,SUPP) \
      Store(CDW3,CTRL) \
      /* Only allow native hot plug control if OS supports: */ \
      /* ASPM */ \
      /* Clock PM */ \
      /* MSI/MSI-X */ \
      If(LNotEqual(And(SUPP, 0x16), 0x16)) { \
        And(CTRL,0x1E,CTRL) \
      }\
      \
      /* Always allow native PME, AER */ \
      /* Never allow SHPC (no SHPC controller in this system)*/ \
      And(CTRL,0x1D,CTRL) \
      If(LNotEqual(Arg1,One)) { /* Unknown revision */ \
        Or(CDW1,0x08,CDW1) \
      } \
      \
      If(LNotEqual(CDW3,CTRL)) { /* Capabilities bits were masked */ \
        Or(CDW1,0x10,CDW1) \
      } \
      \
      /* Update DWORD3 in the buffer */ \
      Store(CTRL,CDW3) \
      Return(Arg3) \
    } Else { \
      Or(CDW1,4,CDW1) /* Unrecognized UUID */ \
      Return(Arg3) \
    } \
  } // End _OSC

Scope(_SB)
{

  LNK_DEVICE(1, RCA0, 123)
  LNK_DEVICE(2, RCA1, 346)

  // PCIe Root bus
  Device (PCI0)
  {
    Name (_HID, "PNP0A08") // PCI Express Root Bridge
    Name (_CID, "PNP0A03") // Compatible PCI Root Bridge
    Name (_SEG, 0)         // Segment of this Root complex
    Name (_BBN, 0)         // Base Bus Number
    Name (_CCA, 0)

    Name (_DEP, Package () {
      \_SB.INT0
    })

    Method (_PXM, 0, NotSerialized) {
      Return (3)  // proximity domain 3
    }

    Name(_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "cdns,max-outbound-regions", 16 },
        Package () { "cdns,no-bar-match-nbits", 48 },
        Package () { "vendor-id", 0x1E30 },
        Package () { "device-id", 0x2042 },
        Package () { "pcie-id", 0x1 },
        Package () { "link-id", 0x0 },
        Package () { "top-intc-used", 1 },
        Package () { "top-intc-id", 0 },
        Package () { "msix-supported", 1 },
      }
    })

    // PCI Routing Table
    Name(_PRT, Package() {
      ROOT_PRT_ENTRY(0, RCA0),   // INTA
      ROOT_PRT_ENTRY(1, RCA0),   // INTB
      ROOT_PRT_ENTRY(2, RCA0),   // INTC
      ROOT_PRT_ENTRY(3, RCA0),   // INTD
    })

    Name (_CRS, ResourceTemplate () { // Root complex resources
      WordBusNumber ( // Bus numbers assigned to this root
        ResourceProducer, MinFixed, MaxFixed, PosDecode,
        0x0,                // AddressGranularity
        0x0,                // AddressMinimum - Minimum Bus Number
        0x3f,               // AddressMaximum - Maximum Bus Number
        0x0,                  // AddressTranslation - Set to 0
        0x40                // RangeLength - Number of Busses
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0,                // Granularity
        0x00D0000000,       // Min Base Address
        0x00DFFFFFFF,       // Max Base Address
        0x4800000000,       // Translate
        0x0010000000        // Length
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0,                // Granularity
        0x00E0000000,       // Min Base Address
        0x00FFFFFFFF,       // Max Base Address
        0x4800000000,       // Translate
        0x0020000000        // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0,                // Granularity
        0x4A00000000,       // Min Base Address pci address
        0x4BFFFFFFFF,       // Max Base Address
        0x0,                // Translate
        0x0200000000        // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0,                // Granularity
        0x4900000000,       // Min Base Address pci address
        0x49FFFFFFFF,       // Max Base Address
        0x0,                // Translate
        0x0100000000        // Length
      )
      QWordIO (
        ResourceProducer, MinFixed, MaxFixed,
        PosDecode, EntireRange,
        0x0000000000,       // Granularity
        0x00C0000000,       // Min Base Address
        0x00C03FFFFF,       // Max Base Address
        0x4800000000,       // Translate
        0x0000400000        // Length
      )
    })

    PCI_OSC_SUPPORT()

    Name (_DMA, ResourceTemplate() {
      QWordMemory (ResourceProducer,
        ,
        MinFixed,
        MaxFixed,
        Cacheable,
        ReadWrite,
        0x0,
        0x0,          // MIN
        0x1effffffff, // MAX
        0x0,          // TRA
        0x1f00000000, // LEN
        ,
        ,
        )
    })

    Device (RES0)
    {
      Name (_HID, "SOPH0000" /* PNP Motherboard Resources */)  // _HID: Hardware ID
      Name (_UID, 0x0)  // Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
        QWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
        0x0000000000,                       // Granularity
        0x7062000000,                       // Range Minimum
        0x7063FFFFFF,                       // Range Maximum
        0x0000000000,                       // Translation Offset
        0x0002000000,                       // Length
        , , , AddressRangeMemory, TypeStatic)
      })
      Method (_STA) {
        Return (0xF)
      }
    }

  } // Device(PCI0)

  // PCIe Root bus
  Device (PCI1)
  {
    Name (_HID, "PNP0A08") // PCI Express Root Bridge
    Name (_CID, "PNP0A03") // Compatible PCI Root Bridge
    Name (_SEG, 1)         // Segment of this Root complex
    Name (_BBN, 0x80)      // Base Bus Number
    Name (_CCA, 0)

    Name (_DEP, Package () {
      \_SB.INT1
    })


    Method (_PXM, 0, NotSerialized) {
      Return (7)  // proximity domain 7
    }

    Name (_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        // Package () { "interrupt-parent" , Package() { \_SB.INTC }},
        Package () { "cdns,max-outbound-regions", 16 },
        Package () { "cdns,no-bar-match-nbits", 48 },
        Package () { "vendor-id", 0x1E30 },
        Package () { "device-id", 0x2042 },
        Package () { "pcie-id", 0x0 },
        Package () { "link-id", 0x0 },
        Package () { "top-intc-used", 1 },
        Package () { "top-intc-id", 1 },
        Package () { "msix-supported", 1 },
      }
    })

    // PCI Routing Table
    Name(_PRT, Package() {
      ROOT_PRT_ENTRY(0, RCA1),   // INTA
      ROOT_PRT_ENTRY(1, RCA1),   // INTB
      ROOT_PRT_ENTRY(2, RCA1),   // INTC
      ROOT_PRT_ENTRY(3, RCA1),   // INTD
    })

    Name (_CRS, ResourceTemplate () { // Root complex resources
      WordBusNumber ( // Bus numbers assigned to this root
        ResourceProducer, MinFixed, MaxFixed, PosDecode,
        0,                   // AddressGranularity
        0x80,                // AddressMinimum - Minimum Bus Number
        0xff,                // AddressMaximum - Maximum Bus Number
        0,                   // AddressTranslation - Set to 0
        0x80                 // RangeLength - Number of Busses
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0000000000,        // Granularity
        0x00D0000000,        // Min Base Address
        0x00DFFFFFFF,        // Max Base Address
        0xC000000000,        // Translate
        0x0010000000         // Length
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0000000000,        // Granularity
        0x00E0000000,        // Min Base Address
        0x00FFFFFFFF,        // Max Base Address
        0xC000000000,        // Translate
        0x0020000000         // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0000000000,       // Granularity
        0xC200000000,       // Min Base Address pci address
        0xC3FFFFFFFF,       // Max Base Address
        0x0000000000,       // Translate
        0x0200000000        // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0000000000,       // Granularity
        0xC100000000,       // Min Base Address pci address
        0xC1FFFFFFFF,       // Max Base Address
        0x0000000000,       // Translate
        0x0100000000        // Length
      )
      QWordIO (
        ResourceProducer, MinFixed, MaxFixed,
        PosDecode, EntireRange,
        0x0,               // Granularity
        0x00C0800000,      // Min Base Address
        0x00C0FFFFFF,      // Max Base Address
        0xC000000000,      // Translate
        0x0000800000       // Length
      )
    })

    PCI_OSC_SUPPORT()

    Name (_DMA, ResourceTemplate() {
      QWordMemory (
        ResourceProducer, ,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0,
        0x0,               // MIN
        0x1effffffff,      // MAX
        0x0000000000,      // TRA
        0x1f00000000,      // LEN
        , ,)
    })

    Device (RES1)
    {
      Name (_HID, "SOPH0000" /* PNP Motherboard Resources */)  // _HID: Hardware ID
      Name (_UID, 0x1)  // Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
        QWordMemory (
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0xF060000000,                       // Range Minimum
          0xF061FFFFFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0002000000,                       // Length
          , , , AddressRangeMemory, TypeStatic)
      })
      Method (_STA) {
        Return (0xF)
      }
    }

  } // Device(PCI1)

}

