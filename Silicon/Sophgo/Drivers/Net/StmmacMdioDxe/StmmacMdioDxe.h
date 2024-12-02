/** @file
 *
 *  STMMAC registers.
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __MDIO_DXE_H__
#define __MDIO_DXE_H__

#include <Uefi.h>

#define BIT(nr)              (1UL << (nr))
#define GENMASK(end, start)  (((1ULL << ((end) - (start) + 1)) - 1) << (start))

//
// Synopsys Core versions
//
#define GMAC4_VERSION                   0x00000110  /* GMAC4+ CORE Version */
#define DWXGMAC_CORE_2_20               0x22

//
// MII address register definitions.
//
#define MII_BUSY                        BIT0
#define MII_WRITE                       BIT1
#define MII_DATA_MASK                   GENMASK(15, 0)

//
// GMAC4 defines
//
#define GMAC_MDIO_ADDR			0x00000200
#define GMAC_MDIO_DATA			0x00000204

#define MII_GMAC4_GOC_SHIFT             2
#define MII_GMAC4_REG_ADDR_SHIFT        16
#define MII_GMAC4_WRITE                 (1 << MII_GMAC4_GOC_SHIFT)
#define MII_GMAC4_READ                  (3 << MII_GMAC4_GOC_SHIFT)
#define MII_GMAC4_C45E                  BIT(1)

#define STMMAC_MDIO_TIMEOUT             10000   // 10000us
#define PHY_REG_MASK                    0xFFFF
#define PHY_ADDR_MASK                   0x1F

#endif // __MDIO_DXE_H__
