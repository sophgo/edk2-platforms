/** @file
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
  Method (_OSC,4) {
    // Create DWord-adressable for Arg3 First DWORD.
    CreateDWordField(Arg3,0,CDW1)
    CreateDWordField(Arg3,4,CDW2)

    // Check for proper UUID
    If (LEqual(Arg0,ToUUID("0811B06E-4A27-44F9-8D60-3CBBC22E7B48"))) {

      If (LNotEqual(Arg1,One)) {// Unknown revision
        Or (CDW1,0x0A,CDW1)
      }
      Else {
        And (CDW2,0xC0,CDW2)
      }

      Return (Arg3)
    }
    Else {
      Or (CDW1,0x6,CDW1) // Unrecognized UUID
      Return (Arg3)
    }
  } // End _OSC

  Name (CLPI, Package () {  /* LPI for Cluster, support 1 LPI state */
    0,                      // Version
    0,                      // Level Index
    1,                      // Count
    // LPI3
    Package () LPI_PACKAGE_INIT(3500, 100, 1, 0, 100, 1, 0x1000000080000000, "RISC-V NONRET_DEFAULT")
  })

  Name (PLPI, Package () {  /* LPI for Processor, support 3 LPI states */
    0,                      // Version
    0,                      // Level Index
    3,                      // Count
    // LPI1
    Package () LPI_PACKAGE_INIT(1, 1, 1, 0, 100, 0, 0x0000000000000000, "RISC-V WFI"),

    // LPI2
    Package () LPI_PACKAGE_INIT(10, 10, 1, 0, 100, 1, 0x1000000000000000, "RISC-V RET_DEFAULT"),

    // LPI3
    Package () LPI_PACKAGE_INIT(3500, 100, 1, 0, 100, 1, 0x1000000080000000, "RISC-V NONRET_DEFAULT")
  })

  //
  // SG2042 Processor declaration
  //
  Device (CL00) {   // Cluster 0
    Name (_HID, "ACPI0010")
    Name (_UID, 0)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P000) { // SG2042 Cluster 0, core 0
      Name (_HID, "ACPI0007")
      Name (_UID, 0)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Hart ID */
        0x00, 0x00, 0x00, 0x00,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x01,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P001) { // SG2042 Cluster 0, core 1
      Name (_HID, "ACPI0007")
      Name (_UID, 1)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, /* Hart ID */
        0x00, 0x00, 0x00, 0x01,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x03,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P002) { // SG2042 Cluster 0, core 2
      Name (_HID, "ACPI0007")
      Name (_UID, 2)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, /* Hart ID */
        0x00, 0x00, 0x00, 0x02,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x05,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P003) { // SG2042 Cluster 0, core 3
      Name (_HID, "ACPI0007")
      Name (_UID, 3)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, /* Hart ID */
        0x00, 0x00, 0x00, 0x03,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x07,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL01) {   // Cluster 1
    Name (_HID, "ACPI0010")
    Name (_UID, 1)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P004) { // SG2042 Cluster 1, core 4
      Name (_HID, "ACPI0007")
      Name (_UID, 4)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, /* Hart ID */
        0x00, 0x00, 0x00, 0x04,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x09,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P005) { // SG2042 Cluster 1, core 5
      Name (_HID, "ACPI0007")
      Name (_UID, 5)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, /* Hart ID */
        0x00, 0x00, 0x00, 0x05,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x0B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P006) { // SG2042 Cluster 1, core 6
      Name (_HID, "ACPI0007")
      Name (_UID, 6)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, /* Hart ID */
        0x00, 0x00, 0x00, 0x06,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x0D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P007) { // SG2042 Cluster 1, core 7
      Name (_HID, "ACPI0007")
      Name (_UID, 7)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, /* Hart ID */
        0x00, 0x00, 0x00, 0x07,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x0F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL02) {   // Cluster 2
    Name (_HID, "ACPI0010")
    Name (_UID, 2)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P008) { // SG2042 Cluster 2, core 8
      Name (_HID, "ACPI0007")
      Name (_UID, 8)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, /* Hart ID */
        0x00, 0x00, 0x00, 0x08,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x11,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P009) { // SG2042 Cluster 2, core 9
      Name (_HID, "ACPI0007")
      Name (_UID, 9)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, /* Hart ID */
        0x00, 0x00, 0x00, 0x09,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x13,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P010) { // SG2042 Cluster 2, core 10
      Name (_HID, "ACPI0007")
      Name (_UID, 10)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, /* Hart ID */
        0x00, 0x00, 0x00, 0x0A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x15,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P011) { // SG2042 Cluster 2, core 11
      Name (_HID, "ACPI0007")
      Name (_UID, 11)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, /* Hart ID */
        0x00, 0x00, 0x00, 0x0B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x17,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL03) {   // Cluster 3
    Name (_HID, "ACPI0010")
    Name (_UID, 3)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P012) { // SG2042 Cluster 3, core 12
      Name (_HID, "ACPI0007")
      Name (_UID, 12)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, /* Hart ID */
        0x00, 0x00, 0x00, 0x0C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x19,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P013) { // SG2042 Cluster 3, core 13
      Name (_HID, "ACPI0007")
      Name (_UID, 13)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, /* Hart ID */
        0x00, 0x00, 0x00, 0x0D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x1B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P014) { // SG2042 Cluster 3, core 14
      Name (_HID, "ACPI0007")
      Name (_UID, 14)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, /* Hart ID */
        0x00, 0x00, 0x00, 0x0E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x1D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P015) { // SG2042 Cluster 3, core 15
      Name (_HID, "ACPI0007")
      Name (_UID, 15)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, /* Hart ID */
        0x00, 0x00, 0x00, 0x0F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x1F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL04) {   // Cluster 4
    Name (_HID, "ACPI0010")
    Name (_UID, 4)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P016) { // SG2042 Cluster 4, core 16
      Name (_HID, "ACPI0007")
      Name (_UID, 16)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* Hart ID */
        0x00, 0x00, 0x00, 0x10,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x21,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P017) { // SG2042 Cluster 4, core 17
      Name (_HID, "ACPI0007")
      Name (_UID, 17)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, /* Hart ID */
        0x00, 0x00, 0x00, 0x11,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x23,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P018) { // SG2042 Cluster 4, core 18
      Name (_HID, "ACPI0007")
      Name (_UID, 18)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, /* Hart ID */
        0x00, 0x00, 0x00, 0x12,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x25,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P019) { // SG2042 Cluster 4, core 19
      Name (_HID, "ACPI0007")
      Name (_UID, 19)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, /* Hart ID */
        0x00, 0x00, 0x00, 0x13,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x27,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL05) {   // Cluster 5
    Name (_HID, "ACPI0010")
    Name (_UID, 5)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P020) { // SG2042 Cluster 5, core 20
      Name (_HID, "ACPI0007")
      Name (_UID, 20)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, /* Hart ID */
        0x00, 0x00, 0x00, 0x14,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x29,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P021) { // SG2042 Cluster 5, core 21
      Name (_HID, "ACPI0007")
      Name (_UID, 21)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, /* Hart ID */
        0x00, 0x00, 0x00, 0x15,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x2B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P022) { // SG2042 Cluster 5, core 22
      Name (_HID, "ACPI0007")
      Name (_UID, 22)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, /* Hart ID */
        0x00, 0x00, 0x00, 0x16,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x2D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P023) { // SG2042 Cluster 5, core 23
      Name (_HID, "ACPI0007")
      Name (_UID, 23)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, /* Hart ID */
        0x00, 0x00, 0x00, 0x17,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x2F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL06) {   // Cluster 6
    Name (_HID, "ACPI0010")
    Name (_UID, 6)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P024) { // SG2042 Cluster 6, core 24
      Name (_HID, "ACPI0007")
      Name (_UID, 24)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, /* Hart ID */
        0x00, 0x00, 0x00, 0x18,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x31,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P025) { // SG2042 Cluster 6, core 25
      Name (_HID, "ACPI0007")
      Name (_UID, 25)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, /* Hart ID */
        0x00, 0x00, 0x00, 0x19,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x33,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P026) { // SG2042 Cluster 6, core 26
      Name (_HID, "ACPI0007")
      Name (_UID, 26)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, /* Hart ID */
        0x00, 0x00, 0x00, 0x1A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x35,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P027) { // SG2042 Cluster 6, core 27
      Name (_HID, "ACPI0007")
      Name (_UID, 27)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, /* Hart ID */
        0x00, 0x00, 0x00, 0x1B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x37,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL07) {   // Cluster 7
    Name (_HID, "ACPI0010")
    Name (_UID, 7)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P028) { // SG2042 Cluster 7, core 28
      Name (_HID, "ACPI0007")
      Name (_UID, 28)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, /* Hart ID */
        0x00, 0x00, 0x00, 0x1C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x39,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P029) { // SG2042 Cluster 7, core 29
      Name (_HID, "ACPI0007")
      Name (_UID, 29)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, /* Hart ID */
        0x00, 0x00, 0x00, 0x1D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x3B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P030) { // SG2042 Cluster 7, core 30
      Name (_HID, "ACPI0007")
      Name (_UID, 30)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, /* Hart ID */
        0x00, 0x00, 0x00, 0x1E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x3D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P031) { // SG2042 Cluster 7, core 31
      Name (_HID, "ACPI0007")
      Name (_UID, 31)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, /* Hart ID */
        0x00, 0x00, 0x00, 0x1F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x3F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL08) {   // Cluster 8
    Name (_HID, "ACPI0010")
    Name (_UID, 8)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P032) { // SG2042 Cluster 8, core 32
      Name (_HID, "ACPI0007")
      Name (_UID, 32)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, /* Hart ID */
        0x00, 0x00, 0x00, 0x20,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x41,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P033) { // SG2042 Cluster 8, core 33
      Name (_HID, "ACPI0007")
      Name (_UID, 33)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, /* Hart ID */
        0x00, 0x00, 0x00, 0x21,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x43,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P034) { // SG2042 Cluster 8, core 34
      Name (_HID, "ACPI0007")
      Name (_UID, 34)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, /* Hart ID */
        0x00, 0x00, 0x00, 0x22,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x45,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P035) { // SG2042 Cluster 8, core 35
      Name (_HID, "ACPI0007")
      Name (_UID, 35)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, /* Hart ID */
        0x00, 0x00, 0x00, 0x23,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x47,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL09) {   // Cluster 9
    Name (_HID, "ACPI0010")
    Name (_UID, 9)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P036) { // SG2042 Cluster 9, core 36
      Name (_HID, "ACPI0007")
      Name (_UID, 36)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, /* Hart ID */
        0x00, 0x00, 0x00, 0x24,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x49,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P037) { // SG2042 Cluster 9, core 37
      Name (_HID, "ACPI0007")
      Name (_UID, 37)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, /* Hart ID */
        0x00, 0x00, 0x00, 0x25,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x4B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P038) { // SG2042 Cluster 9, core 38
      Name (_HID, "ACPI0007")
      Name (_UID, 38)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, /* Hart ID */
        0x00, 0x00, 0x00, 0x26,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x4D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P039) { // SG2042 Cluster 9, core 39
      Name (_HID, "ACPI0007")
      Name (_UID, 39)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, /* Hart ID */
        0x00, 0x00, 0x00, 0x27,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x4F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL10) {   // Cluster 10
    Name (_HID, "ACPI0010")
    Name (_UID, 10)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P040) { // SG2042 Cluster 10, core 40
      Name (_HID, "ACPI0007")
      Name (_UID, 40)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, /* Hart ID */
        0x00, 0x00, 0x00, 0x28,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x51,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P041) { // SG2042 Cluster 10, core 41
      Name (_HID, "ACPI0007")
      Name (_UID, 41)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, /* Hart ID */
        0x00, 0x00, 0x00, 0x29,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x53,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P042) { // SG2042 Cluster 10, core 42
      Name (_HID, "ACPI0007")
      Name (_UID, 42)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, /* Hart ID */
        0x00, 0x00, 0x00, 0x2A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x55,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P043) { // SG2042 Cluster 10, core 43
      Name (_HID, "ACPI0007")
      Name (_UID, 43)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, /* Hart ID */
        0x00, 0x00, 0x00, 0x2B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x57,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL11) {   // Cluster 11
    Name (_HID, "ACPI0010")
    Name (_UID, 11)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P044) { // SG2042 Cluster 11, core 44
      Name (_HID, "ACPI0007")
      Name (_UID, 44)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2C, /* Hart ID */
        0x00, 0x00, 0x00, 0x2C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x59,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P045) { // SG2042 Cluster 11, core 45
      Name (_HID, "ACPI0007")
      Name (_UID, 45)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2D, /* Hart ID */
        0x00, 0x00, 0x00, 0x2D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x5B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P046) { // SG2042 Cluster 11, core 46
      Name (_HID, "ACPI0007")
      Name (_UID, 46)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2E, /* Hart ID */
        0x00, 0x00, 0x00, 0x2E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x5D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P047) { // SG2042 Cluster 0, core 47
      Name (_HID, "ACPI0007")
      Name (_UID, 47)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2F, /* Hart ID */
        0x00, 0x00, 0x00, 0x2F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x5F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL12) {   // Cluster 12
    Name (_HID, "ACPI0010")
    Name (_UID, 12)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P048) { // SG2042 Cluster 12, core 48
      Name (_HID, "ACPI0007")
      Name (_UID, 48)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, /* Hart ID */
        0x00, 0x00, 0x00, 0x30,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x61,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P049) { // SG2042 Cluster 12, core 49
      Name (_HID, "ACPI0007")
      Name (_UID, 49)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, /* Hart ID */
        0x00, 0x00, 0x00, 0x31,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x63,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P050) { // SG2042 Cluster 12, core 50
      Name (_HID, "ACPI0007")
      Name (_UID, 50)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, /* Hart ID */
        0x00, 0x00, 0x00, 0x32,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x65,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P051) { // SG2042 Cluster 12, core 51
      Name (_HID, "ACPI0007")
      Name (_UID, 51)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, /* Hart ID */
        0x00, 0x00, 0x00, 0x33,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x67,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL13) {   // Cluster 13
    Name (_HID, "ACPI0010")
    Name (_UID, 13)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P052) { // SG2042 Cluster 13, core 52
      Name (_HID, "ACPI0007")
      Name (_UID, 52)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, /* Hart ID */
        0x00, 0x00, 0x00, 0x34,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x69,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P053) { // SG2042 Cluster 13, core 53
      Name (_HID, "ACPI0007")
      Name (_UID, 53)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, /* Hart ID */
        0x00, 0x00, 0x00, 0x35,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x6B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P054) { // SG2042 Cluster 13, core 54
      Name (_HID, "ACPI0007")
      Name (_UID, 54)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, /* Hart ID */
        0x00, 0x00, 0x00, 0x36,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x6D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P055) { // SG2042 Cluster 13, core 55
      Name (_HID, "ACPI0007")
      Name (_UID, 55)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, /* Hart ID */
        0x00, 0x00, 0x00, 0x37,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x6F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL14) {   // Cluster 14
    Name (_HID, "ACPI0010")
    Name (_UID, 14)

    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P056) { // SG2042 Cluster 14, core 56
      Name (_HID, "ACPI0007")
      Name (_UID, 56)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, /* Hart ID */
        0x00, 0x00, 0x00, 0x38,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x71,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P057) { // SG2042 Cluster 14, core 57
      Name (_HID, "ACPI0007")
      Name (_UID, 57)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, /* Hart ID */
        0x00, 0x00, 0x00, 0x39,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x73,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P058) { // SG2042 Cluster 14, core 58
      Name (_HID, "ACPI0007")
      Name (_UID, 58)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, /* Hart ID */
        0x00, 0x00, 0x00, 0x3A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x75,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P059) { // SG2042 Cluster 14, core 59
      Name (_HID, "ACPI0007")
      Name (_UID, 59)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, /* Hart ID */
        0x00, 0x00, 0x00, 0x3B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x77,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL15) {   // Cluster 15
    Name (_HID, "ACPI0010")
    Name (_UID, 15)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P060) { // SG2042 Cluster 15, core 60
      Name (_HID, "ACPI0007")
      Name (_UID, 60)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, /* Hart ID */
        0x00, 0x00, 0x00, 0x3C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x79,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P061) { // SG2042 Cluster 15, core 61
      Name (_HID, "ACPI0007")
      Name (_UID, 61)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3D, /* Hart ID */
        0x00, 0x00, 0x00, 0x3D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x7B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P062) { // SG2042 Cluster 15, core 62
      Name (_HID, "ACPI0007")
      Name (_UID, 62)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, /* Hart ID */
        0x00, 0x00, 0x00, 0x3E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x7D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P063) { // SG2042 Cluster 15, core 63
      Name (_HID, "ACPI0007")
      Name (_UID, 63)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, /* Hart ID */
        0x00, 0x00, 0x00, 0x3F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x7F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL16) {   // Cluster 16
    Name (_HID, "ACPI0010")
    Name (_UID, 16)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P064) { // SG2042 Cluster 16, core 64
      Name (_HID, "ACPI0007")
      Name (_UID, 64)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, /* Hart ID */
        0x00, 0x00, 0x00, 0x40,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x81,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P065) { // SG2042 Cluster 16, core 65
      Name (_HID, "ACPI0007")
      Name (_UID, 65)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, /* Hart ID */
        0x00, 0x00, 0x00, 0x41,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x83,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P066) { // SG2042 Cluster 16, core 66
      Name (_HID, "ACPI0007")
      Name (_UID, 66)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, /* Hart ID */
        0x00, 0x00, 0x00, 0x42,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x85,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P067) { // SG2042 Cluster 16, core 67
      Name (_HID, "ACPI0007")
      Name (_UID, 67)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, /* Hart ID */
        0x00, 0x00, 0x00, 0x43,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x87,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL17) {   // Cluster 17
    Name (_HID, "ACPI0010")
    Name (_UID, 17)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P068) { // SG2042 Cluster 17, core 68
      Name (_HID, "ACPI0007")
      Name (_UID, 68)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, /* Hart ID */
        0x00, 0x00, 0x00, 0x44,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x89,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P069) { // SG2042 Cluster 17, core 69
      Name (_HID, "ACPI0007")
      Name (_UID, 69)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45, /* Hart ID */
        0x00, 0x00, 0x00, 0x45,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x8B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P070) { // SG2042 Cluster 17, core 70
      Name (_HID, "ACPI0007")
      Name (_UID, 70)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, /* Hart ID */
        0x00, 0x00, 0x00, 0x46,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x8D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P071) { // SG2042 Cluster 17, core 71
      Name (_HID, "ACPI0007")
      Name (_UID, 71)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, /* Hart ID */
        0x00, 0x00, 0x00, 0x47,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x8F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL18) {   // Cluster 18
    Name (_HID, "ACPI0010")
    Name (_UID, 18)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P072) { // SG2042 Cluster 18, core 72
      Name (_HID, "ACPI0007")
      Name (_UID, 72)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, /* Hart ID */
        0x00, 0x00, 0x00, 0x48,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x91,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P073) { // SG2042 Cluster 18, core 73
      Name (_HID, "ACPI0007")
      Name (_UID, 73)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, /* Hart ID */
        0x00, 0x00, 0x00, 0x49,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x93,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P074) { // SG2042 Cluster 18, core 74
      Name (_HID, "ACPI0007")
      Name (_UID, 74)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4A, /* Hart ID */
        0x00, 0x00, 0x00, 0x4A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x95,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P075) { // SG2042 Cluster 18, core 75
      Name (_HID, "ACPI0007")
      Name (_UID, 75)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4B, /* Hart ID */
        0x00, 0x00, 0x00, 0x4B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x97,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL19) {   // Cluster 19
    Name (_HID, "ACPI0010")
    Name (_UID, 19)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P076) { // SG2042 Cluster 19, core 76
      Name (_HID, "ACPI0007")
      Name (_UID, 76)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, /* Hart ID */
        0x00, 0x00, 0x00, 0x4C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x99,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P077) { // SG2042 Cluster 19, core 77
      Name (_HID, "ACPI0007")
      Name (_UID, 77)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4D, /* Hart ID */
        0x00, 0x00, 0x00, 0x4D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x9B,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P078) { // SG2042 Cluster 19, core 78
      Name (_HID, "ACPI0007")
      Name (_UID, 78)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E, /* Hart ID */
        0x00, 0x00, 0x00, 0x4E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x9D,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P079) { // SG2042 Cluster 19, core 79
      Name (_HID, "ACPI0007")
      Name (_UID, 79)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4F, /* Hart ID */
        0x00, 0x00, 0x00, 0x4F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0x9F,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL20) {   // Cluster 20
    Name (_HID, "ACPI0010")
    Name (_UID, 20)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P080) { // SG2042 Cluster 20, core 80
      Name (_HID, "ACPI0007")
      Name (_UID, 80)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, /* Hart ID */
        0x00, 0x00, 0x00, 0x50,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xA1,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P081) { // SG2042 Cluster 20, core 81
      Name (_HID, "ACPI0007")
      Name (_UID, 81)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, /* Hart ID */
        0x00, 0x00, 0x00, 0x51,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xA3,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P082) { // SG2042 Cluster 20, core 82
      Name (_HID, "ACPI0007")
      Name (_UID, 82)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, /* Hart ID */
        0x00, 0x00, 0x00, 0x52,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xA5,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P083) { // SG2042 Cluster 20, core 83
      Name (_HID, "ACPI0007")
      Name (_UID, 83)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, /* Hart ID */
        0x00, 0x00, 0x00, 0x53,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xA7,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL21) {   // Cluster 21
    Name (_HID, "ACPI0010")
    Name (_UID, 21)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P084) { // SG2042 Cluster 21, core 84
      Name (_HID, "ACPI0007")
      Name (_UID, 84)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x54, /* Hart ID */
        0x00, 0x00, 0x00, 0x54,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xA9,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P085) { // SG2042 Cluster 21, core 85
      Name (_HID, "ACPI0007")
      Name (_UID, 85)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, /* Hart ID */
        0x00, 0x00, 0x00, 0x55,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xAB,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P086) { // SG2042 Cluster 21, core 86
      Name (_HID, "ACPI0007")
      Name (_UID, 86)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x56, /* Hart ID */
        0x00, 0x00, 0x00, 0x56,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xAD,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P087) { // SG2042 Cluster 21, core 87
      Name (_HID, "ACPI0007")
      Name (_UID, 87)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x57, /* Hart ID */
        0x00, 0x00, 0x00, 0x57,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xAF,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL22) {   // Cluster 22
    Name (_HID, "ACPI0010")
    Name (_UID, 22)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P088) { // SG2042 Cluster 22, core 88
      Name (_HID, "ACPI0007")
      Name (_UID, 88)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, /* Hart ID */
        0x00, 0x00, 0x00, 0x58,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xB1,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P089) { // SG2042 Cluster 22, core 89
      Name (_HID, "ACPI0007")
      Name (_UID, 89)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59, /* Hart ID */
        0x00, 0x00, 0x00, 0x59,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xB3,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P090) { // SG2042 Cluster 22, core 90
      Name (_HID, "ACPI0007")
      Name (_UID, 90)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, /* Hart ID */
        0x00, 0x00, 0x00, 0x5A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xB5,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P091) { // SG2042 Cluster 22, core 91
      Name (_HID, "ACPI0007")
      Name (_UID, 91)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5B, /* Hart ID */
        0x00, 0x00, 0x00, 0x5B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xB7,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL23) {   // Cluster 23
    Name (_HID, "ACPI0010")
    Name (_UID, 23)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P092) { // SG2042 Cluster 23, core 92
      Name (_HID, "ACPI0007")
      Name (_UID, 92)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, /* Hart ID */
        0x00, 0x00, 0x00, 0x5C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xB9,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P093) { // SG2042 Cluster 23, core 93
      Name (_HID, "ACPI0007")
      Name (_UID, 93)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5D, /* Hart ID */
        0x00, 0x00, 0x00, 0x5D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xBB,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P094) { // SG2042 Cluster 23, core 94
      Name (_HID, "ACPI0007")
      Name (_UID, 94)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5E, /* Hart ID */
        0x00, 0x00, 0x00, 0x5E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xBD,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P095) { // SG2042 Cluster 23, core 95
      Name (_HID, "ACPI0007")
      Name (_UID, 95)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, /* Hart ID */
        0x00, 0x00, 0x00, 0x5F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xBF,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL24) {   // Cluster 24
    Name (_HID, "ACPI0010")
    Name (_UID, 24)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P096) { // SG2042 Cluster 24, core 96
      Name (_HID, "ACPI0007")
      Name (_UID, 96)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, /* Hart ID */
        0x00, 0x00, 0x00, 0x60,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xC1,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P097) { // SG2042 Cluster 24, core 97
      Name (_HID, "ACPI0007")
      Name (_UID, 97)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, /* Hart ID */
        0x00, 0x00, 0x00, 0x61,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xC3,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P098) { // SG2042 Cluster 24, core 98
      Name (_HID, "ACPI0007")
      Name (_UID, 98)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, /* Hart ID */
        0x00, 0x00, 0x00, 0x62,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xC5,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P099) { // SG2042 Cluster 24, core 99
      Name (_HID, "ACPI0007")
      Name (_UID, 99)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, /* Hart ID */
        0x00, 0x00, 0x00, 0x63,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xC7,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL25) {   // Cluster 25
    Name (_HID, "ACPI0010")
    Name (_UID, 25)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P100) { // SG2042 Cluster 25, core 100
      Name (_HID, "ACPI0007")
      Name (_UID, 100)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, /* Hart ID */
        0x00, 0x00, 0x00, 0x64,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xC9,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P101) { // SG2042 Cluster 25, core 101
      Name (_HID, "ACPI0007")
      Name (_UID, 101)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, /* Hart ID */
        0x00, 0x00, 0x00, 0x65,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xCB,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P102) { // SG2042 Cluster 25, core 102
      Name (_HID, "ACPI0007")
      Name (_UID, 102)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, /* Hart ID */
        0x00, 0x00, 0x00, 0x66,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xCD,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P103) { // SG2042 Cluster 25, core 103
      Name (_HID, "ACPI0007")
      Name (_UID, 103)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, /* Hart ID */
        0x00, 0x00, 0x00, 0x67,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xCF,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL26) {   // Cluster 26
    Name (_HID, "ACPI0010")
    Name (_UID, 26)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P104) { // SG2042 Cluster 26, core 104
      Name (_HID, "ACPI0007")
      Name (_UID, 104)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, /* Hart ID */
        0x00, 0x00, 0x00, 0x68,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xD1,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P105) { // SG2042 Cluster 26, core 105
      Name (_HID, "ACPI0007")
      Name (_UID, 105)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69, /* Hart ID */
        0x00, 0x00, 0x00, 0x69,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xD3,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P106) { // SG2042 Cluster 26, core 106
      Name (_HID, "ACPI0007")
      Name (_UID, 106)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6A, /* Hart ID */
        0x00, 0x00, 0x00, 0x6A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xD5,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P107) { // SG2042 Cluster 26, core 107
      Name (_HID, "ACPI0007")
      Name (_UID, 107)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6B, /* Hart ID */
        0x00, 0x00, 0x00, 0x6B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xD7,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL27) {   // Cluster 27
    Name (_HID, "ACPI0010")
    Name (_UID, 27)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P108) { // SG2042 Cluster 27, core 108
      Name (_HID, "ACPI0007")
      Name (_UID, 108)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6C, /* Hart ID */
        0x00, 0x00, 0x00, 0x6C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xD9,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P109) { // SG2042 Cluster 27, core 109
      Name (_HID, "ACPI0007")
      Name (_UID, 109)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6D, /* Hart ID */
        0x00, 0x00, 0x00, 0x6D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xDB,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P110) { // SG2042 Cluster 27, core 110
      Name (_HID, "ACPI0007")
      Name (_UID, 110)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6E, /* Hart ID */
        0x00, 0x00, 0x00, 0x6E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xDD,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P111) { // SG2042 Cluster 27, core 111
      Name (_HID, "ACPI0007")
      Name (_UID, 111)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, /* Hart ID */
        0x00, 0x00, 0x00, 0x6F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xDF,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL28) {   // Cluster 28
    Name (_HID, "ACPI0010")
    Name (_UID, 28)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P112) { // SG2042 Cluster 28, core 112
      Name (_HID, "ACPI0007")
      Name (_UID, 112)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, /* Hart ID */
        0x00, 0x00, 0x00, 0x70,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xE1,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P113) { // SG2042 Cluster 28, core 113
      Name (_HID, "ACPI0007")
      Name (_UID, 113)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x71, /* Hart ID */
        0x00, 0x00, 0x00, 0x71,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xE3,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P114) { // SG2042 Cluster 28, core 114
      Name (_HID, "ACPI0007")
      Name (_UID, 114)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, /* Hart ID */
        0x00, 0x00, 0x00, 0x72,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xE5,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P115) { // SG2042 Cluster 28, core 115
      Name (_HID, "ACPI0007")
      Name (_UID, 115)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, /* Hart ID */
        0x00, 0x00, 0x00, 0x73,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xE7,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL29) {   // Cluster 29
    Name (_HID, "ACPI0010")
    Name (_UID, 29)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P116) { // SG2042 Cluster 29, core 116
      Name (_HID, "ACPI0007")
      Name (_UID, 116)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, /* Hart ID */
        0x00, 0x00, 0x00, 0x74,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xE9,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P117) { // SG2042 Cluster 29, core 117
      Name (_HID, "ACPI0007")
      Name (_UID, 117)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x75, /* Hart ID */
        0x00, 0x00, 0x00, 0x75,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xEB,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P118) { // SG2042 Cluster 29, core 118
      Name (_HID, "ACPI0007")
      Name (_UID, 118)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, /* Hart ID */
        0x00, 0x00, 0x00, 0x76,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xED,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P119) { // SG2042 Cluster 29, core 119
      Name (_HID, "ACPI0007")
      Name (_UID, 119)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77, /* Hart ID */
        0x00, 0x00, 0x00, 0x77,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xEF,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL30) {   // Cluster 30
    Name (_HID, "ACPI0010")
    Name (_UID, 30)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P120) { // SG2042 Cluster 30, core 120
      Name (_HID, "ACPI0007")
      Name (_UID, 120)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, /* Hart ID */
        0x00, 0x00, 0x00, 0x78,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xF1,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P121) { // SG2042 Cluster 30, core 121
      Name (_HID, "ACPI0007")
      Name (_UID, 121)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79, /* Hart ID */
        0x00, 0x00, 0x00, 0x79,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xF3,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P122) { // SG2042 Cluster 30, core 122
      Name (_HID, "ACPI0007")
      Name (_UID, 122)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7A, /* Hart ID */
        0x00, 0x00, 0x00, 0x7A,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xF5,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P123) { // SG2042 Cluster 30, core 123
      Name (_HID, "ACPI0007")
      Name (_UID, 123)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7B, /* Hart ID */
        0x00, 0x00, 0x00, 0x7B,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xF7,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }

  Device (CL31) {   // Cluster 31
    Name (_HID, "ACPI0010")
    Name (_UID, 31)
    Method (_LPI, 0, NotSerialized) {
      Return (\_SB.CLPI)
    }

    Device (P124) { // SG2042 Cluster 31, core 124
      Name (_HID, "ACPI0007")
      Name (_UID, 124)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, /* Hart ID */
        0x00, 0x00, 0x00, 0x7C,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xF9,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P125) { // SG2042 Cluster 31, core 125
      Name (_HID, "ACPI0007")
      Name (_UID, 125)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7D, /* Hart ID */
        0x00, 0x00, 0x00, 0x7D,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xFB,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P126) { // SG2042 Cluster 31, core 126
      Name (_HID, "ACPI0007")
      Name (_UID, 126)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, /* Hart ID */
        0x00, 0x00, 0x00, 0x7E,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xFD,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }

    Device (P127) { // SG2042 Cluster 31, core 127
      Name (_HID, "ACPI0007")
      Name (_UID, 127)
      Name (_STA, 0xF)

      Name (_MAT, Buffer() {
        0x18, 0x24, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, /* Hart ID */
        0x00, 0x00, 0x00, 0x7F,                         /* AcpiProcessorUid */
        0x00, 0x00, 0x00, 0xFF,                         /* External Interrupt Controller ID */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* IMSIC Base address */
        0x00, 0x00, 0x00, 0x00
      })

      Name (_CPC, Package()
        CPPC_PACKAGE_INIT (0x1000000000000005, 0x100000000000000D, 20, 6, 5, 2, 1, 20)
      )

      Name (_PSD, Package () {
        Package ()
          PSD_INIT (0)
      })

      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.PLPI)
      }
    }
  }
}
