## @file
#  RISC-V EFI on Sophgo SG2042 SRA1-20 RISC-V platform
#
#  Copyright (c) 2025, SOPHGO Inc. All rights reserved.
#  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
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
  PLATFORM_NAME                  = SRA1-20
  PLATFORM_GUID                  = 8014637B-6999-4110-9762-464BE11E935F
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001c
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  DEFINE RTC_VIRTUAL             = TRUE
  DEFINE FIRMWARE_MANAGER_ENABLE = FALSE
  DEFINE IS_SERVER_PLATFORM      = TRUE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Sophgo/SG2042Pkg/SRA1-20/SRA1-20.fdf


!include Platform/Sophgo/SG2042Pkg/SG2042Common.dsc.inc
################################################################################
#
# PCDs that differ per board.
#
################################################################################
[PcdsFixedAtBuild]
  gSophgoTokenSpaceGuid.PcdSmbiosBoardType|2

