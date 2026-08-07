## @file SRA3-40-LB.dsc
#
#  RISC-V EFI on SOPHGO SRA3-40 RISC-V platform (Large-BAR variant)
#
#  Same board as SRA3-40, but C2C0 Wrapper1 x8 is disabled so that C2C0
#  Wrapper0 x8 (Segment 0) owns the whole 128G C2C0 aperture as a single
#  prefetchable BAR, for GPGPU cards needing very large BARs (>=73.5G).
#
#  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = SRA3-40-LB
  PLATFORM_GUID                  = B8311BC8-0D5F-48DB-A7C9-24403AAFD37F
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001c
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Sophgo/SG2044Pkg/SRA3-40-LB/SRA3-40-LB.fdf

  #
  # Enable below options may cause build error or may not work on
  # the initial version of RISC-V package
  # Defines for default states.  These can be changed on the command line.
  # -D FLAG=VALUE
  #
  DEFINE SECURE_BOOT_ENABLE      = TRUE
  DEFINE DEBUG_ON_SERIAL_PORT    = TRUE
  DEFINE TPM2_ENABLE             = FALSE

  #
  # Network definition (IP6/TLS/HTTP_BOOT/ISCSI defaults come from
  # SG2044Common.dsc.inc; the SRA3-40 family enables SNP/IP4 and the
  # extra NETWORK_ENABLE / NETWORK_PXE_BOOT_ENABLE flags).
  #
  DEFINE NETWORK_ENABLE           = TRUE
  DEFINE NETWORK_SNP_ENABLE       = TRUE
  DEFINE NETWORK_IP4_ENABLE       = TRUE
  DEFINE NETWORK_PXE_BOOT_ENABLE  = FALSE

  DEFINE FLASH_ENABLE             = TRUE
  DEFINE ETH_ENABLE               = FALSE
  DEFINE ACPI_ENABLE              = TRUE

  #
  # Prebuilt Intel UNDI drivers (native RISCV64 PE32): Gigabit, 10G (ixgbe/X540),
  # ICE (E8xx), and I40e/700-series. Paths under Silicon/Sophgo/SG2044/iPXE/.
  # Enable with: -D INTEL_GIG_UNDI_ENABLE=TRUE or define TRUE below.
  #
  DEFINE INTEL_GIG_UNDI_ENABLE    = TRUE

  #
  # BMC: TRUE, using the Sophgo serial manageability transport.
  #
  DEFINE BMC_ENABLE               = TRUE
  DEFINE BMC_SSIF_ENABLE          = FALSE

  #
  # RTC: Ds1307 hardware RTC.
  #
  DEFINE RTC_VIRTUAL              = FALSE

  #
  # Sietium GPU GOP driver (only on SD3-10 / SD3-10-LB).
  #
  DEFINE SIETIUM_GOP_ENABLE       = FALSE

!include Platform/Sophgo/SG2044Pkg/SG2044Common.dsc.inc

################################################################################
#
# Variant-specific PCDs (board identity + MCU/SSIF I2C bus numbers).
#
################################################################################
[PcdsFixedAtBuild]
  gSophgoTokenSpaceGuid.PcdMCUI2cBus|1
  gSophgoTokenSpaceGuid.PcdSsifI2cBusNum|3

[PcdsFixedAtBuild.common]
  #
  # MCU existence / platform class / board+product identity
  #
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdMcuExistence|FALSE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdIsServerPlatform|TRUE
  # [board]
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdBoardName|L"SRA3"
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdBoardVersion|L"1.0"
  # [product]
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdProductName|L"SRA3-40-LB"
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdProductVersion|L"1.0"
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdProductSN|L"CYKJR32BDJFJF0001"
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdProductUUID|L"2FD7AA46-0C9C-4F92-9462-B8B7B9BEECA2"
