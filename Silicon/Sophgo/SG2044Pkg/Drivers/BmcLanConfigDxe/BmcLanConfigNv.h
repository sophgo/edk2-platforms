/** @file
This is a device manager driver might export data to the HII protocol to be
later utilized by the Setup Protocol.

Copyright (c) 2025  Sophgo Corporation. All rights reserved.<BR>
**/

#ifndef _BMC_LAN_CONFIG_NV_H_
#define _BMC_LAN_CONFIG_NV_H_

#define BMC_FORMSET_GUID                    { 0x84618f61, 0xed56, 0x430d, { 0x9a, 0xea, 0x7a, 0xa4, 0x8e, 0x01, 0x21, 0xf4 } }
#define VAR_BMC_DATA_GUID                   { 0x11a0dfca, 0x0cf7, 0x4d03, { 0xb5, 0x2a, 0x97, 0xe6, 0x9a, 0xdb, 0xc6, 0xbc } }
#define BMC_FORM_ID                         0x1006
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
//
// NV Data Structure Definition
//
#pragma pack(4)
typedef struct {
  CHAR16 IpAddress[64];
  CHAR16 SubnetMask[64];
  CHAR16 Gateway[64];
  UINT8  EnableDHCP;
} BMC_DATA;

#pragma pack()
#endif