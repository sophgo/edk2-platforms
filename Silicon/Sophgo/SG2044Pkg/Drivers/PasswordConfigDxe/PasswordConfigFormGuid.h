/** @file
The guid define header file of HII Config Access protocol implementation of password configuration module.

Copyright (c) 2024, Sophgo. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef  PASSWORDCONFIG_FORM_GUID_H_
#define  PASSWORDCONFIG_FORM_GUID_H_


#define  PASSWORD_CONFIG_VARIABLE          L"PasswordConfigSetup"

#define  FORM_PASSWORDCONFIG_ID            0x9001

#define  TRIGGER_ID                        0x9101

#define  FORM_USER_PASSWD_OPEN               0x9f05
#define  FORM_ADMIN_PASSWD_OPEN              0x9f06
#define  FORM_CLEAN_USER_PASSWORD            0x9f07
#define  FORM_USER_PASSWD_ENABLE             0x9f08

#define  LABEL_FORM_PASSWORDCONFIG_START   0xff0c
#define  LABEL_FORM_PASSWORDCONFIG_END     0xff0d
#define  LABEL_END                         0xffff

#endif
