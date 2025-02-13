/** @file
  Header file for vfr definition.

  Copyright (c) 2024, Sophgo. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef FRONT_PAGE_NV_DATA_STRUC_H_
#define FRONT_PAGE_NV_DATA_STRUC_H_
#include <Library/PasswordConfigData.h>

#define TIME_DATA_VARSTORE_NAME           L"DynamicTimeData"
#define DYNAMIC_TIME_QUESTION_ID          0x1002
#define FORMSET_GUID                      { 0xadf98142, 0x42c4, 0x429c, { 0x9f, 0xa4, 0x62, 0x3f, 0xf9, 0x94, 0xa1, 0x40 } }
#define TIME_SET_FORMSET_GUID             { 0x308a3744, 0x6aa6, 0x4f37, { 0xae, 0x9d, 0xfd, 0xc3, 0xc6, 0xb0, 0xd6, 0x86 } }
#define CONFIG_INI_FORMSET_GUID           { 0x4a618233, 0x07f9, 0x4d73, { 0x91, 0x53, 0x51, 0x1f, 0x28, 0x93, 0xa0, 0x1e } }
#define BMC_FORMSET_GUID                  { 0x84618f61, 0xed56, 0x430d, { 0x9a, 0xea, 0x7a, 0xa4, 0x8e, 0x01, 0x21, 0xf4 } }
#define FRONT_PAGE_FORM_ID             0x1000
#define SYSTEM_INFORMATION_ID          0x3000
#define SYSTEM_SETTING_ID              0x3100
#define BMC_FORM_ID                    0x3200
#define INFORMATION_FORM_ID            0x3300
#define FORM_PASSWORDCONFIG_ID         0x3400
#define TIME_SET_ID                    0x3500
#define SERIAL_PORT_FORM_ID            0x3600
#define VARSTORE_ID_PASSWORD_CHECK     0x4000
#define RESTORE_DEFAULTS_QUESTION_ID   0x4001
#define SERIAL_PORT_QUESTION_ID        0x5000

#define LABEL_TIME_START               0x2000
#define LABEL_LANGUAGE                 0x2001
#define LABEL_END                      0x2FFF
#define LABEL_MANAGER                  0x2002
#pragma pack()
typedef struct {
  UINT8 PasswordCheckEnabled;
  UINT8 IsFirst;
  UINT8 UserPriv;
  UINT8 IsEvb;
} PASSWORD_TOGGLE_DATA;
#pragma pack()
#endif


