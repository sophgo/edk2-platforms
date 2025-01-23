/** @file
  Header file for vfr definition.

  Copyright (c) 2024, Sophgo. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef FRONT_PAGE_NV_DATA_STRUC_H_
#define FRONT_PAGE_NV_DATA_STRUC_H_

#define CONFIG_INI_FORMSET_GUID        {0x4a618233, 0x07f9, 0x4d73, {0x91, 0x53, 0x51, 0x1f, 0x28, 0x93, 0xa0, 0x1e}}
#define LABEL_START		       0x1000
#define LABEL_END                      0xffff
#define CONFIG_FORM_ID                 0x1000
#define BIOS_INFORMATION_FORM_ID       0x1001
#define DDR_FORM_ID                    0x1002
#define CPU_FORM_ID                    0x1003
#define CHASSIS_FORM_ID                0x1004
#define PRODUCT_FORM_ID                0x1005
#define BOARD_FORM_ID                  0x1006
#define VAR_INFORMATION_VARID          0x1007
#define MAX_LENGTH                     64
#pragma pack(2)
typedef struct {
  UINT16 ProcessorMaxSpeed;
  UINT32 L1ICacheSize;
  UINT32 L1DCacheSize;
  UINT32 L2CacheSize;
  UINT32 L3CacheSize;
  UINT32 ExtendSize;
  UINT16 ExtendedSpeed;
  UINT8  MemoryRank;
  CHAR16 BiosVersion[MAX_LENGTH];
  CHAR16 BiosReleaseDate[MAX_LENGTH];
  CHAR16 BiosVendor[MAX_LENGTH];
  CHAR16 ProcessorVersion[MAX_LENGTH];
  CHAR16 PartNumber[MAX_LENGTH];
  CHAR16 MemoryType[MAX_LENGTH];
  CHAR16 BoardProductName[MAX_LENGTH];
  CHAR16 BoardVersion[MAX_LENGTH];
  CHAR16 ProductName[MAX_LENGTH];
  CHAR16 ProductVersion[MAX_LENGTH];
  CHAR16 Manufacturer[MAX_LENGTH];
} INFORMATION_DATA;
#pragma pack()
#endif
