#include "BmcConfigLan.h"
#include "BmcConfigIpmi.h"

extern EFI_GUID gBmcConfigFormSetGuid;

EFI_STATUS
EFIAPI
ConvertChar16ToIpmiLanIpAddress(
    IN CHAR16 *IpAddressStr,
    OUT UINT8 *IpAddress
  )
{
  UINTN Index = 0;
  UINTN OctetValue = 0;

  if ((IpAddressStr == NULL) || (IpAddress == NULL))
    return EFI_INVALID_PARAMETER;

  while (*IpAddressStr != L'\0')
  {
    if ((*IpAddressStr >= L'0') && (*IpAddressStr <= L'9'))
    {
      OctetValue = OctetValue * 10 + (*IpAddressStr - L'0');
      if (OctetValue > 255)
      {
        return EFI_INVALID_PARAMETER;
      }
    }
    else if (*IpAddressStr == L'.')
    {
      if (Index >= 4)
      {
        return EFI_INVALID_PARAMETER;
      }
      IpAddress[Index++] = (UINT8)OctetValue;
      OctetValue = 0;
    }
    else
    {
      return EFI_INVALID_PARAMETER;
    }
    IpAddressStr++;
  }

  if (Index < 3)
  {
    return EFI_INVALID_PARAMETER;
  }
  IpAddress[Index] = (UINT8)OctetValue;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UpdateBmcLanConfigData(
    NET_PRIVATE_DATA *PrivateData
  )
{
  EFI_STATUS Status;
  UINT8 AddrSrc;
  IPMI_LAN_IP_ADDRESS BmcIpAddress;
  IPMI_LAN_SUBNET_MASK BmcSubnetMask;
  IPMI_LAN_DEFAULT_GATEWAY BmcGateway;
  BMC_DATA *BmcData;

  BmcData = &PrivateData->BmcConfigData;

  if (BmcData == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }
  Status = IpmiGetBmcLanInfo(&AddrSrc, &BmcIpAddress, &BmcSubnetMask, &BmcGateway);
  if (EFI_ERROR(Status))
  {
    DEBUG((
      DEBUG_ERROR,
      "Failed to get BMC LAN information: %r\n",
      Status
      ));
  } else {
    BmcData->EnableDHCP = AddrSrc;

    UnicodeSPrint(
        BmcData->IpAddress,
        sizeof(BmcData->IpAddress),
        L"%d.%d.%d.%d",
        BmcIpAddress.IpAddress[0],
        BmcIpAddress.IpAddress[1],
        BmcIpAddress.IpAddress[2],
        BmcIpAddress.IpAddress[3]);

    UnicodeSPrint(
        BmcData->SubnetMask,
        sizeof(BmcData->SubnetMask),
        L"%d.%d.%d.%d",
        BmcSubnetMask.IpAddress[0],
        BmcSubnetMask.IpAddress[1],
        BmcSubnetMask.IpAddress[2],
        BmcSubnetMask.IpAddress[3]);

    UnicodeSPrint(
        BmcData->Gateway,
        sizeof(BmcData->Gateway),
        L"%d.%d.%d.%d",
        BmcGateway.IpAddress[0],
        BmcGateway.IpAddress[1],
        BmcGateway.IpAddress[2],
        BmcGateway.IpAddress[3]);

    UpdateBmcVarStore(BmcData);

    if (EFI_ERROR(Status))
    {
      DEBUG((
        DEBUG_ERROR,
        "Failed to store BMC LAN information in UEFI variable: %r\n",
        Status
        ));
    }
  }
  return Status;
}

BOOLEAN
IsValidIpAndGateway(
    IN CHAR16 *Ip
  )
{
  UINTN OctetCount = 0;
  UINTN OctetValue = 0;
  CHAR16 *Current = Ip;

  if (Ip == NULL || StrLen(Ip) == 0)
  {
    return FALSE;
  }
  while (*Current != L'\0')
  {
    if (*Current == L'.')
    {
      if (OctetValue > 255)
      {
        return FALSE;
      }
      OctetCount++;
      OctetValue = 0;
    }
    else if (*Current >= L'0' && *Current <= L'9')
    {
      OctetValue = OctetValue * 10 + (*Current - L'0');
      if (OctetValue > 255)
        return FALSE;
    }
    else
    {
      return FALSE;
    }
    Current++;
  }

  if (OctetValue > 255)
    return FALSE;

  OctetCount++;
  return (OctetCount == 4);
}

BOOLEAN
IsValidSubnetMask(
    IN CHAR16 *SubnetMask
  )
{
  UINT32 Mask = 0;
  CHAR16 *Current = SubnetMask;
  if (SubnetMask == NULL || StrLen(SubnetMask) == 0)
    return FALSE;

  if (!IsValidIpAndGateway(SubnetMask))
    return FALSE;

  while (*Current != L'\0')
  {
    Mask = (Mask << 8) | (UINT32)StrDecimalToUintn(Current);
    while (*Current != L'.' && *Current != L'\0')
    {
      Current++;
    }
    if (*Current == L'.')
      Current++;
  }
  UINT32 InverseMask = ~Mask;
  if ((InverseMask & (InverseMask + 1)) != 0)
    return FALSE;

  return TRUE;
}

EFI_STATUS
ProcessIpSourceSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  )
{
  EFI_STATUS                       Status;
  UINT8                            TempEnableDHCP;

  TempEnableDHCP = Value->u8;
  Status = IpmiSetBmcLanIpSrc(&TempEnableDHCP);
  if (EFI_ERROR(Status)) {
    DEBUG((
      DEBUG_ERROR,
      "Failed to update BMC with IP Source: %r\n",
      Status
      ));
    CreatePopUp(
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      NULL,
      L"Set BMC IP source failed!",
      NULL);
		gBS->Stall(1000000);
    UpdateNetworkForm(Private);
    SendBmcNetForm(Private);
  } else {
    Private->BmcConfigData.EnableDHCP = Value->u8;
  }
  Status = UpdateBmcVarStore(&Private->BmcConfigData);
  return Status;
}

EFI_STATUS
ProcessIpAddrSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  )
{
	EFI_STATUS                       Status;
	CHAR16                          *IpAddress;
	IPMI_LAN_IP_ADDRESS              BmcIpAddress;
  CHAR16                           TempIpAddress[64];
	IpAddress = HiiGetString(Private->HiiHandle, Value->string, NULL);
	if (IpAddress == NULL) {
		DEBUG((
			DEBUG_ERROR,
			"Failed to retrieve string for IpAddress Address.\n"
			));
		Status = EFI_OUT_OF_RESOURCES;
	} else {
		if (!IsValidIpAndGateway(IpAddress)) {
			CreatePopUp(
						EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
						NULL,
						L"Invalid IpAddress!",
						L"The entered IpAddress is not valid.",
						L"Ensure the value is true",
						NULL);
			gBS->Stall(1000000);
			Status = EFI_OUT_OF_RESOURCES;
		} else {
			StrCpyS(TempIpAddress,
        sizeof(TempIpAddress) / sizeof(CHAR16),
        IpAddress
        );

			ConvertChar16ToIpmiLanIpAddress(TempIpAddress, BmcIpAddress.IpAddress);

			Status = IpmiSetBmcLanIpAddr(&BmcIpAddress);

			if (EFI_ERROR(Status)) {
				DEBUG((
					DEBUG_ERROR,
					"Failed to update BMC with IpAddress: %r\n",
					Status
					));
        CreatePopUp(
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          NULL,
          L"Set BMC IP ADDR failed!",
          NULL);
        gBS->Stall(2000000);
        UpdateNetworkForm(Private);
        SendBmcNetForm(Private);
			} else {
        StrCpyS(Private->BmcConfigData.IpAddress,
          sizeof(Private->BmcConfigData.IpAddress) / sizeof(CHAR16),
          IpAddress
          );
      }
		}
    Status = UpdateBmcVarStore(&Private->BmcConfigData);

		FreePool(IpAddress);
	}
	return Status;
}

EFI_STATUS
ProcessGateWayAddrSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  )
{
	EFI_STATUS                       Status;
	CHAR16                          *GateWayAddr;
	IPMI_LAN_DEFAULT_GATEWAY         BmcGateway;
  CHAR16                           TempGateway[64];
	GateWayAddr = HiiGetString(Private->HiiHandle, Value->string, NULL);
	if (GateWayAddr == NULL) {
		DEBUG((
			DEBUG_ERROR,
			"Failed to retrieve string for GateWay Address.\n"
			));
		Status = EFI_OUT_OF_RESOURCES;
	} else {
		if (!IsValidIpAndGateway(GateWayAddr)) {
			CreatePopUp(
						EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
						NULL,
						L"Invalid GateWay Address!",
						L"The entered GateWay address is not valid.",
						L"Ensure the value is true",
						NULL);
			gBS->Stall(1000000);
			Status = EFI_OUT_OF_RESOURCES;
		} else {
			StrCpyS(
        TempGateway,
        sizeof(TempGateway) / sizeof(CHAR16),
        GateWayAddr
        );

			ConvertChar16ToIpmiLanIpAddress(TempGateway, BmcGateway.IpAddress);

			Status = IpmiSetBmcLanGateWay(&BmcGateway);

			if (EFI_ERROR(Status)) {
				DEBUG((
					DEBUG_ERROR,
					"Failed to update BMC with Gateway Address: %r\n",
					Status
					));
        CreatePopUp(
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          NULL,
          L"Set BMC Gateway ADDR failed!",
          NULL);
        gBS->Stall(2000000);
        UpdateNetworkForm(Private);
        SendBmcNetForm(Private);
			} else {
        StrCpyS(
          Private->BmcConfigData.Gateway,
          sizeof(Private->BmcConfigData.Gateway) / sizeof(CHAR16),
          GateWayAddr
          );
      }
		}

    Status = UpdateBmcVarStore(&Private->BmcConfigData);

		FreePool(GateWayAddr);
	}
	return Status;
}

EFI_STATUS
ProcessSubnetMaskSet (
  IN  NET_PRIVATE_DATA             *Private,
  IN  EFI_QUESTION_ID               QuestionId,
  IN  EFI_IFR_TYPE_VALUE            *Value
  )
{
	EFI_STATUS                       Status;
	CHAR16                          *SubnetMask;
	IPMI_LAN_SUBNET_MASK             BmcSubnetMask;
  CHAR16                           TempSubnetMask[64];
	SubnetMask = HiiGetString(Private->HiiHandle, Value->string, NULL);
	if (SubnetMask == NULL) {
		DEBUG((
			DEBUG_ERROR,
			"Failed to retrieve string for SubnetMask Address.\n"
			));
		Status = EFI_OUT_OF_RESOURCES;
	} else {
		if (!IsValidSubnetMask(SubnetMask)) {
			CreatePopUp(
						EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
						NULL,
						L"Invalid SubnetMask!",
						L"The entered SubnetMask is not valid.",
						L"Ensure the value is true",
						NULL);
			gBS->Stall(1000000);
			Status = EFI_OUT_OF_RESOURCES;
		} else {
			StrCpyS(TempSubnetMask,
        sizeof(TempSubnetMask) / sizeof(CHAR16),
        SubnetMask
        );

			ConvertChar16ToIpmiLanIpAddress(TempSubnetMask, BmcSubnetMask.IpAddress);

			Status = IpmiSetBmcLanSubnetMask(&BmcSubnetMask);

			if (EFI_ERROR(Status)) {
				DEBUG((
					DEBUG_ERROR,
					"Failed to update BMC with SubnetMask: %r\n",
					Status
					));
        CreatePopUp(
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          NULL,
          L"Set BMC SubnetMask failed!",
          NULL);
        gBS->Stall(2000000);
        UpdateNetworkForm(Private);
        SendBmcNetForm(Private);
			} else {
        StrCpyS(Private->BmcConfigData.SubnetMask,
          sizeof(Private->BmcConfigData.SubnetMask) / sizeof(CHAR16),
          SubnetMask
          );
      }
		}

    Status = UpdateBmcVarStore(&Private->BmcConfigData);

		FreePool(SubnetMask);
	}
	return Status;
}



/**
  Set/Get Lan Param
  @retval void
**/
VOID
UpdateNetworkForm(
    NET_PRIVATE_DATA *PrivateData
  )
{
  EFI_STATUS Status;
  VOID *StartOpCodeHandle;
  VOID *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL *StartLabel;
  EFI_IFR_GUID_LABEL *EndLabel;

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
  StartLabel->Number = LABEL_SET;

  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(
      EndOpCodeHandle,
      &gEfiIfrTianoGuid,
      NULL,
      sizeof(EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number = LABEL_END;


  HiiCreateStringOpCode(
      StartOpCodeHandle,
      NETWORK_SET_IP_KEY_ID,
      VAR_BMC_VARID,
      OFFSET_OF(BMC_DATA, IpAddress),
      STRING_TOKEN(STR_IP_PROMPT),
      STRING_TOKEN(STR_IP_HELP),
      EFI_IFR_FLAG_CALLBACK,
      0,
      7,
      15,
      NULL);

  HiiCreateStringOpCode(
      StartOpCodeHandle,
      NETWORK_SET_SUBNET_KEY_ID,
      VAR_BMC_VARID,
      OFFSET_OF(BMC_DATA, SubnetMask),
      STRING_TOKEN(STR_SUBNET_PROMPT),
      STRING_TOKEN(STR_SUBNET_HELP),
      EFI_IFR_FLAG_CALLBACK,
      0,
      7,
      15,
      NULL);

  HiiCreateStringOpCode(
      StartOpCodeHandle,
      NETWORK_SET_GATEWAY_KEY_ID,
      VAR_BMC_VARID,
      OFFSET_OF(BMC_DATA, Gateway),
      STRING_TOKEN(STR_GATEWAY_PROMPT),
      STRING_TOKEN(STR_GATEWAY_HELP),
      EFI_IFR_FLAG_CALLBACK,
      0,
      7,
      15,
      NULL);

  Status = HiiUpdateForm(
      PrivateData->HiiHandle,
      &gBmcConfigFormSetGuid,
      NETWORK_SET_FORM_ID,
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
EFIAPI
SendBmcNetForm(
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
			NETWORK_SET_FORM_ID,
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
