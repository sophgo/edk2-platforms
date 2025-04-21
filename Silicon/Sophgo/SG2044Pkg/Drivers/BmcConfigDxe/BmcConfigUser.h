#ifndef __BMC_CONFIG_USER_H__
#define __BMC_CONFIG_USER_H__

#include "BmcConfigNv.h"
#include "BmcConfig.h"


EFI_STATUS
EFIAPI
SendBmcUserForm(
    NET_PRIVATE_DATA *PrivateData
  );

VOID
UpdateBmcUserForm(
    NET_PRIVATE_DATA *PrivateData
  );

EFI_STATUS
ValidatePasswordFormat (
  IN CONST CHAR16                    *Password,
  IN NET_PRIVATE_DATA                *Private
  );

BOOLEAN
IsSpecialChar(
  CHAR16 c
  );

EFI_STATUS
RecordPassword (
  IN   NET_PRIVATE_DATA             *Private,
  IN   EFI_STRING_ID                 StringId,
  IN   CHAR16                        *StringBuffer,
  IN   UINTN                         StringBufferLen
  );

EFI_STATUS
ProcessPasswordSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  );

EFI_STATUS
EFIAPI
UpdateBmcUserInfo(
    NET_PRIVATE_DATA *PrivateData
  );
#endif
