/** @file
  Copyright (c) 2016, Hisilicon Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Protocol/FdtClient.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Include/SG2042AcpiHeader.h>

#define CORECOUNT(X) ((X) * CORE_NUM_PER_SOCKET)

  typedef struct {
    UINT64 MinBase;
    UINT64 TotalSize;
    BOOLEAN Valid;
  } NODE_INFO;

/*

STATIC
VOID
RemoveUnusedMemoryNode (
  IN OUT EFI_ACPI_STATIC_RESOURCE_AFFINITY_TABLE_SERVER  *Table,
  IN     UINTN                        MemoryNodeNum
)
{
  UINTN                   CurrPtr, NewPtr;
  UINTN                   OriginalLength = Table->Header.Header.Length;

  if (MemoryNodeNum >= EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT_SERVER) {
    DEBUG((DEBUG_INFO,
           "No unused nodes (MemoryNodeNum >= %d)\n",
           EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT_SERVER));
    return;
  }

  CurrPtr = (UINTN) &(Table->Memory[EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT_SERVER]);
  NewPtr = (UINTN) &(Table->Memory[MemoryNodeNum]);

  UINTN   MoveSize = (UINTN)Table + OriginalLength - CurrPtr;
  UINTN   UnusedSize = CurrPtr - NewPtr;

  DEBUG((DEBUG_INFO,
         "Unused region: %d bytes (%d nodes)\n",
         UnusedSize,
         EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT_SERVER - MemoryNodeNum));

  CopyMem((VOID *)NewPtr, (VOID *)CurrPtr, MoveSize);

  Table->Header.Header.Length = OriginalLength - UnusedSize;

  DEBUG((DEBUG_INFO, "Updated SRAT Memory Nodes (%d valid):\n", MemoryNodeNum));
  for (UINTN i = 0; i < MemoryNodeNum; i++) {
    DEBUG((DEBUG_INFO,
           "  Node %d: Domain=%d, Base=0x%lx%08lx, Size=0x%lx%08lx\n",
           i,
           Table->Memory[i].ProximityDomain,
           Table->Memory[i].AddressBaseHigh,
           Table->Memory[i].AddressBaseLow,
           Table->Memory[i].LengthHigh,
           Table->Memory[i].LengthLow));
  }

  return;
}

*/

STATIC
EFI_STATUS
UpdateSrat (
  IN OUT EFI_ACPI_STATIC_RESOURCE_AFFINITY_TABLE_SERVER *Table
  )
{
  FDT_CLIENT_PROTOCOL              *FdtClient;
  EFI_STATUS                       Status, FindNodeStatus;
  INT32                            Node;
  CONST UINT32                     *Reg;
  UINT32                           RegSize;
  UINTN                            AddressCells, SizeCells;
  UINT64                           CurBase;
  UINT64                           CurSize;
  CONST UINT32                     *NodeId;
  UINT32                           NodeIdLen;
  UINTN                            MemoryNode = 0;

  NODE_INFO NodeInfo[EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT_SERVER];
  ZeroMem(NodeInfo, sizeof(NodeInfo));

  DEBUG((DEBUG_INFO, "SRAT: Updating SRAT memory information.\n"));

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  for (UINTN i = 0; i < EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT_SERVER; i++) {
    NodeInfo[i].MinBase = MAX_UINT64;
    NodeInfo[i].TotalSize = 0;
    NodeInfo[i].Valid = FALSE;
  }

  for (FindNodeStatus = FdtClient->FindMemoryNodeReg (
                                     FdtClient,
                                     &Node,
                                     (CONST VOID **)&Reg,
                                     &AddressCells,
                                     &SizeCells,
                                     &RegSize
                                     );
       !EFI_ERROR (FindNodeStatus);
       FindNodeStatus = FdtClient->FindNextMemoryNodeReg (
                                     FdtClient,
                                     Node,
                                     &Node,
                                     (CONST VOID **)&Reg,
                                     &AddressCells,
                                     &SizeCells,
                                     &RegSize
                                     ))
  {
    ASSERT (AddressCells <= 2);
    ASSERT (SizeCells <= 2);

    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "numa-node-id",
                          (CONST VOID **)&NodeId,
                          &NodeIdLen
                          );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: GetNodeProperty ('numa-node-id') failed (Status == %r)\n",
        __func__,
        Status
      ));

      Reg += (AddressCells + SizeCells) * (RegSize / ((AddressCells + SizeCells) * sizeof(UINT32)));
      RegSize = 0;
      continue;
    }

    UINT32 ProximityDomain = SwapBytes32 (NodeId[0]);
    DEBUG((DEBUG_ERROR, "SRAT:   NodeIdLen=%d, NodeId=%d\n", NodeIdLen, ProximityDomain));

    if (ProximityDomain >= EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT_SERVER) {
      DEBUG((DEBUG_ERROR, "SRAT: Invalid ProximityDomain %d (>= %d)\n",
             ProximityDomain, EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT_SERVER));
      continue;
    }

    NodeInfo[ProximityDomain].Valid = TRUE;

    CONST UINT32 *CurrentReg = Reg;
    UINT32 CurrentRegSize = RegSize;
    while (CurrentRegSize > 0) {
      CurBase = SwapBytes32 (*CurrentReg++);
      if (AddressCells > 1) {
        CurBase = (CurBase << 32) | SwapBytes32 (*CurrentReg++);
      }

      CurSize = SwapBytes32 (*CurrentReg++);
      if (SizeCells > 1) {
        CurSize = (CurSize << 32) | SwapBytes32 (*CurrentReg++);
      }

      CurrentRegSize -= (AddressCells + SizeCells) * sizeof (UINT32);

      if (CurBase < NodeInfo[ProximityDomain].MinBase) {
        NodeInfo[ProximityDomain].MinBase = CurBase;
      }
      NodeInfo[ProximityDomain].TotalSize += CurSize;
    }
  }

  for (UINTN i = 0; i < EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT_SERVER; i++) {
    if (!NodeInfo[i].Valid) continue;

    UINT64 RoundedSize = NodeInfo[i].TotalSize;
    if (NodeInfo[i].TotalSize <= SIZE_8GB) {
      RoundedSize = SIZE_8GB;
    } else if (NodeInfo[i].TotalSize <= SIZE_16GB) {
      RoundedSize = SIZE_16GB;
    } else if (NodeInfo[i].TotalSize <= SIZE_32GB) {
      RoundedSize = SIZE_32GB;
    }

    Table->Memory[MemoryNode].ProximityDomain = i;
    Table->Memory[MemoryNode].Flags = NodeInfo[i].Valid;
    Table->Memory[MemoryNode].AddressBaseLow = (UINT32)NodeInfo[i].MinBase;
    Table->Memory[MemoryNode].AddressBaseHigh = (UINT32)(NodeInfo[i].MinBase >> 32);
    Table->Memory[MemoryNode].LengthLow = (UINT32)RoundedSize;
    Table->Memory[MemoryNode].LengthHigh = (UINT32)(RoundedSize >> 32);
    MemoryNode++;
  }

//   RemoveUnusedMemoryNode (Table, MemoryNode);

  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
UpdateSlit (
  IN OUT EFI_ACPI_DESCRIPTION_HEADER  *Table
  )
{
  return  EFI_SUCCESS;
}

EFI_STATUS
UpdateAcpiTable (
  IN OUT EFI_ACPI_DESCRIPTION_HEADER      *TableHeader
)
{
  EFI_STATUS Status = EFI_SUCCESS;

  switch (TableHeader->Signature) {

  case EFI_ACPI_6_5_SYSTEM_RESOURCE_AFFINITY_TABLE_SIGNATURE:
    Status = UpdateSrat ((EFI_ACPI_STATIC_RESOURCE_AFFINITY_TABLE_SERVER *) TableHeader);
    break;

  case EFI_ACPI_6_5_SYSTEM_LOCALITY_INFORMATION_TABLE_SIGNATURE:
    Status = UpdateSlit (TableHeader);
    break;
  }
  return Status;
}
