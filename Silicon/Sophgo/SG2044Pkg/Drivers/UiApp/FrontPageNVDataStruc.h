/** @file
  Header file for vfr definition.

  Copyright (c) 2024, Sophgo. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef FRONT_PAGE_NV_DATA_STRUC_H_
#define FRONT_PAGE_NV_DATA_STRUC_H_

//
// VarStore 名称和相关定义
//
#define TIME_DATA_VARSTORE_NAME           L"DynamicTimeData"
#define VAR_DYNAMIC_TIME_VARID            0x1001
#define DYNAMIC_TIME_QUESTION_ID          0x1002
#define FORMSET_GUID                      { 0xadf98142, 0x42c4, 0x429c, { 0x9f, 0xa4, 0x62, 0x3f, 0xf9, 0x94, 0xa1, 0x40 } }
#define CONFIG_INI_FORMSET_GUID {0x4a618233, 0x07f9, 0x4d73, { 0x91, 0x53, 0x51, 0x1f, 0x28, 0x93, 0xa0, 0x1e } }
#define FRONT_PAGE_FORM_ID             0x1000
#define NEW_FORM_ID                    0x1000
#define LABEL_TIME_START               0xfffe
#define LABEL_FRONTPAGE_INFORMATION    0x1000
#define LABEL_END                      0xffff
#define VAR_DYNAMIC_TIME_VARID         0x1001
#define CONFIG_FORM_ID                 0x1000
#define LABEL_CONFIG_START        0x1000
#define LABEL_CONFIG_END          0xffff
#define FRONT_PAGE_FORMSET_GUID {0xadf98142, 0x42c4, 0x429c, { 0x9f, 0xa4, 0x62, 0x3f, 0xf9, 0x94, 0xa1, 0x40 } }

#pragma pack(1)

//
// NV Data Structure Definition
//
typedef struct {
CHAR16 Time[10];
} TIME_DATA;

#define DYNAMIC_TIME_OFFSET OFFSET_OF(TIME_DATA, TimeInSeconds)

#pragma pack()

#endif

