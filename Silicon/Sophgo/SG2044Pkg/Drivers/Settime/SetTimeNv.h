/** @file
This is a device manager driver might export data to the HII protocol to be
later utilized by the Setup Protocol.

Copyright (c) 2024, Sophgo Corporation. All rights reserved.<BR>
**/

#ifndef _SETTIME_NV_H_
#define _SETTIME_NV_H_

#define TIME_SET_FORMSET_GUID { 0x308a3744, 0x6aa6, 0x4f37,{ 0xae, 0x9d, 0xfd, 0xc3, 0xc6, 0xb0, 0xd6, 0x86 } }
#define TIME_SET_FORM_ID 0x8000
#define LABEL_TIME 	 0x1000
#define LABEL_END  	 0xffff
#define QUESTION_ID_TIME 0x8005
#define QUESTION_ID_DATE 0x8004
#define VAR_DYNAMIC_TIME_VARID 0x1004
//
// NV Data Structure Definition
//
#pragma pack(1)
typedef struct {
  UINT16 Year;
  UINT8  Month;
  UINT8  Day;
  UINT8  Hour;
  UINT8  Minute;
  UINT8  Second;
  UINT8  FocusField;
} TIME_DATA;

#pragma pack()
#endif
