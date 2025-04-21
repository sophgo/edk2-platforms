/** @file
This is a device manager driver might export data to the HII protocol to be
later utilized by the Setup Protocol.

Copyright (c) 2025  Sophgo Corporation. All rights reserved.<BR>
**/

#ifndef _BMC_CONFIG_NV_H_
#define _BMC_CONFIG_NV_H_

#define BMC_FORMSET_GUID                    { 0x84618f61, 0xed56, 0x430d, { 0x9a, 0xea, 0x7a, 0xa4, 0x8e, 0x01, 0x21, 0xf4 } }
#define BMC_FORM_ID                         0x3200
#define DHCP_QUESTION_ID                    0x1001
#define VAR_BMC_VARID                       0x1010
#define NETWORK_SET_FORM_ID                 0x1235
#define NETWORK_SET_IP_KEY_ID               0x2235
#define NETWORK_SET_SUBNET_KEY_ID           0x2236
#define NETWORK_SET_GATEWAY_KEY_ID          0x2237
#define NETWORK_GET_FORM_ID                 0x1234
#define LABEL_START                         0x3234
#define LABEL_END                           0x3235
#define LABEL_SET                           0x2345
#define REFRESH_QUESTION_ID                 0x3236
#define BASIC_INFO_FORM_ID                  0x4001
#define BASIC_INFO_REFRESH_ID               0x4002
#define PASSWORD_SUBMIT_ID                  0x4003
#define PASSWORD_QUESTION_ID                0x4004
#define LABEL_INFO_START                    0x4321
#define LABEL_INFO_END                      0x4123
#define MAC_ADDR_FORM_ID                    0x5001
#define MAC_ADDR_SET_KEY_ID                 0x5002
#define MAC_ADDR_REFRESH_ID                 0x5003
#define LABEL_MAC_START                     0x5432
#define LABEL_MAC_END                       0x5234
#define BMC_USER_FORM_ID                    0x6001
#define USER_REFRESH_ID                     0x6002
#define LABEL_USER_START                    0x6543
#define LABEL_USER_END                      0x6345

#define KEY_PASSWORD_BASE                   0x8900
#define KEY_PASSWORD_0                      0x8910
#define KEY_PASSWORD_1                      0x8901
#define KEY_PASSWORD_2                      0x8902
#define KEY_PASSWORD_3                      0x8903
#define KEY_PASSWORD_4                      0x8904
#define KEY_PASSWORD_5                      0x8905
#define KEY_PASSWORD_6                      0x8906
#define KEY_PASSWORD_7                      0x8907
#define KEY_PASSWORD_8                      0x8908
#define KEY_PASSWORD_9                      0x8909
#define KEY_PASSWORD_10                     0x890A
#define KEY_PASSWORD_11                     0x890B
#define KEY_PASSWORD_12                     0x890C
#define KEY_PASSWORD_13                     0x890D
#define KEY_PASSWORD_14                     0x890E
#define KEY_PASSWORD_15                     0x890F

#define MAX_USER_NUMBER                     0xf
#define MAX_LEN_USER_NAME                   20
#define MIN_LEN_USER_PASSWORD               8
#define MAX_LEN_USER_PASSWORD               20

//
// NV Data Structure Definition
//
#pragma pack()
// #pragma pack(1)
typedef struct {
  UINT8   Flag;
  CHAR16  UserName[64];
  CHAR16  Password[64];
} USERNAME_ENTRY;

typedef struct {
  UINT8           UserTotalNum;
  USERNAME_ENTRY  UserEntry[16];
} USER_CONFIG_DATA;

typedef struct {
  CHAR16           IpAddress[64];
  CHAR16           SubnetMask[64];
  CHAR16           Gateway[64];
  CHAR16           UserName[64];
  CHAR16           FmVersion[64];
  CHAR16           IpmiVersion[64];
  CHAR16           MacAddr[64];
  CHAR16           BootDeviceSelector[2];
  CHAR16           BootInitiator[2];
  CHAR16           BootIsPersistent[2];
  UINT8            UserTotalNum;
  UINT8            EnableDHCP;
  UINT8            BootSource;
  USER_CONFIG_DATA UserConfigData;
} BMC_DATA;

#pragma pack()
#endif
