
#include "BmcConfigBasicInfo.h"
#include "BmcConfigIpmi.h"

extern EFI_GUID gBmcConfigFormSetGuid;

/**
 * Obtain the BMC's basic info through IPMI and update it to the Varstore
 * @retval void
**/
EFI_STATUS
EFIAPI
UpdateBmcBasicInfo(
    NET_PRIVATE_DATA *PrivateData
  )
{
  EFI_STATUS Status;
  BMC_DATA *BmcData;
  UINT8    BootDeviceSelector;
  UINT8    BootInitiator;
  BOOLEAN  IsPersistent;

  BmcData = &PrivateData->BmcConfigData;
  if (BmcData == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }
  Status = GetBmcBasicInfo(BmcData);

  Status = GetBmcBootOption(&BootDeviceSelector, &BootInitiator,&IsPersistent);

  UnicodeSPrint(
    BmcData->BootDeviceSelector,
    sizeof(BmcData->BootDeviceSelector),
    L"%d",
    BootDeviceSelector);
  UnicodeSPrint(
    BmcData->BootInitiator,
    sizeof(BmcData->BootInitiator),
    L"%d",
    BootInitiator);
  UnicodeSPrint(
    BmcData->BootIsPersistent,
    sizeof(BmcData->BootIsPersistent),
    L"%d",
    IsPersistent);

  Status = UpdateBmcVarStore(BmcData);

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to store BMC MAC Addr in UEFI variable: %r\n", Status));
  }
  return Status;
}

/**
  Show Bmc Info
  @retval void
**/
VOID
UpdateBmcInfoForm(
    NET_PRIVATE_DATA *PrivateData
  )
{
  EFI_STATUS Status;
  VOID *StartOpCodeHandle;
  VOID *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL *StartLabel;
  EFI_IFR_GUID_LABEL *EndLabel;
  EFI_STRING_ID VersionStringId, IpmiVerStringId, IpmiInfTypeId,
                BootSelectorId, BootInitiatorId, BootIsPersistentId;


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
  StartLabel->Number = LABEL_INFO_START;

  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(
      EndOpCodeHandle,
      &gEfiIfrTianoGuid,
      NULL,
      sizeof(EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number = LABEL_INFO_END;

  VersionStringId      = HiiSetString(PrivateData->HiiHandle, 0, PrivateData->BmcConfigData.FmVersion, NULL);
  IpmiVerStringId      = HiiSetString(PrivateData->HiiHandle, 0, PrivateData->BmcConfigData.IpmiVersion, NULL);
  IpmiInfTypeId        = HiiSetString(PrivateData->HiiHandle, 0, L"SSIF", NULL);

  BootSelectorId       = HiiSetString(PrivateData->HiiHandle, 0, PrivateData->BmcConfigData.BootDeviceSelector, NULL);
  BootInitiatorId      = HiiSetString(PrivateData->HiiHandle, 0, PrivateData->BmcConfigData.BootInitiator, NULL);
  BootIsPersistentId   = HiiSetString(PrivateData->HiiHandle, 0, PrivateData->BmcConfigData.BootIsPersistent, NULL);


  if (IpmiInfTypeId == 0 || VersionStringId == 0 || IpmiVerStringId == 0)
  {
    DEBUG((DEBUG_ERROR, "Failed to set dynamic strings.\n"));
    HiiFreeOpCodeHandle(StartOpCodeHandle);
    HiiFreeOpCodeHandle(EndOpCodeHandle);
    return;
  }
  HiiCreateTextOpCode(StartOpCodeHandle, STRING_TOKEN(STR_VERSION_PROMPT), STRING_TOKEN(STR_VERSION_HELP), VersionStringId);
  HiiCreateTextOpCode(StartOpCodeHandle, STRING_TOKEN(STR_IPMI_VERSION_PROMPT), STRING_TOKEN(STR_IPMI_VERSION_HELP), IpmiVerStringId);
  HiiCreateTextOpCode(StartOpCodeHandle, STRING_TOKEN(STR_IPMIINF_PROMPT), STRING_TOKEN(STR_IPMIINF_HELP), IpmiInfTypeId);

  // HiiCreateTextOpCode(StartOpCodeHandle, STRING_TOKEN(STR_BOOT1_PROMPT), STRING_TOKEN(STR_BOOT1_HELP), BootSelectorId);
  // HiiCreateTextOpCode(StartOpCodeHandle, STRING_TOKEN(STR_BOOT2_PROMPT), STRING_TOKEN(STR_BOOT2_HELP), BootInitiatorId);
  // HiiCreateTextOpCode(StartOpCodeHandle, STRING_TOKEN(STR_BOOT3_PROMPT), STRING_TOKEN(STR_BOOT3_HELP), BootIsPersistentId);

  Status = HiiUpdateForm(
      PrivateData->HiiHandle,
      &gBmcConfigFormSetGuid,
      BASIC_INFO_FORM_ID,
      StartOpCodeHandle,
      EndOpCodeHandle);
  HiiFreeOpCodeHandle(StartOpCodeHandle);
  HiiFreeOpCodeHandle(EndOpCodeHandle);
}
