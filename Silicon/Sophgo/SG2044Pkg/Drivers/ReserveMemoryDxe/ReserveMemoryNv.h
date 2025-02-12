/** @file
  The header file of Reserve Memory form guid defines.

  Copyright (c) 2025, SOPHGO Technology Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef  RESERVE_MEMORY_NV_H_
#define  RESERVE_MEMORY_NV_H_

#define  RESERVE_MEMORY_FORMSET_GUID\
  { 0x77EA1480, 0xA86F, 0x4E5A, { 0x9E, 0x48, 0x48, 0xDC, 0xF3, 0x7C, 0x97, 0x02 } }

#define  RESERVE_MEMORY_VAR_GUID\
  { 0x33A06A72, 0x4735, 0x41EF, { 0x81, 0x67, 0xD9, 0x8E, 0x5D, 0x21, 0x57, 0xBD } }

#define  RESERVE_MEMORY_VARIABLE      L"ReservedMemorySize"
#define  FORM_RESERVE_MEMORY_ID       0x4000
#define  RESERVED_MEM_QUESTION_ID     0x4100
#define  VARSTORE_ID_RESERVE_MEMORY   0x4200

#pragma pack(4)
typedef struct {
  UINT32 Value;
} RESERVE_MEMORY_DATA;
#define pack()
#endif
