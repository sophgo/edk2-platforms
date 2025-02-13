/** @file
  The header file of Reserve Memory form guid defines.

  Copyright (c) 2025, SOPHGO Technology Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef  RESERVE_MEMORY_NV_H_
#define  RESERVE_MEMORY_NV_H_

#define  RESERVE_MEMORY_FORMSET_GUID\
  { 0x77EA1480, 0xA86F, 0x4E5A, { 0x9E, 0x48, 0x48, 0xDC, 0xF3, 0x7C, 0x97, 0x02 } }

#define  FORM_RESERVE_MEMORY_ID       0x4000
#define  RESERVED_MEM_QUESTION_ID     0x4100
#define  VARSTORE_ID_RESERVE_MEMORY   0x4200

#pragma pack()
typedef struct {
  UINT32 Value;
} RESERVE_MEMORY_DATA;
#define pack()
#endif
