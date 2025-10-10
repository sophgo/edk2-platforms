/** @file
  Implementation of Error Injection Table (EINJ) functions according to
  ACPI 6.5 specification (Section 18.6).

  Copyright (c) 2025, Sophgo Technologies Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Einj.h"

STATIC EINJ_CONTEXT  mEinjContext;
STATIC UINTN         mEinjShareMemBase;

/**
  Creates and initializes a minimal Error Injection Table (EINJ) header.

  @param[in,out]  Context    Pointer to EINJ_CONTEXT structure.
  @param[in]      TableSize  Size of EINJ table

  @retval EFI_SUCCESS           EINJ header was created successfully.
  @retval EFI_INVALID_PARAMETER Context is NULL.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for EINJ header.
**/
EFI_STATUS
EinjHeaderCreator (
  IN OUT EINJ_CONTEXT  *Context,
  IN     UINT32        TableSize
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  Header;

  if (Context == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Context parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Context->EinjHeader = AllocateZeroPool (TableSize);
  if (Context->EinjHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate EINJ header\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize EINJ header using RISCV_ACPI_HEADER macro
  //
  Header = (EFI_ACPI_DESCRIPTION_HEADER) RISCV_ACPI_HEADER (
    EFI_ACPI_6_5_ERROR_INJECTION_TABLE_SIGNATURE,
    EFI_ACPI_6_5_ERROR_INJECTION_TABLE_HEADER,
    EFI_ACPI_6_5_ERROR_INJECTION_TABLE_REVISION
    );

  //
  // Copy header to EINJ table
  //
  CopyMem (&Context->EinjHeader->Header, &Header, sizeof (EFI_ACPI_DESCRIPTION_HEADER));

  //
  // Initialize table length to header size
  //
  Context->EinjHeader->Header.Length = sizeof (EFI_ACPI_6_5_ERROR_INJECTION_TABLE_HEADER);

  //
  // Initialize EINJ specific fields
  //
  Context->EinjHeader->InjectionHeaderSize = sizeof (EFI_ACPI_6_5_ERROR_INJECTION_TABLE_HEADER)
                                              - sizeof (EFI_ACPI_DESCRIPTION_HEADER);

  return EFI_SUCCESS;
}

/**
  Adds an instruction entry to EINJ.

  @param[in,out] Context             Pointer to EINJ context
  @param[in]     InstructionEntry    Pointer to instruction entry
  @param[in]     EntrySize           Size of instruction entry

  @retval EFI_SUCCESS           instruction entry added successfully
  @retval EFI_INVALID_PARAMETER Invalid parameter
  @retval EFI_BUFFER_TOO_SMALL  Not enough space in EINJ
**/
EFI_STATUS
EinjAddInstructionEntry (
  IN OUT EINJ_CONTEXT  *Context,
  IN     VOID          *InstructionEntry,
  IN     UINT32        EntrySize
  )
{
  UINT8   *CurrentPtr;
  UINT32  NewLength;

  if (Context == NULL || Context->EinjHeader == NULL || InstructionEntry == NULL || EntrySize == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate new table length
  //
  NewLength = Context->EinjHeader->Header.Length + EntrySize;
  if (NewLength > EINJ_TABLE_SIZE) {
    DEBUG ((DEBUG_ERROR, "%a: EINJ table size exceeded\n", __func__));
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Add instruction entry
  //
  CurrentPtr = (UINT8 *)Context->EinjHeader + Context->EinjHeader->Header.Length;
  CopyMem (CurrentPtr, InstructionEntry, EntrySize);

  //
  // Update EINJ header
  //
  Context->EinjHeader->Header.Length = NewLength;
  Context->EinjHeader->InjectionEntryCount++;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: New instruction entry count: %d, new length: %d\n",
    __func__,
    Context->EinjHeader->InjectionEntryCount,
    NewLength
    ));

  return EFI_SUCCESS;
}

STATIC
UINT64
GetAddrByInjectionAction (
  IN     UINT8    InjectionAction
  )
{
  UINT64 Offset = 0;

  switch (InjectionAction) {
  case EFI_ACPI_6_5_EINJ_BEGIN_INJECTION_OPERATION:
    Offset = EINJ_BEGIN_OPERATION_OFF;
    break;
  case EFI_ACPI_6_5_EINJ_GET_TRIGGER_ERROR_ACTION_TABLE:
    Offset = EINJ_GET_TRIGGER_TABLE_OFF;
    break;
  case EFI_ACPI_6_5_EINJ_SET_ERROR_TYPE:
    Offset = EINJ_SET_ERROR_TYPE_OFF;
    break;
  case EFI_ACPI_6_5_EINJ_GET_ERROR_TYPE:
    Offset = EINJ_GET_ERROR_TYPE_OFF;
    break;
  case EFI_ACPI_6_5_EINJ_END_OPERATION:
    Offset = EINJ_END_OPERATION_OFF;
    break;
  case EFI_ACPI_6_5_EINJ_EXECUTE_OPERATION:
    Offset = EINJ_EXECUTE_OPERATION_OFF;
    break;
  case EFI_ACPI_6_5_EINJ_CHECK_BUSY_STATUS:
    Offset = EINJ_CHECK_BUSY_STATUS_OFF;
    break;
  case EFI_ACPI_6_5_EINJ_GET_COMMAND_STATUS:
    Offset = EINJ_GET_COMMAND_STATUS_OFF;
    break;
  case EFI_ACPI_6_5_EINJ_SET_ERROR_TYPE_WITH_ADDRESS:
    Offset = EINJ_SET_ERR_TYPE_WITH_ADDR_OFF;
    break;
  case EFI_ACPI_6_5_EINJ_TRIGGER_ERROR:
    Offset = EINJ_TRIGGER_ERROR_OFF;
    break;
  default:
    DEBUG ((DEBUG_ERROR, "%a: Unsupport injection action: %r\n", __func__, EFI_UNSUPPORTED));
    ASSERT (FALSE);
    return 0;
  }

  return mEinjShareMemBase + Offset;
}

STATIC
VOID
SetEinjRegisterDefaultVal (
  VOID
  )
{
  UINT64 RegisterAddr;
  UINT64 DefaultVal;

  RegisterAddr = GetAddrByInjectionAction (EFI_ACPI_6_5_EINJ_BEGIN_INJECTION_OPERATION);
  DefaultVal = ACTION_END_OPERATION_VAL;
  MmioWrite64 (RegisterAddr, DefaultVal);

  RegisterAddr = GetAddrByInjectionAction (EFI_ACPI_6_5_EINJ_GET_TRIGGER_ERROR_ACTION_TABLE);
  DefaultVal = mEinjShareMemBase + EINJ_TRIGGER_ERROR_ACTION_TABLE_OFF;
  MmioWrite64 (RegisterAddr, DefaultVal);

  RegisterAddr = GetAddrByInjectionAction (EFI_ACPI_6_5_EINJ_SET_ERROR_TYPE);
  DefaultVal = 0;
  MmioWrite64 (RegisterAddr, DefaultVal);

  //
  // Set the supported error types for injection using EINJ
  //
  RegisterAddr = GetAddrByInjectionAction (EFI_ACPI_6_5_EINJ_GET_ERROR_TYPE);
  DefaultVal = EFI_ACPI_6_5_EINJ_ERROR_MEMORY_CORRECTABLE
                | EFI_ACPI_6_5_EINJ_ERROR_MEMORY_UNCORRECTABLE_NONFATAL
                | EFI_ACPI_6_5_EINJ_ERROR_MEMORY_UNCORRECTABLE_FATAL
                | EFI_ACPI_6_5_EINJ_ERROR_PCI_EXPRESS_CORRECTABLE
                | EFI_ACPI_6_5_EINJ_ERROR_PCI_EXPRESS_UNCORRECTABLE_NONFATAL
                | EFI_ACPI_6_5_EINJ_ERROR_PCI_EXPRESS_UNCORRECTABLE_FATAL;
  MmioWrite64 (RegisterAddr, DefaultVal);

  RegisterAddr = GetAddrByInjectionAction (EFI_ACPI_6_5_EINJ_END_OPERATION);
  DefaultVal = ACTION_BEGIN_OPERATION_VAL;
  MmioWrite64 (RegisterAddr, DefaultVal);

  RegisterAddr = GetAddrByInjectionAction (EFI_ACPI_6_5_EINJ_EXECUTE_OPERATION);
  DefaultVal = 0;
  MmioWrite64 (RegisterAddr, DefaultVal);

  RegisterAddr = GetAddrByInjectionAction (EFI_ACPI_6_5_EINJ_CHECK_BUSY_STATUS);
  DefaultVal = 1;
  MmioWrite64 (RegisterAddr, DefaultVal);

  RegisterAddr = GetAddrByInjectionAction (EFI_ACPI_6_5_EINJ_GET_COMMAND_STATUS);
  DefaultVal = EFI_ACPI_6_5_EINJ_STATUS_SUCCESS;
  MmioWrite64 (RegisterAddr, DefaultVal);

  RegisterAddr = GetAddrByInjectionAction (EFI_ACPI_6_5_EINJ_SET_ERROR_TYPE_WITH_ADDRESS);
  ZeroMem ((VOID *)RegisterAddr, 36);

  RegisterAddr = GetAddrByInjectionAction (EFI_ACPI_6_5_EINJ_TRIGGER_ERROR);
  DefaultVal = 0;
  MmioWrite64 (RegisterAddr, DefaultVal);
}

STATIC
VOID
InitInstructionEntry (
  IN OUT EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY *InstructionEntry,
  IN     UINT8    InjectionAction,
  IN     UINT8    Instruction,
  IN     UINT8    Flags,
  IN     UINT64   Value,
  IN     UINT64   Mask
  )
{
  InstructionEntry->InjectionAction = InjectionAction;
  InstructionEntry->Instruction = Instruction;
  InstructionEntry->Flags = Flags;
  InstructionEntry->Reserved0 = 0;

  InstructionEntry->RegisterRegion.AddressSpaceId = EFI_ACPI_6_5_SYSTEM_MEMORY;
  InstructionEntry->RegisterRegion.RegisterBitWidth = 64;
  InstructionEntry->RegisterRegion.RegisterBitOffset = 0;
  InstructionEntry->RegisterRegion.AccessSize = EFI_ACPI_6_5_QWORD;
  InstructionEntry->RegisterRegion.Address = GetAddrByInjectionAction (InjectionAction);

  InstructionEntry->Value = Value;
  InstructionEntry->Mask = Mask;
}

STATIC
VOID
InitTriggerActionTable (
  IN OUT EFI_ACPI_6_5_EINJ_TRIGGER_ACTION_TABLE *TriggerActionTable
  )
{
  EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY  *InstructionEntry;
  UINT8  *CurrentPtr;

  TriggerActionTable->HeaderSize = sizeof (EFI_ACPI_6_5_EINJ_TRIGGER_ACTION_TABLE);
  TriggerActionTable->TableSize = sizeof (EFI_ACPI_6_5_EINJ_TRIGGER_ACTION_TABLE)
                                  + sizeof (EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY);
  TriggerActionTable->EntryCount = 1;

  InstructionEntry = (EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY *)(TriggerActionTable + 1);
  InitInstructionEntry (
    InstructionEntry,
    EFI_ACPI_6_5_EINJ_TRIGGER_ERROR,
    EFI_ACPI_6_5_EINJ_WRITE_REGISTER_VALUE,
    0,
    ACTION_TRIGGER_ERROR_VAL,
    0xFFFFFFFFULL
  );

  CurrentPtr = (UINT8  *)(mEinjShareMemBase + EINJ_TRIGGER_ERROR_ACTION_TABLE_OFF);
  CopyMem (CurrentPtr, TriggerActionTable, TriggerActionTable->TableSize);
}

STATIC
VOID
CreatInstructionEntries (
  IN OUT EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY *InstructionEntry
  )
{
  InitInstructionEntry (
    &InstructionEntry[0],
    EFI_ACPI_6_5_EINJ_BEGIN_INJECTION_OPERATION,
    EFI_ACPI_6_5_EINJ_WRITE_REGISTER_VALUE,
    0,
    ACTION_BEGIN_OPERATION_VAL,
    0xFFFFFFFFULL
  );

  InitInstructionEntry (
    &InstructionEntry[1],
    EFI_ACPI_6_5_EINJ_GET_TRIGGER_ERROR_ACTION_TABLE,
    EFI_ACPI_6_5_EINJ_READ_REGISTER,
    0,
    0,
    0xFFFFFFFFFFFFFFFFULL
  );

  InitInstructionEntry (
    &InstructionEntry[2],
    EFI_ACPI_6_5_EINJ_SET_ERROR_TYPE,
    EFI_ACPI_6_5_EINJ_WRITE_REGISTER,
    EFI_ACPI_6_5_EINJ_PRESERVE_REGISTER,
    0,
    0xFFFFFFFFULL
  );

  InitInstructionEntry (
    &InstructionEntry[3],
    EFI_ACPI_6_5_EINJ_GET_ERROR_TYPE,
    EFI_ACPI_6_5_EINJ_READ_REGISTER,
    0,
    0,
    0xFFFFFFFFULL
  );

  InitInstructionEntry (
    &InstructionEntry[4],
    EFI_ACPI_6_5_EINJ_END_OPERATION,
    EFI_ACPI_6_5_EINJ_WRITE_REGISTER_VALUE,
    0,
    ACTION_END_OPERATION_VAL,
    0xFFFFFFFFULL
  );

  InitInstructionEntry (
    &InstructionEntry[5],
    EFI_ACPI_6_5_EINJ_EXECUTE_OPERATION,
    EFI_ACPI_6_5_EINJ_WRITE_REGISTER_VALUE,
    0,
    ACTION_EXECUTE_OPERATION_VAL,
    0xFFFFFFFFULL
  );

  InitInstructionEntry (
    &InstructionEntry[6],
    EFI_ACPI_6_5_EINJ_CHECK_BUSY_STATUS,
    EFI_ACPI_6_5_EINJ_READ_REGISTER_VALUE,
    0,
    ACTION_CHECK_BUSY_STATUS_VAL,
    1
  );

  InitInstructionEntry (
    &InstructionEntry[7],
    EFI_ACPI_6_5_EINJ_GET_COMMAND_STATUS,
    EFI_ACPI_6_5_EINJ_READ_REGISTER,
    0,
    0,
    3
  );

  InitInstructionEntry (
    &InstructionEntry[8],
    EFI_ACPI_6_5_EINJ_SET_ERROR_TYPE_WITH_ADDRESS,
    EFI_ACPI_6_5_EINJ_WRITE_REGISTER,
    0,
    0,
    0xFFFFFFFFFFFFFFFFULL
  );
}

EINJ_CONTEXT *
GetEinjContext (
  VOID
  )
{
  return &mEinjContext;
}

VOID
FreeEinjContextHeader (
  VOID
  )
{
  if (mEinjContext.EinjHeader != NULL)
    FreePool (mEinjContext.EinjHeader);
}

/**
  Initializes the Error Injection Table (EINJ).

  @param[in]  ErrorBlockBase  The base address to place EINJ related register and data
  @param[out] MemUsedSize     The byte size of the SHARED_MEMORY used by EINJ

  @retval EFI_SUCCESS      EINJ table was initialized successfully.
  @retval EFI_DEVICE_ERROR Failed to create or initialize EINJ header.
**/
EFI_STATUS
EinjInitTable (
  IN  UINTN     ErrorBlockBase,
  OUT UINT32    *MemUsedSize
  )
{
  UINT8       Checksum;
  EFI_STATUS  Status;
  UINT8       Index;
  EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY *InstructionEntry = NULL;
  EFI_ACPI_6_5_EINJ_TRIGGER_ACTION_TABLE *TriggerActionTable = NULL;

  if (IS_ALIGNED (ErrorBlockBase, 8) == 0){
    DEBUG ((DEBUG_ERROR, "%a: Invalid ErrorBlockBase: 0x%llx\n", __func__, ErrorBlockBase));
    DEBUG ((DEBUG_ERROR, "%a: ErrorBlockBase must be aligned on 8-byte boundaries!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Clear EINJ context
  //
  ZeroMem (&mEinjContext, sizeof (EINJ_CONTEXT));
  mEinjShareMemBase = ErrorBlockBase;

  Status = EinjHeaderCreator (&mEinjContext, EINJ_TABLE_SIZE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create EINJ header: %r\n", __func__, Status));
    return EFI_DEVICE_ERROR;
  }

  InstructionEntry = AllocateZeroPool (
                      sizeof (EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY) * EINJ_INJECTION_ENTRY_COUNT
                     );
  if (InstructionEntry == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate Instruction Entry\n", __func__));
    FreeEinjContextHeader ();
    return EFI_OUT_OF_RESOURCES;
  }

  TriggerActionTable = AllocateZeroPool (
                      sizeof (EFI_ACPI_6_5_EINJ_TRIGGER_ACTION_TABLE) 
                      + sizeof (EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY)
                     );
  if (TriggerActionTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate TriggerActionTable\n", __func__));
    FreePool (InstructionEntry);
    FreeEinjContextHeader ();
    return EFI_OUT_OF_RESOURCES;
  }
  InitTriggerActionTable (TriggerActionTable);
  CreatInstructionEntries (InstructionEntry);
  SetEinjRegisterDefaultVal ();
  *MemUsedSize = EINJ_MEM_USED_SIZE;

  //
  // Add instruction entries to EINJ
  //
  for (Index = 0; Index < EINJ_INJECTION_ENTRY_COUNT; Index++) {
    Status = EinjAddInstructionEntry (
                &mEinjContext,
                &InstructionEntry[Index],
                sizeof (EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY)
              );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to add instruction entry %d - %r\n",
              __func__, Index, Status));
      FreePool (TriggerActionTable);
      FreePool (InstructionEntry);
      FreeEinjContextHeader ();
      return Status;
    }
  }

  //
  // Calculate and update checksum
  //
  Checksum = CalculateCheckSum8 (
               (UINT8 *)(mEinjContext.EinjHeader),
               mEinjContext.EinjHeader->Header.Length
               );
  mEinjContext.EinjHeader->Header.Checksum = Checksum;

  DEBUG ((DEBUG_INFO, "%a: EINJ table initialized successfully\n", __func__));

  return EFI_SUCCESS;
}
