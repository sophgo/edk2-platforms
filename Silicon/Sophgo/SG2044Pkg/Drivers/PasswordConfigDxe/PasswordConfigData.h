
/** @file
The header file of Password configuration Data.

Copyright (c) 2024, Sophgp. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef  PASSWORD_CONFIG_DATA_H_
#define  PASSWORD_CONFIG_DATA_H

#define VAR_PASSWORD_CONFIG_NAME                 L"PasswordConfigVar"
#define PLATFORM_SETUP_VARIABLE_FLAG             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE)
#define VAR_USR_PASSWD                           L"UsrPasswd"
#define VAR_ADMIN_PASSWD                         L"AdminPasswd"
#define PASSWD_MAXLEN                            64
#define PASSWD_MAXLEN_INPUT                      30
#define PASSWD_MAXLEN_ALLOW                      20
#define PASSWD_MINLEN                            0
#define PASSWORDCONFIG_FORMSET_GUID              { 0x11fc78c1, 0x805e, 0x458f, { 0x88, 0xf1, 0xe5, 0x28, 0xa2, 0x96, 0x5f, 0x6c }}
#define PASSWORDCONFIG_VAR_GUID                  { 0xc6a5204a, 0xc150, 0x4ae9, { 0x92, 0x96, 0x03, 0xbe, 0x8e, 0x38, 0x62, 0xc9 }}
#define PASSWORD_PRIV_GUID                       { 0x6e56303d, 0x5fb5, 0x41c5, { 0x9f, 0x2e, 0x06, 0x31, 0x97, 0xf2, 0xb6, 0xc9 }}
#define VARSTORE_ID_PASSWORD_CONFIG              0x9100
#define PASSWORD_PRIV                            L"PasswordPriv"

#pragma pack(2)
typedef struct {
  UINT8         UserPasswordEnable;
  UINT8         AdminPasswordEnable;
  UINT8         UserPriv;
  CHAR16        UserPassword[PASSWD_MAXLEN];
  CHAR16        AdminPassword[PASSWD_MAXLEN];
} PASSWORD_CONFIG_DATA;
#define pack()
#endif
