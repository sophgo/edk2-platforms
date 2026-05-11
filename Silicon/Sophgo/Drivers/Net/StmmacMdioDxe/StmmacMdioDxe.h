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

//
// XGMAC defines.
//
#define XGMAC_MDIO_ADDR                 0x00000200
#define XGMAC_MDIO_DATA                 0x00000204
#define XGMAC_MDIO_C22P                 0x00000220

#define MII_XGMAC_SADDR                 BIT(18)
#define MII_XGMAC_CMD_SHIFT             16
#define MII_XGMAC_WRITE                 (1 << MII_XGMAC_CMD_SHIFT)
#define MII_XGMAC_READ                  (3 << MII_XGMAC_CMD_SHIFT)
#define MII_XGMAC_BUSY                  BIT(22)
#define MII_XGMAC_MAX_C22ADDR           3
#define MII_XGMAC_C22P_MASK             GENMASK(MII_XGMAC_MAX_C22ADDR, 0)
#define MII_XGMAC_PA_SHIFT              16
#define MII_XGMAC_DA_SHIFT              21

#define STMMAC_MDIO_TIMEOUT             10000   // 10000us
#define PHY_REG_MASK                    0xFFFF
#define PHY_ADDR_MASK                   0x1F

#endif // __MDIO_DXE_H__
