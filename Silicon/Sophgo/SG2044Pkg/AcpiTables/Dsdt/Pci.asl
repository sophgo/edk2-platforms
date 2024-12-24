/** @file
*
*  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
*  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

/*
  See ACPI 6.5 Spec, 6.2.11, PCI Firmware Spec 3.0, 4.5
*/
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
  // PCIe Root bus
  Device (PCI0)
  {
    Name (_HID, "PNP0A08") // PCI Express Root Bridge
    Name (_CID, "PNP0A03") // Compatible PCI Root Bridge
    Name (_SEG, 0)         // Segment of this Root complex
    Name (_BBN, 0)         // Base Bus Number
    Name (_CCA, 1)

    // PCI Routing Table
    Name (_PRT, Package () {
      Package () { 0xFFFF, 0, 0, 64 },   // INTA
      Package () { 0xFFFF, 1, 0, 64 },   // INTB
      Package () { 0xFFFF, 2, 0, 64 },   // INTC
      Package () { 0xFFFF, 3, 0, 64 },   // INTD
    })

    Name (_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "interrupt-parent" , Package() { \_SB.MSI }},
      }
    })

    Name (_CRS, ResourceTemplate () { // Root complex resources
      WordBusNumber ( // Bus numbers assigned to this root
        ResourceProducer, MinFixed, MaxFixed, PosDecode,
        0x0,                // AddressGranularity
        0x0,                // AddressMinimum - Minimum Bus Number
        0xff,               // AddressMaximum - Maximum Bus Number
        0x0,                  // AddressTranslation - Set to 0
        0x100                // RangeLength - Number of Busses
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0,                // Granularity
        0x0000000000,       // Min Base Address
        0x0003FFFFFF,       // Max Base Address
        0x0000000000,       // Translate
        0x0004000000        // Length
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0,                // Granularity
        0x0004000000,       // Min Base Address
        0x0007FFFFFF,       // Max Base Address
        0x0000000000,       // Translate
        0x0004000000        // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0,                // Granularity
        0x4200000000,       // Min Base Address pci address
        0x43FFFFFFFF,       // Max Base Address
        0x0,                // Translate
        0x0200000000        // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0,                // Granularity
        0x4100000000,       // Min Base Address pci address
        0x41FFFFFFFF,       // Max Base Address
        0x0,                // Translate
        0x0100000000        // Length
      )
      QWordIO (
        ResourceProducer, MinFixed, MaxFixed,
        PosDecode, EntireRange,
        0x0000000000,       // Granularity
        0x0000000000,       // Min Base Address
        0x00001FFFFF,       // Max Base Address
        0x4010000000,       // Translate
        0x0000200000        // Length
      )
    })

    PCI_OSC_SUPPORT()

    Name (_DMA, ResourceTemplate() {
      QWordMemory (ResourceProducer,
        ,
        MinFixed,
        MaxFixed,
        Prefetchable,
        ReadWrite,
        0x0,
        0x0,          // MIN
        0x3FFFFFFFFF, // MAX
        0x0,          // TRA
        0x4000000000, // LEN
        ,
        ,
        )
    })

    Device (RES0)
    {
      Name (_HID, "SGPH0001" /* PNP Motherboard Resources */)  // _HID: Hardware ID
      Name (_UID, 0x0)  // Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
        QWordMemory ( // dbi
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C00400000,                       // Range Minimum
          0x6C00400FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000001000,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // ctrl
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C00780C00,                       // Range Minimum
          0x6C00780FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000000400,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // atu
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C00700000,                       // Range Minimum
          0x6C00703FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000004000,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // config
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x4000000000,                       // Range Minimum
          0x4000000FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000001000,                       // Length
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
    Name (_BBN, 0x0)      // Base Bus Number
    Name (_CCA, 1)

    Name (_PRT, Package (){
      Package () {0xFFFF, 0, 0, 65},         // INT_A
      Package () {0xFFFF, 1, 0, 65},         // INT_B
      Package () {0xFFFF, 2, 0, 65},         // INT_C
      Package () {0xFFFF, 3, 0, 65},         // INT_D
    })

    Name (_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "interrupt-parent" , Package() { \_SB.MSI }},
      }
    })

    Name (_CRS, ResourceTemplate () { // Root complex resources
      WordBusNumber ( // Bus numbers assigned to this root
        ResourceProducer, MinFixed, MaxFixed, PosDecode,
        0x0,                    // AddressGranularity
        0x00,                   // AddressMinimum - Minimum Bus Number
        0xFF,                   // AddressMaximum - Maximum Bus Number
        0,                      // AddressTranslation - Set to 0
        0x100                    // RangeLength - Number of Busses
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0,                 // Granularity
        0x0010000000,        // Min Base Address
        0x0013FFFFFF,        // Max Base Address
        0x0000000000,        // Translate
        0x0004000000         // Length
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0,                 // Granularity
        0x0014000000,        // Min Base Address
        0x0017FFFFFF,        // Max Base Address
        0x0000000000,        // Translate
        0x0004000000         // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0,               // Granularity
        0x4A00000000,      // Min Base Address pci address
        0x4BFFFFFFFF,      // Max Base Address
        0x0000000000,      // Translate
        0x0200000000       // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0,               // Granularity
        0x4900000000,      // Min Base Address pci address
        0x49FFFFFFFF,      // Max Base Address
        0x0000000000,      // Translate
        0x0100000000       // Length
      )
      QWordIO (
        ResourceProducer, MinFixed, MaxFixed,
        PosDecode, EntireRange,
        0x0,               // Granularity
        0x0000000000,      // Min Base Address
        0x00001FFFFF,      // Max Base Address
        0x4810000000,      // Translate
        0x0000200000       // Length
      )
    })

    PCI_OSC_SUPPORT()

    Name (_DMA, ResourceTemplate() {
      QWordMemory (ResourceProducer,
        ,
        MinFixed,
        MaxFixed,
        Prefetchable,
        ReadWrite,
        0x0,
        0x0,          // MIN
        0x3FFFFFFFFF, // MAX
        0x0,          // TRA
        0x4000000000, // LEN
        ,
        ,
        )
    })

    Device (RES1)
    {
      Name (_HID, "SGPH0001" /* PNP Motherboard Resources */)  // _HID: Hardware ID
      Name (_UID, 0x1)  // Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
        QWordMemory ( // dbi
          ResourceProducer, PosDecode, 
          MinFixed, MaxFixed, 
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C00000000,                       // Range Minimum
          0x6C00000FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000001000,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // ctrl
          ResourceProducer, PosDecode, 
          MinFixed, MaxFixed, 
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C000C0C00,                       // Range Minimum
          0x6C000C0FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000000400,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // atu
          ResourceProducer, PosDecode, 
          MinFixed, MaxFixed, 
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C00300000,                       // Range Minimum
          0x6C00303FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000004000,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // config
          ResourceProducer, PosDecode, 
          MinFixed, MaxFixed, 
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x4800000000,                       // Range Minimum
          0x4800000FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000001000,                       // Length
          , , , AddressRangeMemory, TypeStatic)
      })
      Method (_STA) {
        Return (0xF)
      }
    }

  } // Device(PCI1)

  // PCIe Root bus
  Device (PCI2)
  {
    Name (_HID, "PNP0A08") // PCI Express Root Bridge
    Name (_CID, "PNP0A03") // Compatible PCI Root Bridge
    Name (_SEG, 2)         // Segment of this Root complex
    Name (_BBN, 0x0)      // Base Bus Number
    Name (_CCA, 1)

    // PCI Routing Table
    Name (_PRT, Package () {
      Package () { 0xFFFF, 0, 0, 73 },   // INTA
      Package () { 0xFFFF, 1, 0, 73 },   // INTB
      Package () { 0xFFFF, 2, 0, 73 },   // INTC
      Package () { 0xFFFF, 3, 0, 73 },   // INTD
    })

    Name (_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "interrupt-parent" , Package() { \_SB.MSI }},
      }
    })

    Name (_CRS, ResourceTemplate () { // Root complex resources
      WordBusNumber ( // Bus numbers assigned to this root
        ResourceProducer, MinFixed, MaxFixed, PosDecode,
        0,                   // AddressGranularity
        0x00,                // AddressMinimum - Minimum Bus Number
        0xFF,                // AddressMaximum - Maximum Bus Number
        0,                   // AddressTranslation - Set to 0
        0x100                 // RangeLength - Number of Busses
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0000000000,        // Granularity
        0x0020000000,        // Min Base Address
        0x0023FFFFFF,        // Max Base Address
        0x0000000000,        // Translate
        0x0004000000         // Length
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0000000000,        // Granularity
        0x0024000000,        // Min Base Address
        0x0027FFFFFF,        // Max Base Address
        0x0000000000,        // Translate
        0x0004000000         // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0000000000,       // Granularity
        0x5200000000,       // Min Base Address pci address
        0x53FFFFFFFF,       // Max Base Address
        0x0000000000,       // Translate
        0x0200000000        // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0000000000,       // Granularity
        0x5100000000,       // Min Base Address pci address
        0x51FFFFFFFF,       // Max Base Address
        0x0000000000,       // Translate
        0x0100000000        // Length
      )
      QWordIO (
        ResourceProducer, MinFixed, MaxFixed,
        PosDecode, EntireRange,
        0x0,               // Granularity
        0x0000000000,      // Min Base Address
        0x00001FFFFF,      // Max Base Address
        0x5010000000,      // Translate
        0x0000200000       // Length
      )
    })

    PCI_OSC_SUPPORT()

    Name (_DMA, ResourceTemplate() {
      QWordMemory (ResourceProducer,
        ,
        MinFixed,
        MaxFixed,
        Prefetchable,
        ReadWrite,
        0x0,
        0x0,          // MIN
        0x3FFFFFFFFF, // MAX
        0x0,          // TRA
        0x4000000000, // LEN
        ,
        ,
        )
    })

    Device (RES2)
    {
      Name (_HID, "SGPH0001" /* PNP Motherboard Resources */)  // _HID: Hardware ID
      Name (_UID, 0x2)  // Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
        QWordMemory ( // dbi
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C04400000,                       // Range Minimum
          0x6C04400FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000001000,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // ctrl
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C04780C00,                       // Range Minimum
          0x6C04780FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000000400,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // atu
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C04700000,                       // Range Minimum
          0x6C04703FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000004000,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // config
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x5000000000,                       // Range Minimum
          0x5000000FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000001000,                       // Length
          , , , AddressRangeMemory, TypeStatic)
      })
      Method (_STA) {
        Return (0xF)
      }
    }

  } // Device(PCI2)

  // PCIe Root bus
  Device (PCI3)
  {
    Name (_HID, "PNP0A08") // PCI Express Root Bridge
    Name (_CID, "PNP0A03") // Compatible PCI Root Bridge
    Name (_SEG, 3)         // Segment of this Root complex
    Name (_BBN, 0x0)      // Base Bus Number
    Name (_CCA, 1)

    // PCI Routing Table
    Name (_PRT, Package () {
      Package () { 0xFFFF, 0, 0, 74 },   // INTA
      Package () { 0xFFFF, 1, 0, 74 },   // INTB
      Package () { 0xFFFF, 2, 0, 74 },   // INTC
      Package () { 0xFFFF, 3, 0, 74 },   // INTD
    })

    Name (_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "interrupt-parent" , Package() { \_SB.MSI }},
      }
    })

    Name (_CRS, ResourceTemplate () { // Root complex resources
      WordBusNumber ( // Bus numbers assigned to this root
        ResourceProducer, MinFixed, MaxFixed, PosDecode,
        0,                   // AddressGranularity
        0x00,                // AddressMinimum - Minimum Bus Number
        0xFF,                // AddressMaximum - Maximum Bus Number
        0,                   // AddressTranslation - Set to 0
        0x100                 // RangeLength - Number of Busses
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0000000000,        // Granularity
        0x0030000000,        // Min Base Address
        0x0033FFFFFF,        // Max Base Address
        0x0000000000,        // Translate
        0x0004000000         // Length
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0000000000,        // Granularity
        0x0034000000,        // Min Base Address
        0x0037FFFFFF,        // Max Base Address
        0x0000000000,        // Translate
        0x0004000000         // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0000000000,       // Granularity
        0x5A00000000,       // Min Base Address pci address
        0x5BFFFFFFFF,       // Max Base Address
        0x0000000000,       // Translate
        0x0200000000        // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0000000000,       // Granularity
        0x5900000000,       // Min Base Address pci address
        0x59FFFFFFFF,       // Max Base Address
        0x0000000000,       // Translate
        0x0100000000        // Length
      )
      QWordIO (
        ResourceProducer, MinFixed, MaxFixed,
        PosDecode, EntireRange,
        0x0,               // Granularity
        0x0000000000,      // Min Base Address
        0x00001FFFFF,      // Max Base Address
        0x5810000000,      // Translate
        0x0000200000       // Length
      )
    })

    PCI_OSC_SUPPORT()

    Name (_DMA, ResourceTemplate() {
      QWordMemory (ResourceProducer,
        ,
        MinFixed,
        MaxFixed,
        Prefetchable,
        ReadWrite,
        0x0,
        0x0,          // MIN
        0x3FFFFFFFFF, // MAX
        0x0,          // TRA
        0x4000000000, // LEN
        ,
        ,
        )
    })

    Device (RES3)
    {
      Name (_HID, "SGPH0001" /* PNP Motherboard Resources */)  // _HID: Hardware ID
      Name (_UID, 0x3)  // Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
        QWordMemory ( // dbi
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C04000000,                       // Range Minimum
          0x6C04000FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000001000,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // ctrl
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C040C0C00,                       // Range Minimum
          0x6C040C0FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000000400,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( //atu
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C04300000,                       // Range Minimum
          0x6C04303FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000004000,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // config
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x5800000000,                       // Range Minimum
          0x5800000FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000001000,                       // Length
          , , , AddressRangeMemory, TypeStatic)
      })
      Method (_STA) {
        Return (0xF)
      }
    }

  } // Device(PCI3)

  // PCIe Root bus
  Device (PCI4)
  {
    Name (_HID, "PNP0A08") // PCI Express Root Bridge
    Name (_CID, "PNP0A03") // Compatible PCI Root Bridge
    Name (_SEG, 0x4)         // Segment of this Root complex
    Name (_BBN, 0x0)      // Base Bus Number
    Name (_CCA, 1)

    // PCI Routing Table
    Name (_PRT, Package () {
      Package () { 0xFFFF, 0, 0, 125 },   // INTA
      Package () { 0xFFFF, 1, 0, 125 },   // INTB
      Package () { 0xFFFF, 2, 0, 125 },   // INTC
      Package () { 0xFFFF, 3, 0, 125 },   // INTD
    })

    Name (_DSD, Package () {
      ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
      Package () {
        Package () { "interrupt-parent" , Package() { \_SB.MSI }},
      }
    })

    Name (_CRS, ResourceTemplate () { // Root complex resources
      WordBusNumber ( // Bus numbers assigned to this root
        ResourceProducer, MinFixed, MaxFixed, PosDecode,
        0,                   // AddressGranularity
        0x00,                // AddressMinimum - Minimum Bus Number
        0xFF,                // AddressMaximum - Maximum Bus Number
        0,                   // AddressTranslation - Set to 0
        0x100                 // RangeLength - Number of Busses
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0000000000,        // Granularity
        0x0040000000,        // Min Base Address
        0x0043FFFFFF,        // Max Base Address
        0x0000000000,        // Translate
        0x0004000000         // Length
      )
      QWordMemory ( // 32-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0000000000,        // Granularity
        0x0044000000,        // Min Base Address
        0x0047FFFFFF,        // Max Base Address
        0x0000000000,        // Translate
        0x0004000000         // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Prefetchable, ReadWrite,
        0x0000000000,       // Granularity
        0x6200000000,       // Min Base Address pci address
        0x63FFFFFFFF,       // Max Base Address
        0x0000000000,       // Translate
        0x0200000000        // Length
      )
      QWordMemory ( // 64-bit BAR Windows
        ResourceProducer, PosDecode,
        MinFixed, MaxFixed,
        Cacheable, ReadWrite,
        0x0000000000,       // Granularity
        0x6100000000,       // Min Base Address pci address
        0x61FFFFFFFF,       // Max Base Address
        0x0000000000,       // Translate
        0x0100000000        // Length
      )
      QWordIO (
        ResourceProducer, MinFixed, MaxFixed,
        PosDecode, EntireRange,
        0x0,               // Granularity
        0x0000000000,      // Min Base Address
        0x00001FFFFF,      // Max Base Address
        0x6010000000,      // Translate
        0x0000200000       // Length
      )
    })

    PCI_OSC_SUPPORT()

    Name (_DMA, ResourceTemplate() {
      QWordMemory (ResourceProducer,
        ,
        MinFixed,
        MaxFixed,
        Prefetchable,
        ReadWrite,
        0x0,
        0x0,          // MIN
        0x3FFFFFFFFF, // MAX
        0x0,          // TRA
        0x4000000000, // LEN
        ,
        ,
        )
    })

    Device (RES4)
    {
      Name (_HID, "SGPH0001" /* PNP Motherboard Resources */)  // _HID: Hardware ID
      Name (_UID, 0x4)  // Unique ID
      Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
      {
        QWordMemory ( // dbi
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C08400000,                       // Range Minimum
          0x6C08400FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000001000,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // ctrl
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C08780C00,                       // Range Minimum
          0x6C08780FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000000400,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory ( // atu
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C08700000,                       // Range Minimum
          0x6C08703FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000004000,                       // Length
          , , , AddressRangeMemory, TypeStatic)

        QWordMemory (
          ResourceProducer, PosDecode,
          MinFixed, MaxFixed,
          NonCacheable, ReadWrite,
          0x0000000000,                       // Granularity
          0x6C00000000,                       // Range Minimum
          0x6C00000FFF,                       // Range Maximum
          0x0000000000,                       // Translation Offset
          0x0000001000,                       // Length
          , , , AddressRangeMemory, TypeStatic)
      })
      Method (_STA) {
        Return (0xF)
      }
    }

  } // Device(PCI4)
}