/** @file
  The "DebugConfigData" NV variable layout and Debug form definitions.

  Maintained by DebugConfigDxe (Setup -> Debug) and consumed by other
  drivers that need the switches (e.g. AcpiPlatformDxe gates SPCR on
  EnableSerialPort).

  Copyright (c) 2026, SOPHGO Technology Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef  DEBUG_CONFIG_NV_H_
#define  DEBUG_CONFIG_NV_H_

#pragma pack(1)
typedef struct {
  UINT8  EnableSerialPort;
  UINT8  EnableEmulation;
} DEBUG_CONFIG_DATA;
#pragma pack()

#define  DEBUG_CONFIG_FORMSET_GUID\
  { 0x3A7F9B12, 0xC4E1, 0x4D86, { 0xA9, 0x2B, 0xE8, 0x55, 0xF0, 0xD3, 0x71, 0x44 } }

#define  FORM_DEBUG_CONFIG_ID            0x3700
#define  ENABLE_SERIAL_PORT_QUESTION_ID  0x6100
#define  ENABLE_EMULATION_QUESTION_ID    0x6101
#define  VARSTORE_ID_DEBUG_CONFIG        0x4300

#endif
