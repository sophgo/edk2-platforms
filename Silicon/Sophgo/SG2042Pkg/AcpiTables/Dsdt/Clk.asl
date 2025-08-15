/** @file
*  Differentiated System Description Table Fields (DSDT)
*
*  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include "SophgoClock.h"

Scope(_SB)
{
    // CGI
    Device (CGI0) {
      Name (_HID, "SGPH0022")
      Name (_UID, 0x0)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package (2) { "compatible", Package () { "fixed-clock" } },
          Package (2) { "clock-frequency", 25000000 },
          Package (2) { "clock-output-names", Package () { "cgi" } },
        }
      })
    }

    // pll clock
    Device (MPLL) {
      Name (_HID, "SGPH0023")
      Name (_UID, 0x0)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package (2) { "compatible", Package () { "mango, pll-clock" } },
          Package (2) { "id", 0x0 },
          Package (2) { "mode", 0x00 },
          Package (2) { "subctrl-syscon", Package() { \_SB.SCTL } },
          Package (2) { "clocks", Package() { \_SB.CGI0 } },
          Package (2) { "clock-output-names", Package () { "mpll_clock" } },
        }
      })
    }

    Device (FPLL) {
      Name (_HID, "SGPH0023")
      Name (_UID, 0x1)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package (2) { "compatible", Package () { "mango, pll-clock" } },
          Package (2) { "id", 0x3 },
          Package (2) { "mode", 0x00 },
          Package (2) { "subctrl-syscon", Package() { \_SB.SCTL } },
          Package (2) { "clocks", Package() { \_SB.CGI0 } },
          Package (2) { "clock-output-names", Package () { "fpll_clock" } },
        }
      })
    }

    Device (DPL0) {
      Name (_HID, "SGPH0023")
      Name (_UID, 0x2)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package (2) { "compatible", Package () { "mango, pll-clock" } },
          Package (2) { "id", 0x4 },
          Package (2) { "mode", 0x00 },
          Package (2) { "subctrl-syscon", Package() { \_SB.SCTL } },
          Package (2) { "clocks", Package() { \_SB.CGI0 } },
          Package (2) { "clock-output-names", Package () { "dpll0_clock" } },
        }
      })
    }

    Device (DPL1) {
      Name (_HID, "SGPH0023")
      Name (_UID, 0x3)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package (2) { "compatible", Package () { "mango, pll-clock" } },
          Package (2) { "id", 0x5 },
          Package (2) { "mode", 0x00 },
          Package (2) { "subctrl-syscon", Package() { \_SB.SCTL } },
          Package (2) { "clocks", Package() { \_SB.CGI0 } },
          Package (2) { "clock-output-names", Package () { "dpll1_clock" } },
        }
      })
    }

    Device (DIV0) {
      Name (_HID, "SGPH0024")
      Name (_UID, 0x0)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package (2) { "compatible", Package () { "mango, pll-child-clock" } },
          Package (2) { "id", 0x0 },
          Package (2) { "subctrl-syscon", Package() { \_SB.SCTL } },
        }
      })
    }

    Device (MUX0) {
      Name (_HID, "SGPH0025")
      Name (_UID, 0x0)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package (2) { "compatible", Package () { "mango, pll-mux-clock" } },
          Package (2) { "id", 0x2 },
          Package (2) { "subctrl-syscon", Package() { \_SB.SCTL } },
        }
      })
    }

    Device (RATE) {
      Name (_HID, "SGPH0026")
      Name (_UID, 0x0)
      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
          Package (2) { "compatible", Package () { "mango, clk-default-rates" } },
          Package (2) { "subctrl-syscon", Package() { \_SB.SCTL } },
        }
      })

      Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
          ClockInput (2000000000, 1, Hz, Variable, "\\_SB.MPLL", 0)
          ClockInput (1000000000, 1, Hz, Variable, "\\_SB.FPLL", 0)

          ClockInput (2000000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_RP_CPU_NORMAL_1)
          ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_50M_A53)
          ClockInput (1000000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_50M_A53)
          ClockInput (500000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_UART_500M)
          ClockInput (200000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_AHB_LPC)
          ClockInput (25000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_EFUSE)
          ClockInput (125000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_TX_ETH0)
          ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_PTP_REF_I_ETH0)
          ClockInput (25000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_REF_ETH0)
          ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_EMMC)
          ClockInput (25000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_SD)
          ClockInput (100000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_TOP_AXI0)
          ClockInput (250000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_TOP_AXI_HSPERI)
          ClockInput (1000000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_AXI_DDR_1)
          ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_DIV_TIMER1)
          ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_DIV_TIMER2)
          ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_DIV_TIMER3)
          ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_DIV_TIMER4)
          ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_DIV_TIMER5)
          ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_DIV_TIMER6)
          ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_DIV_TIMER7)
          ClockInput (50000000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_DIV_TIMER8)
          ClockInput (100000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_100K_EMMC)
          ClockInput (100000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_100K_SD)
          ClockInput (100000, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_FPLL_GPIO_DB)

          ClockInput (2000000001, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_MPLL_RP_CPU_NORMAL_0)
          ClockInput (1000000001, 1, Hz, Variable, "\\_SB.DIV0", DIV_CLK_MPLL_AXI_DDR_0)
        })
        Return (RBUF)
      }
    }
}