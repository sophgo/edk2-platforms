/** @file
*  Differentiated System Description Table Fields (DSDT)
*
*  Copyright (c) 2025, SOPHGO Inc. All rights reserved.
*  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include "SG2042AcpiHeader.h"

DefinitionBlock ("DsdtTable.aml", "DSDT", 2, "SOPHGO", "2042    ",
                 EFI_ACPI_RISCV_OEM_REVISION) {
  include ("Cpu.asl")
  include ("CommonDevices.asl")
  include ("Uart.asl")
  include ("Mmc.asl")
  include ("Ethernet.asl")
  include ("Intc.asl")
  include ("Pci.asl")

  Scope (\_SB_.I2C1)
  {
    Device (FAN0) {
      Name (_HID, "PNP0C0B")
      Name (_UID, 0)
      Name (FAS0, 0)
      OperationRegion(PR0, SystemMemory, 0x703000C000, 0x20)
      Field(PR0, DWordAcc, NoLock, Preserve) {
        HLP0, 32,
        PER0, 32,
      }

      Method (_STA)
      {
        Return (0xF)
      }

      Method (_FIF, 0, NotSerialized)
      {
        Return (Package ()
        {
          0x00,
          0x00,
          0x1,
          0x80,
        })
      }

      Method (_FPS, 0, NotSerialized)
      {
        Return (Package ()
        {
          0x00,
          Package ()
          {
            3900,
            0x00,
            3000,
            0xFFFFFFFF,
            0xFFFFFFFF,
          },
          Package ()
          {
            3200,
            0x01,
            2400,
            0xFFFFFFFF,
            0xFFFFFFFF,
          },
          Package ()
          {
            2400,
            0x02,
            1800,
            0xFFFFFFFF,
            0xFFFFFFFF,
          },
          Package ()
          {
            1600,
            0x03,
            1200,
            0xFFFFFFFF,
            0xFFFFFFFF,
          },
          Package ()
          {
            800,
            0x04,
            600,
            0xFFFFFFFF,
            0xFFFFFFFF,
          }
        })
      }
      Method (_FSL, 1, NotSerialized)
      {
        Store (2000, PER0)
        Store (Arg0, HLP0)
        Store (Arg0, FAS0)
      }
      Method (_FST, 0, NotSerialized)
      {
        Return (Package ()
        {
          0,
          FAS0,
          0XFFFFFFFF
        })
      }
    }

    ThermalZone (TZ00) {
      OperationRegion(TZ0, SystemMemory, 0x7030006000, 0x1000)
      Field(TZ0, DWordAcc, NoLock, Preserve) {
        CON, 32,
        TAR, 32,
        Offset (0x10),
        DATA, 32,
        HCNT, 32,
        LCNT, 32,
        FCNH, 32,
        FCNL, 32,
        Offset (0x30),
        MASK, 32,
        INT, 32,
        RXTL, 32,
        TXTL, 32,
        Offset (0x6c),
        EN, 1
      }
      Method (_STA)
      {
        Return (0xF)
      }
      Method (_TMP, 0, Serialized)
      {
          Store (0, EN)
          Store (0x63, CON)
          Store (0x17, TAR)
          Store (0x69, HCNT)
          Store (0x7c, LCNT)
          Store (0x14, FCNH)
          Store (0x27, FCNL)
          Store (0, MASK)
          Store (0, RXTL)
          Store (1, TXTL)
          Store (1, EN)

          Store (0x404, DATA)
          Store (0x300, DATA)

          Store (DATA, Local1)
          Local1 = (Local1 & 0xff)
          if (Local1 & 0x80) {
            Local1 = Local1 - 0x100
          }
          Return (((Local1) * 10) + 2732)
      }
      Method(_AC0) { Return ( 3432 ) }
      Method(_AC1) { Return ( 3312 ) }
      Method(_AC2) { Return ( 3132 ) }
      Method(_CRT) { Return ( 3582 ) }
      Name(_AL0, Package(){FAN0})
      Name(_AL1, Package(){FAN0})
      Name(_AL2, Package(){FAN0})
      Name(_AL3, Package(){FAN0})
      Name(_AL4, Package(){FAN0})
      Name(_TZP, 200)
      Name (_STR, Unicode ("System thermal zone0"))
    }

    ThermalZone (TZ01) {
      OperationRegion(TZ1, SystemMemory, 0x7030006000, 0x1000)
      Field(TZ1, DWordAcc, NoLock, Preserve) {
        CON, 32,
        TAR, 32,
        Offset (0x10),
        DATA, 32,
        HCNT, 32,
        LCNT, 32,
        FCNH, 32,
        FCNL, 32,
        Offset (0x30),
        MASK, 32,
        INT, 32,
        RXTL, 32,
        TXTL, 32,
        Offset (0x6c),
        EN, 1
      }
      Method (_STA)
      {
        Return (0xF)
      }
      Method (_TMP, 0, Serialized)
      {
          Store (0, EN)
          Store (0x63, CON)
          Store (0x17, TAR)
          Store (0x69, HCNT)
          Store (0x7c, LCNT)
          Store (0x14, FCNH)
          Store (0x27, FCNL)
          Store (0, MASK)
          Store (0, RXTL)
          Store (1, TXTL)
          Store (1, EN)

          Store (0x405, DATA)
          Store (0x300, DATA)

          Store (DATA, Local1)
          Local1 = (Local1 & 0xff)
          if (Local1 & 0x80) {
            Local1 = Local1 - 0x100
          }
          Return (((Local1) * 10) + 2732)
      }
      Method(_AC0) { Return ( 3482 ) }
      Name(_AL0, Package(){FAN0})
      Name(_AL1, Package(){FAN0})
      Name(_AL2, Package(){FAN0})
      Name(_AL3, Package(){FAN0})
      Name(_AL4, Package(){FAN0})
      Name(_TZP, 200)
      Name (_STR, Unicode ("System thermal zone1"))
    }
  }
}
