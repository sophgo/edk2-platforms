/** @file
*
*  Copyright (c) 2023, SOPHGO Inc. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/PcdLib.h>
#include <Include/PlatformPciLib.h>

MANGO_PCI_RESOURCE mPciResource[PCIE_MAX_PORT][PCIE_MAX_LINK] = {
 { // Port 0
  /* Link 0 */
  {
    MANGO_PCIE0_SLV0_BASE,          // PciSlvAddress
    MANGO_PCIE0_LINK0_CFG_BASE,     // ConfigSpaceAddress
    0,                              // Segment
    MANGO_PCIE0_LINK0_REGION1_BASE, // BusBase
    MANGO_PCIE0_LINK0_REGION1_SIZE, // BusSize
    0,                              // IoTranslation
    MANGO_PCIE0_LINK0_REGION2_BASE, // IoBase
    MANGO_PCIE0_LINK0_REGION2_SIZE, // IoSize
    (- MANGO_PCIE0_SLV0_BASE),      // Mem32Translation
    MANGO_PCIE0_LINK0_REGION3_BASE, // Mem32Base
    MANGO_PCIE0_LINK0_REGION3_SIZE, // Mem32Size
    0,                              // Mem64Translation
    MANGO_PCIE0_LINK0_REGION4_BASE, // Mem64Base
    MANGO_PCIE0_LINK0_REGION4_SIZE  // Mem64Size
  },
  /* Link 1 */
  {
    MANGO_PCIE0_SLV1_BASE,          // PciRegAddress
    MANGO_PCIE0_LINK1_CFG_BASE,     // ConfigSpaceAddress
    1,                              // Segment
    MANGO_PCIE0_LINK1_REGION1_BASE, // BusBase
    MANGO_PCIE0_LINK1_REGION1_SIZE, // BusSize
    0,                              // IoTranslation
    MANGO_PCIE0_LINK1_REGION2_BASE, // IoBase
    MANGO_PCIE0_LINK1_REGION2_SIZE, // IoSize
    (- MANGO_PCIE0_SLV1_BASE),      // Mem32Translation
    MANGO_PCIE0_LINK1_REGION3_BASE, // Mem32Base
    MANGO_PCIE0_LINK1_REGION3_SIZE, // Mem32Size
    0,                              // Mem64Translation
    MANGO_PCIE0_LINK1_REGION4_BASE, // Mem64Base
    MANGO_PCIE0_LINK1_REGION4_SIZE  // Mem64Size
  }
 },
{ // Port 1
  /* Link 0 */
  {
    MANGO_PCIE1_SLV0_BASE,          // PciRegAddress
    MANGO_PCIE1_LINK0_CFG_BASE,     // ConfigSpaceAddress
    2,                              // Segment
    MANGO_PCIE1_LINK0_REGION1_BASE, // BusBase
    MANGO_PCIE1_LINK0_REGION1_SIZE, // BusSize
    0,                              // IoTranslation
    MANGO_PCIE1_LINK0_REGION2_BASE, // IoBase
    MANGO_PCIE1_LINK0_REGION2_SIZE, // IoSize
    (- MANGO_PCIE1_SLV0_BASE),      // Mem32Translation
    MANGO_PCIE1_LINK0_REGION3_BASE, // Mem32Base
    MANGO_PCIE1_LINK0_REGION3_SIZE, // Mem32Size
    0,                              // Mem64Translation
    MANGO_PCIE1_LINK0_REGION4_BASE, // Mem64Base
    MANGO_PCIE1_LINK0_REGION4_SIZE  // Mem64Size
  },
  /* Link 1 */
  {
    MANGO_PCIE1_SLV1_BASE,          // PciRegAddress
    MANGO_PCIE1_LINK1_CFG_BASE,     // ConfigSpaceAddress
    3,                              // Segment
    MANGO_PCIE1_LINK1_REGION1_BASE, // BusBase
    MANGO_PCIE1_LINK1_REGION1_SIZE, // BusSize
    0,                              // IoTranslation
    MANGO_PCIE1_LINK1_REGION2_BASE, // IoBase
    MANGO_PCIE1_LINK1_REGION2_SIZE, // IoSize
    (- MANGO_PCIE1_SLV1_BASE),      // Mem32Translation
    MANGO_PCIE1_LINK1_REGION3_BASE, // Mem32Base
    MANGO_PCIE1_LINK1_REGION3_SIZE, // Mem32Size
    0,                              // Mem64Translation
    MANGO_PCIE1_LINK1_REGION4_BASE, // Mem64Base
    MANGO_PCIE1_LINK1_REGION4_SIZE  // Mem64Size
  }
 }
};