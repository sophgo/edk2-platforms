/** @file
  The header file of Debug Config form guid defines.

  Copyright (c) 2026, SOPHGO Technology Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef  DEBUG_CONFIG_NV_H_
#define  DEBUG_CONFIG_NV_H_

#define  DEBUG_CONFIG_FORMSET_GUID\
  { 0x3A7F9B12, 0xC4E1, 0x4D86, { 0xA9, 0x2B, 0xE8, 0x55, 0xF0, 0xD3, 0x71, 0x44 } }

#define  FORM_DEBUG_CONFIG_ID            0x3700
#define  ENABLE_SERIAL_PORT_QUESTION_ID  0x6100
#define  VARSTORE_ID_DEBUG_CONFIG        0x4300

#pragma pack()
typedef struct {
  UINT8 EnableSerialPort;
} DEBUG_CONFIG_DATA;
#pragma pack()
#endif