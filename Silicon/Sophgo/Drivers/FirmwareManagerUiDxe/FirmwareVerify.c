/** @file
  The functions for firmware verification.

  Copyright (c) 2025, SOPHGO Technology Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "FirmwareVerify.h"

#define RSA_OID_SIZE 13
#define SM2_OID_SIZE 20

#define RSA_SIG_FILE_SIZE  256
#define DPT_MAGIC          0x55aa55aaU
#define EFIE_OFFSET        0x80000
#define VER_STR_MAX_LEN    20
#define PRINT_PUBKEY_INFO  0

typedef enum {
  RSA_ALG = 0,
  SM2_ALG
} FLAG_ALG;

typedef struct {
  /* disk partition table magic number */
  UINT32 Magic;
  UINT8  Name[32];
  UINT32 Offset;
  UINT32 FileSize;
  UINT8  Reserve[4];
  /* load memory address*/
  UINT64 Lma;
}PART_INFO;

struct SM2_PARAMETER {
  UINT8 Pubx[32];
  UINT8 Puby[32];
} Sm2Parm;

struct RSA_PARAMETER {
  UINT8 Modulus[256];
  UINT8 Exponent[256];
} RsaParm;

STATIC UINT8 RsaOid[RSA_OID_SIZE] = {
  0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
  0x01, 0x01, 0x01, 0x05, 0x00
};

STATIC UINT8 Sm2Oid[SM2_OID_SIZE] = {
  0x06, 0x08, 0x2a, 0x81, 0x1c, 0xcf, 0x55, 0x01,
  0x82, 0x2d, 0x06, 0x08, 0x2a, 0x81, 0x1c, 0xcf,
  0x55, 0x01, 0x82, 0x2d
};

STATIC
INT32
CmpAlgOid (
  UINT8 *Data,
  UINT8 *Oid,
  UINT32 Len
  )
{
  INT32 Index;

  for (Index = 0; Index < Len; Index++) {
    if(Data[Index] != Oid[Index])
      return 0;
  }

  return 1;
}

STATIC
UINT32
ParseSeqLength (
  IN     UINT8  *Data,
  IN OUT UINT32 *CtxLen
  )
{
  UINT32 Offset = 0;

  if (Data[Offset] & 0x80) {
    UINT32 LengthBytes = Data[Offset++] & 0x7F;
    if (!CtxLen)
      return (Offset + LengthBytes);
    for (INT32 Index = 0; Index < LengthBytes; Index++) {
      *CtxLen = (*CtxLen << 8) | Data[Offset++];
    }
  } else {
    if (!CtxLen)
      return (Offset + 1);
    *CtxLen = Data[Offset++];
  }

  return Offset;
}

STATIC
INT32
ParseAlgOid (
  IN  UINT8  *Data,
  IN  UINT32 Len,
  OUT INT32  *Flag
  )
{
  switch (Len) {
  case RSA_OID_SIZE:
    if (CmpAlgOid(Data, RsaOid, Len)) {
      *Flag = RSA_ALG;
      break;
    }

  case SM2_OID_SIZE:
    if (CmpAlgOid(Data, Sm2Oid, Len)) {
      *Flag = SM2_ALG;
      break;
    }

  default:
    return -1;
  }

  return 0;
}

STATIC
INT32
ExtractRsaKey (
  IN UINT8  *Data,
  IN UINT32 *ModulusLength
  )
{
  UINT32 ExponentLength = 0;
  UINT32 Offset = 0;
  INT32 Index;

  /* checkout sequence flag */
  if (Data[Offset++] != 0x30) {
    DEBUG((DEBUG_ERROR, "failed: Data type is not sequence!\n"));
    return -1;
  }

  /* skip sequence length */
  Offset += ParseSeqLength (Data + Offset, NULL);

  /* parse m head */
  if (Data[Offset++] != 0x02) {
    DEBUG((DEBUG_ERROR, "failed: not a valid module head!\n"));
    return -1;
  }

  /* read m length */
  Offset += ParseSeqLength (Data + Offset, ModulusLength);

  while (!Data[Offset]) {
    Offset++;
    (*ModulusLength)--;
  }

  /* save m */
  for (Index = 0; Index < (*ModulusLength); Index++) {
    RsaParm.Modulus[Index] = Data[Offset + Index];
  }

  Offset += *ModulusLength;

  /* parse e head */
  if (Data[Offset++] != 0x02) {
    DEBUG((DEBUG_ERROR, "failed: not a valid exponent!\n"));
    return -1;
  }

  /* read e length */
  Offset += ParseSeqLength (Data + Offset, &ExponentLength);

  /* save e */
  for (Index = 0; Index < 256 - ExponentLength; Index++) {
    RsaParm.Exponent[Index] = 0;
  }
  for (; Index < 256 ; Index++) {
    RsaParm.Exponent[Index] = Data[Offset + Index + ExponentLength - 256];
  }

  return 0;
}

STATIC
INT32
ExtractSm2Key (
  UINT8 *Data,
  UINT32 *ModulusLength
  )
{
  UINT32 Offset = 0;

  if (Data[Offset++] != 0x04) {
    DEBUG((DEBUG_ERROR, "failed: sm2 key is not uncompressed!\n"));
    return -1;
  }

  /* save x */
  DEBUG((DEBUG_ERROR, "sm2 x:\n"));
  for (INT32 Index = 0; Index < 32; Index++) {
    Sm2Parm.Pubx[Index] = Data[Offset++];
    DEBUG((DEBUG_ERROR, "%02X", Sm2Parm.Pubx[Index]));
    DEBUG((DEBUG_ERROR, (Index + 1)%16 ? "" : "\n"));
  }
  DEBUG((DEBUG_ERROR, "\n"));

  /* save y */
  DEBUG((DEBUG_ERROR, "sm2 y:\n"));
  for (INT32 Index = 0; Index < 32; Index++) {
    Sm2Parm.Puby[Index] = Data[Offset++];
    DEBUG((DEBUG_ERROR, "%02X", Sm2Parm.Puby[Index]));
    DEBUG((DEBUG_ERROR, (Index + 1)%16 ? "" : "\n"));
  }
  DEBUG((DEBUG_ERROR, "\n"));

  *ModulusLength = 32;

  return 0;
}

INT32
ParsePublicKey (
  IN  UINT8 *Data,
  IN  UINT32 Len,
  OUT UINT32 *ModulusLen,
  OUT INT32 *Flag
  )
{
  UINT32 OidLen = 0;
  UINT32 Offset = 0;
  INT32 Ret =0;

  /* check the pubkey length */
  if (Len < 15) {
    DEBUG((DEBUG_ERROR, "failed: Data len is too short!\n"));
    return -1;
  }

  /* check if the sequence */
  if (Data[Offset++] != 0x30) {
    DEBUG((DEBUG_ERROR, "failed: Data type is not sequence!\n"));
    return -1;
  }

  /* skip the length byte */
  Offset += ParseSeqLength (Data + Offset, NULL);

  /* check if Algorithm Identifier */
  if (Data[Offset++] != 0x30) {
    DEBUG((DEBUG_ERROR, "failed: Data type is not sequence!\n"));
    return -1;
  }

  /* record Algorithm Identifier length */
  Offset += ParseSeqLength (Data + Offset, &OidLen);

  Ret = ParseAlgOid (Data + Offset, OidLen, Flag);
  if (Ret) {
    DEBUG((DEBUG_ERROR, "failed: alg oid is not correct!\n"));
    return -1;
  }

  Offset += OidLen;

  /* skip fixed context */
  if (Data[Offset++] != 0x03) {
    DEBUG((DEBUG_ERROR, "failed: Data type is not sequence!\n"));
    return -1;
  }

  /* skip BIT STRING , which has 0 */
  Offset += ParseSeqLength (Data + Offset, NULL) + 1;

  /* select an alg */
  switch (*Flag) {
  case RSA_ALG:
    Ret = ExtractRsaKey (Data + Offset, ModulusLen);
    if (Ret) return -1;
    break;
  case SM2_ALG:
    Ret = ExtractSm2Key (Data + Offset, ModulusLen);
    if (Ret) return -1;
    break;
  default:
    return -1;
  }

  return 0;
}

STATIC
EFI_STATUS
RsaSha256Verify (
  IN UINT8  *DataToVerify,
  IN UINTN  DataToVerifyLen,
  IN UINT8  *SigFileBuf,
  IN UINTN  SigFileLen,
  IN UINT32 ModulusLen
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  VOID *RsaContext = NULL;
  UINT8 MessageHash[SHA256_DIGEST_SIZE] = { 0 };

  Sha256HashAll (DataToVerify, DataToVerifyLen, MessageHash);

#if PRINT_PUBKEY_INFO == 1
  DEBUG((DEBUG_ERROR, "SHA-256: "));
  for (INT32 Index = 0; Index < sizeof (MessageHash); Index++) {
    DEBUG((DEBUG_ERROR, "%02X", MessageHash[Index]));
  }
  DEBUG((DEBUG_ERROR, "\n"));
  DEBUG((DEBUG_ERROR, "modules (n):\n"));
  for (INT32 Index = 0; Index < ModulusLen; Index++) {
    DEBUG((DEBUG_ERROR, "%02X", RsaParm.Modulus[Index]));
    DEBUG((DEBUG_ERROR, (Index + 1)%16 ? "" : "\n"));
  }
  DEBUG((DEBUG_ERROR, "exponent:\n"));
  for (INT32 Index = 0; Index < sizeof (RsaParm.Exponent); Index++) {
    DEBUG((DEBUG_ERROR, "%02X", RsaParm.Exponent[Index]));
    DEBUG((DEBUG_ERROR, (Index + 1)%16 ? "" : "\n"));
  }
#endif

  RsaContext = RsaNew();
  if (RsaContext == NULL) {
    DEBUG((DEBUG_ERROR, "Failed to create RSA context\n"));
    return EFI_ABORTED;
  }
  if (!RsaSetKey(RsaContext, RsaKeyN, RsaParm.Modulus, sizeof(RsaParm.Modulus))) {
    DEBUG((DEBUG_ERROR, "Failed to set RSA modulus\n"));
    Status = EFI_ABORTED;
    goto Cleanup;
  }
  if (!RsaSetKey(RsaContext, RsaKeyE, RsaParm.Exponent, sizeof(RsaParm.Exponent))) {
    DEBUG((DEBUG_ERROR, "Failed to set RSA public exponent\n"));
    Status = EFI_ABORTED;
    goto Cleanup;
  }

  if (RsaPkcs1Verify(RsaContext,
                     MessageHash,
                     SHA256_DIGEST_SIZE,
                     SigFileBuf,
                     SigFileLen)) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_ABORTED;
  }

Cleanup:
  if (RsaContext != NULL) {
      RsaFree(RsaContext);
  }

  return Status;
}

STATIC
EFI_STATUS
GetPubKeyOffset (
  IN  UINT8 *DataToVerify,
  IN  UINTN DataToVerifyLen,
  OUT UINTN *PubKeyDerLen,
  OUT UINTN *PubKeyOffset
  )
{
  UINT8  *EfieData = DataToVerify + EFIE_OFFSET;
  UINT8  *DataToVerifyEnd = DataToVerify + DataToVerifyLen;
  UINTN  Index;
  UINTN  PartInfoSize = sizeof (PART_INFO);
  PART_INFO   *PartInfo = NULL;
  CONST CHAR8 *PubKeyName = "public_key.der";

  for (Index = 0; (EfieData + Index * PartInfoSize) < DataToVerifyEnd; ++Index) {
    PartInfo = (PART_INFO *)(EfieData + Index * PartInfoSize);
    if (PartInfo->Magic != DPT_MAGIC)
      break;
    if (!AsciiStrCmp (PubKeyName, (CONST CHAR8 *)(PartInfo->Name))) {
      *PubKeyDerLen = PartInfo->FileSize;
      *PubKeyOffset = PartInfo->Offset;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

STATIC
VOID
ParseVersionString (
  CHAR8  *VersionString,
  UINT32 *VerList
  )
{
  CHAR8 *CurrentChar = VersionString;
  CHAR8 Buffer[VER_STR_MAX_LEN] = {0};
  UINTN BufferIndex = 0;
  UINTN ListIndex = 0;
  UINT32 List[VER_STR_MAX_LEN] = {0};

  while (*CurrentChar != '\0') {
    if (*CurrentChar == '.') {
      Buffer[BufferIndex] = '\0';
      List[ListIndex] = AsciiStrDecimalToUintn (Buffer);
      ListIndex++;
      BufferIndex = 0;
    } else {
      Buffer[BufferIndex++] = *CurrentChar;
    }
    CurrentChar++;
  }

  if (BufferIndex > 0) {
    Buffer[BufferIndex] = '\0';
    List[ListIndex] = AsciiStrDecimalToUintn (Buffer);
    ListIndex++;
  }
  for (UINT8 Index = 0; Index < 3; ++Index) {
    VerList[Index] = List[Index];
  }
}

STATIC
INT32
VersionCompare (
  IN  UINT32 *FirmwareVerList,
  IN  UINT32 *CurrentVerList
  )
{
  INT32 Index = 0;

  for (Index = 0; Index < 3; Index++) {
    if (FirmwareVerList[Index] > CurrentVerList[Index]) {
        return 1;
    } else if (FirmwareVerList[Index] < CurrentVerList[Index]) {
        return -1;
    }
  }
  return 0;
}

/**
  Get the version information of the firmware currently in use from NOR flash.

  @param[in]   Nor              Structure of nor flash.
  @param[in]   NorFlashProtocol Nor Flash protocol.
  @param[out]  CurrentVer       The firmware version read from NOR flash.
  @param[in]   CurrentVerSize   The buffer size of the CurrentVer.

  @retval  EFI_UNSUPPORTED      The buffer size of CurrentVer is not greater than VER_STR_MAX_LEN
  @retval  EFI_SUCCESS          Successfully obtained firmware version.
**/
EFI_STATUS
GetCurrentVer (
  IN  SPI_NOR *Nor,
  IN  SOPHGO_NOR_FLASH_PROTOCOL *NorFlashProtocol,
  OUT CHAR8   *CurrentVer,
  IN  UINTN   CurrentVerSize
  )
{
  UINTN  BlockSize;
  UINTN  Count;
  UINT8  *TempBuffer;

  if (CurrentVerSize < VER_STR_MAX_LEN) {
    DEBUG((DEBUG_ERROR, "Buffer size cannot be less than %d!\n", VER_STR_MAX_LEN));
    return EFI_UNSUPPORTED;
  }

  BlockSize = Nor->Info->SectorSize;
  Count = VER_STR_MAX_LEN / BlockSize;
  if ((VER_STR_MAX_LEN % BlockSize) != 0)
    Count += 1;

  TempBuffer = AllocateZeroPool (Count * BlockSize);

  for (UINTN Index = 0; Index < Count; Index++) {
    NorFlashProtocol->ReadData (
      Nor,
      Index * BlockSize,
      BlockSize,
      TempBuffer + Index * BlockSize
      );
  }

  gBS->CopyMem ((UINT8 *)CurrentVer, TempBuffer, VER_STR_MAX_LEN);
  FreePool (TempBuffer);
  CurrentVer[VER_STR_MAX_LEN - 1] = 0;

  return EFI_SUCCESS;
}

/**
  Verify the version number and signature information of the firmware to be upgraded

  @param[in]   DataToVerify          The data buffer of the firmware to be upgraded that has
                                     been loaded into memory.
  @param[in]   DataToVerifyLen       The byte length of the firmware data buffer.
  @param[in]   Nor                   Structure of nor flash.
  @param[in]   NorFlashProtocol      Nor Flash protocol.

  @retval  EFI_UNSUPPORTED           1: Unable to obtain the version of the current firmware.
                                     2: The signature verification algorithm does not support.
  @retval  EFI_INCOMPATIBLE_VERSION  The version of the firmware to be upgraded is lower
                                     than the version of the current firmware.
  @retval  EFI_NOT_FOUND             Unable to find the public key file (public_key.der) from
                                     the firmware to be upgraded
  @retval  EFI_ABORTED               1: Failed to parse public key.
                                     2: Signature verification of the firmware to be upgraded
                                        failed.
  @retval  EFI_SUCCESS               The firmware to be upgraded has passed verification.
**/
EFI_STATUS
FirmwareVerify (
  IN UINT8                      *DataToVerify,
  IN UINTN                      DataToVerifyLen,
  IN SPI_NOR                    *Nor,
  IN SOPHGO_NOR_FLASH_PROTOCOL  *NorFlashProtocol
  )
{
  EFI_STATUS Status;
  UINT32 ModulusLen = 0;
  INT32  Flag = 0;
  INT32  Ret  = 0;
  UINT8  *PubKeyDerBuf = NULL;
  UINTN  PubKeyDerLen  = 0;
  UINTN  PubKeyOffset  = 0;
  UINT8  *SigFileBuf   = NULL;
  UINTN  SigFileLen    = 0;
  CHAR8  *FirmwareVer  = (CHAR8 *)DataToVerify;
  CHAR8  CurrentVer[VER_STR_MAX_LEN] = {0};
  UINT32 FirmwareVerList[3] = {0};
  UINT32 CurrentVerList[3]  = {0};
  UINT8  IsSecureBootEnable = 0;
  UINTN  SecureBootVarSize  = 1;

  Status = GetCurrentVer (Nor, NorFlashProtocol, CurrentVer, sizeof (CurrentVer));
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (AsciiStrLen (FirmwareVer) + 1 > VER_STR_MAX_LEN) {
    DEBUG((DEBUG_ERROR, "Error! Firmware version string is too long!\n"));
    return EFI_INCOMPATIBLE_VERSION;
  }

  ParseVersionString (FirmwareVer, FirmwareVerList);
  ParseVersionString (CurrentVer, CurrentVerList);

  Ret = VersionCompare (FirmwareVerList, CurrentVerList);
  if ( Ret < 0) {
    DEBUG((DEBUG_ERROR, "Current version: %u.%u.%u\n",
      CurrentVerList[0], CurrentVerList[1], CurrentVerList[2]));
    DEBUG((DEBUG_ERROR, "Firmware version: %u.%u.%u\n",
      FirmwareVerList[0], FirmwareVerList[1], FirmwareVerList[2]));
    DEBUG((DEBUG_ERROR, "Firmware version is smaller than current version!\n"));
    DEBUG((DEBUG_ERROR, "Skip the update.\n"));
    return EFI_INCOMPATIBLE_VERSION;
  }

  Status = gRT->GetVariable(L"SecureBoot",
                            &gEfiGlobalVariableGuid,
                            NULL,
                            &SecureBootVarSize,
                            &IsSecureBootEnable);
  if (EFI_ERROR(Status)) {
    IsSecureBootEnable = 0;
  }

  if (IsSecureBootEnable == 0) {
    DEBUG((DEBUG_ERROR, "Secure boot is disabled, skip signature verify.\n"));
    Status = EFI_SUCCESS;
    goto End;
  } else {
    DEBUG((DEBUG_ERROR, "Secure boot is enabled, start signature verify.\n"));
  
    Status = GetPubKeyOffset (DataToVerify, DataToVerifyLen, &PubKeyDerLen, &PubKeyOffset);
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "Cannot find public_key.der!\n"));
      return EFI_NOT_FOUND;
    }

    PubKeyDerBuf = DataToVerify + PubKeyOffset;
    Ret = ParsePublicKey (PubKeyDerBuf, (UINT32)PubKeyDerLen, &ModulusLen, &Flag);
    if (Ret < 0)
      return EFI_ABORTED;

    switch (Flag) {
    case RSA_ALG:
      SigFileBuf = DataToVerify + DataToVerifyLen - RSA_SIG_FILE_SIZE;
      SigFileLen = RSA_SIG_FILE_SIZE;
      Status = RsaSha256Verify (DataToVerify,
                                DataToVerifyLen - SigFileLen,
                                SigFileBuf,
                                SigFileLen,
                                ModulusLen);
      goto End;
    case SM2_ALG:
      DEBUG((DEBUG_ERROR, "SM2 unsupported.\n"));
      return EFI_UNSUPPORTED;
    default:
      DEBUG((DEBUG_ERROR, "Unsupportable signature verification.\n"));
      return EFI_UNSUPPORTED;
    }
  }

End:
  DEBUG((DEBUG_ERROR, "Update from version <%u.%u.%u> to <%u.%u.%u>\n", 
         CurrentVerList[0], CurrentVerList[1], CurrentVerList[2],
         FirmwareVerList[0], FirmwareVerList[1], FirmwareVerList[2]));

  return Status;
}
