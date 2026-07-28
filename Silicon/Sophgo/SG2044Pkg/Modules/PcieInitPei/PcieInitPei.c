/** @file
  SG2044 PCIe RC host bring-up PEIM.

  Ports the FSBL sg2044_pcie_init() RC sequence (the
  #ifndef CONFIG_DRIVER_PCIE_SLT version, line ~1958 of sg2260_pcie.c) into
  EDK2 PEI. The bring-up runs unconditionally on every board -- the per-variant
  controller topology (which links, ss_mode, gen, lanes) comes from
  PcdPcieHostBridgeTable, so the same PEIM serves all seven SG2044 variants.

  The sequence driven per controller is, in FSBL call order:
    pcie_init_phy -> pcie_wait_core_clk -> pcie_check_radm_status ->
    pcie_config_ctrl -> pcie_config_eq -> pcie_config_link ->
    pcie_config_rc_bar -> pcie_config_rc_cap -> pcie_enable_ltssm ->
    pcie_wait_link -> pcie_config_resp_monitor_bypass ->
    pcie_config_intx_irq_en -> pcie_config_axi_route(SERVER) ->
    pcie_config_mps(0) -> pcie_config_mrrs(0) -> pcie_config_wrapper.

  pcie_clear_slv_mapping / pcie_config_slv_mapping are deliberately NOT ported:
  the EDK2 DesignWare PCIe DXE driver (DwPcieSetSlaveMap) owns the slave map,
  fed from the same PcdPcieHostBridgeTable, so doing it here would double-program.

  The PHY firmware blob path (CONFIG_DRIVER_PCIE_PHY_FW_RELOAD) is unset in
  production, so pcie_update_phy_sram / sg2260_pcie_laod_phy_firmware are not
  ported either.

  FSBL's pcie_wait_sram_init_done / pcie_wait_core_clk / pcie_check_radm_status
  spin forever. Here they are bounded (PCIE_WAIT_RETRIES) and return
  EFI_TIMEOUT on expiry so one dead link cannot hang the boot.

  MMIO is direct: PEI runs S-mode with the MMU off (RiscVConfigureMmu is a DXE
  step), so MmioRead32/MmioWrite32 from IoLib hit the physical addresses
  directly -- no GCD/attribute mapping needed. This mirrors DwGpioPei/DwGpioLib.

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/PcieSlotInfoLib.h>
#include <Ppi/SophgoGpio.h>
#include <Include/PcieHostPcd.h>

//
// FSBL register-base macros (sg2260_pcie.h). Replicated here verbatim because
// that header is not in this repo. Each has a comment pointing to its FSBL
// counterpart.
//
// C2C0_CFG_BASE = 0x6c00000000.
//
#define SG_C2C0_CFG_BASE                     0x6c00000000ULL
// EDK2 (main-processor view) c2c numbering: {0=C2C0, 1=C2C1, 2=CXP}, stride
// 0x04000000 (C2C0=0x6c00, C2C1=0x6c04, CXP=0x6c08). This differs from FSBL,
// which runs on the coprocessor where C2C1 maps to 0x6c02 and uses c2c in
// {0,1,4} with a 0x02000000 stride. EDK2 runs on the main processor, so it
// must use the 0x6c04 view; see SG2044-PCIe-Memory-Mapping.
#define SG_C2C_CFG_BASE(c2c)                 (SG_C2C0_CFG_BASE + ((UINT64)(c2c) * 0x04000000ULL))
// Mirrors C2C_PCIEX8_1_CTRL_DBI_REG_BASE(c2c) = C2C_CFG_BASE(c2c) + 0x0.
#define SG_C2C_PCIEX8_1_CTRL_DBI_BASE(c2c)   (SG_C2C_CFG_BASE(c2c) + 0x0ULL)
// Mirrors C2C_PCIEX8_1_SII_REG_BASE(c2c) = C2C_CFG_BASE(c2c) + 0x000c0400.
#define SG_C2C_PCIEX8_1_SII_BASE(c2c)        (SG_C2C_CFG_BASE(c2c) + 0x000c0400ULL)
// Mirrors C2C_PCIEX8_1_CTRL_REG_BASE(c2c) = C2C_CFG_BASE(c2c) + 0x000c0c00.
#define SG_C2C_PCIEX8_1_CTRL_BASE(c2c)       (SG_C2C_CFG_BASE(c2c) + 0x000c0c00ULL)
// Mirrors C2C_PCIEX8_0_CTRL_DBI_REG_BASE(c2c) = C2C_CFG_BASE(c2c) + 0x00400000.
#define SG_C2C_PCIEX8_0_CTRL_DBI_BASE(c2c)   (SG_C2C_CFG_BASE(c2c) + 0x00400000ULL)
// Mirrors C2C_PCIEX8_0_SII_REG_BASE(c2c) = C2C_CFG_BASE(c2c) + 0x00780400.
#define SG_C2C_PCIEX8_0_SII_BASE(c2c)        (SG_C2C_CFG_BASE(c2c) + 0x00780400ULL)
// Mirrors C2C_PCIEX8_0_CTRL_REG_BASE(c2c) = C2C_CFG_BASE(c2c) + 0x00780c00.
#define SG_C2C_PCIEX8_0_CTRL_BASE(c2c)       (SG_C2C_CFG_BASE(c2c) + 0x00780c00ULL)
// Mirrors C2C_PCIE_0_PHY_INTF_REG_BASE(c2c) = C2C_CFG_BASE(c2c) + 0x00781000.
#define SG_C2C_PCIE_0_PHY_INTF_BASE(c2c)     (SG_C2C_CFG_BASE(c2c) + 0x00781000ULL)
// Mirrors C2C_PCIE_TOP_REG(c2c) = C2C_CFG_BASE(c2c) + 0x007d0000.
#define SG_C2C_PCIE_TOP_BASE(c2c)            (SG_C2C_CFG_BASE(c2c) + 0x007d0000ULL)
// Mirrors C2C_PCIE_WAPPER_0_REG(c2c) = C2C_CFG_BASE(c2c) + 0x00f82000.
#define SG_C2C_PCIE_WAPPER_0_BASE(c2c)       (SG_C2C_CFG_BASE(c2c) + 0x00f82000ULL)
// Mirrors C2C_PCIE_MSI_RX_0_REG(c2c) = C2C_CFG_BASE(c2c) + 0x00f82400.
#define SG_C2C_PCIE_MSI_RX_0_BASE(c2c)       (SG_C2C_CFG_BASE(c2c) + 0x00f82400ULL)

//
// FSBL register offsets (sg2260_pcie.h).
//
#define SG_CXP_TOP_REG_RX000                 0x000
#define SG_CXP_TOP_REG_RX054                 0x054
#define SG_CXP_TOP_REG_RX058                 0x058
#define SG_CXP_TOP_REG_RX060                 0x060
#define SG_CXP_TOP_REG_RX068                 0x068
#define SG_CXP_TOP_REG_RX06C                 0x06c
#define SG_CXP_TOP_REG_RX90C                 0x90c

#define SG_CXP_TOP_REG_RX058_PHY0_REF_REPEAT_CLK_EN_BIT   16
#define SG_CXP_TOP_REG_RX90C_UPCS_PWR_STABLE_BIT          23
#define SG_CXP_TOP_REG_PHYX_PCS_PWR_STABLE_BIT            26
#define SG_CXP_TOP_REG_PHYX_PMA_PWR_STABLE_BIT            28
#define SG_CXP_TOP_REG_PHYX_SRAM_BL_BYPASS_BIT            5
#define SG_CXP_TOP_REG_PHYX_SRAM_BYPASS_BIT               6
#define SG_CXP_TOP_REG_PHYX_SRAM_EXT_LD_DONE_BIT          9
#define SG_CXP_TOP_REG_PHYX_SRAM_INIT_DONE_BIT            10

#define SG_CXP_TOP_REG_RX000_SS_MODE_MASK                 0x00000007u
#define SG_CXP_TOP_REG_RX068_PHY1_REFA_CLK_SEL_MASK      0x00300000u
#define SG_CXP_TOP_REG_RX06C_PHY1_REFB_CLK_SEL_MASK      0x00000600u
#define SG_CXP_TOP_REG_PHYX_SRAM_BYPASS_MASK             0x00000060u

// C2C TOP REG offsets.
#define SG_C2C_TOP_IRQ_CTRL_REG                          0x60
#define SG_C2C_TOP_MSI_GEN_MODE_REG                      0xd8
#define SG_C2C_TOP_RESP_MON_BYPASS_REG                   0x54

#define SG_C2C_TOP_IRQ_CTRL_PCIE_INT_MASK_MASK           0x0000001eu

// PCIE CTRL REG offsets.
#define SG_PCIE_CTRL_SFT_RST_SIG_REG                     0x050
#define SG_PCIE_CTRL_REMAPPING_EN_REG                    0x060
#define SG_PCIE_CTRL_HNI_UP_START_ADDR_REG               0x064
#define SG_PCIE_CTRL_HNI_UP_END_ADDR_REG                 0x068
#define SG_PCIE_CTRL_HNI_DW_ADDR_REG                     0x06c
#define SG_PCIE_CTRL_SN_UP_START_ADDR_REG                0x070
#define SG_PCIE_CTRL_SN_UP_END_ADDR_REG                  0x074
#define SG_PCIE_CTRL_SN_DW_ADDR_REG                      0x078
#define SG_PCIE_CTRL_AXI_MSI_GEN_CTRL_REG                0x07c
#define SG_PCIE_CTRL_AXI_MSI_GEN_LOWER_ADDR_REG          0x088
#define SG_PCIE_CTRL_AXI_MSI_GEN_UPPER_ADDR_REG          0x08c
#define SG_PCIE_CTRL_AXI_MSI_GEN_USER_DATA_REG           0x090
#define SG_PCIE_CTRL_AXI_MSI_GEN_MASK_IRQ_REG            0x094
#define SG_PCIE_CTRL_IRQ_EN_REG                          0x0a0

#define SG_PCIE_SII_GENERAL_CTRL1_REG                    0x050
#define SG_PCIE_SII_GENERAL_CTRL3_REG                    0x058

#define SG_PCIE_CTRL_SFT_RST_SIG_COLD_RSTN_BIT           0
#define SG_PCIE_CTRL_SFT_RST_SIG_PHY_RSTN_BIT            1
#define SG_PCIE_CTRL_REMAP_EN_HNI_TO_PCIE_UP4G_EN_BIT    0
#define SG_PCIE_CTRL_REMAP_EN_HNI_TO_PCIE_DW4G_EN_BIT    1
#define SG_PCIE_CTRL_REMAP_EN_SN_TO_PCIE_UP4G_EN_BIT     2
#define SG_PCIE_CTRL_REMAP_EN_SN_TO_PCIE_DW4G_EN_BIT     3
#define SG_PCIE_CTRL_AXI_MSI_GEN_CTRL_MSI_GEN_EN_BIT     0

#define SG_PCIE_SII_GENERAL_CTRL1_DEVICE_TYPE_MASK       0x00001e00u
#define SG_PCIE_CTRL_IRQ_EN_INTX_EN_MASK                 0x0000001eu

// DBI2 / iATU offsets (sg2260_pcie.h).
#define SG_C2C_PCIE_DBI2_OFFSET                          0x100000
#define SG_C2C_PCIE_ATU_OFFSET                           0x300000

// FSBL enums (sg2260_pcie.h).
#define SG_PCIE_SERDES_MODE_X8                           0u
#define SG_PCIE_PHY_NO_SRAM_BYPASS                       0u
#define SG_SOC_WORK_MODE_SERVER                          0u
#define SG_PCIE_RST_ASSERT                              0u
#define SG_PCIE_RST_DE_ASSERT                            1u
#define SG_PCIE_DEV_TYPE_RC                              4u

//
// Bound for FSBL's infinite-wait loops (pcie_wait_sram_init_done,
// pcie_wait_core_clk, pcie_check_radm_status, pcie_wait_link). Each iteration
// delays 1-20us, so 100000 iters is well into the multi-hundred-ms range --
// generous enough for a healthy link but bounded so a dead one cannot hang PEI.
//
#define PCIE_WAIT_RETRIES                                100000u

//
// FSBL perst_gpio[10] = {6, 1, 2, 3, 20, 21, 22, 23, 7, 19} (sg2260_pcie.c:1558),
// the SoC-fixed PERST# pin per (subsys, wrapper, phy), laid out as four pins
// per C2C slot (w0p0,w0p1,w1p0,w1p1) then two for CXP. EDK2's c2c numbering
// {0=C2C0, 1=C2C1, 2=CXP} already matches this slot order, so the index is just
// c2c*4 + wrapper*2 + phy (no fold, unlike FSBL which used {0,1,4}+4->2).
//
STATIC CONST UINT32  mPerstGpio[10] = { 6, 1, 2, 3, 20, 21, 22, 23, 7, 19 };

//
// FSBL eq_coef_tbl[11] in pcie_config_eq (sg2260_pcie.c line ~997).
//
typedef struct {
  UINT32    Cursor;
  UINT32    PreCursor;
  UINT32    PostCursor;
} PCIE_EQ_COEF;

STATIC CONST PCIE_EQ_COEF  mEqCoefTbl[11] = {
  { 36, 0, 12 }, { 40, 0, 8 }, { 38, 0, 10 }, { 42, 0, 6 }, { 48, 0, 0 },
  { 44, 4, 0 }, { 42, 6, 0 }, { 34, 5, 9 }, { 36, 6, 6 }, { 40, 8, 0 },
  { 32, 0, 16 }
};

//
// Cached GPIO PPI pointer, resolved once in the entry point.
//
STATIC SOPHGO_GPIO_PPI  *mGpioPpi = NULL;

/**
  Resolve the controller register apertures for (c2c, wrapper, phy).

  Mirrors the w0/w1 + phy-multiplier selection that FSBL open-codes at the top
  of every pcie_config_* function (sg2260_pcie.c). The w0/w1 split is keyed on
  wrapper_id; the phy multiplier differs per aperture type (CTRL/SII use
  0x400000 for w0 and 0xec0000 for w1, DBI uses 0x400000 for w0 and 0xc00000
  for w1 -- see sg2260_pcie.h).
**/
STATIC
VOID
PcieGetBases (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId,
  OUT UINTN   *DbiBase,
  OUT UINTN   *SiiBase,
  OUT UINTN   *CtrlBase
  )
{
  UINTN  C2cBase;

  C2cBase = (UINTN)SG_C2C_CFG_BASE (C2cId);

  if (WrapperId == 0) {
    if (DbiBase != NULL) {
      *DbiBase = C2cBase + 0x00400000ULL + (UINT64)PhyId * 0x400000ULL;
    }

    if (SiiBase != NULL) {
      *SiiBase = C2cBase + 0x00780400ULL + (UINT64)PhyId * 0x400000ULL;
    }

    if (CtrlBase != NULL) {
      *CtrlBase = C2cBase + 0x00780c00ULL + (UINT64)PhyId * 0x400000ULL;
    }
  } else {
    if (DbiBase != NULL) {
      *DbiBase = C2cBase + 0x0ULL + (UINT64)PhyId * 0xc00000ULL;
    }

    if (SiiBase != NULL) {
      *SiiBase = C2cBase + 0x000c0400ULL + (UINT64)PhyId * 0xec0000ULL;
    }

    if (CtrlBase != NULL) {
      *CtrlBase = C2cBase + 0x000c0c00ULL + (UINT64)PhyId * 0xec0000ULL;
    }
  }
}

/**
  Resolve the PHY-intf (CXP top) register base for (c2c, wrapper).

  Mirrors FSBL: C2C_PCIE_0_PHY_INTF_REG_BASE(c2c) + wrapper*0x800000, used by
  every pcie_config_phy_* / pcie_wait_sram_init_done / pcie_config_exload_phy.
  (sg2260_pcie.c lines 60, 99, 119, 146, 391, 405.)
**/
STATIC
UINTN
PcieGetPhyIntfBase (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId
  )
{
  return (UINTN)(SG_C2C_PCIE_0_PHY_INTF_BASE (C2cId) + (UINT64)WrapperId * 0x800000ULL);
}

//
// ---- PHY-wrapper setup (FSBL pcie_init_phy, phy_id==0 block) ----
//

/**
  Port of pcie_config_ss_mode (sg2260_pcie.c line 55).
**/
STATIC
VOID
PcieConfigSsMode (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  SsMode
  )
{
  UINTN   BaseAddr;
  UINT32  Val;

  BaseAddr = PcieGetPhyIntfBase (C2cId, WrapperId);

  Val  = MmioRead32 (BaseAddr + SG_CXP_TOP_REG_RX000);
  Val &= ~SG_CXP_TOP_REG_RX000_SS_MODE_MASK;
  Val |= SsMode & SG_CXP_TOP_REG_RX000_SS_MODE_MASK;
  MmioWrite32 (BaseAddr + SG_CXP_TOP_REG_RX000, Val);
}

/**
  Port of pcie_config_phy_bl_bypass (sg2260_pcie.c line 91).
  Loops phy_id 0..1 (PCIE_PHY_ID_BUTT == 2), writing RX060 + phy*0x10.
**/
STATIC
VOID
PcieConfigPhyBlBypass (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  SramBypassMode
  )
{
  UINTN   RegBase;
  UINT32  Val;
  UINT32  RegAddr;
  UINT32  PhyId;

  RegBase = PcieGetPhyIntfBase (C2cId, WrapperId);

  for (PhyId = 0; PhyId < 2; PhyId++) {
    RegAddr = SG_CXP_TOP_REG_RX060 + (PhyId * 0x10);

    Val  = MmioRead32 (RegBase + RegAddr);
    Val &= ~SG_CXP_TOP_REG_PHYX_SRAM_BYPASS_MASK;
    Val |= (SramBypassMode & 0x3u) << SG_CXP_TOP_REG_PHYX_SRAM_BL_BYPASS_BIT;
    MmioWrite32 (RegBase + RegAddr, Val);
  }
}

/**
  Port of pcie_config_phy_pwr_stable (sg2260_pcie.c line 112).
**/
STATIC
VOID
PcieConfigPhyPwrStable (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId
  )
{
  UINTN   BaseAddr;
  UINT32  Val;
  UINT32  RegAddr;
  UINT32  PhyId;

  BaseAddr = PcieGetPhyIntfBase (C2cId, WrapperId);

  // cfg phy0_upcs_pwr_stable (RX90C bit 23).
  Val  = MmioRead32 (BaseAddr + SG_CXP_TOP_REG_RX90C);
  Val |= 0x1u << SG_CXP_TOP_REG_RX90C_UPCS_PWR_STABLE_BIT;
  MmioWrite32 (BaseAddr + SG_CXP_TOP_REG_RX90C, Val);

  for (PhyId = 0; PhyId < 2; PhyId++) {
    RegAddr = SG_CXP_TOP_REG_RX054 + (PhyId * 0x10);

    Val  = MmioRead32 (BaseAddr + RegAddr);
    Val |= 0x1u << SG_CXP_TOP_REG_PHYX_PCS_PWR_STABLE_BIT;
    Val |= 0x1u << SG_CXP_TOP_REG_PHYX_PMA_PWR_STABLE_BIT;
    MmioWrite32 (BaseAddr + RegAddr, Val);
  }
}

/**
  Port of pcie_config_phy_eq_afe (sg2260_pcie.c line 138).
  Writes 0x20002 to each of 7 register sets x 4 lanes.
**/
STATIC
VOID
PcieConfigPhyEqAfe (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId
  )
{
  STATIC CONST UINT32  RegBase[7] = {
    0x974, 0x994, 0xa14, 0xa34, 0xa54, 0xa74, 0xa94
  };
  UINTN   PcieBaseAddr;
  UINT32  RegAddr;
  UINT32  RegSet;
  UINT32  RegOff;

  PcieBaseAddr = PcieGetPhyIntfBase (C2cId, WrapperId);

  for (RegSet = 0; RegSet < 7; RegSet++) {
    for (RegOff = 0; RegOff < 4; RegOff++) {
      RegAddr = RegBase[RegSet] + (RegOff * 4);
      MmioWrite32 (PcieBaseAddr + RegAddr, 0x20002);
    }
  }
}

/**
  Port of pcie_config_soft_cold_reset (sg2260_pcie.c line 155).
  Two passes over phy_id 0..1: deassert COLD_RSTN, then assert.
**/
STATIC
VOID
PcieConfigSoftColdReset (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId
  )
{
  UINT32  Val;
  UINT32  PhyId;
  UINTN   RegBase;

  for (PhyId = 0; PhyId < 2; PhyId++) {
    PcieGetBases (C2cId, WrapperId, PhyId, NULL, NULL, &RegBase);

    Val  = MmioRead32 (RegBase + SG_PCIE_CTRL_SFT_RST_SIG_REG);
    Val &= ~SG_PCIE_CTRL_SFT_RST_SIG_COLD_RSTN_BIT;
    MmioWrite32 (RegBase + SG_PCIE_CTRL_SFT_RST_SIG_REG, Val);
  }

  for (PhyId = 0; PhyId < 2; PhyId++) {
    PcieGetBases (C2cId, WrapperId, PhyId, NULL, NULL, &RegBase);

    Val  = MmioRead32 (RegBase + SG_PCIE_CTRL_SFT_RST_SIG_REG);
    Val |= 0x1u << SG_PCIE_CTRL_SFT_RST_SIG_COLD_RSTN_BIT;
    MmioWrite32 (RegBase + SG_PCIE_CTRL_SFT_RST_SIG_REG, Val);
  }
}

/**
  Port of pcie_config_soft_phy_reset (sg2260_pcie.c line 188).
  rst_status 0 = assert, 1 = deassert. udelay(1) at end preserved.
**/
STATIC
VOID
PcieConfigSoftPhyReset (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  RstStatus
  )
{
  UINT32  Val;
  UINT32  PhyId;
  UINTN   RegBase;

  if ((RstStatus != 0) && (RstStatus != 1)) {
    return;
  }

  for (PhyId = 0; PhyId < 2; PhyId++) {
    PcieGetBases (C2cId, WrapperId, PhyId, NULL, NULL, &RegBase);

    Val = MmioRead32 (RegBase + SG_PCIE_CTRL_SFT_RST_SIG_REG);
    if (RstStatus == 1) {
      Val |= 0x1u << SG_PCIE_CTRL_SFT_RST_SIG_PHY_RSTN_BIT;
    } else {
      Val &= ~SG_PCIE_CTRL_SFT_RST_SIG_PHY_RSTN_BIT;
    }

    MmioWrite32 (RegBase + SG_PCIE_CTRL_SFT_RST_SIG_REG, Val);
  }

  MicroSecondDelay (1);
}

//
// ---- PERST (FSBL pcie_assert_perst / pcie_deassert_perst) ----
//

/**
  Drive PERST for (c2c, wrapper, phy) to the given level via SOPHGO_GPIO_PPI.

  Mirrors FSBL perst_gpio table: pos = c2c*4 + wrapper*2 + phy (EDK2 uses
  c2c {0,1,2} with no fold, unlike FSBL which used {0,1,4}+4->2);
  pin = perst_gpio[pos]; gpio_set_direction(OUT) then gpio_set_value(level).
**/
STATIC
EFI_STATUS
PcieSetPerst (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId,
  IN  UINT32  Level
  )
{
  UINT32  Pos;
  UINT32  Pin;
  UINT32  Bus;

  if (mGpioPpi == NULL) {
    return EFI_NOT_STARTED;
  }

  //
  // EDK2 c2c numbering is {0=C2C0, 1=C2C1, 2=CXP} with no fold. Index into
  // mPerstGpio[10] is c2c*4 + wrapper*2 + phy (SoC-fixed PERST# pin table,
  // see sg2260_pcie.c:1558).
  //
  Pos = C2cId * 4 + WrapperId * 2 + PhyId;
  if (Pos >= ARRAY_SIZE (mPerstGpio)) {
    return EFI_INVALID_PARAMETER;
  }

  Pin = mPerstGpio[Pos];

  //
  // FSBL gpio numbering is flat across DW controllers, each 32 pins wide
  // (DW_GPIO_PORT_WIDTH == 32, see dw_gpio.c). The EDK2 DwGpioLib splits that
  // into (Bus, Pin) -- Bus = gpio / 32, Pin = gpio % 32. Each Bus maps to one
  // of the PcdGpioBaseAddresses entries.
  //
  Bus  = Pin / 32;
  Pin &= 0x1f;

  //
  // ModeConfig(OUT_LOW / OUT_HIGH) atomically sets direction and output value,
  // matching FSBL's gpio_set_direction(OUT) + gpio_set_value(level) pair.
  //
  return mGpioPpi->ModeConfig (
                      mGpioPpi,
                      Bus,
                      Pin,
                      (Level != 0) ? DwGpioConfigOutHigh : DwGpioConfigOutLow
                      );
}

//
// ---- SRAM / core-clk / radm waits (FSBL infinite loops, bounded here) ----
//

/**
  Port of pcie_wait_sram_init_done (sg2260_pcie.c line 384).
  FSBL spins `while (val != 1)` on SRAM_INIT_DONE_BIT. Here it is bounded.
**/
STATIC
EFI_STATUS
PcieWaitSramInitDone (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId
  )
{
  UINTN   RegBase;
  UINT32  RegAddr;
  UINT32  Val;
  UINT32  Retries;

  RegAddr = SG_CXP_TOP_REG_RX060;
  RegBase = PcieGetPhyIntfBase (C2cId, WrapperId);

  for (Retries = 0; Retries < PCIE_WAIT_RETRIES; Retries++) {
    Val = MmioRead32 (RegBase + RegAddr + PhyId * 0x10);
    Val = (Val >> SG_CXP_TOP_REG_PHYX_SRAM_INIT_DONE_BIT) & 0x1u;
    if (Val == 1) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay (1);
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: timeout C2C%u W%u P%u SRAM_INIT_DONE\n",
    __func__,
    C2cId,
    WrapperId,
    PhyId
    ));
  return EFI_TIMEOUT;
}

/**
  Port of pcie_config_exload_phy (sg2260_pcie.c line 399).
  Sets PHYX_SRAM_EXT_LD_DONE_BIT.
**/
STATIC
VOID
PcieConfigExloadPhy (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId
  )
{
  UINTN   PhyIntfAddr;
  UINT32  RegAddr;
  UINT32  Val;

  PhyIntfAddr = PcieGetPhyIntfBase (C2cId, WrapperId);
  RegAddr     = SG_CXP_TOP_REG_RX060 + (PhyId * 0x10);

  Val  = MmioRead32 (PhyIntfAddr + RegAddr);
  Val |= 0x1u << SG_CXP_TOP_REG_PHYX_SRAM_EXT_LD_DONE_BIT;
  MmioWrite32 (PhyIntfAddr + RegAddr, Val);
}

/**
  Port of pcie_wait_core_clk (sg2260_pcie.c line 415).
  FSBL spins `while (val != 1)` on SII[0x5c] bit 8 (pcie_rst_n). Bounded here.
**/
STATIC
EFI_STATUS
PcieWaitCoreClk (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId
  )
{
  UINTN   BaseAddr;
  UINT32  Val;
  UINT32  Retries;

  PcieGetBases (C2cId, WrapperId, PhyId, NULL, &BaseAddr, NULL);

  for (Retries = 0; Retries < PCIE_WAIT_RETRIES; Retries++) {
    MicroSecondDelay (10);
    Val = MmioRead32 (BaseAddr + 0x5c);   // GEN_CTRL_4
    Val = (Val >> 8) & 0x1u;              // bit8, pcie_rst_n
    if (Val == 1) {
      return EFI_SUCCESS;
    }
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: timeout C2C%u W%u P%u CORE_CLK\n",
    __func__,
    C2cId,
    WrapperId,
    PhyId
    ));
  return EFI_TIMEOUT;
}

/**
  Port of pcie_check_radm_status (sg2260_pcie.c line 432).
  FSBL spins `while (val != 1)` on radm_idle (bit 29 for phy 0, bit 21 for phy 1).
  Bounded here.
**/
STATIC
EFI_STATUS
PcieCheckRadmStatus (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId
  )
{
  UINTN   BaseAddr;
  UINT32  Val;
  UINT32  Retries;

  PcieGetBases (C2cId, WrapperId, PhyId, NULL, NULL, &BaseAddr);

  for (Retries = 0; Retries < PCIE_WAIT_RETRIES; Retries++) {
    MicroSecondDelay (10);
    if (PhyId == 0) {
      Val = MmioRead32 (BaseAddr + 0xfc);
      Val = (Val >> 29) & 0x1u;     // bit29, radm_idle
    } else {
      Val = MmioRead32 (BaseAddr + 0xe8);
      Val = (Val >> 21) & 0x1u;     // bit21, radm_idle
    }

    if (Val == 1) {
      return EFI_SUCCESS;
    }
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: timeout C2C%u W%u P%u RADM_IDLE\n",
    __func__,
    C2cId,
    WrapperId,
    PhyId
    ));
  return EFI_TIMEOUT;
}

//
// ---- Per-controller bring-up (FSBL sg2044_pcie_init sequence) ----
//

/**
  Port of pcie_config_link (sg2260_pcie.c line 454).
**/
STATIC
VOID
PcieConfigLink (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId,
  IN  UINT32  PcieLnCnt
  )
{
  UINTN   BaseAddr;
  UINT32  Val;

  PcieGetBases (C2cId, WrapperId, PhyId, &BaseAddr, NULL, NULL);

  // config lane_count (DBI 0x8c0 low 6 bits).
  Val  = MmioRead32 (BaseAddr + 0x8c0);
  Val  = (Val & 0xffffffc0u) | PcieLnCnt;
  MmioWrite32 (BaseAddr + 0x8c0, Val);

  // config eq bypass highest rate disable (DBI 0x1c0 bit0).
  Val  = MmioRead32 (BaseAddr + 0x1c0);
  Val |= 0x1u;
  MmioWrite32 (BaseAddr + 0x1c0, Val);
}

/**
  Port of pcie_config_rc_cap (sg2260_pcie.c line 476).

  When SlotNumber is not PCIE_SLOT_NUMBER_NONE the root port feeds a
  physical slot: set the "Slot Implemented" bit and program the
  Physical Slot Number into the Slot Capabilities register.
**/
STATIC
VOID
PcieConfigRcCap (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId,
  IN  UINT32  PcieLnCnt,
  IN  UINT16  SlotNumber
  )
{
  UINTN   PcieDbiBase;
  UINT32  Val;
  UINT32  LkCap;

  PcieGetBases (C2cId, WrapperId, PhyId, &PcieDbiBase, NULL, NULL);

  // enable DBI_RO_WR_EN (DBI 0x8bc bit0).
  Val  = MmioRead32 (PcieDbiBase + 0x8bc);
  Val  = (Val & 0xfffffffeu) | 0x1u;
  MmioWrite32 (PcieDbiBase + 0x8bc, Val);

  MmioWrite32 (PcieDbiBase, 0x20441f1cu);

  if (PcieLnCnt == 4) {
    LkCap = 7;
  } else if (PcieLnCnt == 2) {
    LkCap = 3;
  } else if (PcieLnCnt == 1) {
    LkCap = 1;
  } else {
    LkCap = 0;
  }

  if (PcieLnCnt != 8) {
    Val  = MmioRead32 (PcieDbiBase + 0x710);
    Val &= 0xffc0ffffu;       // bit[21:16]
    Val |= LkCap << 16;
    MmioWrite32 (PcieDbiBase + 0x710, Val);

    Val  = MmioRead32 (PcieDbiBase + 0x80c);
    Val &= 0xffffe0ffu;       // bit[12:8]
    Val |= PcieLnCnt << 8;
    MmioWrite32 (PcieDbiBase + 0x80c, Val);

    Val  = MmioRead32 (PcieDbiBase + 0x7c);
    Val &= 0xfffffc0fu;       // bit[9:4]
    Val |= PcieLnCnt << 4;
    MmioWrite32 (PcieDbiBase + 0x7c, Val);
  }

  if (C2cId == 0) {
    // C2C_0 only: support legacy card, clear Data Link Feature Exchange Enable
    // (DBI 0x330 bit31).
    Val  = MmioRead32 (PcieDbiBase + 0x330);
    Val &= 0x7fffffffu;
    MmioWrite32 (PcieDbiBase + 0x330, Val);
  }

  //
  // Slot configuration, written deterministically in both directions
  // (no reliance on reset defaults).  RMW is required on the word at
  // 0x70: it also holds Cap ID / Next pointer / the rest of the PCIe
  // Capabilities register (measured 0x0042: v2, Root Port).  SLTCAP
  // (DBI 0x84) is written whole: measured reset is 0x00000000 on all
  // controllers, and 0 in every other field is exactly the fixed,
  // non-hotplug definition (no hotplug, no indicators, no slot power
  // limit declared).
  //
  Val  = MmioRead32 (PcieDbiBase + 0x70);
  if (SlotNumber != PCIE_SLOT_NUMBER_NONE) {
    // root port feeds a physical slot: Slot Implemented = 1,
    // Physical Slot Number[31:19] = SlotNumber.
    Val |= 0x01000000u;
    MmioWrite32 (PcieDbiBase + 0x70, Val);
    MmioWrite32 (PcieDbiBase + 0x84, (UINT32)SlotNumber << 19);
  } else {
    // no slot on this port: Slot Implemented = 0, SLTCAP all zero
    // (Physical Slot Number cleared along with everything else).
    Val &= 0xfeffffffu;
    MmioWrite32 (PcieDbiBase + 0x70, Val);
    MmioWrite32 (PcieDbiBase + 0x84, 0);
  }

  // disable DBI_RO_WR_EN.
  Val  = MmioRead32 (PcieDbiBase + 0x8bc);
  Val &= 0xfffffffeu;
  MmioWrite32 (PcieDbiBase + 0x8bc, Val);
}

/**
  Port of pcie_config_rc_bar (sg2260_pcie.c line 564).
  Zeroes BAR0/1/2/4 in both DBI and DBI2 for function 0.
**/
STATIC
VOID
PcieConfigRcBar (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId
  )
{
  UINTN   PcieDbiBase;
  UINT32  Val;
  UINT32  Func;

  PcieGetBases (C2cId, WrapperId, PhyId, &PcieDbiBase, NULL, NULL);
  Func = 0;

  // enable DBI_RO_WR_EN.
  Val  = MmioRead32 (PcieDbiBase + 0x8bc);
  Val  = (Val & 0xfffffffeu) | 0x1u;
  MmioWrite32 (PcieDbiBase + 0x8bc, Val);

  MmioWrite32 ((PcieDbiBase + 0x10) | (Func << 16), 0);
  MmioWrite32 ((PcieDbiBase + 0x14) | (Func << 16), 0);
  MmioWrite32 ((PcieDbiBase + 0x18) | (Func << 16), 0);
  MmioWrite32 ((PcieDbiBase + 0x20) | (Func << 16), 0);

  MmioWrite32 ((PcieDbiBase + SG_C2C_PCIE_DBI2_OFFSET + 0x10) | (Func << 16), 0);
  MmioWrite32 ((PcieDbiBase + SG_C2C_PCIE_DBI2_OFFSET + 0x14) | (Func << 16), 0);
  MmioWrite32 ((PcieDbiBase + SG_C2C_PCIE_DBI2_OFFSET + 0x18) | (Func << 16), 0);
  MmioWrite32 ((PcieDbiBase + SG_C2C_PCIE_DBI2_OFFSET + 0x20) | (Func << 16), 0);

  // disable DBI_RO_WR_EN.
  Val  = MmioRead32 (PcieDbiBase + 0x8bc);
  Val &= 0xfffffffeu;
  MmioWrite32 (PcieDbiBase + 0x8bc, Val);
}

/**
  Port of pcie_config_ctrl (sg2260_pcie.c line 631).
  Programs SII device_type and DBI speed-change / target-link-speed / ecrc.
**/
STATIC
VOID
PcieConfigCtrl (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId,
  IN  UINT32  PcieGenSlt,
  IN  UINT32  PcieDevType
  )
{
  UINTN   SiiRegBase;
  UINTN   DbiRegBase;
  UINT32  Val;

  PcieGetBases (C2cId, WrapperId, PhyId, &DbiRegBase, &SiiRegBase, NULL);

  // config device_type (SII GENERAL_CTRL1 bit[12:9]).
  Val  = MmioRead32 (SiiRegBase + SG_PCIE_SII_GENERAL_CTRL1_REG);
  Val &= ~SG_PCIE_SII_GENERAL_CTRL1_DEVICE_TYPE_MASK;
  Val |= PcieDevType << 9;
  MmioWrite32 (SiiRegBase + SG_PCIE_SII_GENERAL_CTRL1_REG, Val);

  // Directed Speed Change (DBI 0x80c bit17).
  Val  = MmioRead32 (DbiRegBase + 0x80c);
  Val |= 0x20000u;
  MmioWrite32 (DbiRegBase + 0x80c, Val);

  // target_link_speed (DBI 0xa0 low nibble).
  Val  = MmioRead32 (DbiRegBase + 0xa0);
  Val  = (Val & 0xfffffff0u) | PcieGenSlt;
  MmioWrite32 (DbiRegBase + 0xa0, Val);

  // ecrc generation enable (DBI 0x118).
  Val = 0x3e0u;
  MmioWrite32 (DbiRegBase + 0x118, Val);
}

/**
  Port of pcie_config_eq (sg2260_pcie.c line 991).
  Programs the 11-preset EQ coefficient table across 3 speed settings.
**/
STATIC
VOID
PcieConfigEq (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId
  )
{
  UINTN   PcieDbiBase;
  UINT32  Val;
  UINT32  Speed;
  UINT32  PsetId;

  PcieGetBases (C2cId, WrapperId, PhyId, &PcieDbiBase, NULL, NULL);

  for (Speed = 0; Speed < 3; Speed++) {
    Val  = MmioRead32 (PcieDbiBase + 0x890);
    Val &= 0xfcffffffu;
    Val |= Speed << 24;
    MmioWrite32 (PcieDbiBase + 0x890, Val);

    Val  = MmioRead32 (PcieDbiBase + 0x894);
    Val &= 0xfffff000u;         // bit[11:0]
    Val |= 16;
    Val |= 48u << 6;
    MmioWrite32 (PcieDbiBase + 0x894, Val);

    for (PsetId = 0; PsetId < 11; PsetId++) {
      Val  = MmioRead32 (PcieDbiBase + 0x89c);
      Val &= 0xfffffff0u;       // bit[3:0]
      Val |= PsetId;
      MmioWrite32 (PcieDbiBase + 0x89c, Val);

      Val  = MmioRead32 (PcieDbiBase + 0x898);
      Val &= 0xfffc0000u;       // bit[17:0]
      Val |= mEqCoefTbl[PsetId].PreCursor;
      Val |= mEqCoefTbl[PsetId].Cursor << 6;
      Val |= mEqCoefTbl[PsetId].PostCursor << 12;
      MmioWrite32 (PcieDbiBase + 0x898, Val);

      Val = MmioRead32 (PcieDbiBase + 0x8a4);
      if (Val & 0x1u) {
        DEBUG ((
          DEBUG_INFO,
          "%a: illegal coef speed[%u] pset[%u]\n",
          __func__,
          Speed,
          PsetId
          ));
      }
    }
  }
}

/**
  Port of pcie_enable_ltssm (sg2260_pcie.c line 1040).
**/
STATIC
VOID
PcieEnableLtssm (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId
  )
{
  UINTN   BaseAddr;
  UINT32  Val;

  PcieGetBases (C2cId, WrapperId, PhyId, NULL, &BaseAddr, NULL);

  Val  = MmioRead32 (BaseAddr + SG_PCIE_SII_GENERAL_CTRL3_REG);
  Val |= 0x1u;
  MmioWrite32 (BaseAddr + SG_PCIE_SII_GENERAL_CTRL3_REG, Val);
}

/**
  Port of pcie_wait_link (sg2260_pcie.c line 1056).
  FSBL has an internal cap of 10000 iters; we cap at PCIE_WAIT_RETRIES (larger).
**/
STATIC
EFI_STATUS
PcieWaitLink (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId
  )
{
  UINTN   BaseAddr;
  UINT32  Val;
  UINT32  Times;

  PcieGetBases (C2cId, WrapperId, PhyId, NULL, &BaseAddr, NULL);

  Times = 0;
  do {
    MicroSecondDelay (20);
    Times++;
    Val = MmioRead32 (BaseAddr + 0xb4);      // LNK_DBG_2
    Val = (Val >> 6) & 0x3u;                 // bit6 SMLH_LINK_UP, bit7 RDLH_LINK_UP
  } while ((Val != 0x3u) && (Times < PCIE_WAIT_RETRIES));

  if (Val != 0x3u) {
    //
    // An empty PCIe slot never trains a link, so this timeout is the normal
    // case, not a fault. Keep it at DEBUG_INFO so RELEASE builds stay quiet.
    //
    DEBUG ((
      DEBUG_INFO,
      "%a: timeout C2C%u W%u P%u LINK_UP (val=0x%x)\n",
      __func__,
      C2cId,
      WrapperId,
      PhyId,
      Val
      ));
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Port of pcie_config_mps (sg2260_pcie.c line 1110).
  MPS field hardcoded to 3 (1024B, IC-design limit) inside the DBI_RO_WR_EN
  guard; the mps arg goes to the PCIE_DEVICE_CTRL register outside the guard.
**/
STATIC
VOID
PcieConfigMps (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId,
  IN  UINT32  Mps
  )
{
  UINTN   BaseAddr;
  UINT32  Val;

  PcieGetBases (C2cId, WrapperId, PhyId, &BaseAddr, NULL, NULL);

  // enable DBI_RO_WR_EN.
  Val  = MmioRead32 (BaseAddr + 0x8bc);
  Val  = (Val & 0xfffffffeu) | 0x1u;
  MmioWrite32 (BaseAddr + 0x8bc, Val);

  // supported mps is 1024, limited by IC design (DBI 0x74 low 3 bits = 3).
  Val  = MmioRead32 (BaseAddr + 0x74);
  Val &= 0xfffffff8u;
  Val |= (3u & 0x7u) << 0;
  MmioWrite32 (BaseAddr + 0x74, Val);

  // disable DBI_RO_WR_EN.
  Val  = MmioRead32 (BaseAddr + 0x8bc);
  Val &= 0xfffffffeu;
  MmioWrite32 (BaseAddr + 0x8bc, Val);

  Val  = MmioRead32 (BaseAddr + 0x78);
  Val &= 0xffffff1fu;
  Val |= (Mps & 0x7u) << 5;
  MmioWrite32 (BaseAddr + 0x78, Val);
}

/**
  Port of pcie_config_mrrs (sg2260_pcie.c line 1142).
**/
STATIC
VOID
PcieConfigMrrs (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId,
  IN  UINT32  Mrrs
  )
{
  UINTN   BaseAddr;
  UINT32  Val;

  PcieGetBases (C2cId, WrapperId, PhyId, &BaseAddr, NULL, NULL);

  Val  = MmioRead32 (BaseAddr + 0x78);
  Val &= 0xffff8fffu;
  Val |= (Mrrs & 0x7u) << 12;
  MmioWrite32 (BaseAddr + 0x78, Val);
}

/**
  Port of pcie_config_wrapper (sg2260_pcie.c line 1158).
  FSBL uses mmio_write_64 here; in PEI we split each 64-bit write into two
  32-bit writes (low then high) since the wrapper apertures are 32-bit.
**/
STATIC
VOID
PcieConfigWrapper (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId
  )
{
  UINTN   BaseAddr;
  UINT64  Val64;
  UINT32  Val;

  BaseAddr = (UINTN)(SG_C2C_PCIE_WAPPER_0_BASE (C2cId) + (UINT64)WrapperId * 0x500ULL);
  Val64    = (UINT64)(SG_C2C_PCIE_MSI_RX_0_BASE (C2cId) + (UINT64)WrapperId * 0x500ULL);

  MmioWrite32 (BaseAddr + 0x0, (UINT32)(Val64 & 0xffffffffu));
  MmioWrite32 (BaseAddr + 0x4, (UINT32)(Val64 >> 32));
  MmioWrite32 (BaseAddr + 0x8, (UINT32)(Val64 & 0xffffffffu));
  MmioWrite32 (BaseAddr + 0xc, (UINT32)(Val64 >> 32));

  Val64 = 0x7ee0u;
  MmioWrite32 (BaseAddr + 0x10, (UINT32)(Val64 & 0xffffffffu));
  MmioWrite32 (BaseAddr + 0x14, (UINT32)(Val64 >> 32));

  Val  = MmioRead32 (BaseAddr + 0x6c);
  Val &= 0x0fffffffu;       // bit[31:28]
  Val |= 0x9u << 28;
  MmioWrite32 (BaseAddr + 0x6c, Val);

  Val  = MmioRead32 (BaseAddr + 0x1c);
  Val &= 0x0fffffffu;
  Val |= 0x9u << 28;
  MmioWrite32 (BaseAddr + 0x1c, Val);

  Val  = MmioRead32 (BaseAddr + 0x24);
  Val &= 0x0fffffffu;
  Val |= 0x9u << 28;
  MmioWrite32 (BaseAddr + 0x24, Val);
}

/**
  Port of pcie_config_resp_monitor_bypass (sg2260_pcie.c line 1193).
  bit_map[wrapper][phy] mirrors FSBL's table {{2,4},{3,5}}.
**/
STATIC
VOID
PcieConfigRespMonitorBypass (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId
  )
{
  STATIC CONST UINT32  BitMap[2][2] = { { 2, 4 }, { 3, 5 } };
  UINTN   PcieTopBase;
  UINT32  Val;

  PcieTopBase = (UINTN)SG_C2C_PCIE_TOP_BASE (C2cId);

  Val  = MmioRead32 (PcieTopBase + SG_C2C_TOP_RESP_MON_BYPASS_REG);
  Val |= 0x1u << BitMap[WrapperId][PhyId];
  MmioWrite32 (PcieTopBase + SG_C2C_TOP_RESP_MON_BYPASS_REG, Val);
}

/**
  Port of pcie_config_intx_irq_en (sg2260_pcie.c line 1207).
**/
STATIC
VOID
PcieConfigIntxIrqEn (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId
  )
{
  UINTN   PcieCtrlBase;
  UINTN   PcieTopBase;
  UINT32  Val;

  PcieGetBases (C2cId, WrapperId, PhyId, NULL, NULL, &PcieCtrlBase);
  PcieTopBase = (UINTN)SG_C2C_PCIE_TOP_BASE (C2cId);

  // irq_intx_en enable.
  Val  = MmioRead32 (PcieCtrlBase + SG_PCIE_CTRL_IRQ_EN_REG);
  Val |= SG_PCIE_CTRL_IRQ_EN_INTX_EN_MASK;
  MmioWrite32 (PcieCtrlBase + SG_PCIE_CTRL_IRQ_EN_REG, Val);

  // clear intx mask.
  Val  = MmioRead32 (PcieTopBase + SG_C2C_TOP_IRQ_CTRL_REG);
  Val &= ~SG_C2C_TOP_IRQ_CTRL_PCIE_INT_MASK_MASK;
  MmioWrite32 (PcieTopBase + SG_C2C_TOP_IRQ_CTRL_REG, Val);
}

/**
  Port of pcie_config_axi_route (sg2260_pcie.c line 1375) for server mode only.
  FSBL's CONFIG_DRIVER_PCIE_FOLLOW_ORDERING_RULES block is dropped (per spec).
**/
STATIC
VOID
PcieConfigAxiRoute (
  IN  UINT32  C2cId,
  IN  UINT32  WorkMode
  )
{
  UINT64  CfgStartAddr;
  UINT64  CfgEndAddr;
  UINTN   TopBase;

  CfgStartAddr = SG_C2C_CFG_BASE (C2cId);
  CfgEndAddr   = SG_C2C_CFG_BASE (C2cId) + 0x01ffffffULL;
  TopBase      = (UINTN)SG_C2C_PCIE_TOP_BASE (C2cId);

  if (WorkMode == SG_SOC_WORK_MODE_SERVER) {
    MmioWrite32 (TopBase + 0x04, 0xffffffffu);  // rn.start
    MmioWrite32 (TopBase + 0x08, 0xffffffffu);
    MmioWrite32 (TopBase + 0x0c, 0xffffffffu);  // rn.end
    MmioWrite32 (TopBase + 0x10, 0xffffffffu);
    MmioWrite32 (TopBase + 0x14, 0x0u);         // rni.start
    MmioWrite32 (TopBase + 0x18, 0x0u);
    MmioWrite32 (TopBase + 0x1c, 0xffffffffu);  // rni.end
    MmioWrite32 (TopBase + 0x20, 0xffffffffu);
  }

  MmioWrite32 (TopBase + 0x24, (UINT32)(CfgStartAddr & 0xffffffffu));
  MmioWrite32 (TopBase + 0x28, (UINT32)((CfgStartAddr >> 32) & 0xffffffffu));
  MmioWrite32 (TopBase + 0x2c, (UINT32)(CfgEndAddr & 0xffffffffu));
  MmioWrite32 (TopBase + 0x30, (UINT32)((CfgEndAddr >> 32) & 0xffffffffu));
}

/**
  Port of pcie_init_phy (sg2260_pcie.c line 1596) for the RC path.

  The phy_id==0 block does the per-wrapper PHY setup (ss_mode, bl_bypass,
  pwr_stable, eq_afe, soft_phy_reset assert+deassert, soft_cold_reset). After
  that, RC mode deasserts PERST and udelay(20), then waits for SRAM init done
  and sets ext_ld_done on both phys when ss_mode==0.

  Returns EFI_TIMEOUT if either SRAM wait expires.
**/
STATIC
EFI_STATUS
PcieInitPhy (
  IN  UINT32  C2cId,
  IN  UINT32  WrapperId,
  IN  UINT32  PhyId,
  IN  UINT32  SsMode,
  IN  UINT32  PcieDevType
  )
{
  EFI_STATUS  Status;

  //
  // The phy_id==0 per-wrapper PHY setup (ss_mode / bl_bypass / pwr_stable /
  // eq_afe / soft_phy_reset x2 / soft_cold_reset) is NOT done here -- it runs
  // once per wrapper in the entry-point pre-pass (PcieInitPhyWrapper) before
  // any controller starts. FSBL gates that block on (phy_id == 0) inside
  // pcie_init_phy because it has no separate pre-pass; here we hoist it out so
  // each wrapper's PHY reset sequence runs exactly once, not once per phy0
  // controller on that wrapper (which would re-assert PHY/cold reset after the
  // pre-pass already released them).
  //

  if (PcieDevType == SG_PCIE_DEV_TYPE_RC) {
    Status = PcieSetPerst (C2cId, WrapperId, PhyId, 1);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: deassert_perst C2C%u W%u P%u failed: %r\n",
        __func__,
        C2cId,
        WrapperId,
        PhyId,
        Status
        ));
      return Status;
    }

    MicroSecondDelay (20);
  }

  Status = PcieWaitSramInitDone (C2cId, WrapperId, PhyId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SsMode == 0) {
    Status = PcieWaitSramInitDone (C2cId, WrapperId, 1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  // CONFIG_DRIVER_PCIE_PHY_FW_RELOAD is unset in production -- skip
  // pcie_update_phy_sram. PEI runs S-mode with MMU off, so the FSBL
  // sync_is()/wbinv_va_range() cache sync around it is a no-op here.
  PcieConfigExloadPhy (C2cId, WrapperId, PhyId);
  if (SsMode == 0) {
    PcieConfigExloadPhy (C2cId, WrapperId, 1);
  }

  return EFI_SUCCESS;
}

//
// ---- Top-level: per-wrapper PHY setup, per-controller sequence ----
//

/**
  Run the phy_id==0 per-wrapper PHY setup block once for one wrapper.

  Reads C2cId / WrapperId / SerdesMode (ss_mode) from the PCD table entry.
  This MUST run for every wrapper before any controller on that wrapper starts
  the per-controller sequence -- it programs ss_mode and asserts the PHY/COLD
  resets that the subsequent pcie_wait_sram_init_done depends on.
**/
STATIC
EFI_STATUS
PcieInitPhyWrapper (
  IN CONST PCIE_PHY  *Phy
  )
{
  if (Phy == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: PHY setup C2C%u W%u ss_mode=%u\n",
    __func__,
    Phy->C2cId,
    Phy->WrapperId,
    Phy->SerdesMode
    ));

  PcieConfigSsMode (Phy->C2cId, Phy->WrapperId, Phy->SerdesMode);
  PcieConfigPhyBlBypass (Phy->C2cId, Phy->WrapperId, SG_PCIE_PHY_NO_SRAM_BYPASS);
  PcieConfigPhyPwrStable (Phy->C2cId, Phy->WrapperId);
  PcieConfigPhyEqAfe (Phy->C2cId, Phy->WrapperId);
  PcieConfigSoftPhyReset (Phy->C2cId, Phy->WrapperId, SG_PCIE_RST_ASSERT);
  PcieConfigSoftPhyReset (Phy->C2cId, Phy->WrapperId, SG_PCIE_RST_DE_ASSERT);
  PcieConfigSoftColdReset (Phy->C2cId, Phy->WrapperId);

  return EFI_SUCCESS;
}

/**
  Run the full RC bring-up sequence for one controller.

  Mirrors FSBL sg2044_pcie_init() (line 1958) in call order, minus the
  slave-map calls (DwPcieSetSlaveMap owns those in EDK2). Returns EFI_TIMEOUT
  on any bounded wait expiry; the caller logs and continues with the next
  controller so one dead link cannot brick the boot.
**/
STATIC
EFI_STATUS
PcieInitController (
  IN CONST PCIE_CONTROLLER  *Controller
  )
{
  EFI_STATUS        Status;
  UINT32            C2cId;
  UINT32            WrapperId;
  UINT32            PhyId;
  UINT32            GenSpeed;
  UINT32            LaneCount;
  UINT32            SsMode;
  CONST BOARD_SLOT  *Slot;
  UINT16            SlotNumber;

  if (Controller == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  C2cId     = Controller->CtrlInit.C2cId;
  WrapperId = Controller->CtrlInit.WrapperId;
  PhyId     = Controller->CtrlInit.PhyId;
  GenSpeed  = Controller->CtrlInit.GenSpeed;
  LaneCount = Controller->CtrlInit.LaneCount;

  //
  // ss_mode is a per-wrapper property. The PHY/wrapper setup ran once per
  // wrapper in the entry point; for the per-controller pcie_init_phy call we
  // look it up again from the matching Phy[] entry -- same wrapper owns both
  // controllers, so ss_mode is identical. (If no matching Phy entry exists we
  // default to X8, matching FSBL's PCIE_SERDES_MODE_X8 = 0.)
  //
  SsMode = SG_PCIE_SERDES_MODE_X8;
  {
    PCIE_HOST_BRIDGE_TABLE  *Table;
    UINT8                   Idx;

    Table = (PCIE_HOST_BRIDGE_TABLE *)PcdGetPtr (PcdPcieHostBridgeTable);
    if (Table != NULL) {
      for (Idx = 0; Idx < Table->NumOfPhys; Idx++) {
        if ((Table->Phy[Idx].C2cId == C2cId) &&
            (Table->Phy[Idx].WrapperId == WrapperId))
        {
          SsMode = Table->Phy[Idx].SerdesMode;
          break;
        }
      }
    }
  }

  if (C2cId == 2) {
    DEBUG ((
      DEBUG_INFO,
      "PcieInitPei: CXP-P%u Gen%u x%u\n",
      PhyId,
      GenSpeed,
      LaneCount
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "PcieInitPei: C2C%u-W%u-P%u Gen%u x%u\n",
      C2cId,
      WrapperId,
      PhyId,
      GenSpeed,
      LaneCount
      ));
  }

  Status = PcieInitPhy (C2cId, WrapperId, PhyId, SsMode, SG_PCIE_DEV_TYPE_RC);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PcieWaitCoreClk (C2cId, WrapperId, PhyId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PcieCheckRadmStatus (C2cId, WrapperId, PhyId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PcieConfigCtrl (C2cId, WrapperId, PhyId, GenSpeed, SG_PCIE_DEV_TYPE_RC);
  PcieConfigEq (C2cId, WrapperId, PhyId);
  PcieConfigLink (C2cId, WrapperId, PhyId, LaneCount);
  PcieConfigRcBar (C2cId, WrapperId, PhyId);

  //
  // Physical slot number for the root port's config space: only
  // RC-direct slots qualify (PcieSlotInfoByDomain ignores switch
  // downstream slots).  Controllers whose lanes feed an onboard
  // switch or device get PCIE_SLOT_NUMBER_NONE and are configured
  // Slot Implemented = 0, SLTCAP = 0 explicitly.
  //
  Slot       = PcieSlotInfoByDomain (Controller->Domain);
  SlotNumber = (Slot != NULL) ? Slot->SlotNumber : PCIE_SLOT_NUMBER_NONE;
  PcieConfigRcCap (C2cId, WrapperId, PhyId, LaneCount, SlotNumber);

  PcieEnableLtssm (C2cId, WrapperId, PhyId);

  Status = PcieWaitLink (C2cId, WrapperId, PhyId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PcieConfigRespMonitorBypass (C2cId, WrapperId, PhyId);
  PcieConfigIntxIrqEn (C2cId, WrapperId, PhyId);
  PcieConfigAxiRoute (C2cId, SG_SOC_WORK_MODE_SERVER);
  PcieConfigMps (C2cId, WrapperId, PhyId, 0);
  PcieConfigMrrs (C2cId, WrapperId, PhyId, 0);

  // pcie_clear_slv_mapping + pcie_config_slv_mapping deliberately SKIPPED --
  // the EDK2 DesignWare PCIe DXE driver (DwPcieSetSlaveMap) owns the slave
  // map, fed from PcdPcieHostBridgeTable Space32/Space64.

  PcieConfigWrapper (C2cId, WrapperId);

  return EFI_SUCCESS;
}

/**
  PEIM entry point.

  @param[in]  FileHandle  EFI_PEI_FILE_HANDLE for this module.
  @param[in]  PeiServices Pointer to the PEI Services table.

  @retval EFI_SUCCESS  Bring-up completed (some controllers may have failed
                       but we continued). The PEIM never returns an error just
                       because one link failed to train.
**/
EFI_STATUS
EFIAPI
PcieInitPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS              Status;
  PCIE_HOST_BRIDGE_TABLE  *Table;
  UINT8                   Idx;
  UINT8                   FailedControllers;

  //
  // Locate SOPHGO_GPIO_PPI for PERST. Depex guarantees it is available.
  //
  Status = PeiServicesLocatePpi (
             &gSophgoGpioPpiGuid,
             0,
             NULL,
             (VOID **)&mGpioPpi
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: SOPHGO_GPIO_PPI not found: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Table = (PCIE_HOST_BRIDGE_TABLE *)PcdGetPtr (PcdPcieHostBridgeTable);
  if ((Table == NULL) || (Table->NumOfControllers == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: PcdPcieHostBridgeTable empty\n",
      __func__
      ));
    return EFI_SUCCESS;
  }

  //
  // (c) Iterate Phy[] and run the per-wrapper PHY setup once per wrapper,
  // BEFORE any per-controller sequence. This programs ss_mode and asserts /
  // deasserts the PHY/COLD resets that the subsequent SRAM waits depend on.
  //
  for (Idx = 0; Idx < Table->NumOfPhys; Idx++) {
    PcieInitPhyWrapper (&Table->Phy[Idx]);
  }

  //
  // (d) Iterate Controller[] and run the per-controller RC bring-up sequence.
  // A controller failure (typically a link-timeout on an empty slot) does NOT
  // abort the loop -- one dead link must not brick the boot. It is logged at
  // DEBUG_INFO because an empty slot is a normal state, not a fault.
  //
  FailedControllers = 0;
  for (Idx = 0; Idx < Table->NumOfControllers; Idx++) {
    Status = PcieInitController (&Table->Controller[Idx]);
    if (EFI_ERROR (Status)) {
      FailedControllers++;
      DEBUG ((
        DEBUG_INFO,
        "%a: controller[%u] (C2C%u W%u P%u) FAILED: %r -- continuing\n",
        __func__,
        Idx,
        Table->Controller[Idx].CtrlInit.C2cId,
        Table->Controller[Idx].CtrlInit.WrapperId,
        Table->Controller[Idx].CtrlInit.PhyId,
        Status
        ));
    } else {
      DEBUG ((
        DEBUG_INFO,
        "%a: controller[%u] (C2C%u W%u P%u) link up\n",
        __func__,
        Idx,
        Table->Controller[Idx].CtrlInit.C2cId,
        Table->Controller[Idx].CtrlInit.WrapperId,
        Table->Controller[Idx].CtrlInit.PhyId
        ));
    }
  }

  //
  // (e) Always return EFI_SUCCESS: a partial failure (some links up, some not)
  // is still a bootable state -- empty slots are expected. Summarised at
  // DEBUG_INFO so RELEASE builds stay silent when nothing is plugged in.
  //
  if (FailedControllers != 0) {
    DEBUG ((
      DEBUG_INFO,
      "%a: %u/%u controllers failed bring-up (continuing)\n",
      __func__,
      FailedControllers,
      Table->NumOfControllers
      ));
  }

  return EFI_SUCCESS;
}
