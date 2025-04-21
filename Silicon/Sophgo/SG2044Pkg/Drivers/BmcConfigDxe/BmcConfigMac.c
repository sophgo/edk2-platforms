
#include "BmcConfigMac.h"
#include "BmcConfigIpmi.h"
extern EFI_GUID gBmcConfigFormSetGuid;

//Verify the validity of the MAC address
BOOLEAN
isValidMacFormat(
  IN CONST CHAR16 *MacStr
  )
{
  STATIC CONST BOOLEAN mHexCharTable[0x80] = {
    ['0']=1,['1']=1,['2']=1,['3']=1,['4']=1,['5']=1,['6']=1,['7']=1,
    ['8']=1,['9']=1,['A']=1,['B']=1,['C']=1,['D']=1,['E']=1,['F']=1,
    ['a']=1,['b']=1,['c']=1,['d']=1,['e']=1,['f']=1
  };

  if (MacStr == NULL) {
    DEBUG((DEBUG_ERROR, "MAC string is NULL pointer\n"));
    return 0;
  }

  if (StrLen(MacStr) != 17) {
    DEBUG((DEBUG_ERROR, "Invalid MAC length: %d\n", StrLen(MacStr)));
    return 0;
  }

  for (UINTN i = 0; i < 17; i++) {
    CHAR16 c = MacStr[i];

    if ((i % 3) == 2) {
      if ((i % 3) == 2) {
      if (c != L':' && c != L'-') {
        DEBUG((DEBUG_ERROR, "Invalid separator '%c' at pos %d\n", c, i));
        return 0;
      }
    }
    } else {
      if (c > 0x007F || c >= ARRAY_SIZE(mHexCharTable) || !mHexCharTable[c]) {
        DEBUG((DEBUG_ERROR, "Invalid char 0x%04x at %d: %a\n",
        c, i,
        (c > 0x7F) ? "Non-ASCII" : "Bad HEX format"));
        return 0;
      }
    }
  }
  return 1;
}

UINT8
CharToHex (
  IN CHAR16  c
  )
{
  if (c >= L'0' && c <= L'9') {
    return (UINT8)(c - L'0');
  } else if (c >= L'A' && c <= L'F') {
    return (UINT8)(0xA + (c - L'A'));
  } else if (c >= L'a' && c <= L'f') {
    return (UINT8)(0xA + (c - L'a'));
  }
  return 0xFF;
}

EFI_STATUS
EFIAPI
ConvertChar16ToIpmiMac(
  IN  CHAR16  *MacStr,
  OUT UINT8   *Mac
  )
{
  UINTN   Index = 0;     // 输入字符串索引
  UINTN   ByteIndex = 0; // 输出数组索引
  UINT8   High, Low;

  if (MacStr == NULL || Mac == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  while (MacStr[Index] != L'\0' && ByteIndex < 6) {
    if (MacStr[Index] == L':') {
      Index++;
      continue;
    }

    High = CharToHex(MacStr[Index]);
    Low  = CharToHex(MacStr[Index + 1]);

    if (High == 0xFF || Low == 0xFF) {
      DEBUG((DEBUG_ERROR, "Invalid MAC character: %c%c\n", MacStr[Index], MacStr[Index + 1]));
      return RETURN_INVALID_PARAMETER;
    }

    Mac[ByteIndex++] = (High << 4) | Low;
    Index += 2;

    if (ByteIndex < 5 && MacStr[Index] != L':') {
      DEBUG((DEBUG_ERROR, "Missing colon separator at position %d\n", Index));
      return RETURN_INVALID_PARAMETER;
    }

    if (ByteIndex < 5) {
      Index++;
    }
  }

  if (ByteIndex != 6 || MacStr[Index] != L'\0') {
    DEBUG((DEBUG_ERROR, "Invalid MAC string length or format\n"));
    return RETURN_INVALID_PARAMETER;
  }

  return RETURN_SUCCESS;
}

/**
 * Obtain the BMC's Mac address through IPMI and update it to the Varstore
 * @retval void
**/
EFI_STATUS
EFIAPI
UpdateBmcMacInfo(
    NET_PRIVATE_DATA *PrivateData
  )
{
  EFI_STATUS Status;
  IPMI_LAN_MAC_ADDRESS BmcMacAddr;
  BMC_DATA *BmcData;

  BmcData = &PrivateData->BmcConfigData;
  if (BmcData == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }
  Status = IpmiGetBmcMacAddr(&BmcMacAddr);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to get BMC MAC Addr: %r\n", Status));
    return Status;
  }

  UnicodeSPrint(
      BmcData->MacAddr,
      sizeof(BmcData->MacAddr),
      L"%02x:%02x:%02x:%02x:%02x:%02x",
      BmcMacAddr.MacAddress[0],
      BmcMacAddr.MacAddress[1],
      BmcMacAddr.MacAddress[2],
      BmcMacAddr.MacAddress[3],
      BmcMacAddr.MacAddress[4],
      BmcMacAddr.MacAddress[5]);

  Status = UpdateBmcVarStore(BmcData);

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to store BMC MAC Addr in UEFI variable: %r\n", Status));
  }
  return Status;
}

/**
  Show/Set Bmc MAC Addr
  @retval void
**/
VOID
UpdateBmcMacForm(
    NET_PRIVATE_DATA *PrivateData
  )
{
  EFI_STATUS Status;
  VOID *StartOpCodeHandle;
  VOID *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL *StartLabel;
  EFI_IFR_GUID_LABEL *EndLabel;
  EFI_STRING_ID MacAddrId;

  StartOpCodeHandle = HiiAllocateOpCodeHandle();
  EndOpCodeHandle = HiiAllocateOpCodeHandle();

  if (StartOpCodeHandle == NULL || EndOpCodeHandle == NULL)
  {
    DEBUG((DEBUG_ERROR, "Failed to allocate opcode handles.\n"));
    return;
  }

  StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(
      StartOpCodeHandle,
      &gEfiIfrTianoGuid,
      NULL,
      sizeof(EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number = LABEL_MAC_START;

  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(
      EndOpCodeHandle,
      &gEfiIfrTianoGuid,
      NULL,
      sizeof(EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number = LABEL_MAC_END;

  MacAddrId   = HiiSetString(PrivateData->HiiHandle, 0, PrivateData->BmcConfigData.MacAddr, NULL);

  if (MacAddrId == 0)
  {
    DEBUG((DEBUG_ERROR, "Failed to set dynamic strings.\n"));
    HiiFreeOpCodeHandle(StartOpCodeHandle);
    HiiFreeOpCodeHandle(EndOpCodeHandle);
    return;
  }
  HiiCreateStringOpCode(
      StartOpCodeHandle,
      MAC_ADDR_SET_KEY_ID,
      VAR_BMC_VARID,
      OFFSET_OF(BMC_DATA, MacAddr),
      STRING_TOKEN(STR_SET_MAC_PROMPT),
      STRING_TOKEN(STR_SET_MAC_HELP),
      EFI_IFR_FLAG_CALLBACK,
      0,
      17,
      17,
      NULL);
  Status = HiiUpdateForm(
      PrivateData->HiiHandle,
      &gBmcConfigFormSetGuid,
      MAC_ADDR_FORM_ID,
      StartOpCodeHandle,
      EndOpCodeHandle);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to update the form: %r\n", Status));
  }
  HiiFreeOpCodeHandle(StartOpCodeHandle);
  HiiFreeOpCodeHandle(EndOpCodeHandle);
}

EFI_STATUS
ProcessMacAddrSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  )
{
	EFI_STATUS                       Status;
	CHAR16                          *MacAddr;
	IPMI_LAN_MAC_ADDRESS             BmcMacAddr;
	MacAddr = HiiGetString(Private->HiiHandle, Value->string, NULL);
	if (MacAddr == NULL) {
		DEBUG((
			DEBUG_ERROR,
			"Failed to retrieve string for IP Address.\n"
			));
		Status = EFI_OUT_OF_RESOURCES;
	} else {
		if (!isValidMacFormat(MacAddr)) {
			CreatePopUp(
						EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
						NULL,
						L"Invalid MAC Address!",
						L"The entered MAC address is not valid.",
						L"Ensure the value is true",
						NULL);
			gBS->Stall(1000000);
			Status = EFI_OUT_OF_RESOURCES;
		} else {
			StrCpyS(Private->BmcConfigData.MacAddr,
						sizeof(Private->BmcConfigData.MacAddr) / sizeof(CHAR16),
						MacAddr
						);

			ConvertChar16ToIpmiMac(Private->BmcConfigData.MacAddr, BmcMacAddr.MacAddress);

			Status = IpmiSetBmcMacAddr(BmcMacAddr.MacAddress);

			if (EFI_ERROR(Status)) {
				DEBUG((
					DEBUG_ERROR,
					"Failed to update BMC with Gateway Address: %r\n",
					Status
					));
			} else {
				UpdateBmcVarStore(&Private->BmcConfigData);
			}
		}
		FreePool(MacAddr);
	}
	return Status;
}
