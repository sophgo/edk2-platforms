

#include "BmcConfigUser.h"
#include "BmcConfigIpmi.h"

UINT8                 mCheckFlag;
extern EFI_GUID       gBmcConfigFormSetGuid;
EFI_STRING_ID TokenList[16] = {
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_0),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_1),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_2),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_3),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_4),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_5),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_6),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_7),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_8),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_9),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_10),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_11),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_12),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_13),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_14),
  STRING_TOKEN(STR_BMC_USER_NAME_VAL_15),
};

/**
  Show Bmc Info
  @retval void
**/
EFI_STATUS
EFIAPI
UpdateBmcUserInfo(
    NET_PRIVATE_DATA *PrivateData
  )
{
  UINT8                       UserId;
  EFI_STATUS                  Status;
  BMC_DATA                   *BmcData;

  BmcData = &PrivateData->BmcConfigData;
  if (BmcData == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }


	BmcData->UserConfigData.UserTotalNum = 0;
	for(UserId = 1; UserId < MAX_USER_NUMBER; UserId ++) {
		BmcData->UserConfigData.UserEntry[UserId].Flag = 0;
		Status = IpmiGetUserName(UserId, BmcData->UserConfigData.UserEntry[UserId].UserName,
					sizeof(BmcData->UserConfigData.UserEntry[UserId].UserName));

		if (!EFI_ERROR(Status)) {
			BmcData->UserConfigData.UserTotalNum ++;
			HiiSetString(PrivateData->HiiHandle, TokenList[UserId], BmcData->UserConfigData.UserEntry[UserId].UserName, NULL);
			BmcData->UserConfigData.UserEntry[UserId].Flag = 1;
      DEBUG ((DEBUG_INFO, "User %d: %s\n", UserId, BmcData->UserConfigData.UserEntry[UserId].UserName));
		}
	}
	Status = UpdateBmcVarStore(BmcData);

	if (EFI_ERROR(Status))  {
		DEBUG((
		DEBUG_ERROR,
		"%a: gRT->SetVariable - %r\n",
		__func__,
		Status));
	}
  return EFI_SUCCESS;
}

/**
  Show Bmc Info
  @retval void
**/
EFI_STATUS
EFIAPI
SendBmcUserForm(
    NET_PRIVATE_DATA *PrivateData
  )
{
  EFI_STATUS                  Status;
	EFI_FORM_BROWSER2_PROTOCOL *FormBrowser;

	Status = gBS->LocateProtocol(&gEfiFormBrowser2ProtocolGuid, NULL, (VOID**)&FormBrowser);

	if (!EFI_ERROR(Status)) {
		Status = FormBrowser->SendForm(
			FormBrowser,
			&PrivateData->HiiHandle,
			1,
			&gBmcConfigFormSetGuid,
			BMC_USER_FORM_ID,
			NULL,
			NULL
		);

		if (EFI_ERROR(Status))  {
			DEBUG((
			DEBUG_ERROR,
			"%a: FormBrowser->SendForm - %r\n",
			__func__,
			Status));
		}

	} else {
		DEBUG((
			DEBUG_ERROR,
			"%a: gBS->LocateProtocol - %r\n",
			__func__,
			Status));
	}

  return Status;
}

/**
  Set/Get Lan Param
  @retval void
**/
VOID
UpdateBmcUserForm(
    NET_PRIVATE_DATA *PrivateData
  )
{
  EFI_STATUS Status;
  VOID *StartOpCodeHandle;
  VOID *EndOpCodeHandle;

  StartOpCodeHandle = HiiAllocateOpCodeHandle();
  EndOpCodeHandle = HiiAllocateOpCodeHandle();

  if (StartOpCodeHandle == NULL || EndOpCodeHandle == NULL)
  {
    DEBUG((DEBUG_ERROR, "Failed to allocate opcode handles.\n"));
    return;
  }

  Status = HiiUpdateForm(
      PrivateData->HiiHandle,
      &gBmcConfigFormSetGuid,
      BMC_USER_FORM_ID,
      StartOpCodeHandle,
      NULL);

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to update the form: %r\n", Status));
  }
  HiiFreeOpCodeHandle(StartOpCodeHandle);
  HiiFreeOpCodeHandle(EndOpCodeHandle);
}

/**
  Display a centered popup message on the screen.

  @param[in]  Message    The message to display
  @param[out] Key       Pointer to receive the key pressed by user

  @retval EFI_SUCCESS   The message was displayed successfully
**/
STATIC
EFI_STATUS
ShowCenteredPopup (
  IN  CONST CHAR16    *Message,
  OUT EFI_INPUT_KEY   *Key
  )
{
  UINTN     Columns;
  UINTN     Rows;
  UINTN     MaxLineLength;
  UINTN     StartCol;
  UINTN     StartRow;
  UINTN     LineCount;
  CHAR16    *CurrentChar;
  UINTN     CurrentLineLength;

  //
  // Get current screen size
  //
  gST->ConOut->QueryMode (
                 gST->ConOut,
                 gST->ConOut->Mode->Mode,
                 &Columns,
                 &Rows
                 );

  //
  // Set background color
  //
  gST->ConOut->SetAttribute (gST->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE);

  //
  // Count lines and find maximum line length
  //
  LineCount = 1;
  MaxLineLength = 0;
  CurrentLineLength = 0;
  CurrentChar = (CHAR16*)Message;

  while (*CurrentChar != L'\0') {
    if (*CurrentChar == L'\n') {
      LineCount++;
      if (CurrentLineLength > MaxLineLength) {
        MaxLineLength = CurrentLineLength;
      }
      CurrentLineLength = 0;
    } else {
      CurrentLineLength++;
    }
    CurrentChar++;
  }

  //
  // Check last line length
  //
  if (CurrentLineLength > MaxLineLength) {
    MaxLineLength = CurrentLineLength;
  }

  //
  // Calculate starting position
  //
  StartCol = (Columns - MaxLineLength) / 2;
  StartRow = (Rows - LineCount) / 2;

  //
  // Set cursor position for first line
  //
  gST->ConOut->SetCursorPosition (gST->ConOut, StartCol, StartRow);

  //
  // Create popup with proper spacing
  //
  CreatePopUp (
    EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
    Key,
    Message,
    NULL
    );

  return EFI_SUCCESS;
}

/**
  Check if the password meets format requirements.
  Password must be 8-20 characters and contain at least two of:
  special characters, uppercase letters, lowercase letters, and digits.

  @param[in]  Password    The password string to check
  @param[in]  Private     Pointer to private data structure for HII message

  @retval EFI_SUCCESS           Password format is valid
  @retval EFI_INVALID_PARAMETER Password format is invalid
**/
EFI_STATUS
ValidatePasswordFormat (
  IN CONST CHAR16                    *Password,
  IN NET_PRIVATE_DATA                *Private
  )
{
  UINTN     Length;
  UINTN     Index;
  BOOLEAN   HasUpper;
  BOOLEAN   HasLower;
  BOOLEAN   HasDigit;
  BOOLEAN   HasSpecial;
  UINT8     TypeCount;
  EFI_INPUT_KEY Key;

  if (Password == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check length (8-20 characters)
  //
  Length = StrLen(Password);
  if (Length < MIN_LEN_USER_PASSWORD || Length > MAX_LEN_USER_PASSWORD) {
    ShowCenteredPopup(
      L"Password must be 8-20 characters long!",
      &Key
    );
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check character types
  //
  HasUpper = FALSE;
  HasLower = FALSE;
  HasDigit = FALSE;
  HasSpecial = FALSE;

  for (Index = 0; Index < Length; Index++) {
    if (Password[Index] >= L'A' && Password[Index] <= L'Z') {
      HasUpper = TRUE;
    } else if (Password[Index] >= L'a' && Password[Index] <= L'z') {
      HasLower = TRUE;
    } else if (Password[Index] >= L'0' && Password[Index] <= L'9') {
      HasDigit = TRUE;
    } else if (Password[Index] == L'!' || Password[Index] == L'@' ||
               Password[Index] == L'#' || Password[Index] == L'$' ||
               Password[Index] == L'%' || Password[Index] == L'^' ||
               Password[Index] == L'&' || Password[Index] == L'*' ||
               Password[Index] == L'(' || Password[Index] == L')' ||
               Password[Index] == L'-' || Password[Index] == L'_' ||
               Password[Index] == L'+' || Password[Index] == L'=' ||
               Password[Index] == L'[' || Password[Index] == L']' ||
               Password[Index] == L'{' || Password[Index] == L'}' ||
               Password[Index] == L';' || Password[Index] == L':' ||
               Password[Index] == L',' || Password[Index] == L'.' ||
               Password[Index] == L'<' || Password[Index] == L'>' ||
               Password[Index] == L'?' || Password[Index] == L'/') {
      HasSpecial = TRUE;
    } else {
      ShowCenteredPopup(
        L"Password contains invalid character!",
        &Key
      );
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Count how many types of characters are present
  //
  TypeCount = 0;
  if (HasUpper) TypeCount++;
  if (HasLower) TypeCount++;
  if (HasDigit) TypeCount++;
  if (HasSpecial) TypeCount++;

  if (TypeCount < 2) {
    ShowCenteredPopup (
      L"Password requires at least two of: A-Z, a-z, 0-9, or special characters!",
      &Key
    );
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Record password from a HII input string.

  @param[in]  Private             The pointer to the global private data structure.
  @param[in]  StringId            The QuestionId received from HII input.
  @param[in]  StringBuffer        The unicode string buffer to store password.
  @param[in]  StringBufferLen     The len of unicode string buffer.

  @retval     EFI_INVALID_PARAMETER   Any input parameter is invalid.
  @retval     EFI_NOT_FOUND           The password string is not found or invalid.
  @retval     EFI_SUCCESS             The operation is completed successfully.
**/
EFI_STATUS
RecordPassword (
  IN   NET_PRIVATE_DATA             *Private,
  IN   EFI_STRING_ID                 StringId,
  IN   CHAR16                        *StringBuffer,
  IN   UINTN                         StringBufferLen
  )
{
  CHAR16  *Password;

  if ((StringId == 0) || (StringBuffer == NULL) || (StringBufferLen <= 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Password = HiiGetString (Private->HiiHandle, StringId, NULL);
  if (Password == NULL) {
    return EFI_NOT_FOUND;
  }

  if (StrLen (Password) > StringBufferLen) {
    FreePool (Password);
    return EFI_NOT_FOUND;
  }

  StrnCpyS (StringBuffer, StringBufferLen, Password, StrLen (Password));
  ZeroMem (Password, (StrLen (Password) + 1) * sizeof (CHAR16));
  FreePool (Password);

  //
  // Clean password in string package
  //
  HiiSetString (Private->HiiHandle, StringId, L"", NULL);
  return EFI_SUCCESS;
}

EFI_STATUS
ProcessPasswordSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  )
{
  EFI_STATUS                     Status;
  CHAR8                          *UserPassword;
  CHAR16                         *TempPassword;
	CHAR16                         *OldPassword;
  UINT8                          UserId;

  UserPassword = AllocateZeroPool (sizeof (CHAR8) * MAX_LEN_USER_PASSWORD);
  TempPassword = AllocateZeroPool (sizeof (CHAR16) * MAX_LEN_USER_PASSWORD);
	OldPassword  = AllocateZeroPool (sizeof (CHAR16) * MAX_LEN_USER_PASSWORD);
  UserId = QuestionId - KEY_PASSWORD_BASE;

  Status = RecordPassword (Private, Value->string, TempPassword, MAX_LEN_USER_PASSWORD);

	if (StrLen(TempPassword) == 0){
		mCheckFlag = 1;
    Status = EFI_SUCCESS;
	}  else if (StrLen(TempPassword) > 0 && mCheckFlag == 1) {
		Status = ValidatePasswordFormat(TempPassword, Private);
		if (EFI_ERROR (Status)) {
			DEBUG ((DEBUG_ERROR, "Error: Failed to input password!"));
		} else {
			if (StrLen (TempPassword) < MIN_LEN_USER_PASSWORD) {
				Status = EFI_SUCCESS;
			} else if (StrLen (TempPassword) < MAX_LEN_USER_PASSWORD) {
				Status = EFI_SUCCESS;
				UnicodeStrToAsciiStrS (TempPassword, UserPassword, StrLen (TempPassword) + 1);
				Status = IpmiSetUserPassword (UserId, IPMI_SET_USER_PASSWORD_PASSWORD_SIZE_20, IPMI_SET_USER_PASSWORD_OPERATION_TYPE_SET_PASSWORD, UserPassword);
				CopyMem (Private->BmcConfigData.UserConfigData.UserEntry[UserId].Password, TempPassword, StrLen (TempPassword));
				if (EFI_ERROR(Status)) {
					DEBUG((DEBUG_ERROR, "\n[IPMI] BMC does not respond (status: %r)!\n\n", Status));
				}
			}
		}
		mCheckFlag = 0;
	}

	if (TempPassword != NULL) {
    FreePool (TempPassword);
  }
  if (OldPassword != NULL) {
    FreePool (OldPassword);
  }
	if (UserPassword != NULL) {
    FreePool (UserPassword);
  }
  return Status;
}
