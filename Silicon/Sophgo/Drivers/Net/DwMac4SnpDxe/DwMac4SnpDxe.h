/** @file

  Copyright (c) 2011 - 2019, Intel Corporaton. All rights reserved.
  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  The original software modules are licensed as follows:

  Copyright (c) 2012-2014, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef DWMAC4_SNP_DXE_H__
#define DWMAC4_SNP_DXE_H__

//
// Protocols used by this driver
//
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleNetwork.h>

#include <Include/Phy.h>
#include "DwMac4DxeUtil.h"

/*------------------------------------------------------------------------------
  Information Structure
------------------------------------------------------------------------------*/

typedef struct {
  MAC_ADDR_DEVICE_PATH                   MacAddrDP;
  EFI_DEVICE_PATH_PROTOCOL               End;
} SOPHGO_SIMPLE_NETWORK_DEVICE_PATH;

/*---------------------------------------------------------------------------------------------------------------------

  UEFI-Compliant functions for EFI_SIMPLE_NETWORK_PROTOCOL

  Refer to the Simple Network Protocol section (24.1) in the UEFI 2.8 Specification for related definitions

---------------------------------------------------------------------------------------------------------------------*/

EFI_STATUS
EFIAPI
SnpStart (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

EFI_STATUS
EFIAPI
SnpStop (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

EFI_STATUS
EFIAPI
SnpInitialize (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN       UINTN                       ExtraRxBufferSize OPTIONAL,
  IN       UINTN                       ExtraTxBufferSize OPTIONAL
  );

EFI_STATUS
EFIAPI
SnpReset (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN       BOOLEAN                     ExtendedVerification
  );

EFI_STATUS
EFIAPI
SnpShutdown (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

EFI_STATUS
EFIAPI
SnpReceiveFilters (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN       UINT32                      Enable,
  IN       UINT32                      Disable,
  IN       BOOLEAN                     ResetMCastFilter,
  IN       UINTN                       MCastFilterCnt  OPTIONAL,
  IN       EFI_MAC_ADDRESS             *MCastFilter    OPTIONAL
  );

EFI_STATUS
EFIAPI
SnpStationAddress (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN       BOOLEAN                     Reset,
  IN       EFI_MAC_ADDRESS             *NewMac
);

EFI_STATUS
EFIAPI
SnpStatistics (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN       BOOLEAN                     Reset,
  IN  OUT  UINTN                       *StatSize,
      OUT  EFI_NETWORK_STATISTICS      *Statistics
  );

EFI_STATUS
EFIAPI
SnpMcastIptoMac (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN       BOOLEAN                     IsIpv6,
  IN       EFI_IP_ADDRESS              *Ip,
      OUT  EFI_MAC_ADDRESS             *McastMac
  );

EFI_STATUS
EFIAPI
SnpNvData (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN       BOOLEAN                     ReadWrite,
  IN       UINTN                       Offset,
  IN       UINTN                       BufferSize,
  IN  OUT  VOID                        *Buffer
  );

EFI_STATUS
EFIAPI
SnpGetStatus (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  OUT      UINT32                      *IrqStat  OPTIONAL,
  OUT      VOID                        **TxBuff  OPTIONAL
  );

EFI_STATUS
EFIAPI
SnpTransmit (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN       UINTN                       HdrSize,
  IN       UINTN                       BufferSize,
  IN       VOID                        *Data,
  IN       EFI_MAC_ADDRESS             *SrcAddr  OPTIONAL,
  IN       EFI_MAC_ADDRESS             *DstAddr  OPTIONAL,
  IN       UINT16                      *Protocol OPTIONAL
  );

EFI_STATUS
EFIAPI
SnpReceive (
  IN       EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
      OUT  UINTN                       *HdrSize      OPTIONAL,
  IN  OUT  UINTN                       *BufferSize,
      OUT  VOID                        *Data,
      OUT  EFI_MAC_ADDRESS             *SrcAddr      OPTIONAL,
      OUT  EFI_MAC_ADDRESS             *DstAddr      OPTIONAL,
      OUT  UINT16                      *Protocol     OPTIONAL
  );

#endif // DWMAC4_SNP_DXE_H__
