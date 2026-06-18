/** @file
  Collect Ethernet NIC inventory and report it through BMC OEM IPMI.

  Prefer SNP ports and map each port to a PCIe Ethernet function (02/00) by
  same-handle or ancestor device-path match. Collapse duplicate SNP handles to
  one entry per PCI function.

  If SNP is unavailable, fall back to PCI class scan for visibility.

  Copyright (c) 2026 SOPHGO Technologies Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <HostHwInfoReportToBmcOem.h>
#include "HwInventoryInternal.h"

#include <IndustryStandard/Pci22.h>

#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <Protocol/SimpleNetwork.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#define NIC_MAX_PORTS        24
#define NIC_MODEL_BUF_CHARS  128
#define NIC_MFG_BUF_CHARS    96
#define NIC_MAC_BUF_CHARS    64

#define NIC_INTEL_IG_CSR_RAL0  0x05400UL
#define NIC_INTEL_IG_CSR_RAH0  0x05404UL
#define NIC_PCI_MAX_BAR       6


typedef struct {
  BOOLEAN     HasPci;
  EFI_HANDLE  SnpHandle;
  EFI_HANDLE  PciHandle;
  UINT64      SortKeyCard;
  UINT64      SortKeyPort;
  UINTN       Segment;
  UINTN       Bus;
  UINTN       Device;
  UINTN       Function;
  UINT16      Vid;
  UINT16      Did;
  UINT16      SubVid;
  UINT16      SubId;
} NIC_PORT_ENTRY;

STATIC CONST CHAR8 *
NicManufacturerFromVid (
  UINT16 Vid
  )
{
  switch (Vid) {
    case 0x8086:
    case 0x8087:
      return "Intel Corporation";
    case 0x10EC:
      return "Realtek Semiconductor Corp.";
    case 0x14E4:
      return "Broadcom Inc.";
    case 0x15B3:
      return "Mellanox Technologies";
    default:
      return NULL;
  }
}

STATIC CONST CHAR8 *
NicModelFromVidDid (
  UINT16 Vid,
  UINT16 Did
  )
{
  if (Vid == 0x8086) {
    switch (Did) {
      case 0x10FB:
        return "Intel 82599ES 10-Gigabit SFI/SFP+ Network Connection";
      case 0x151D:
        return "Intel Ethernet Connection E823-L for QSFP";
      case 0x1520:
        return "Intel I350 Ethernet Controller Virtual Function";
      case 0x1521:
        return "Intel I350 Gigabit Network Connection";
      case 0x1522:
        return "Intel I350 Gigabit Fiber Network Connection";
      case 0x1523:
        return "Intel I350 Gigabit Backplane Connection";
      case 0x1524:
        return "Intel I350 Gigabit Connection";
      case 0x1525:
        return "Intel 82567V-4 Gigabit Network Connection";
      case 0x1526:
        return "Intel 82576 Gigabit Network Connection";
      case 0x1527:
        return "Intel 82580 Gigabit Fiber Network Connection";
      case 0x1528:
        return "Intel Ethernet Controller 10-Gigabit X540-AT2";
      case 0x1533:
        return "Intel I210 Gigabit Ethernet Controller (Copper)";
      case 0x1536:
        return "Intel I210 Gigabit Ethernet Controller (SerDes)";
      case 0x1537:
        return "Intel I210 Gigabit Ethernet Controller (Backplane)";
      case 0x1538:
        return "Intel I210 Gigabit Ethernet Controller (external PHY)";
      case 0x157B:
        return "Intel I210 Gigabit Ethernet Controller";
      case 0x1539:
      case 0x153A:
        return "Intel I211 Gigabit Ethernet Controller";
      default:
        return NULL;
    }
  }

  return NULL;
}

STATIC UINT64
NicPciSortKeyCard (
  IN UINTN  Seg,
  IN UINTN  Bus,
  IN UINTN  Device
  )
{
  return LShiftU64 (Seg, 48) | ((UINT64)Bus << 32) | ((UINT64)Device << 16);
}

STATIC UINT64
NicPciSortKeyPort (
  IN UINTN  Function
  )
{
  return (UINT64)Function;
}

STATIC VOID
NicFormatMacAscii (
  IN  CONST UINT8  *Hw,
  IN  UINT32       HwAddrSizeBytes,
  OUT CHAR8        *Ascii,
  IN  UINTN        AsciiCharsMax
  )
{
  STATIC CONST CHAR8  HexDigit[] = "0123456789ABCDEF";
  CHAR8               *Walk;
  UINTN               Idx;
  UINTN               Bytes;

  if ((AsciiCharsMax == 0) || (Ascii == NULL) || (Hw == NULL)) {
    return;
  }

  Ascii[0] = '\0';
  Bytes    = HwAddrSizeBytes;

  if (Bytes > sizeof (((EFI_SIMPLE_NETWORK_PROTOCOL *)0)->Mode->PermanentAddress.Addr)) {
    Bytes = sizeof (((EFI_SIMPLE_NETWORK_PROTOCOL *)0)->Mode->PermanentAddress.Addr);
  }

  if (Bytes == 0) {
    AsciiStrCpyS (Ascii, AsciiCharsMax, "unset");
    return;
  }

  Walk = Ascii;

  for (Idx = 0; Idx < Bytes; Idx++) {
    if (((UINTN)(Walk - Ascii) + 6U) >= AsciiCharsMax) {
      break;
    }

    if (Idx > 0) {
      *Walk++ = ':';
    }

    *Walk++ = HexDigit[(Hw[Idx] >> 4U) & 0x0FU];
    *Walk++ = HexDigit[Hw[Idx] & 0x0FU];
  }

  *Walk = '\0';
}

STATIC BOOLEAN
NicMacAllZero (
  IN CONST UINT8  *Hw,
  IN UINT32       Len
  )
{
  UINT32  Idx;

  for (Idx = 0; Idx < Len; Idx++) {
    if (Hw[Idx] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}


/**
  Intel I210 / I211 / I350-family (Linux "igb" CSR): RAL0/RAH0 at 0x5400 in MMIO BAR.
**/
STATIC BOOLEAN
NicIntelIgDeviceIdSupported (
  UINT16 Did
  )
{
  switch (Did) {
    case 0x1521:
    case 0x1522:
    case 0x1523:
    case 0x1524:
    case 0x1526:
    case 0x1527:
    case 0x1533:
    case 0x1536:
    case 0x1537:
    case 0x1538:
    case 0x1539:
    case 0x153A:
    case 0x157B:
      return TRUE;
    default:
      return FALSE;
  }
}

STATIC BOOLEAN
NicMacAllOnes (
  IN CONST UINT8  *Hw,
  IN UINT32       Len
  )
{
  UINT32  Idx;

  for (Idx = 0; Idx < Len; Idx++) {
    if (Hw[Idx] != 0xFF) {
      return FALSE;
    }
  }

  return TRUE;
}

STATIC BOOLEAN
NicIgMacSemanticsOk (
  IN CONST UINT8  *Hw
  )
{
  if ((Hw == NULL) || NicMacAllZero (Hw, 6) || NicMacAllOnes (Hw, 6)) {
    return FALSE;
  }

  if ((Hw[0] & (UINT8)0x01) != 0) {
    return FALSE;
  }

  return TRUE;
}

STATIC VOID
NicIntelIgEnsurePciMemDecode (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  EFI_STATUS  Status;
  UINT16      Cmd;

  if ((PciIo == NULL) || (PciIo->Attributes == NULL)) {
    return;
  }

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationEnable,
                    EFI_PCI_IO_ATTRIBUTE_MEMORY | EFI_PCI_IO_ATTRIBUTE_BUS_MASTER,
                    NULL
                    );
  DEBUG ((
    DEBUG_VERBOSE,
    "NicInfoDumpDxe: PciIo Attributes Enable(MEMORY|BUS_MASTER) => %r\n",
    Status
    ));

  Cmd = 0;
  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_COMMAND_OFFSET, 1, &Cmd);
  if (!EFI_ERROR (Status) &&
      ((Cmd & EFI_PCI_COMMAND_MEMORY_SPACE) == 0))
  {
    Cmd = (UINT16)(Cmd |
                   EFI_PCI_COMMAND_MEMORY_SPACE |
                   EFI_PCI_COMMAND_BUS_MASTER);
    Status = PciIo->Pci.Write (
                          PciIo,
                          EfiPciIoWidthUint16,
                          PCI_COMMAND_OFFSET,
                          1,
                          &Cmd
                          );
    DEBUG ((
      DEBUG_VERBOSE,
      "NicInfoDumpDxe: PCI COMMAND MM decode fallback => %r new 0x%04x\n",
      Status,
      Cmd
      ));
  }
}

STATIC EFI_STATUS
NicIntelIgReadPermanentMacMmio (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  OUT UINT8                *Mac6
  )
{
  EFI_STATUS  St;
  UINT8       Bar;
  UINT32      Ral;
  UINT32      Rah;

  if ((PciIo == NULL) || (Mac6 == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  NicIntelIgEnsurePciMemDecode (PciIo);

  for (Bar = 0; Bar < NIC_PCI_MAX_BAR; Bar++) {
    St = PciIo->Mem.Read (
                      PciIo,
                      EfiPciIoWidthUint32,
                      Bar,
                      NIC_INTEL_IG_CSR_RAL0,
                      1,
                      &Ral
                      );
    if (EFI_ERROR (St)) {
      continue;
    }

    St = PciIo->Mem.Read (
                      PciIo,
                      EfiPciIoWidthUint32,
                      Bar,
                      NIC_INTEL_IG_CSR_RAH0,
                      1,
                      &Rah
                      );
    if (EFI_ERROR (St)) {
      continue;
    }

    Mac6[0] = (UINT8)(Ral & 0xFFU);
    Mac6[1] = (UINT8)((Ral >> 8U) & 0xFFU);
    Mac6[2] = (UINT8)((Ral >> 16U) & 0xFFU);
    Mac6[3] = (UINT8)((Ral >> 24U) & 0xFFU);
    Mac6[4] = (UINT8)(Rah & 0xFFU);
    Mac6[5] = (UINT8)((Rah >> 8U) & 0xFFU);

    if (!NicIgMacSemanticsOk (Mac6)) {
      continue;
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC VOID
NicDumpDevicePathText (
  IN EFI_HANDLE  Handle
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Dp;
  EFI_STATUS                 Status;
  CHAR16                     *Uni;
  CHAR8                      *Ascii;
  UINTN                      Idx;
  UINTN                      Len;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&Dp,
                  mHwInventoryImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  Uni = ConvertDevicePathToText (Dp, FALSE, TRUE);
  if (Uni == NULL) {
    return;
  }

  Len = StrLen (Uni);

  Ascii = AllocatePool (Len + 1);
  if (Ascii != NULL) {
    for (Idx = 0; Idx < Len; Idx++) {
      Ascii[Idx] = (CHAR8)(Uni[Idx] & MAX_UINT8);
    }

    Ascii[Len] = '\0';
    DEBUG ((DEBUG_INFO, "  Device path   : %a\n", Ascii));
    FreePool (Ascii);
  }

  FreePool (Uni);
}

STATIC VOID
NicSortPortsAscending (
  IN NIC_PORT_ENTRY *Entry,
  IN UINTN          Count
  )
{
  UINTN           I;
  UINTN           J;
  NIC_PORT_ENTRY  Tmp;

  if ((Entry == NULL) || (Count < 2)) {
    return;
  }

  for (I = 0; I + 1 < Count; I++) {
    for (J = I + 1; J < Count; J++) {
      BOOLEAN Swap;

      Swap =
        Entry[J].SortKeyCard < Entry[I].SortKeyCard ||
        (Entry[J].SortKeyCard == Entry[I].SortKeyCard &&
         Entry[J].SortKeyPort < Entry[I].SortKeyPort);

      if (Swap) {
        CopyMem (&Tmp,     &Entry[I], sizeof (Tmp));
        CopyMem (&Entry[I], &Entry[J], sizeof (Tmp));
        CopyMem (&Entry[J], &Tmp,     sizeof (Tmp));
      }
    }
  }
}

/**
  TRUE if Full device path equals Prefix or continues after the same Prefix nodes.
**/
STATIC BOOLEAN
NicDpIsPrefixOf (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *Prefix,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *Full
  )
{
  CONST EFI_DEVICE_PATH_PROTOCOL  *A;
  CONST EFI_DEVICE_PATH_PROTOCOL  *B;

  A = Prefix;
  B = Full;
  while (!IsDevicePathEnd (A)) {
    if (IsDevicePathEnd (B)) {
      return FALSE;
    }

    if (DevicePathNodeLength (A) != DevicePathNodeLength (B)) {
      return FALSE;
    }

    if (CompareMem (A, B, DevicePathNodeLength (A)) != 0) {
      return FALSE;
    }

    A = NextDevicePathNode (A);
    B = NextDevicePathNode (B);
  }

  return TRUE;
}

/**
  Find PCIe class 02/00 PciIo for an SNP handle: direct on handle, else longest
  DP-prefix match among all PciIo handles (typical BINDING_DRIVER child SNP).
**/
STATIC EFI_STATUS
NicResolveEthernetPciIoForSnpHandle (
  IN  EFI_HANDLE            SnpHandle,
  OUT EFI_PCI_IO_PROTOCOL   **OutPciIo,
  OUT EFI_HANDLE            *OutPciHandle
  )
{
  EFI_STATUS                  Status;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  UINT8                        Bc;
  UINT8                        Sc;
  EFI_DEVICE_PATH_PROTOCOL    *SnpDp;
  EFI_HANDLE                   *pciList;
  UINTN                        pciCount;
  UINTN                        Idx;
  UINTN                        bestSize;
  EFI_HANDLE                   bestHd;
  EFI_PCI_IO_PROTOCOL         *bestIo;

  *OutPciIo     = NULL;
  *OutPciHandle = NULL;

  Status = gBS->OpenProtocol (
                  SnpHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  mHwInventoryImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    Sc = 0;
    Bc = 0;
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET + 1, 1, &Sc);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET + 2, 1, &Bc);
    if ((Bc == PCI_CLASS_NETWORK) && (Sc == PCI_CLASS_NETWORK_ETHERNET)) {
      *OutPciIo     = PciIo;
      *OutPciHandle = SnpHandle;
      return EFI_SUCCESS;
    }
  }

  Status = gBS->HandleProtocol (
                  SnpHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&SnpDp
                  );
  if (EFI_ERROR (Status) || (SnpDp == NULL)) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &pciCount,
                  &pciList
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  bestSize = 0;
  bestHd   = NULL;
  bestIo   = NULL;

  for (Idx = 0; Idx < pciCount; Idx++) {
    EFI_DEVICE_PATH_PROTOCOL  *CandDp;
    UINTN                      CandSize;

    if (EFI_ERROR (gBS->HandleProtocol (
                        pciList[Idx],
                        &gEfiDevicePathProtocolGuid,
                        (VOID **)&CandDp
                        )) ||
        (CandDp == NULL))
    {
      continue;
    }

    if (!NicDpIsPrefixOf (CandDp, SnpDp)) {
      continue;
    }

    CandSize = GetDevicePathSize (CandDp);
    if ((bestIo != NULL) && (CandSize <= bestSize)) {
      continue;
    }

    if (EFI_ERROR (gBS->OpenProtocol (
                        pciList[Idx],
                        &gEfiPciIoProtocolGuid,
                        (VOID **)&PciIo,
                        mHwInventoryImageHandle,
                        NULL,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        )))
    {
      continue;
    }

    Sc = 0;
    Bc = 0;
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET + 1, 1, &Sc);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET + 2, 1, &Bc);
    if ((Bc != PCI_CLASS_NETWORK) || (Sc != PCI_CLASS_NETWORK_ETHERNET)) {
      continue;
    }

    bestSize = CandSize;
    bestHd   = pciList[Idx];
    bestIo   = PciIo;
  }

  FreePool (pciList);

  if ((bestIo == NULL) || (bestHd == NULL)) {
    return EFI_NOT_FOUND;
  }

  *OutPciIo     = bestIo;
  *OutPciHandle = bestHd;
  return EFI_SUCCESS;
}

/**
  Enumerate PCI Ethernet (class 02 / subclass 00) when no SNP exists, for diagnosis.
**/
STATIC VOID
NicDumpPciEthernetFallback (
  VOID
  )
{
  EFI_HANDLE  *PciHandles;
  UINTN       NumPci;
  EFI_STATUS   Status;
  UINTN        Idx;
  UINTN        Found;

  PciHandles = NULL;
  Found      = 0;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &NumPci,
                  &PciHandles
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "NicInfoDumpDxe: PCI fallback: LocateHandleBuffer(PciIo) %r\n",
      Status
      ));
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "NicInfoDumpDxe: --- PCI Ethernet (02/00) scan (SNP not installed) ---\n"
    ));

  for (Idx = 0; Idx < NumPci; Idx++) {
    EFI_PCI_IO_PROTOCOL  *PciIo;
    UINT8                  Bc;
    UINT8                  Sc;
    UINT8                  ProgIf;
    UINTN                  Seg;
    UINTN                  Bus;
    UINTN                  Device;
    UINTN                  Fn;
    UINT16                 Vid;
    UINT16                 Did;
    UINT16                 Sv;
    UINT16                 Ss;

    Status = gBS->OpenProtocol (
                    PciHandles[Idx],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo,
                    mHwInventoryImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Bc      = 0;
    Sc      = 0;
    ProgIf  = 0;
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET + 2, 1, &Bc);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET + 1, 1, &Sc);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET,     1, &ProgIf);

    if ((Bc != PCI_CLASS_NETWORK) || (Sc != PCI_CLASS_NETWORK_ETHERNET)) {
      continue;
    }

    PciIo->GetLocation (PciIo, &Seg, &Bus, &Device, &Fn);
    Vid = 0;
    Did = 0;
    Sv  = 0;
    Ss  = 0;
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_VENDOR_ID_OFFSET, 1, &Vid);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_DEVICE_ID_OFFSET, 1, &Did);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_SUBSYSTEM_VENDOR_ID_OFFSET, 1, &Sv);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_SUBSYSTEM_ID_OFFSET, 1, &Ss);

    DEBUG ((
      DEBUG_INFO,
      "NicInfoDumpDxe [PCI-only] %04lx:%02lx:%02lx.%lx  Class %02x.%02x PI %02x  "
      "VID/DID 0x%04x/0x%04x  SubSys 0x%04x/0x%04x\n",
      Seg,
      Bus,
      Device,
      Fn,
      Bc,
      Sc,
      ProgIf,
      Vid,
      Did,
      Sv,
      Ss
      ));

    NicDumpDevicePathText (PciHandles[Idx]);

    if ((Vid == 0x8086) && NicIntelIgDeviceIdSupported (Did)) {
      UINT8  HwMac[6];

      if (!EFI_ERROR (NicIntelIgReadPermanentMacMmio (PciIo, HwMac))) {
        CHAR8 MacLine[NIC_MAC_BUF_CHARS];

        NicFormatMacAscii (HwMac, 6U, MacLine, sizeof (MacLine));
        DEBUG ((DEBUG_INFO, "  MAC (Intel CSR/MMIO): %a\n", MacLine));
      } else {
        DEBUG ((
          DEBUG_INFO,
          "  MAC (Intel CSR/MMIO): unavailable (RAL/RAH or memory BAR).\n"
          ));
      }
    }

    Found++;
  }

  FreePool (PciHandles);

  if (Found == 0) {
    DEBUG ((
      DEBUG_INFO,
      "NicInfoDumpDxe: PCI fallback: no PCIe device with class 02/00 (Ethernet).\n"
      ));
  }

  DEBUG ((
    DEBUG_INFO,
    "NicInfoDumpDxe: Hint: install SNP driver (e.g. ETH_ENABLE=TRUE for DwMac SNP on SG2044, "
    "or PCIe NIC driver that publishes EFI_SIMPLE_NETWORK_PROTOCOL).\n"
    ));
}

STATIC UINTN
NicSnpDevicePathSize (
  IN EFI_HANDLE  Handle
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Dp;
  EFI_STATUS                St;

  St = gBS->HandleProtocol (
              Handle,
              &gEfiDevicePathProtocolGuid,
              (VOID **)&Dp
              );
  if (EFI_ERROR (St) || (Dp == NULL)) {
    return MAX_UINTN;
  }

  return GetDevicePathSize (Dp);
}

/**
  MNP/IP stacks may install a second SNP on longer DPs (e.g. MAC/IPv4).
  Keep one SNP per PCI function: prefer the shortest device path (hardware-side).
**/
STATIC VOID
NicDedupeSnpPortsPerPciFunction (
  IN OUT NIC_PORT_ENTRY  *Ports,
  IN OUT UINTN           *Count
  )
{
  UINTN  Src;
  UINTN  T;
  UINTN  Dst;
  UINTN  N;
  UINTN  SizeSrc;
  UINTN  SizeT;

  if ((Ports == NULL) || (Count == NULL) || (*Count < 2)) {
    return;
  }

  N = *Count;
  Dst = 0;

  for (Src = 0; Src < N; Src++) {
    BOOLEAN  Keep;

    Keep    = TRUE;
    SizeSrc = NicSnpDevicePathSize (Ports[Src].SnpHandle);

    for (T = 0; T < N; T++) {
      if (T == Src) {
        continue;
      }

      if ((Ports[T].Segment != Ports[Src].Segment) ||
          (Ports[T].Bus != Ports[Src].Bus) ||
          (Ports[T].Device != Ports[Src].Device) ||
          (Ports[T].Function != Ports[Src].Function))
      {
        continue;
      }

      SizeT = NicSnpDevicePathSize (Ports[T].SnpHandle);
      if (SizeT < SizeSrc) {
        Keep = FALSE;
        break;
      }

      if ((SizeT == SizeSrc) && (T < Src)) {
        Keep = FALSE;
        break;
      }
    }

    if (Keep) {
      if (Dst != Src) {
        CopyMem (&Ports[Dst], &Ports[Src], sizeof (Ports[0]));
      }

      Dst++;
    }
  }

  *Count = Dst;
}

STATIC VOID
NicGetPortMacBytes (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN  EFI_PCI_IO_PROTOCOL          *PciIoMac,
  IN  UINT16                       Vid,
  IN  UINT16                       Did,
  OUT UINT8                        Mac[6]
  )
{
  EFI_SIMPLE_NETWORK_MODE  *Mode;
  UINTN                    CopyLen;

  ZeroMem (Mac, 6);
  if ((Snp == NULL) || (Snp->Mode == NULL)) {
    return;
  }

  Mode = Snp->Mode;
  if (Mode->State == EfiSimpleNetworkStopped) {
    Snp->Start (Snp);
    Snp->Initialize (Snp, 0, 0);
  }

  CopyLen = (Mode->HwAddressSize < 6) ? Mode->HwAddressSize : 6;
  CopyMem (Mac, &Mode->PermanentAddress.Addr[0], CopyLen);

  if (NicMacAllZero (Mac, 6) &&
      (Vid == 0x8086) &&
      NicIntelIgDeviceIdSupported (Did) &&
      (PciIoMac != NULL) &&
      !EFI_ERROR (NicIntelIgReadPermanentMacMmio (PciIoMac, Mac)))
  {
    return;
  }
}

STATIC VOID
NicReportPortToBmc (
  IN UINT8         NicIndex,
  IN CONST CHAR8   *MfgBuf,
  IN CONST CHAR8   *ModelBuf,
  IN CONST UINT8   Mac[6],
  IN UINT16        Vid,
  IN UINT16        Did,
  IN UINT16        PciSegment,
  IN EFI_HANDLE    PciHandle
  )
{
  BMC_HW_INFO_NIC_IPMI_PAYLOAD  Payload;
  EFI_STATUS                    Status;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;

  ZeroMem (&Payload, sizeof (Payload));
  Payload.NicIndex      = NicIndex;
  Payload.PlatformType  = HostHwInfoReportToBmcGetPlatformType ();
  Payload.PciSegment    = PciSegment;
  CopyMem (Payload.MacAddress, Mac, 6);
  HostHwInfoReportToBmcAsciiCopyPadded (
    Payload.Manufacturer,
    sizeof (Payload.Manufacturer),
    MfgBuf,
    AsciiStrLen (MfgBuf)
    );
  HostHwInfoReportToBmcAsciiCopyPadded (
    Payload.Model,
    sizeof (Payload.Model),
    ModelBuf,
    AsciiStrLen (ModelBuf)
    );
  Payload.PciVendorId = Vid;
  Payload.PciDeviceId = Did;

  DevicePath = NULL;
  if (!EFI_ERROR (gBS->OpenProtocol (
                      PciHandle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID **)&DevicePath,
                      mHwInventoryImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      )))
  {
    HostHwInfoReportToBmcResolvePhysicalSlot (
      DevicePath,
      &Payload.PhysicalSlot
      );
  }

  Status = HostHwInfoReportToBmcSubmitNic (&Payload);
  DEBUG ((
    DEBUG_INFO,
    "HostHwInfoReportToBmcDxe: BMC NIC[%u] seg=%04x %02x:%02x:%02x:%02x:%02x:%02x %a => %r\n",
    NicIndex,
    Payload.PciSegment,
    Mac[0],
    Mac[1],
    Mac[2],
    Mac[3],
    Mac[4],
    Mac[5],
    ModelBuf,
    Status
    ));
}

VOID
DumpNicInventory (
  VOID
  )
{
  EFI_STATUS      Status;
  UINTN            NumHandles;
  EFI_HANDLE      *Handles;
  UINTN            Idx;
  UINTN            Count;
  NIC_PORT_ENTRY   Ports[NIC_MAX_PORTS];
  UINTN            Outer;
  UINTN            Inner;
  UINTN            PortsInGrp;
  CHAR8            ModelBuf[NIC_MODEL_BUF_CHARS];
  CHAR8            MfgBuf[NIC_MFG_BUF_CHARS];
  CHAR8            MacStr[NIC_MAC_BUF_CHARS];
  CONST CHAR8     *ModelRaw;
  CONST CHAR8     *MfgRaw;
  UINT8            NicReportIndex;

  ZeroMem (&Ports, sizeof (Ports));
  NicReportIndex = 0;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleNetworkProtocolGuid,
                  NULL,
                  &NumHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "NicInfoDumpDxe: LocateHandleBuffer(EFI_SIMPLE_NETWORK_PROTOCOL) failed (%r).\n",
      Status
      ));
    if (Handles != NULL) {
      FreePool (Handles);
    }

    NicDumpPciEthernetFallback ();
    return;
  }

  if (NumHandles == 0) {
    DEBUG ((DEBUG_INFO, "NicInfoDumpDxe: no EFI_SIMPLE_NETWORK_PROTOCOL handles.\n"));
    if (Handles != NULL) {
      FreePool (Handles);
    }

    NicDumpPciEthernetFallback ();
    return;
  }

  Count = 0;

  for (Idx = 0; Idx < NumHandles; Idx++) {
    EFI_PCI_IO_PROTOCOL  *PciIo;
    EFI_HANDLE            pciHd;

    if (Count >= NIC_MAX_PORTS) {
      DEBUG ((DEBUG_WARN, "NicInfoDumpDxe: NIC_MAX_PORTS capped\n"));
      break;
    }

    if (EFI_ERROR (NicResolveEthernetPciIoForSnpHandle (
                    Handles[Idx],
                    &PciIo,
                    &pciHd
                    )) || (PciIo == NULL))
    {
      DEBUG ((
        DEBUG_VERBOSE,
        "NicInfoDumpDxe: skip SNP %p (no PCIe Ethernet 02/00 on handle or DP ancestors).\n",
        Handles[Idx]
        ));
      continue;
    }

    ZeroMem (&Ports[Count], sizeof (Ports[Count]));
    Ports[Count].HasPci     = TRUE;
    Ports[Count].SnpHandle  = Handles[Idx];
    Ports[Count].PciHandle  = pciHd;

    PciIo->GetLocation (
            PciIo,
            &Ports[Count].Segment,
            &Ports[Count].Bus,
            &Ports[Count].Device,
            &Ports[Count].Function
            );
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_VENDOR_ID_OFFSET, 1, &Ports[Count].Vid);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_DEVICE_ID_OFFSET, 1, &Ports[Count].Did);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_SUBSYSTEM_VENDOR_ID_OFFSET, 1, &Ports[Count].SubVid);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_SUBSYSTEM_ID_OFFSET, 1, &Ports[Count].SubId);

    Ports[Count].SortKeyCard = NicPciSortKeyCard (
                                                        Ports[Count].Segment,
                                                        Ports[Count].Bus,
                                                        Ports[Count].Device
                                                        );
    Ports[Count].SortKeyPort = NicPciSortKeyPort (Ports[Count].Function);
    Count++;
  }

  FreePool (Handles);

  NicDedupeSnpPortsPerPciFunction (Ports, &Count);

  if (Count == 0) {
    DEBUG ((
      DEBUG_INFO,
      "NicInfoDumpDxe: no SNP on PCIe Ethernet (02/00) adapters; PCI enumeration follows.\n"
      ));
    NicDumpPciEthernetFallback ();
    return;
  }

  NicSortPortsAscending (Ports, Count);

  Outer = 0;
  while (Outer < Count) {
    PortsInGrp = 1;
    Inner      = Outer + 1;

    while ((Inner < Count) &&
           (Ports[Inner].SortKeyCard == Ports[Outer].SortKeyCard) &&
           (Ports[Inner].HasPci == Ports[Outer].HasPci))
    {
      PortsInGrp++;
      Inner++;
    }

    DEBUG ((
      DEBUG_INFO,
      "=== NIC (group @ card key 0x%016Lx , %Lu port(s)) ===\n",
      Ports[Outer].SortKeyCard,
      (UINT64)PortsInGrp
      ));

    ModelRaw = NicModelFromVidDid (Ports[Outer].Vid, Ports[Outer].Did);
    if (ModelRaw != NULL) {
      AsciiStrCpyS (ModelBuf, sizeof (ModelBuf), ModelRaw);
    } else {
      AsciiSPrint (
        ModelBuf,
        sizeof (ModelBuf),
        "Ethernet (VID_%04x DID_%04x)",
        Ports[Outer].Vid,
        Ports[Outer].Did
        );
    }

    MfgRaw = NicManufacturerFromVid (Ports[Outer].Vid);
    if (MfgRaw != NULL) {
      AsciiStrCpyS (MfgBuf, sizeof (MfgBuf), MfgRaw);
    } else {
      AsciiSPrint (
        MfgBuf,
        sizeof (MfgBuf),
        "Unknown (VID_%04x)",
        Ports[Outer].Vid
        );
    }

    DEBUG ((DEBUG_INFO, "  Name           : %a\n", ModelBuf));
    DEBUG ((DEBUG_INFO, "  Manufacturer   : %a\n", MfgBuf));
    DEBUG ((DEBUG_INFO, "  Model          : %a\n", ModelBuf));
    DEBUG ((DEBUG_INFO, "  Port count     : %Lu\n", (UINT64)PortsInGrp));

    if (Ports[Outer].HasPci && (Ports[Outer].Vid != 0)) {
      DEBUG ((
        DEBUG_INFO,
        "  PCI VID/DID    : 0x%04x / 0x%04x  SubSys 0x%04x/0x%04x\n",
        Ports[Outer].Vid,
        Ports[Outer].Did,
        Ports[Outer].SubVid,
        Ports[Outer].SubId
        ));
    }

    for (Inner = Outer; Inner < Outer + PortsInGrp; Inner++) {
      EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;
      EFI_PCI_IO_PROTOCOL          *PciIoMac;
      UINT8                         HwMac[6];

      PciIoMac = NULL;
      Status   = gBS->OpenProtocol (
                        Ports[Inner].SnpHandle,
                        &gEfiSimpleNetworkProtocolGuid,
                        (VOID **)&Snp,
                        mHwInventoryImageHandle,
                        NULL,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
      if (!EFI_ERROR (Status)) {
        gBS->OpenProtocol (
              Ports[Inner].PciHandle,
              &gEfiPciIoProtocolGuid,
              (VOID **)&PciIoMac,
              mHwInventoryImageHandle,
              NULL,
              EFI_OPEN_PROTOCOL_GET_PROTOCOL
              );
        NicGetPortMacBytes (
          Snp,
          PciIoMac,
          Ports[Inner].Vid,
          Ports[Inner].Did,
          HwMac
          );
        NicFormatMacAscii (HwMac, 6U, MacStr, sizeof (MacStr));
        if (NicMacAllZero (HwMac, 6)) {
          AsciiStrCpyS (MacStr, sizeof (MacStr), "(all-zero)");
        }

        NicReportPortToBmc (
          NicReportIndex,
          MfgBuf,
          ModelBuf,
          HwMac,
          Ports[Inner].Vid,
          Ports[Inner].Did,
          (UINT16)Ports[Inner].Segment,
          Ports[Inner].PciHandle
          );
        NicReportIndex++;
      } else {
        AsciiStrCpyS (MacStr, sizeof (MacStr), "(unknown)");
      }

      DEBUG ((
        DEBUG_INFO,
        "  --- Port FN %Lu (SEG:BUS:DEV.FN %02lx:%02lx:%02lx.%lx) ---\n",
        (UINT64)(Inner - Outer),
        Ports[Inner].Segment,
        Ports[Inner].Bus,
        Ports[Inner].Device,
        Ports[Inner].Function
        ));
      DEBUG ((DEBUG_INFO, "  MAC address    : %a\n", MacStr));

      if (Ports[Inner].HasPci) {
        DEBUG ((
          DEBUG_INFO,
          "    PCI Fn       : %04lx:%02lx:%02lx.%lx VID/DID %04x/%04x\n",
          Ports[Inner].Segment,
          Ports[Inner].Bus,
          Ports[Inner].Device,
          Ports[Inner].Function,
          Ports[Inner].Vid,
          Ports[Inner].Did
          ));
      }

      NicDumpDevicePathText (Ports[Inner].SnpHandle);
    }

    Outer += PortsInGrp;
  }
}
