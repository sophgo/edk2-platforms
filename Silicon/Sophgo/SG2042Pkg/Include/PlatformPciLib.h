/** @file
*
*  Copyright (c) 2023, SOPHGO Inc. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef _PLATFORM_PCI_LIB_H_
#define _PLATFORM_PCI_LIB_H_

#define PCIE_MAX_PORT         2
#define PCIE_MAX_LINK         2
#define PCIE_MAX_REGION       32

typedef enum  {
  RP_BAR_UNDEFINED = -1,
  RP_BAR0,
  RP_BAR1,
  RP_NO_BAR
} PCIE_RP_BAR;

#define MANGO_PCIE_CFG_LINK0_APB       (FixedPcdGet64 (PcdMangoPciCfgLink0ApbBase)) // 0x0
#define MANGO_PCIE_CFG_LINK1_APB       (FixedPcdGet64 (PcdMangoPciCfgLink1ApbBase)) // 0x800000
#define MANGO_PCIE_CFG_PHY_APB         (FixedPcdGet64 (PcdMangoPciCfgPhyApbBase))   // 0x1000000
#define MANGO_PCIE_CFG_MANGO_APB       (FixedPcdGet64 (PcdMangoPciCfgMangoApbBase)) // 0x1800000

#define MANGO_PCIE0_SLV0_BASE          (FixedPcdGet64 (PcdMangoPci0Slv0CfgBase)) // 0x4000000000
#define MANGO_PCIE0_SLV1_BASE          (FixedPcdGet64 (PcdMangoPci0Slv1CfgBase)) // 0x4400000000
#define MANGO_PCIE1_SLV0_BASE          (FixedPcdGet64 (PcdMangoPci1Slv0CfgBase)) // 0x4800000000
#define MANGO_PCIE1_SLV1_BASE          (FixedPcdGet64 (PcdMangoPci1Slv1CfgBase)) // 0x4c00000000

#define MANGO_PCIE0_LINK0_CFG_BASE     (FixedPcdGet64 (PcdMangoPci0Link0CfgBase)) // 0x7060000000
#define MANGO_PCIE0_LINK1_CFG_BASE     (FixedPcdGet64 (PcdMangoPci0Link1CfgBase)) // 0x7060800000
#define MANGO_PCIE1_LINK0_CFG_BASE     (FixedPcdGet64 (PcdMangoPci1Link0CfgBase)) // 0x7062000000
#define MANGO_PCIE1_LINK1_CFG_BASE     (FixedPcdGet64 (PcdMangoPci1Link1CfgBase)) // 0x7062800000

//
// Region 1: Bus Number
// Region 2: IO
// Region 3: Mem32
// Region 4: MemAbove4G
//
#define MANGO_PCIE0_LINK0_REGION1_BASE (FixedPcdGet64 (PcdMangoPci0Link0Region1BaseAddress))
#define MANGO_PCIE0_LINK0_REGION1_SIZE (FixedPcdGet64 (PcdMangoPci0Link0Region1Size))
#define MANGO_PCIE0_LINK0_REGION2_BASE (FixedPcdGet64 (PcdMangoPci0Link0Region2BaseAddress))
#define MANGO_PCIE0_LINK0_REGION2_SIZE (FixedPcdGet64 (PcdMangoPci0Link0Region2Size))
#define MANGO_PCIE0_LINK0_REGION3_BASE (FixedPcdGet64 (PcdMangoPci0Link0Region3BaseAddress))
#define MANGO_PCIE0_LINK0_REGION3_SIZE (FixedPcdGet64 (PcdMangoPci0Link0Region3Size))
#define MANGO_PCIE0_LINK0_REGION4_BASE (FixedPcdGet64 (PcdMangoPci0Link0Region4BaseAddress))
#define MANGO_PCIE0_LINK0_REGION4_SIZE (FixedPcdGet64 (PcdMangoPci0Link0Region4Size))

#define MANGO_PCIE0_LINK1_REGION1_BASE (FixedPcdGet64 (PcdMangoPci0Link1Region1BaseAddress))
#define MANGO_PCIE0_LINK1_REGION1_SIZE (FixedPcdGet64 (PcdMangoPci0Link1Region1Size))
#define MANGO_PCIE0_LINK1_REGION2_BASE (FixedPcdGet64 (PcdMangoPci0Link1Region2BaseAddress))
#define MANGO_PCIE0_LINK1_REGION2_SIZE (FixedPcdGet64 (PcdMangoPci0Link1Region2Size))
#define MANGO_PCIE0_LINK1_REGION3_BASE (FixedPcdGet64 (PcdMangoPci0Link1Region3BaseAddress))
#define MANGO_PCIE0_LINK1_REGION3_SIZE (FixedPcdGet64 (PcdMangoPci0Link1Region3Size))
#define MANGO_PCIE0_LINK1_REGION4_BASE (FixedPcdGet64 (PcdMangoPci0Link1Region4BaseAddress))
#define MANGO_PCIE0_LINK1_REGION4_SIZE (FixedPcdGet64 (PcdMangoPci0Link1Region4Size))

#define MANGO_PCIE1_LINK0_REGION1_BASE (FixedPcdGet64 (PcdMangoPci1Link0Region1BaseAddress))
#define MANGO_PCIE1_LINK0_REGION1_SIZE (FixedPcdGet64 (PcdMangoPci1Link0Region1Size))
#define MANGO_PCIE1_LINK0_REGION2_BASE (FixedPcdGet64 (PcdMangoPci1Link0Region2BaseAddress))
#define MANGO_PCIE1_LINK0_REGION2_SIZE (FixedPcdGet64 (PcdMangoPci1Link0Region2Size))
#define MANGO_PCIE1_LINK0_REGION3_BASE (FixedPcdGet64 (PcdMangoPci1Link0Region3BaseAddress))
#define MANGO_PCIE1_LINK0_REGION3_SIZE (FixedPcdGet64 (PcdMangoPci1Link0Region3Size))
#define MANGO_PCIE1_LINK0_REGION4_BASE (FixedPcdGet64 (PcdMangoPci1Link0Region4BaseAddress))
#define MANGO_PCIE1_LINK0_REGION4_SIZE (FixedPcdGet64 (PcdMangoPci1Link0Region4Size))

#define MANGO_PCIE1_LINK1_REGION1_BASE (FixedPcdGet64 (PcdMangoPci1Link1Region1BaseAddress))
#define MANGO_PCIE1_LINK1_REGION1_SIZE (FixedPcdGet64 (PcdMangoPci1Link1Region1Size))
#define MANGO_PCIE1_LINK1_REGION2_BASE (FixedPcdGet64 (PcdMangoPci1Link1Region2BaseAddress))
#define MANGO_PCIE1_LINK1_REGION2_SIZE (FixedPcdGet64 (PcdMangoPci1Link1Region2Size))
#define MANGO_PCIE1_LINK1_REGION3_BASE (FixedPcdGet64 (PcdMangoPci1Link1Region3BaseAddress))
#define MANGO_PCIE1_LINK1_REGION3_SIZE (FixedPcdGet64 (PcdMangoPci1Link1Region3Size))
#define MANGO_PCIE1_LINK1_REGION4_BASE (FixedPcdGet64 (PcdMangoPci1Link1Region4BaseAddress))
#define MANGO_PCIE1_LINK1_REGION4_SIZE (FixedPcdGet64 (PcdMangoPci1Link1Region4Size))


typedef struct {
  UINT64 PciSlvAddress;
  UINT64 ConfigSpaceAddress;
  UINT32 Segment;
  UINT64 BusBase;
  UINT64 BusSize;
  UINT64 IoTranslation;
  UINT64 IoBase;
  UINT64 IoSize;
  UINT64 Mmio32Translation;
  UINT64 Mmio32Base;
  UINT64 Mmio32Size;
  UINT64 Mmio64Translation;
  UINT64 Mmio64Base;
  UINT64 Mmio64Size;
} MANGO_PCI_RESOURCE;

extern MANGO_PCI_RESOURCE mPciResource[PCIE_MAX_PORT][PCIE_MAX_LINK];
#endif