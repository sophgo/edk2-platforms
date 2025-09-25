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
  // system controller 0
  Device (CTL0) {
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

  // system controller 1
  Device (CTL1) {
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
        0xF030010000,
        0xF030017FFF,
        0x0,
        0x8000
      )
    })
  }
}