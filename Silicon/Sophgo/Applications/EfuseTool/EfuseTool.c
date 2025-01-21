/** @file Operate eFuse

 Copyright (C) 2025, SOPHGO Technologies Inc. All rights reserved.

 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/FileHandleLib.h>
#include <Library/EfuseLib.h>
#include <Include/DwGpio.h>

typedef struct {
  UINT64        ReadSize;     // -s <size>
  UINT64        EfuseIndex;   // -d <index>
  UINT64        Offset;       // -o <offset>
  UINT64        WriteValue;   // -w <value>
  CONST CHAR16  *FilePath;    // -f <file>
} EFUSE_OPERATION;

STATIC CONST SHELL_PARAM_ITEM  mParamList[] = {
  { L"-h", TypeFlag  },
  { L"-v", TypeFlag  },
  { L"-r", TypeFlag  },
  { L"-s", TypeValue },
  { L"-d", TypeValue },
  { L"-o", TypeValue },
  { L"-w", TypeValue },
  { L"-f", TypeValue },
  { NULL,  TypeMax   }
};

EFUSE_OPERATION  mEfuseOp;
CONST CHAR16     gShellEfuseFileName[] = L"ShellCommand";
EFI_HANDLE       gShellEfuseHiiHandle  = NULL;

CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameEfuse (
  VOID
  )
{
  return gShellEfuseFileName;
}

STATIC
VOID
PrintHelp (
  VOID
  )
{
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_HELP_EFUSE), gShellEfuseHiiHandle);
}

STATIC
UINTN
ShellCommandLineGetRawCount (
  IN  LIST_ENTRY  *CheckPackage
  )
{
  LIST_ENTRY  *Node1;
  UINTN       Count;

  if (CheckPackage == NULL)
    return 1;

  for (Node1 = GetFirstNode (CheckPackage), Count = 0;
       !IsNull (CheckPackage, Node1);
       Node1 = GetNextNode (CheckPackage, Node1))
    Count++;

  return Count;
}

STATIC
EFI_STATUS
CheckArguements (
  OUT  LIST_ENTRY  **CheckPackagePoint
  )
{
  EFI_STATUS  Status;
  CHAR16      *ProblemParam = NULL;

  Status = ShellCommandLineParse (mParamList, CheckPackagePoint, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      Print (L"Unrecognized argument: \"%s\"\n\n", ProblemParam);
    } else {
      Print (L"Invalid argument[%r]\n\n", Status);
    }
    PrintHelp ();
  } else if (ShellCommandLineGetCount (*CheckPackagePoint) > 1) {
    CONST CHAR16 *Redundant = ShellCommandLineGetRawValue (*CheckPackagePoint, 1);
    Print (L"Redundant argument: \"%s\"\n\n", Redundant);
    PrintHelp ();
    Status = EFI_INVALID_PARAMETER;
  }

  SHELL_FREE_NON_NULL (ProblemParam);

  return Status;
}

STATIC
EFI_STATUS
EfuseWriteEnable (
  IN BOOLEAN IsEnable
  )
{
  EFI_STATUS           Status;
  UINT8                GpioPin;
  BOOLEAN              IsHighToEnable;
  SOPHGO_GPIO_PROTOCOL *GpioProtocol;
  GPIO_CONFIG_MODE     GpioMode;

  if (PcdGetBool (PcdEfuseWriteEnableGpio) == FALSE)
    return EFI_SUCCESS;

  GpioPin = PcdGet8 (PcdEfuseWriteEnableGpioPin);
  IsHighToEnable = PcdGetBool (PcdEfuseIsGpioHighToEnableWrite);

  Status = gBS->LocateProtocol (
                  &gSophgoGpioProtocolGuid,
                  NULL,
                  (VOID **)&GpioProtocol
                  );
  if (EFI_ERROR (Status)) {
    Print (L"Open gSophgoGpioProtocolGuid error: %r\n",Status);
    return Status;
  }

  if (IsEnable) {
    if (IsHighToEnable)
      GpioMode = GpioConfigOutHigh;
    else
      GpioMode = GpioConfigOutLow;
  } else {
    if (IsHighToEnable)
      GpioMode = GpioConfigOutLow;
    else
      GpioMode = GpioConfigOutHigh;
  }
  Status = GpioProtocol->ModeConfig (GpioProtocol, 0, GpioPin, GpioMode);

  return Status;
}

STATIC
EFI_STATUS
OpenFileAndRead (
  IN  CHAR16   *FilePath,
  OUT VOID     **Buffer,
  OUT UINTN    *BufferSize
  )
{
  EFI_STATUS          Status;
  SHELL_FILE_HANDLE   FileHandle = NULL;
  UINT64              FileSize;
  VOID                *TempBuffer;

  Status = ShellIsFile (FilePath);
  if (EFI_ERROR (Status)) {
    Print (L"Wrong FilePath parameter!\n");
    return EFI_NOT_FOUND;
  }

  Status = ShellOpenFileByName (FilePath, &FileHandle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    Print (L"Cannot open file\n");
    return Status;
  }

  Status = FileHandleSetPosition (FileHandle, 0);
  if (EFI_ERROR(Status)) {
    Print (L"Cannot set file position to first byte\n");
    ShellCloseFile (&FileHandle);
    return Status;
  }

  Status = FileHandleGetSize (FileHandle, &FileSize);
  if (EFI_ERROR (Status)) {
    Print (L"Cannot get file size\n");
  }

  *BufferSize = FileSize;
  TempBuffer = AllocateZeroPool (*BufferSize);
  if (TempBuffer == NULL) {
    Print (L"Failed to allocate memory for file buffer\n");
    ShellCloseFile (&FileHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = FileHandleRead (FileHandle, BufferSize, TempBuffer);
  if (EFI_ERROR (Status)) {
    Print (L"Read from file error\n");
    FreePool (TempBuffer);
    ShellCloseFile (&FileHandle);
    return EFI_ABORTED;
  } else if (*BufferSize != (UINTN) FileSize) {
    Print (L"Not whole file read. Abort\n");
    FreePool (TempBuffer);
    ShellCloseFile (&FileHandle);
    return EFI_ABORTED;
  }

  *Buffer = TempBuffer;
  ShellCloseFile (&FileHandle);

  return EFI_SUCCESS;
}

STATIC
VOID
PrintWithOffset (
  IN  UINT32  EfuseIndex,
  IN  UINT8   *Buffer,
  IN  UINT32  PrintOffset,
  IN  UINT32  PrintSize
  )
{
  UINT32 FinalOffset = PrintOffset + PrintSize;

  Print (L"\neFuse%u\n", EfuseIndex);
  Print (L"          ");
  for (UINT32 Index = 0; Index < 16; ++Index)
    Print (L"%2x ", Index);
  Print (L"\n");

  for (UINT32 Offset = (PrintOffset & (~0xf));
       Offset < FinalOffset;
       Offset += 16) {
    Print (L"%08x: ", (UINT32)Offset);
    for (UINT32 Loop = 0; Loop < 16; ++Loop) {
      INT64 BufIndex = Loop + Offset - PrintOffset;
      if ((BufIndex < 0) || (BufIndex >= PrintSize))
        Print (L".. ");
      else
        Print (L"%02x ", Buffer[BufIndex]);
    }
    Print (L"\n");
  }
}

STATIC
SHELL_STATUS
WriteFileToEfuse (
  IN  EFUSE_OPERATION *EfuseOp
  )
{
  SHELL_PROMPT_RESPONSE  *Resp;
  EFI_STATUS             Status;
  VOID                   *Buffer;
  UINTN                  BufferSize;
  UINT8                  *Data;
  CHAR16                 *Prompt;
  SHELL_STATUS           ShellStatus = SHELL_SUCCESS;

  Status = OpenFileAndRead ((CHAR16 *)EfuseOp->FilePath, &Buffer, &BufferSize);
  if (EFI_ERROR (Status))
    return SHELL_ABORTED;

  Print (L"Write [%s] to device[%u] with offset[%u].\n",
         EfuseOp->FilePath,
         EfuseOp->EfuseIndex,
         EfuseOp->Offset);
  Data = (UINT8 *)Buffer;
  Print (L"File Content will write to eFuse as blow:\n");
  PrintWithOffset (EfuseOp->EfuseIndex, Data, EfuseOp->Offset, BufferSize);

  Prompt = L"Write the above content to the eFuse?(y/n) ";
  Status = ShellPromptForResponse (ShellPromptResponseTypeYesNo, Prompt, (VOID **)&Resp);
  if (Resp == NULL) {
    Print (L"\nCatnot get response!\n");
    FreePool (Buffer);
    return SHELL_ABORTED;
  }
  if (EFI_ERROR (Status) || (*Resp != ShellPromptResponseYes)) {
    SHELL_FREE_NON_NULL (Resp);
    FreePool (Buffer);
    Print (L"\nCancel the operation.\n");
    return SHELL_SUCCESS;
  }

  SHELL_FREE_NON_NULL (Resp);
  Print (L"\nWrite data to the eFuse.\n");

  Status = EfuseWriteEnable (TRUE);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    Print (L"Unable to set GPIO.\n");
    return SHELL_ABORTED;
  }

  Status = EfuseWriteBytes (EfuseOp->EfuseIndex, EfuseOp->Offset, BufferSize, (UINT8 *)Buffer);
  if (EFI_ERROR (Status))
    ShellStatus = SHELL_ABORTED;

  FreePool (Buffer);

  Status = EfuseWriteEnable (FALSE);
  if (EFI_ERROR (Status)) {
    Print (L"Unable to set GPIO.\n");
    return SHELL_ABORTED;
  }

  return ShellStatus;
}

STATIC
SHELL_STATUS
WriteDataToEfuse (
  IN  EFUSE_OPERATION *EfuseOp
  )
{
  SHELL_PROMPT_RESPONSE *Resp;
  CHAR16                *Prompt;
  EFI_STATUS            Status;
  SHELL_STATUS          ShellStatus = SHELL_SUCCESS;
  UINT8                 WriteValue = (UINT8)(mEfuseOp.WriteValue);

  Print (L"Write value[0x%x] into device[%u] at offset[%u].\n",
        WriteValue,
        mEfuseOp.EfuseIndex,
        mEfuseOp.Offset);
  PrintWithOffset (mEfuseOp.EfuseIndex, &WriteValue, mEfuseOp.Offset, 1);

  Prompt = L"Write the above content to the eFuse?(y/n) ";
  Status = ShellPromptForResponse (ShellPromptResponseTypeYesNo, Prompt, (VOID **)&Resp);
  if (Resp == NULL) {
    Print(L"\nCatnot get response!\n");
    return SHELL_ABORTED;
  }
  if (EFI_ERROR (Status) || (*Resp != ShellPromptResponseYes)) {
    SHELL_FREE_NON_NULL (Resp);
    Print(L"\nCancel the operation.\n");
    return SHELL_SUCCESS;
  }

  Print(L"\nWrite data to the eFuse.\n");
  SHELL_FREE_NON_NULL (Resp);

  Status = EfuseWriteEnable (TRUE);
  if (EFI_ERROR (Status)) {
    Print (L"Unable to set GPIO\n");
    return SHELL_ABORTED;
  }

  Status = EfuseWriteBytes (mEfuseOp.EfuseIndex, mEfuseOp.Offset, 1, (VOID *)(&WriteValue));
  if (EFI_ERROR (Status))
    ShellStatus = SHELL_ABORTED;

  Status = EfuseWriteEnable (FALSE);
  if (EFI_ERROR (Status)) {
    Print (L"Unable to set GPIO\n");
    return SHELL_ABORTED;
  }

  return ShellStatus;
}

STATIC
SHELL_STATUS
PrintReadEfuseValue (
  IN  EFUSE_OPERATION *EfuseOp
  )
{
  EFI_STATUS  Status;
  UINT8       *ReadBuffer = NULL;

  if (EfuseOp->ReadSize == 0) {
    Print (L"Read size cannot be 0!\n");
    return SHELL_ABORTED;
  }

  Print (L"Read device[%u] offset[%u] size[%u].\n",
        EfuseOp->EfuseIndex,
        EfuseOp->Offset,
        EfuseOp->ReadSize);

  ReadBuffer = AllocateZeroPool (EfuseOp->ReadSize);
  if (ReadBuffer == NULL) {
    Print (L"Allocate memory error!\n");
    return SHELL_ABORTED;
  }

  Status = EfuseReadBytes (EfuseOp->EfuseIndex,
                           EfuseOp->Offset,
                           EfuseOp->ReadSize,
                           ReadBuffer);
  if (EFI_ERROR (Status))
    return SHELL_ABORTED;

  PrintWithOffset (EfuseOp->EfuseIndex,
                   (UINT8 *)ReadBuffer,
                   EfuseOp->Offset,
                   EfuseOp->ReadSize);

  return EFI_SUCCESS;
}

STATIC
VOID
ShowAllEfuseInfo (
  VOID
  )
{
  EFUSE_INFO *EfuseInfos;
  EFUSE_INFO *InfoPoint;
  UINT32     EfuseNum;
  UINT32     Index;

  GetEfuseInfo (0, NULL, &EfuseNum);
  EfuseInfos = AllocateZeroPool (EfuseNum * sizeof (EFUSE_INFO));
  if (EfuseInfos == NULL) {
    Print (L"Allocate memory error!\n");
    return;
  }

  for (Index = 0; Index < EfuseNum; ++Index) {
    InfoPoint = &(EfuseInfos[Index]);
    GetEfuseInfo (Index, InfoPoint, NULL);
    Print (L"eFuse%d:\n", Index);
    Print (L"    Controller register address: 0x%lx\n", InfoPoint->Regs);
    Print (L"    Size in bytes: %u\n", InfoPoint->NumCells * InfoPoint->CellWidth);
    Print (L"    Cell size in bytes: %u\n", InfoPoint->CellWidth);
    Print (L"    Number of cells: %u\n", InfoPoint->NumCells);
  }

  FreePool (EfuseInfos);
}

STATIC
SHELL_STATUS
ShowEfuseInfo (
  IN UINT32 EfuseIndex
  )
{
  EFUSE_INFO  EfuseInfo;
  EFI_STATUS  Status;

  Status = GetEfuseInfo (EfuseIndex, &EfuseInfo, NULL);
  if (EFI_ERROR (Status))
    return SHELL_ABORTED;

  Print (L"eFuse%d:\n", EfuseIndex);
  Print (L"    Controller register address: 0x%lx\n", EfuseInfo.Regs);
  Print (L"    Size in bytes: %u\n", EfuseInfo.NumCells * EfuseInfo.CellWidth);
  Print (L"    Cell size in bytes: %u\n", EfuseInfo.CellWidth);
  Print (L"    Number of cells: %u\n", EfuseInfo.NumCells);

  return SHELL_SUCCESS;
}

SHELL_STATUS
EFIAPI
ShellCommandRunEfuse (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS   Status;
  SHELL_STATUS ShellStatus = SHELL_SUCCESS;
  LIST_ENTRY   *CheckPackage = NULL;

  ZeroMem (&mEfuseOp, sizeof (EFUSE_OPERATION));

  Status = ShellInitialize ();
  if (EFI_ERROR (Status)) {
    Print (L"ShellInitialize error[%r]\n", Status);
    return SHELL_ABORTED;
  }

  Status = CheckArguements (&CheckPackage);
  if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_ABORTED;
    goto Exit;
  }

  //
  // without command-line parameters.
  //
  if (ShellCommandLineGetRawCount (CheckPackage) == 1) {
    Print (L"Show all eFuse information:\n");
    ShowAllEfuseInfo ();
    ShellStatus = SHELL_SUCCESS;
    goto Exit;
  }

  //
  // with command-line parameter "-h".
  //
  if (ShellCommandLineGetFlag (CheckPackage, L"-h")) {
    PrintHelp ();
    ShellStatus = SHELL_SUCCESS;
    goto Exit;
  }

  //
  // with command-line parameter "-v".
  //
  if (ShellCommandLineGetFlag (CheckPackage, L"-v")) {
    Print(L"\nVersion: 1.0\n\n");
    ShellStatus = SHELL_SUCCESS;
    goto Exit;
  }

  //
  // with command-line parameter "-d" or "-d <num>".
  //
  CONST CHAR16  *EfuseIndexString;
  EfuseIndexString = ShellCommandLineGetValue (CheckPackage, L"-d");
  if (EfuseIndexString != NULL) {
    Status = ShellConvertStringToUint64 (EfuseIndexString, &(mEfuseOp.EfuseIndex), FALSE, TRUE);
    if (EFI_ERROR (Status)) {
      Print (L"Invalid argument[%r]\n",Status);
      PrintHelp ();
      ShellStatus = SHELL_ABORTED;
      goto Exit;
    }
  }

  //
  // "-d" or  "-d <num>".
  //
  if ((ShellCommandLineGetRawCount (CheckPackage) == 2) && (ShellCommandLineGetFlag (CheckPackage, L"-d"))) {
    Print (L"Show eFuse%d information\n", mEfuseOp.EfuseIndex);
    ShellStatus = ShowEfuseInfo (mEfuseOp.EfuseIndex);
    goto Exit;
  }

  //
  // get command-line parameter "-o <num>"
  //
  CONST CHAR16  *OffsetString;
  OffsetString = ShellCommandLineGetValue (CheckPackage, L"-o");
  if (OffsetString != NULL) {
    Status = ShellConvertStringToUint64 (OffsetString, &(mEfuseOp.Offset), FALSE, TRUE);
    if (EFI_ERROR (Status)) {
      Print (L"Invalid argument: %s\n", OffsetString);
      PrintHelp ();
      ShellStatus = SHELL_ABORTED;
      goto Exit;
    }
  }

  //
  // command-line parameter "-r" or "-r -s <num>"
  //
  if (ShellCommandLineGetFlag (CheckPackage, L"-r")) {
    CONST CHAR16  *ReadSizeString;
    mEfuseOp.ReadSize = 1;
    ReadSizeString = ShellCommandLineGetValue (CheckPackage, L"-s");
    if (ReadSizeString != NULL) {
      Status = ShellConvertStringToUint64 (ReadSizeString, &(mEfuseOp.ReadSize), FALSE, TRUE);
      if (EFI_ERROR (Status)) {
        Print (L"Invalid argument: %s\n", ReadSizeString);
        PrintHelp ();
        ShellStatus = SHELL_ABORTED;
        goto Exit;
      }
    }
    ShellStatus = PrintReadEfuseValue (&mEfuseOp);
    goto Exit;
  }

  //
  // command-line parameter "-w"
  //
  CONST CHAR16  *WriteValueString;
  WriteValueString = ShellCommandLineGetValue (CheckPackage, L"-w");
  if (WriteValueString != NULL) {
    Status = ShellConvertStringToUint64 (WriteValueString, &(mEfuseOp.WriteValue), FALSE, TRUE);
    if (EFI_ERROR (Status)) {
      Print (L"Invalid argument: %s\n", WriteValueString);
      PrintHelp ();
      ShellStatus = SHELL_ABORTED;
      goto Exit;
    }
    ShellStatus = WriteDataToEfuse (&mEfuseOp);
    goto Exit;
  } else if (ShellCommandLineGetFlag (CheckPackage, L"-w")) {
    Print (L"No write value, parameter error!\n");
    PrintHelp ();
    ShellStatus = SHELL_ABORTED;
    goto Exit;
  }

  //
  // command-line parameter "-f"
  //
  mEfuseOp.FilePath = ShellCommandLineGetValue (CheckPackage, L"-f");
  if (mEfuseOp.FilePath != NULL) {
    ShellStatus = WriteFileToEfuse (&mEfuseOp);
    goto Exit;
  } else if (ShellCommandLineGetFlag (CheckPackage, L"-f")) {
    Print (L"No file path, parameter error!\n");
    PrintHelp ();
    ShellStatus = SHELL_ABORTED;
    goto Exit;
  }

Exit:
  if (CheckPackage != NULL)
    ShellCommandLineFreeVarList (CheckPackage);

  return ShellStatus;
}


EFI_STATUS
EFIAPI
ShellEfuseToolConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gShellEfuseHiiHandle = NULL;

  gShellEfuseHiiHandle = HiiAddPackages (&gShellEfuseHiiGuid,
                                         gImageHandle,
                                         UefiShellEfuseToolStrings,
                                         NULL);
  if (gShellEfuseHiiHandle == NULL) {
    Print (L"HiiAddPackages ERR!\n");
    return EFI_DEVICE_ERROR;
  }

  ShellCommandRegisterCommandName (
     L"efuse", ShellCommandRunEfuse, ShellCommandGetManFileNameEfuse, 0,
     L"efuse", TRUE , gShellEfuseHiiHandle, STRING_TOKEN (STR_GET_HELP_EFUSE)
     );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ShellEfuseToolDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellEfuseHiiHandle != NULL) {
    HiiRemovePackages (gShellEfuseHiiHandle);
  }

  return EFI_SUCCESS;
}
