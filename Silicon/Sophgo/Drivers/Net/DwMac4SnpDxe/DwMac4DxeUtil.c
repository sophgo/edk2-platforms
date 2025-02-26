/** @file

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/IoLib.h>
#include <Library/NetLib.h>
#include <Library/DmaLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Include/Phy.h>
#include "DwMac4DxeUtil.h"
#include <Protocol/Cpu.h>

struct StmmacRxRouting {
  UINT32 RegMask;
  UINT32 RegShift;
};

UINT32 RxChannelsCount = 1;
UINT32 TxChannelsCount = 1;
UINT32 RxQueuesToUse   = 1;
UINT32 TxQueuesToUse   = 1;

STATIC
UINT32
DwMac4MmioRead (
  IN SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN UINT32                        Offset
  )
{
  ASSERT ((Offset & 3) == 0);

  return MmioRead32 ((UINTN)(DwMac4Driver->RegBase + Offset));
}

STATIC
VOID
DwMac4MmioWrite (
  IN SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN UINT32                        Offset,
  IN UINT32                        Data
  )
{
  ASSERT ((Offset & 3) == 0);

  MemoryFence ();
  MmioWrite32 ((UINTN)(DwMac4Driver->RegBase + Offset), Data);
}

VOID
EFIAPI
StmmacSetUmacAddr (
  IN  EFI_MAC_ADDRESS               *MacAddress,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         RegN
  )
{
  //
  // Note: This MAC_ADDR0 registers programming sequence cannot be swap:
  // Must program HIGH Offset first before LOW Offset
  // because synchronization is triggered when MAC Address0 Low Register are written.
  //
  DwMac4MmioWrite (DwMac4Driver, GMAC_ADDR_HIGH(RegN),
               (UINT32)(MacAddress->Addr[4] & 0xFF) |
                      ((MacAddress->Addr[5] & 0xFF) << 8) |
                      GMAC_HI_REG_AE
                    );
  //
  // MacAddress->Addr[0,1,2] is the 3 bytes OUI
  //
  DwMac4MmioWrite (DwMac4Driver, GMAC_ADDR_LOW(RegN),
                       (MacAddress->Addr[0] & 0xFF) |
                      ((MacAddress->Addr[1] & 0xFF) << 8) |
                      ((MacAddress->Addr[2] & 0xFF) << 16) |
                      ((MacAddress->Addr[3] & 0xFF) << 24)
                    );

  DEBUG ((
    DEBUG_VERBOSE,
    "%a(): GMAC_ADDR_LOW(%d)  = 0x%08X \r\n",
    __func__,
    RegN,
    DwMac4MmioRead (DwMac4Driver, GMAC_ADDR_LOW(RegN))
    ));
  DEBUG ((
    DEBUG_VERBOSE,
    "%a(): GMAC_ADDR_HIGH(%d)  = 0x%08X \r\n",
    __func__,
    RegN,
    DwMac4MmioRead (DwMac4Driver, GMAC_ADDR_HIGH(RegN))
    ));
}

VOID
EFIAPI
StmmacGetMacAddr (
  OUT EFI_MAC_ADDRESS               *MacAddress,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         RegN
  )
{
  UINT32 MacAddrHighValue;
  UINT32 MacAddrLowValue;

  //
  // Read the Mac Addr high register
  //
  MacAddrHighValue = DwMac4MmioRead (DwMac4Driver, GMAC_ADDR_HIGH(RegN)) & 0xFFFF;

  //
  // Read the Mac Addr low register
  //
  MacAddrLowValue = DwMac4MmioRead (DwMac4Driver, GMAC_ADDR_LOW(RegN));

  SetMem (MacAddress, sizeof(*MacAddress), 0);
  MacAddress->Addr[0] = MacAddrLowValue & 0xFF;
  MacAddress->Addr[1] = (MacAddrLowValue >> 8) & 0xFF;
  MacAddress->Addr[2] = (MacAddrLowValue >> 16) & 0xFF;
  MacAddress->Addr[3] = (MacAddrLowValue >> 24) & 0xFF;
  MacAddress->Addr[4] = MacAddrHighValue & 0xFF;
  MacAddress->Addr[5] = (MacAddrHighValue >> 8) & 0xFF;

  DEBUG ((
    DEBUG_INFO,
    "%a(): MAC Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n",
    __func__,
    MacAddress->Addr[0],
    MacAddress->Addr[1],
    MacAddress->Addr[2],
    MacAddress->Addr[3],
    MacAddress->Addr[4],
    MacAddress->Addr[5]
  ));
}

VOID
DwMac4DmaAxi (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32 Value;
  UINT32 AxiWrOsrLmt;
  UINT32 AxiRdOsrLmt;

  AxiWrOsrLmt = 1;
  AxiRdOsrLmt = 1;
  Value = DwMac4MmioRead (DwMac4Driver, DMA_SYS_BUS_MODE);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a(): Master AXI performs %s burst length\n",
    __func__,
    (Value & DMA_SYS_BUS_FB) ? L"fixed" : L"any"
    ));

  Value &= ~DMA_AXI_WR_OSR_LMT;
  Value |= (AxiWrOsrLmt & DMA_AXI_OSR_MAX) << DMA_AXI_WR_OSR_LMT_SHIFT;

  Value &= ~DMA_AXI_RD_OSR_LMT;
  Value |= (AxiRdOsrLmt & DMA_AXI_OSR_MAX) << DMA_AXI_RD_OSR_LMT_SHIFT;

  Value |= DMA_AXI_BLEN16 | DMA_AXI_BLEN8 | DMA_AXI_BLEN4;

  Value |= DMA_AXI_1KBBE;

  DwMac4MmioWrite (DwMac4Driver, DMA_SYS_BUS_MODE, Value);
}

VOID
StmmacSetRxTailPtr (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         TailPtr,
  IN  UINT32                        Channel
  )
{
  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_RX_END_ADDR(Channel), TailPtr);
}

VOID
StmmacSetTxTailPtr (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         TailPtr,
  IN  UINT32                        Channel
  )
{
  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_TX_END_ADDR(Channel), TailPtr);
}

VOID
DwMac4DmaStartTx (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        Channel
  )
{
  UINT32 Value;

  Value = DwMac4MmioRead (DwMac4Driver, DMA_CHAN_TX_CONTROL(Channel));
  Value |= DMA_CONTROL_ST;

  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_TX_CONTROL(Channel), Value);

  Value = DwMac4MmioRead (DwMac4Driver, GMAC_CONFIG);
  Value |= GMAC_CONFIG_TE;

  DwMac4MmioWrite (DwMac4Driver, GMAC_CONFIG, Value);
}

VOID
DwMac4DmaStopTx (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        Channel
  )
{
  UINT32 Value;

  Value = DwMac4MmioRead (DwMac4Driver, DMA_CHAN_TX_CONTROL(Channel));
  Value &= ~DMA_CONTROL_ST;

  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_TX_CONTROL(Channel), Value);
}

VOID
DwMac4DmaStartRx (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        Channel
  )
{
  UINT32 Value;

  Value = DwMac4MmioRead (DwMac4Driver, DMA_CHAN_RX_CONTROL(Channel));
  Value |= DMA_CONTROL_SR;

  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_RX_CONTROL(Channel), Value);

  Value = DwMac4MmioRead (DwMac4Driver, GMAC_CONFIG);
  Value |= GMAC_CONFIG_RE;

  DwMac4MmioWrite (DwMac4Driver, GMAC_CONFIG, Value);
}

VOID
DwMac4DmaStopRx (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        Channel
  )
{
  UINT32 Value;

  Value = DwMac4MmioRead (DwMac4Driver, DMA_CHAN_RX_CONTROL(Channel));
  Value &= ~DMA_CONTROL_SR;

  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_RX_CONTROL(Channel), Value);
}

/*
 * Start all RX and TX DMA channels
 */
VOID
EFIAPI
StmmacStartAllDma (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32 Channel;

  for (Channel = 0; Channel < RxChannelsCount; Channel++) {
    DwMac4DmaStartRx (DwMac4Driver, Channel);
  }

  for (Channel = 0; Channel < TxChannelsCount; Channel++) {
    DwMac4DmaStartTx (DwMac4Driver, Channel);
  }
}

/*
 * Stop all RX and TX DMA channels
 */
VOID
EFIAPI
StmmacStopAllDma (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32 Channel;

  for (Channel = 0; Channel < RxChannelsCount; Channel++) {
    DwMac4DmaStopRx (DwMac4Driver, Channel);
  }

  for (Channel = 0; Channel < TxChannelsCount; Channel++) {
    DwMac4DmaStopTx (DwMac4Driver, Channel);
  }
}

EFI_STATUS
EFIAPI
DwMac4DmaReset (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32     Value;
  UINT32     Timeout;

  Timeout = 1000000;
  Value = DwMac4MmioRead (DwMac4Driver, DMA_BUS_MODE);

  //
  // DMA SW reset
  //
  Value |= DMA_BUS_MODE_SFT_RESET;
  DwMac4MmioWrite (DwMac4Driver, DMA_BUS_MODE, Value);

  //
  // wait till the bus software reset
  //
  do {
    Value = DwMac4MmioRead (DwMac4Driver, DMA_BUS_MODE);
    if (Timeout-- == 10000) {
      DEBUG ((
        DEBUG_ERROR,
        "%a(): Bus software reset timeout\n",
        __func__
        ));

      return EFI_TIMEOUT;
    }
  } while (Value & DMA_BUS_MODE_SFT_RESET);

  return EFI_SUCCESS;
}

VOID
DwMac4DmaInit (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32 Value;

  Value = DwMac4MmioRead (DwMac4Driver, DMA_SYS_BUS_MODE);

  Value |= DMA_SYS_BUS_AAL;
  Value |= DMA_SYS_BUS_EAME;

  DwMac4MmioWrite (DwMac4Driver, DMA_SYS_BUS_MODE, Value);

  Value = DwMac4MmioRead (DwMac4Driver, DMA_BUS_MODE);

  //
  // Only DWMAC core version 5.20 onwards supports HW descriptor prefetch.
  //
  if (DwMac4MmioRead (DwMac4Driver, GMAC4_VERSION) >= 0x52) {
    Value |= DMA_BUS_MODE_DCHE;
  }

  DwMac4MmioWrite (DwMac4Driver, DMA_BUS_MODE, Value);
}

VOID
DwMac4InitChannel (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         Channel
  )
{
  UINT32  Value;
  BOOLEAN Pblx8;

  Pblx8 = TRUE;

  //
  // Common channel control register config
  //
  Value = DwMac4MmioRead (DwMac4Driver, DMA_CHAN_CONTROL(Channel));

  if (Pblx8) {
    Value = Value | DMA_BUS_MODE_PBL;
  }

  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_CONTROL(Channel), Value);
}

VOID
DwMac4DmaInitRxChan (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         Channel
  )
{
  UINT32           Value;
  UINT32           RxPbl;

  RxPbl = 32;

  Value = DwMac4MmioRead (DwMac4Driver, DMA_CHAN_RX_CONTROL(Channel));
  Value = Value | (RxPbl << DMA_BUS_MODE_RPBL_SHIFT);

  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_RX_CONTROL(Channel), Value);
}

VOID
DwMac4DmaInitTxChan (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         Channel
  )
{
  UINT32          Value;
  UINT32          TxPbl;

  TxPbl = 32;

  Value = DwMac4MmioRead (DwMac4Driver, DMA_CHAN_TX_CONTROL(Channel));
  Value = Value | (TxPbl << DMA_BUS_MODE_PBL_SHIFT);

  //
  // Enable OSP to get best performance
  //
  Value |= DMA_CONTROL_OSP;

  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_TX_CONTROL(Channel), Value);
}

VOID
DwMac4SetupRxDescriptor (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32           Index;
  DMA_DESCRIPTOR   *RxDescriptor;
  UINT32           Channel;

  Channel = 0;

  for (Index = 0; Index < RX_DESC_NUM; Index++) {
    RxDescriptor = (VOID *)(UINTN)DwMac4Driver->MacDriver.RxDescRingMap[Index].PhysAddress;

    RxDescriptor->Des0 = LOWER_32_BITS(DwMac4Driver->MacDriver.RxBufNum[Index].PhysAddress);
    RxDescriptor->Des1 = UPPER_32_BITS(DwMac4Driver->MacDriver.RxBufNum[Index].PhysAddress);
    RxDescriptor->Des2 = 0;
    RxDescriptor->Des3 = (UINT32)(RDES3_OWN | RDES3_BUFFER1_VALID_ADDR);

    DEBUG ((
      DEBUG_VERBOSE,
      "%a():\n"
      "\tRxDescriptor[%d] Addr:0x%lx\n"
      "\tDes0=0x%x\tDesc1=0x%x\n"
      "\tDes1=0x%x\tDesc3=0x%x\n"
      "\tRxBuffer Addr=0x%lx\n",
      __func__,
      Index,
      RxDescriptor,
      RxDescriptor->Des0,
      RxDescriptor->Des1,
      RxDescriptor->Des2,
      RxDescriptor->Des3,
      DwMac4Driver->MacDriver.RxBuffer + Index * ETH_BUFFER_SIZE
    ));
  }

  //
  // Write the address of Rx descriptor list
  //
  DwMac4MmioWrite (
    DwMac4Driver,
    DMA_CHAN_RX_BASE_ADDR_HI(Channel),
    UPPER_32_BITS((UINTN)DwMac4Driver->MacDriver.RxDescRingMap[0].PhysAddress)
  );

  DwMac4MmioWrite (
    DwMac4Driver,
    DMA_CHAN_RX_BASE_ADDR(Channel),
    LOWER_32_BITS((UINTN)DwMac4Driver->MacDriver.RxDescRingMap[0].PhysAddress)
  );

  //
  // Initialize the descriptor number
  //
  DwMac4Driver->MacDriver.RxCurrentDescriptorNum = 0;
  DwMac4Driver->MacDriver.RxNextDescriptorNum = 0;
}

VOID
DwMac4SetupTxDescriptor (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32          Index;
  DMA_DESCRIPTOR  *TxDescriptor;
  UINT32          Channel;
  UINTN           TxBuffer;

  Channel = 0;

  for (Index = 0; Index < TX_DESC_NUM; Index++) {
    TxBuffer = (UINTN)DwMac4Driver->MacDriver.TxBuffer + Index * ETH_BUFFER_SIZE;
    TxDescriptor = (VOID *)(UINTN)DwMac4Driver->MacDriver.TxDescRingMap[Index].PhysAddress;
    TxDescriptor->Des0 = LOWER_32_BITS(TxBuffer);
    TxDescriptor->Des1 = UPPER_32_BITS(TxBuffer);
    TxDescriptor->Des2 = 0;
    TxDescriptor->Des3 = 0;

    DEBUG ((
      DEBUG_VERBOSE,
      "%a():\n"
      "\tTxDescriptor[%d] Addr:0x%lx\n"
      "\tDes0=0x%x\tDesc1=0x%x\n"
      "\tDes1=0x%x\tDesc3=0x%x\n"
      "\tTxBuffer Addr=0x%lx\n",
      __func__,
      Index,
      TxDescriptor,
      TxDescriptor->Des0,
      TxDescriptor->Des1,
      TxDescriptor->Des2,
      TxDescriptor->Des3,
      DwMac4Driver->MacDriver.TxBuffer + Index * ETH_BUFFER_SIZE
    ));
  }

  //
  // Write the address of tx descriptor list
  //
  DwMac4MmioWrite (
    DwMac4Driver,
    DMA_CHAN_TX_BASE_ADDR_HI(Channel),
    UPPER_32_BITS((UINTN)DwMac4Driver->MacDriver.TxDescRingMap[0].PhysAddress)
  );

  DwMac4MmioWrite (
    DwMac4Driver,
    DMA_CHAN_TX_BASE_ADDR(Channel),
    LOWER_32_BITS((UINTN)DwMac4Driver->MacDriver.TxDescRingMap[0].PhysAddress)
  );

  //
  // Initialize the descriptor number
  //
  DwMac4Driver->MacDriver.TxCurrentDescriptorNum = 0;
  DwMac4Driver->MacDriver.TxNextDescriptorNum = 0;
}

VOID
DwMac4DmaRxChanOpMode (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         Channel,
  IN  UINT32                        FifoSize,
  IN  UINT32                        Mode,
  IN  UINT8                         Qmode
  )
{
  UINT32 Rqs;
  UINT32 MtlRxOp;
  UINT32 Rfd;
  UINT32 Rfa;

  Rqs = FifoSize / 256 - 1;
  MtlRxOp = DwMac4MmioRead (DwMac4Driver, MTL_CHAN_RX_OP_MODE(Channel));

  if (Mode == SF_DMA_MODE) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a(): enable RX store and forward mode\n",
      __func__
      ));
    MtlRxOp |= MTL_OP_MODE_RSF;
  } else {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a(): disable RX SF mode (threshold %d)\n",
      __func__,
      Mode
      ));

    MtlRxOp &= ~MTL_OP_MODE_RSF;
    MtlRxOp &= MTL_OP_MODE_RTC_MASK;

    if (Mode <= 32) {
      MtlRxOp |= MTL_OP_MODE_RTC_32;
    } else if (Mode <= 64) {
      MtlRxOp |= MTL_OP_MODE_RTC_64;
    } else if (Mode <= 96) {
      MtlRxOp |= MTL_OP_MODE_RTC_96;
    } else {
      MtlRxOp |= MTL_OP_MODE_RTC_128;
    }
  }

  MtlRxOp &= ~MTL_OP_MODE_RQS_MASK;
  MtlRxOp |= Rqs << MTL_OP_MODE_RQS_SHIFT;

  //
  // Enable flow control only if each channel gets 4 KiB or more FIFO and
  // only if channel is not an AVB channel.
  //
  if ((FifoSize >= 4096) && (Qmode != MTL_QUEUE_AVB)) {
    MtlRxOp |= MTL_OP_MODE_EHFC;

    //
    // Set Threshold for Activating Flow Control to min 2 frames,
    // i.e. 1500 * 2 = 3000 bytes.
    //
    // Set Threshold for Deactivating Flow Control to min 1 frame,
    // i.e. 1500 bytes.
    //
    switch (FifoSize) {
    case 4096:
      //
      // This violates the above formula because of FIFO size
      // limit therefore overflow may occur in spite of this.
      //
      Rfd = 0x03; /* Full-2.5K */
      Rfa = 0x01; /* Full-1.5K */
      break;

    default:
      Rfd = 0x07; /* Full-4.5K */
      Rfa = 0x04; /* Full-3K */
      break;
    }

    MtlRxOp &= ~MTL_OP_MODE_RFD_MASK;
    MtlRxOp |= Rfd << MTL_OP_MODE_RFD_SHIFT;

    MtlRxOp &= ~MTL_OP_MODE_RFA_MASK;
    MtlRxOp |= Rfa << MTL_OP_MODE_RFA_SHIFT;
  }

  DwMac4MmioWrite (DwMac4Driver, MTL_CHAN_RX_OP_MODE(Channel), MtlRxOp);
  DEBUG((DEBUG_VERBOSE, "MTL_CHAN_RX_OP_MODE(%d)=0x%x\n", Channel, DwMac4MmioRead (DwMac4Driver, MTL_CHAN_RX_OP_MODE(Channel))));
}

VOID
DwMac4DmaTxChanOpMode (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         Channel,
  IN  UINT32                        FifoSize,
  IN  UINT32                        Mode,
  IN  UINT8                         Qmode
  )
{
 UINT32 MtlTxOp;
 UINT32 Tqs;

 MtlTxOp = DwMac4MmioRead (DwMac4Driver, MTL_CHAN_TX_OP_MODE(Channel));
 Tqs = FifoSize / 256 - 1;

 if (Mode == SF_DMA_MODE) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a(): enable TX store and forward mode\n",
      __func__
      ));
    //
    // Transmit COE type 2 cannot be done in cut-through mode.
    //
    MtlTxOp |= MTL_OP_MODE_TSF;
  } else {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a(): disabling TX SF (threshold %d)\n",
      __func__,
      Mode
      ));
    MtlTxOp &= ~MTL_OP_MODE_TSF;
    MtlTxOp &= MTL_OP_MODE_TTC_MASK;

    //
    // Set the transmit threshold
    //
    if (Mode <= 32) {
      MtlTxOp |= MTL_OP_MODE_TTC_32;
    } else if (Mode <= 64) {
      MtlTxOp |= MTL_OP_MODE_TTC_64;
    } else if (Mode <= 96) {
      MtlTxOp |= MTL_OP_MODE_TTC_96;
    } else if (Mode <= 128) {
      MtlTxOp |= MTL_OP_MODE_TTC_128;
    } else if (Mode <= 192) {
      MtlTxOp |= MTL_OP_MODE_TTC_192;
    } else if (Mode <= 256) {
      MtlTxOp |= MTL_OP_MODE_TTC_256;
    } else if (Mode <= 384) {
      MtlTxOp |= MTL_OP_MODE_TTC_384;
    } else {
      MtlTxOp |= MTL_OP_MODE_TTC_512;
    }
  }

  //
  // For an IP with DWC_EQOS_NUM_TXQ == 1, the fields TXQEN and TQS are RO
  // with reset values: TXQEN on, TQS == DWC_EQOS_TXFIFO_SIZE.
  // For an IP with DWC_EQOS_NUM_TXQ > 1, the fields TXQEN and TQS are R/W
  // with reset values: TXQEN off, TQS 256 bytes.
  //
  // TXQEN must be written for multi-channel operation and TQS must
  // reflect the available fifo size per queue (total fifo size / number
  // of enabled queues).
  //
  MtlTxOp &= ~MTL_OP_MODE_TXQEN_MASK;
  if (Qmode != MTL_QUEUE_AVB) {
    MtlTxOp |= MTL_OP_MODE_TXQEN;
  } else {
    MtlTxOp |= MTL_OP_MODE_TXQEN_AV;
  }

  MtlTxOp &= ~MTL_OP_MODE_TQS_MASK;
  MtlTxOp |= Tqs << MTL_OP_MODE_TQS_SHIFT;

  DwMac4MmioWrite(DwMac4Driver, MTL_CHAN_TX_OP_MODE(Channel), MtlTxOp);
  DEBUG((DEBUG_VERBOSE, "MTL_CHAN_TX_OP_MODE(%d)=0x%x\n", Channel, DwMac4MmioRead (DwMac4Driver, MTL_CHAN_TX_OP_MODE(Channel))));
}

VOID
DwMac4ProgMtlRxAlgorithms (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        RxAlg
  )
{
  UINT32 Value;

  Value = DwMac4MmioRead (DwMac4Driver, MTL_OPERATION_MODE);
  Value &= ~MTL_OPERATION_RAA;

  switch (RxAlg) {
  case MTL_RX_ALGORITHM_SP:
    Value |= MTL_OPERATION_RAA_SP;
    break;
  case MTL_RX_ALGORITHM_WSP:
    Value |= MTL_OPERATION_RAA_WSP;
    break;
  default:
    break;
  }

  DwMac4MmioWrite (DwMac4Driver, MTL_OPERATION_MODE, Value);
}

VOID
DwMac4ProgMtlTxAlgorithms (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        TxAlg
  )
{
  UINT32 Value;

  Value = DwMac4MmioRead (DwMac4Driver, MTL_OPERATION_MODE);
  Value &= ~MTL_OPERATION_SCHALG_MASK;

  switch (TxAlg) {
  case MTL_TX_ALGORITHM_WRR:
    Value |= MTL_OPERATION_SCHALG_WRR;
    break;
  case MTL_TX_ALGORITHM_WFQ:
    Value |= MTL_OPERATION_SCHALG_WFQ;
    break;
  case MTL_TX_ALGORITHM_DWRR:
    Value |= MTL_OPERATION_SCHALG_DWRR;
    break;
  case MTL_TX_ALGORITHM_SP:
    Value |= MTL_OPERATION_SCHALG_SP;
    break;
  default:
    break;
  }

  DwMac4MmioWrite (DwMac4Driver, MTL_OPERATION_MODE, Value);
}

VOID
DwMac4SetTxRingLen (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        Length,
  IN  UINT32                        Channel
  )
{
  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_TX_RING_LEN(Channel), Length);
}

VOID
DwMac4SetRxRingLen (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        Length,
  IN  UINT32                        Channel
  )
{
  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_RX_RING_LEN(Channel), Length);
}

VOID
EFIAPI
DwMac4CoreInit (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32 Value;

  Value = DwMac4MmioRead (DwMac4Driver, GMAC_CONFIG);
  Value |= GMAC_CORE_INIT;

  DwMac4MmioWrite (DwMac4Driver, GMAC_CONFIG, Value);
}

VOID
DwMac4EnableDmaInterrupt (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINTN                         Channel,
  IN  BOOLEAN                       Rx,
  IN  BOOLEAN                       Tx
  )
{
  UINT32 Value;

  Value = DwMac4MmioRead (DwMac4Driver, DMA_CHAN_INTR_ENA(Channel));

  if (Rx) {
    Value |= DMA_CHAN_INTR_DEFAULT_RX;
  }

  if (Tx) {
    Value |= DMA_CHAN_INTR_DEFAULT_TX;
  }

  DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_INTR_ENA(Channel), Value);
}

EFI_STATUS
EFIAPI
StmmacAllocDesc (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  EFI_STATUS  Status;
  UINTN       DescriptorSize;
  UINTN       BufferSize;
  UINT32      Index;

  //
  // Size for descriptor
  //
  DescriptorSize = sizeof (DMA_DESCRIPTOR);

  //
  // Size for transmit and receive buffer
  //
  BufferSize = ETH_BUFFER_SIZE;
  DEBUG ((
    DEBUG_VERBOSE,
    "%a[%d] DescriptorSize=0x%lx\tRxBufferSize=0x%lx\n",
    __func__, __LINE__, DescriptorSize, BufferSize
  ));

  Status = DmaAllocateBuffer (
    EfiBootServicesData,
    EFI_SIZE_TO_PAGES (BufferSize * TX_DESC_NUM),
    (VOID *)&DwMac4Driver->MacDriver.TxBuffer
  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a() for Tx Buffer: %r\n", __func__, Status));
    return Status;
  }

  Status = DmaAllocateBuffer (
    EfiBootServicesData,
    EFI_SIZE_TO_PAGES (BufferSize * RX_DESC_NUM),
    (VOID *)&DwMac4Driver->MacDriver.RxBuffer
  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a() for Rx Buffer: %r\n", __func__, Status));
    return Status;
  }

  //
  // DMA TxDescRing allocate buffer
  //
  Status = DmaAllocateBuffer (
    EfiBootServicesData,
    EFI_SIZE_TO_PAGES (DescriptorSize * TX_DESC_NUM),
    (VOID *)&DwMac4Driver->MacDriver.TxDescRing
  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a() for TxDescRing: %r\n", __func__, Status));
    return Status;
  }

  ZeroMem (DwMac4Driver->MacDriver.TxDescRing, DescriptorSize * TX_DESC_NUM);

  //
  // DMA RxDescRing allocte buffer
  //
  Status = DmaAllocateBuffer (
    EfiBootServicesData,
    EFI_SIZE_TO_PAGES (DescriptorSize * RX_DESC_NUM),
    (VOID *)&DwMac4Driver->MacDriver.RxDescRing
  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a() for RxDescRing: %r\n", __func__, Status));
    return Status;
  }

  ZeroMem (DwMac4Driver->MacDriver.RxDescRing, DescriptorSize * RX_DESC_NUM);

  for (Index = 0; Index < TX_DESC_NUM; Index++) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a[%d] TxDesc[%d], BaseAddr=0x%lx, Size=0x%lx\n\n",
      __func__, __LINE__, Index,
      (EFI_PHYSICAL_ADDRESS)(DwMac4Driver->MacDriver.TxDescRing + Index),
      DescriptorSize
    ));

    Status = DmaMap (
      MapOperationBusMasterCommonBuffer,
      DwMac4Driver->MacDriver.TxDescRing + Index,
      &DescriptorSize,
      &DwMac4Driver->MacDriver.TxDescRingMap[Index].PhysAddress,
      &DwMac4Driver->MacDriver.TxDescRingMap[Index].Mapping
    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a() for TxDescRing: %r\n", __func__, Status));
      return Status;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "%a[%d] RxDesc[%d] BaseAddr=0x%lx, Size=0x%lx\n\n",
      __func__, __LINE__, Index,
      (EFI_PHYSICAL_ADDRESS)(DwMac4Driver->MacDriver.RxDescRing + Index),
      DescriptorSize
    ));

    Status = DmaMap (
      MapOperationBusMasterCommonBuffer,
      DwMac4Driver->MacDriver.RxDescRing + Index,
      &DescriptorSize,
      &DwMac4Driver->MacDriver.RxDescRingMap[Index].PhysAddress,
      &DwMac4Driver->MacDriver.RxDescRingMap[Index].Mapping
    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a() for RxDescRing: %r\n", __func__, Status));
      return Status;
    }

    //
    // DMA mapping for receive buffer
    //
    Status = DmaMap (
      MapOperationBusMasterWrite,
      DwMac4Driver->MacDriver.RxBuffer + Index * BufferSize,
      &BufferSize,
      &DwMac4Driver->MacDriver.RxBufNum[Index].PhysAddress,
      &DwMac4Driver->MacDriver.RxBufNum[Index].Mapping
    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a() for Rxbuffer: %r\n", __func__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/*
 * DMA init.
 * Description:
 * It inits the DMA invoking the specific MAC/GMAC callback.
 * Some DMA parameters can be passed from the platform;
 * in case of these are not passed a default is kept for the MAC or GMAC.
 */
EFI_STATUS
EFIAPI
StmmacInitDmaEngine (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32          DmaCsrCh;
  UINT32          Channel;
  EFI_STATUS      Status;
  UINT32          Value;

  DEBUG ((
    DEBUG_INFO,
    "%a(): MacBaseAddress=0x%lx\r\n",
    __func__,
    DwMac4Driver->RegBase
    ));

  DmaCsrCh = MAX (RxChannelsCount, TxChannelsCount);

  //
  // Step 1. Provide a software reset. This resets all of the MAC internal
  // registers and logic (bit-0 of DMA_Mode).
  // Step 2. Wait for the completion of the reset process (poll bit 0 of the
  // DMA_Mode, which is only cleared after the reset operation is completed).
  //
  Status = DwMac4DmaReset (DwMac4Driver);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Reset the dma failed!\n",
      __func__
      ));
    return Status;
  }

  //
  // Step 3. Program the following fields to initialize the DMA_SysBus_Mode
  // register:
  // (1) AAL.
  // (2) Fixed burst or undefined burst.
  // (3) Burst mode values in case of AHB bus interface, OSR_LMT in case of
  //     AXI bus interface.
  // (4) If fixed length value is enabled, select the maximum burst length
  //     possible on the AXI Bus (bits [7:1]).
  //
  DwMac4DmaInit (DwMac4Driver);

  DwMac4DmaAxi (DwMac4Driver);

  //
  // Step 4. Create a descriptor list for transmit and receive.
  // In addition, ensure that the descriptors are owned by DMA
  // (set bit 31 of descriptor TDES3/RDES3).
  //
  // DMA CSR Channel configuration
  //
  DwMac4SetupTxDescriptor (DwMac4Driver);
  DwMac4SetupRxDescriptor (DwMac4Driver);

  //
  // Step 5. Program the Transmit and Receive Ring length registers
  // (DMA_CH(#i)_TxDesc_Ring_Length (for i = 0; i <= DWC_EQOS_NUM_DMA_TX_CH-1)
  // and DMA_CH(#i)_RxDesc_Ring_Length (for i = 0; i <= DWC_EQOS_NUM_DMA_RX_CH-1)).
  // The ring length programmed must be at least 4.
  //
  for (Channel = 0; Channel < TxChannelsCount; Channel++) {
    DwMac4SetTxRingLen (DwMac4Driver, TX_DESC_NUM - 1, Channel);
  }

  for (Channel = 0; Channel < RxChannelsCount; Channel++) {
    DwMac4SetRxRingLen (DwMac4Driver, RX_DESC_NUM - 1, Channel);
  }

  //
  // Step 6. Initialize receive and transmit descriptor list address with
  // the base address of the transmit and receive descriptor
  // (DMA_CH(#i)_TxDesc_List_Address (for i = 0; i <= DWC_EQOS_NUM_DMA_TX_CH-1),
  // DMA_CH(#i)_RxDesc_List_Address (for i = 0; i <= DWC_EQOS_NUM_DMA_RX_CH-1)).
  // Also, program transmit and receive tail pointer registers indicating to
  // the DMA about the available descriptors
  // (DMA_CH(#i)_TxDesc_Tail_Pointer (for i = 0; i <= DWC_EQOS_NUM_DMA_TX_CH-1)
  // and DMA_CH(#i)_RxDesc_Tail_Pointer (for i = 0; i <= DWC_EQOS_NUM_DMA_RX_CH-1)).
  //
  // Step 7. Program the settings of the following registers for the parameters
  // like maximum burst-length (PBL) initiated by DMA, descriptor skip lengths,
  // OSP in case of Tx DMA, RBSZ in case of Rx DMA, and so on.
  //
  for (Channel = 0; Channel < DmaCsrCh; Channel++) {
    DwMac4InitChannel (DwMac4Driver, Channel);
  }

  for (Channel = 0; Channel < RxChannelsCount; Channel++) {
    DwMac4DmaInitRxChan (DwMac4Driver, Channel);
    //
    // RX buffer size. Must be a multiple of bus width
    //
    Value = DwMac4MmioRead (DwMac4Driver, DMA_CHAN_RX_CONTROL(Channel));
    Value &= ~DMA_RBSZ_MASK;
    Value |= (RX_MAX_PACKET << DMA_RBSZ_SHIFT) & DMA_RBSZ_MASK;
    DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_RX_CONTROL(Channel), Value);
    DEBUG((DEBUG_VERBOSE, "%a(): DMA_CHAN_RX_CONTROL(%d)=0x%x\n", __func__, Channel, DwMac4MmioRead (DwMac4Driver, DMA_CHAN_RX_CONTROL(Channel))));
  }

  //
  // DMA TX Channel Configuration
  //
  for (Channel = 0; Channel < TxChannelsCount; Channel++) {
    DwMac4DmaInitTxChan (DwMac4Driver, Channel);
  }

  //
  // Step 8. Enable the interrupts by programming the
  // DMA_CH(#i)_Interrupt_Enable (for i = 0; i <= DWC_EQOS_NUM_DMA_TX_CH-1) register.
  //
  DwMac4CoreInit (DwMac4Driver);
  //
  // Step 9. Start the Receive and Transmit DMAs by setting SR (bit 0) of the
  // DMA_CH(#i)_RX_Control (for i = 0; i <= DWC_EQOS_NUM_DMA_RX_CH-1) and
  // ST (bit 0) of the DMA_CH(#i)_TX_Control (for i = 0; i <= DWC_EQOS_NUM_DMA_TX_CH-1) register.
  //
  StmmacStartAllDma (DwMac4Driver);

  //
  // Step 10. Repeat steps 4 to 9 for all the Tx DMA and Rx DMA channels
  // selected in the hardware.
  //

  return EFI_SUCCESS;
}

VOID
DwMac4SetMtlTxQueueWeight (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        Weight,
  IN  UINT32                        Queue
  )
{
  UINT32 Value;
  Value = DwMac4MmioRead (DwMac4Driver, MTL_TXQ_WEIGHT_BASE_ADDR + (Queue * MTL_TXQ_WEIGHT_BASE_OFFSET));

  Value &= ~MTL_TXQ_WEIGHT_ISCQW_MASK;
  Value |= Weight & MTL_TXQ_WEIGHT_ISCQW_MASK;

  DwMac4MmioWrite (DwMac4Driver, MTL_TXQ_WEIGHT_BASE_ADDR + (Queue * MTL_TXQ_WEIGHT_BASE_OFFSET), Value);
}

VOID
DwMac4MapMtlDma (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        Queue,
  IN  UINT32                        Channel
  )
{
  UINT32 Value;

  if (Queue < 4) {
    Value = DwMac4MmioRead (DwMac4Driver, MTL_RXQ_DMA_MAP0);
    Value &= ~MTL_RXQ_DMA_QXMDMACH_MASK(Queue);
    Value |= MTL_RXQ_DMA_QXMDMACH(Channel, Queue);
    DwMac4MmioWrite (DwMac4Driver, MTL_RXQ_DMA_MAP0, Value);
  } else {
    Value = DwMac4MmioRead (DwMac4Driver, MTL_RXQ_DMA_MAP1);
    Value &= ~MTL_RXQ_DMA_QXMDMACH_MASK(Queue - 4);
    Value |= MTL_RXQ_DMA_QXMDMACH(Channel, Queue - 4);
    DwMac4MmioWrite (DwMac4Driver, MTL_RXQ_DMA_MAP1, Value);
  }
}

VOID
DwMac4RxQueueEnable (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT8                         Mode,
  IN  UINT32                        Queue
  )
{
  UINT32 Value;

  Value = DwMac4MmioRead (DwMac4Driver, GMAC_RXQ_CTRL0);

  Value &= GMAC_RX_QUEUE_CLEAR(Queue);
  if (Mode == MTL_QUEUE_AVB) {
    Value |= GMAC_RX_AV_QUEUE_ENABLE(Queue);
  } else if (Mode == MTL_QUEUE_DCB) {
    Value |= GMAC_RX_DCB_QUEUE_ENABLE(Queue);
  }

  DwMac4MmioWrite (DwMac4Driver, GMAC_RXQ_CTRL0, Value);
}

/**
 *  stmmac_mtl_configuration - Configure MTL
 *  @priv: driver private structure
 *  Description: It is used for configurring MTL
 */
VOID
StmmacMtlConfiguration (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32 HwCap;
  UINT32 TxFifoSize;
  UINT32 RxFifoSize;
  UINT32 TxAlgorithm;
  UINT32 RxAlgorithm;
  UINT32 Queue;
  UINT32 Weight;
  UINT32 Channel;
  UINT8  Mode;
  UINT32 Value;

  TxAlgorithm = MTL_TX_ALGORITHM_SP;
  RxAlgorithm = MTL_RX_ALGORITHM_WSP;
  HwCap = DwMac4MmioRead (DwMac4Driver, GMAC_HW_FEATURE1);

  //
  // RX and TX FIFO sizes are encoded as log2(n / 128). Undo that by
  // shifting and store the sizes in bytes.
  //
  TxFifoSize = 128 << ((HwCap & GMAC_HW_TXFIFOSIZE) >> 6);
  RxFifoSize = 128 << ((HwCap & GMAC_HW_RXFIFOSIZE) >> 0);

  //
  // Adjust for real per queue fifo size
  //
  TxFifoSize /= TxQueuesToUse;
  RxFifoSize /= RxQueuesToUse;

  if (TxQueuesToUse > 1) {
    for (Queue = 0; Queue < TxQueuesToUse; Queue++) {
      Weight = 0x10 + Queue;
      DwMac4SetMtlTxQueueWeight (DwMac4Driver, Weight, Queue);
    }
  }

  //
  // Configure MTL RX algorithms
  //
  if (RxQueuesToUse > 1) {
    DwMac4ProgMtlRxAlgorithms (DwMac4Driver, RxAlgorithm);
  }

  //
  // Configure MTL TX algorithms
  //
  if (TxQueuesToUse > 1) {
    DwMac4ProgMtlTxAlgorithms (DwMac4Driver, TxAlgorithm);
  }

  //
  // Map RX MTL to DMA channels
  //
  for (Queue = 0; Queue < RxQueuesToUse; Queue++) {
    Channel = Queue;
    DwMac4MapMtlDma (DwMac4Driver, Queue, Channel);
  }

  //
  // Enable MAC RX Queues
  //
  for (Queue = 0; Queue < RxQueuesToUse; Queue++) {
    Mode = MTL_QUEUE_DCB;
    DwMac4RxQueueEnable (DwMac4Driver, Mode, Queue);
  }

  //
  // Step 3. Program the following fields to initialize the mode of operation
  //         in the MTL_TxQ0_Operation_Mode register.
  // a. Transmit Store And Forward (TSF) or Transmit Threshold Control (TTC) in case of threshold mode
  // b. Transmit Queue Enable (TXQEN) to value 2‘b10 to enable Transmit Queue0
  // c. Transmit Queue Size (TQS)
  //
  for (Channel = 0; Channel < TxChannelsCount; Channel++) {
    DwMac4DmaTxChanOpMode (DwMac4Driver, Channel, TxFifoSize, SF_DMA_MODE, MTL_QUEUE_DCB);
  }

  //
  // Step 4. Program the following fields to initialize the mode of operation
  //         in the MTL_RxQ0_Operation_Mode register:
  // a. Receive Store and Forward (RSF) or RTC in case of Threshold mode
  // b. Flow Control Activation and De-activation thresholds for MTL Receive FIFO (RFA and RFD)
  // c. Error Packet and undersized good Packet forwarding enable (FEP and FUP)
  // d. Receive Queue Size (RQS)
  //
  for (Channel = 0; Channel < RxChannelsCount; Channel++) {
    DwMac4DmaRxChanOpMode (DwMac4Driver, Channel, RxFifoSize, SF_DMA_MODE, MTL_QUEUE_DCB);
  }

  //
  // Set TX priorities
  //
  Value = DwMac4MmioRead (DwMac4Driver, GMAC_TXQ_PRTY_MAP0);
  Value &= ~GMAC_TXQCTRL_PSTQX_MASK(0);
  Value |= (0 << GMAC_TXQCTRL_PSTQX_SHIFT(0)) & GMAC_TXQCTRL_PSTQX_MASK(0);
  DwMac4MmioWrite (DwMac4Driver, GMAC_TXQ_PRTY_MAP0, Value);

  //
  // Set RX priorities
  //
  Value = DwMac4MmioRead (DwMac4Driver, GMAC_RXQ_CTRL2);
  Value &= ~GMAC_RXQCTRL_PSRQX_MASK(0);
  Value |= (0 << GMAC_RXQCTRL_PSRQX_SHIFT(0)) & GMAC_RXQCTRL_PSRQX_MASK(0);
  DwMac4MmioWrite (DwMac4Driver, GMAC_RXQ_CTRL2, Value);

  //
  // Set RX routing: Multicast and Broadcast Queue Enable
  //
  Value = DwMac4MmioRead (DwMac4Driver, GMAC_RXQ_CTRL1);
  Value &= ~GMAC_RXQCTRL_MCBCQEN;
  Value |= 0x1 << GMAC_RXQCTRL_MCBCQEN_SHIFT;
  DwMac4MmioWrite (DwMac4Driver, GMAC_RXQ_CTRL1, Value);

  //
  // Enable promiscuous mode
  //
  Value = DwMac4MmioRead (DwMac4Driver, GMAC_PACKET_FILTER);
  Value |= GMAC_PACKET_FILTER_PR;
  DwMac4MmioWrite (DwMac4Driver, GMAC_PACKET_FILTER, Value);
}

VOID
StmmacMacFlowControl (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver,
  IN  UINT32                        Duplex,
  IN  UINT32                        FlowCtrl
  )
{
  UINT32 Flow;
  UINT32 Queue;
  UINT32 PauseTime;

  Queue = 0;
  PauseTime = PAUSE_TIME;

  Flow = DwMac4MmioRead (DwMac4Driver, GMAC_RX_FLOW_CTRL);
  DEBUG ((DEBUG_VERBOSE, "GMAC Flow-Control:\n"));
  if (FlowCtrl & FLOW_RX) {
    DEBUG ((DEBUG_VERBOSE, "\tReceive Flow-Control ON\n"));
    Flow |= GMAC_RX_FLOW_CTRL_RFE;
  } else {
    DEBUG ((DEBUG_VERBOSE, "\tReceive Flow-Control OFF\n"));
    Flow &= ~GMAC_RX_FLOW_CTRL_RFE;
  }

  DwMac4MmioWrite (DwMac4Driver, GMAC_RX_FLOW_CTRL, Flow);

  if (FlowCtrl & FLOW_TX) {
    DEBUG ((DEBUG_VERBOSE, "\tTransmit Flow-Control ON\n"));

    if (Duplex) {
      DEBUG ((DEBUG_VERBOSE, "\tduplex mode: PAUSE %d\n", PauseTime));
    }

    for (Queue = 0; Queue < TxQueuesToUse; Queue++) {
      Flow = DwMac4MmioRead (DwMac4Driver, GMAC_QX_TX_FLOW_CTRL(Queue));
      Flow |= GMAC_TX_FLOW_CTRL_TFE;

      if (Duplex) {
        Flow |= (PauseTime << GMAC_TX_FLOW_CTRL_PT_SHIFT);
      }

      DwMac4MmioWrite (DwMac4Driver, GMAC_QX_TX_FLOW_CTRL(Queue), Flow);
    }
  } else {
    for (Queue = 0; Queue < TxQueuesToUse; Queue++) {
      DwMac4MmioWrite (DwMac4Driver, GMAC_QX_TX_FLOW_CTRL(Queue), 0);
    }
  }
}

EFI_STATUS
EFIAPI
StmmacSetFilters (
  IN  UINT32                        ReceiveFilterSetting,
  IN  BOOLEAN                       Reset,
  IN  UINTN                         NumMfilter          OPTIONAL,
  IN  EFI_MAC_ADDRESS               *Mfilter            OPTIONAL,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32  MacFilter;
  UINT32  Crc;
  UINT32  Index;
  UINT32  HashReg;
  UINT32  HashBit;
  UINT32  Register;
  UINT32  Value;

  //
  // If reset then clear the filter registers
  //
  if (Reset) {
    for (Index = 0; Index < NumMfilter; Index++) {
      DwMac4MmioWrite (DwMac4Driver, GMAC_HASH_TAB(Index), 0x0);
    }
  }

  //
  // Set MacFilter to the reset value of the GMAC_PACKET_FILTER register.
  //
  MacFilter = DwMac4MmioRead (DwMac4Driver, GMAC_PACKET_FILTER);
  MacFilter &= ~GMAC_PACKET_FILTER_HMC;
  MacFilter &= ~GMAC_PACKET_FILTER_HPF;
  MacFilter &= ~GMAC_PACKET_FILTER_PCF;
  MacFilter &= ~GMAC_PACKET_FILTER_PM;
  MacFilter &= ~GMAC_PACKET_FILTER_PR;
  MacFilter &= ~GMAC_PACKET_FILTER_RA;

  if (ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) {
    MacFilter |=  GMAC_PACKET_FILTER_HMC;

    //
    // Set the hash tables.
    //
    if ((NumMfilter > 0) && (!Reset)) {
      //
      // Go through each filter address and set appropriate bits on hash table.
      //
      for (Index = 0; Index < NumMfilter; Index++) {
        //
        // Generate a 32-bit CRC.
        //
        Crc = GenEtherCrc32 (&Mfilter[Index], NET_ETHER_ADDR_LEN);
        //
        // Reserve CRC + take upper 8 bit = take lower 8 bit and reverse it.
        //
        Value = BitReverse (Crc & 0xff);
        //
        // The most significant bits determines the register to be used (Hash Table Register X),
        // and the least significant five bits determine the bit within the register.
        // For example, a hash value of 8b'10111111 selects Bit 31 of the Hash Table Register 5.
        //
        HashReg = (Value >> 5);
        HashBit = (Value & 0x1f);

        Register = DwMac4MmioRead (DwMac4Driver, GMAC_HASH_TAB(HashReg));
        //
        // Set 1 to HashBit of HashReg.
        // For example, set 1 to bit 31 to Reg 5 as in above example.
        //
        Register |= (1 << HashBit);
        DwMac4MmioWrite (DwMac4Driver, GMAC_HASH_TAB(HashReg), Register);
      }
    }
  }

  if ((ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST) == 0) {
    MacFilter |= GMAC_PACKET_FILTER_DBF;
  }

  if (ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) {
    MacFilter |= GMAC_PACKET_FILTER_PR;
  }

  if (ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) {
    MacFilter |= GMAC_PACKET_FILTER_PM;
  }

  //
  // Set MacFilter to GMAC_PACKET_FILTER register.
  //
  DwMac4MmioWrite (DwMac4Driver, GMAC_PACKET_FILTER, MacFilter);

  return EFI_SUCCESS;
}

/*
 * Create Ethernet CRC
 * INFO USED:
 * 1. http://en.wikipedia.org/wiki/Cyclic_redundancy_check
 * 2. http://en.wikipedia.org/wiki/Computation_of_CRC
 */
UINT32
EFIAPI
GenEtherCrc32 (
  IN  EFI_MAC_ADDRESS   *Mac,
  IN  UINT32            AddrLen
  )
{
  INT32   Iter;
  UINT32  Remainder;
  UINT8   *Ptr;

  Iter = 0;
  Remainder = 0xFFFFFFFF; // 0xFFFFFFFF is standard seed for Ethernet

  //
  // Convert Mac Address to array of bytes
  //
  Ptr = (UINT8 *)Mac;

  //
  // Generate the Crc bit-by-bit (LSB first).
  //
  while (AddrLen--) {
    Remainder ^= *Ptr++;
    for (Iter = 0; Iter < 8; Iter++) {
      //
      // Check if exponent is set.
      //
      if (Remainder & 1) {
        Remainder = (Remainder >> 1) ^ CRC_POLYNOMIAL;
      } else {
        Remainder = (Remainder >> 1) ^ 0;
      }
    }
  }

  return (~Remainder);
}

STATIC CONST UINT8 NibbleTab[] = {
    /* 0x0 0000 -> 0000 */  0x0,
    /* 0x1 0001 -> 1000 */  0x8,
    /* 0x2 0010 -> 0100 */  0x4,
    /* 0x3 0011 -> 1100 */  0xc,
    /* 0x4 0100 -> 0010 */  0x2,
    /* 0x5 0101 -> 1010 */  0xa,
    /* 0x6 0110 -> 0110 */  0x6,
    /* 0x7 0111 -> 1110 */  0xe,
    /* 0x8 1000 -> 0001 */  0x1,
    /* 0x9 1001 -> 1001 */  0x9,
    /* 0xa 1010 -> 0101 */  0x5,
    /* 0xb 1011 -> 1101 */  0xd,
    /* 0xc 1100 -> 0011 */  0x3,
    /* 0xd 1101 -> 1011 */  0xb,
    /* 0xe 1110 -> 0111 */  0x7,
    /* 0xf 1111 -> 1111 */  0xf
};

UINT8
EFIAPI
BitReverse (
  UINT8   Value
  )
{
  return (NibbleTab[Value & 0xf] << 4) | NibbleTab[Value >> 4];
}

/*
 * Get DMA Interrupt Stauts.
 * dwmac4_dma_interrupt
 */
VOID
EFIAPI
StmmacGetDmaStatus (
  OUT UINT32                        *IrqStat  OPTIONAL,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32  DmaStatus;
  UINT32  IntrEnable;
  UINT32  ErrorBit;
  UINT32  Mask;
  UINT32  Channel;

  ErrorBit = 0;
  Mask = 0;

  if (IrqStat != NULL) {
    *IrqStat = 0;
  }

  for (Channel = 0; Channel < TxChannelsCount; Channel++) {
    DmaStatus = DwMac4MmioRead (DwMac4Driver, DMA_CHAN_STATUS(Channel));
    IntrEnable = DwMac4MmioRead (DwMac4Driver, DMA_CHAN_INTR_ENA(Channel));

    DEBUG ((DEBUG_INFO, "%a() DMA_CHAN_STATUS(0)=0x%x\n", __func__, DmaStatus));
    DEBUG ((DEBUG_INFO, "%a() DMA_CHAN_INTR_ENA(0)=0x%x\n", __func__, IntrEnable));

    //
    // TX/RX NORMAL interrupts.
    //
    if (DmaStatus & DMA_CHAN_STATUS_NIS) {
      Mask |= DMA_CHAN_STATUS_NIS;
      //
      // Rx interrupt.
      //
      if (DmaStatus & DMA_CHAN_STATUS_RI) {
        DEBUG ((
          DEBUG_INFO,
          "%a(): Rx interrupt enabled\n",
          __func__
          ));
        if (IrqStat != NULL) {
          *IrqStat |= EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT;
          Mask |= DMA_CHAN_STATUS_RI;
        }
      }

      //
      // Tx interrupt.
      //
      if (DmaStatus & DMA_CHAN_STATUS_TI) {
        DEBUG ((
          DEBUG_INFO,
          "%a(): Tx interrupt enabled\n",
          __func__
          ));
        if (IrqStat != NULL) {
          *IrqStat |= EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
          Mask |= DMA_CHAN_STATUS_TI;
        }
      }

      //
      // Tx Buffer.
      //
      if (DmaStatus & DMA_CHAN_STATUS_TBU) {
        Mask |= DMA_CHAN_STATUS_TBU;
      }
      //
      // Early receive interrupt.
      //
      if (DmaStatus & DMA_CHAN_STATUS_ERI) {
        Mask |= DMA_CHAN_STATUS_ERI;
      }
    }

    //
    // ABNORMAL interrupts.
    //
    if (DmaStatus & DMA_CHAN_STATUS_AIS) {
      Mask |= DMA_CHAN_STATUS_AIS;
      //
      // Transmit process stopped.
      //
      if (DmaStatus & DMA_CHAN_STATUS_TPS) {
        DEBUG ((
          DEBUG_INFO,
          "%a(): Transmit process stopped\n",
          __func__
          ));
        Mask |= DMA_CHAN_STATUS_TPS;
      }

      //
      // Receive buffer unavailable.
      //
      if (DmaStatus & DMA_CHAN_STATUS_RBU) {
        Mask |= DMA_CHAN_STATUS_RBU;
      }

      //
      // Receive process stopped.
      //
      if (DmaStatus & DMA_CHAN_STATUS_RPS) {
        DEBUG ((
          DEBUG_INFO,
          "%a(): Receive process stop\n",
          __func__
          ));
        Mask |= DMA_CHAN_STATUS_RPS;
      }

      //
      // Receive watchdog timeout
      //
      if (DmaStatus & DMA_CHAN_STATUS_RWT) {
        DEBUG ((
          DEBUG_INFO,
          "%a(): Receive watchdog timeout\n",
          __func__
          ));
        Mask |= DMA_CHAN_STATUS_RWT;
      }

      //
      // Early transmit interrupt.
      //
      if (DmaStatus & DMA_CHAN_STATUS_ETI) {
        Mask |= DMA_CHAN_STATUS_ETI;
      }

      //
      // Fatal bus error.
      //
      if (DmaStatus & DMA_CHAN_STATUS_FBE) {
        DEBUG ((
          DEBUG_INFO,
          "%a(): Fatal bus error:\n",
          __func__
          ));
        Mask |= DMA_CHAN_STATUS_FBE;

        ErrorBit = DmaStatus & DMA_CHAN_STATUS_TEB >> DMA_CHAN_STATUS_TEB_SHIFT;
        switch (ErrorBit) {
        case DMA_TX_WRITE_DATA_BUFFER_ERROR:
          DEBUG ((
            DEBUG_INFO,
            "%a(): Tx DMA write buffer error\n",
            __func__
           ));
          break;
        case DMA_TX_WRITE_DESCRIPTOR_ERROR:
          DEBUG ((
            DEBUG_INFO,
            "%a(): Tx DMA write descriptor error\n",
            __func__
            ));
          break;
        case DMA_TX_READ_DATA_BUFFER_ERROR:
          DEBUG ((
            DEBUG_INFO,
            "%a(): Tx DMA read buffer error\n",
            __func__
            ));
          break;
        case DMA_TX_READ_DESCRIPTOR_ERROR:
          DEBUG ((
            DEBUG_INFO,
            "%a(): Tx DMA read descriptor error\n",
            __func__
            ));
          break;
        default:
          DEBUG ((
            DEBUG_INFO,
            "%a(): Undefined error\n",
            __func__
            ));
          break;
        }

        ErrorBit = DmaStatus & DMA_CHAN_STATUS_REB >> DMA_CHAN_STATUS_REB_SHIFT;
        switch (ErrorBit) {
        case DMA_RX_WRITE_DATA_BUFFER_ERROR:
          DEBUG ((
            DEBUG_INFO,
            "%a(): Rx DMA write buffer error\n",
            __func__
            ));
          break;
        case DMA_RX_WRITE_DESCRIPTOR_ERROR:
          DEBUG ((
            DEBUG_INFO,
            "%a(): Rx DMA write descriptor error\n",
            __func__
            ));
          break;
        case DMA_RX_READ_DATA_BUFFER_ERROR:
          DEBUG ((
            DEBUG_INFO,
            "%a(): Rx DMA read buffer error\n",
            __func__
            ));
          break;
        case DMA_RX_READ_DESCRIPTOR_ERROR:
          DEBUG ((
            DEBUG_INFO,
            "%a(): Rx DMA read descriptor error\n",
            __func__
            ));
          break;
        default:
          DEBUG ((
            DEBUG_INFO,
            "%a(): Undefined error\n",
            __func__
            ));
          break;
        }
      }
    }
    DwMac4MmioWrite (DwMac4Driver, DMA_CHAN_STATUS(Channel), Mask & IntrEnable);
  }
}

/*
 * MMC: MAC Management Counters
 * drivers/net/ethernet/stmicro/stmmac/mmc_core.c
 */
VOID
EFIAPI
StmmacGetStatistic (
  OUT EFI_NETWORK_STATISTICS        *Statistic,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  EFI_NETWORK_STATISTICS   *Stats;

  DEBUG ((
    DEBUG_INFO,
    "%a()\r\n",
    __func__
    ));

  //
  // Allocate Resources
  //
  Stats = AllocateZeroPool (sizeof (EFI_NETWORK_STATISTICS));
  if (Stats == NULL) {
    return;
  }

  Stats->RxTotalFrames     = DwMac4MmioRead (DwMac4Driver, MMC_RX_FRAMECOUNT_GB);
  Stats->RxUndersizeFrames = DwMac4MmioRead (DwMac4Driver, MMC_RX_UNDERSIZE_G);
  Stats->RxOversizeFrames  = DwMac4MmioRead (DwMac4Driver, MMC_RX_OVERSIZE_G);
  Stats->RxUnicastFrames   = DwMac4MmioRead (DwMac4Driver, MMC_RX_UNICAST_G);
  Stats->RxBroadcastFrames = DwMac4MmioRead (DwMac4Driver, MMC_RX_BROADCASTFRAME_G);
  Stats->RxMulticastFrames = DwMac4MmioRead (DwMac4Driver, MMC_RX_MULTICASTFRAME_G);
  Stats->RxCrcErrorFrames  = DwMac4MmioRead (DwMac4Driver, MMC_RX_CRC_ERROR);
  Stats->RxTotalBytes      = DwMac4MmioRead (DwMac4Driver, MMC_RX_OCTETCOUNT_GB);
  Stats->RxGoodFrames      = Stats->RxUnicastFrames +
                             Stats->RxBroadcastFrames +
                             Stats->RxMulticastFrames;

  Stats->TxTotalFrames     = DwMac4MmioRead (DwMac4Driver, MMC_TX_FRAMECOUNT_GB);
  Stats->TxGoodFrames      = DwMac4MmioRead (DwMac4Driver, MMC_TX_FRAMECOUNT_G);
  Stats->TxOversizeFrames  = DwMac4MmioRead (DwMac4Driver, MMC_TX_OVERSIZE_G);
  Stats->TxUnicastFrames   = DwMac4MmioRead (DwMac4Driver, MMC_TX_UNICAST_GB);
  Stats->TxBroadcastFrames = DwMac4MmioRead (DwMac4Driver, MMC_TX_BROADCASTFRAME_G);
  Stats->TxMulticastFrames = DwMac4MmioRead (DwMac4Driver, MMC_TX_MULTICASTFRAME_G);
  Stats->TxTotalBytes      = DwMac4MmioRead (DwMac4Driver, MMC_TX_OCTETCOUNT_GB);
  Stats->Collisions        = DwMac4MmioRead (DwMac4Driver, MMC_TX_LATECOL) +
                             DwMac4MmioRead (DwMac4Driver, MMC_TX_EXESSCOL);

  //
  // Fill in the statistics
  //
  CopyMem (Statistic, Stats, sizeof (EFI_NETWORK_STATISTICS));
}

VOID
EFIAPI
StmmacMacLinkUp (
  IN  UINT32                        Speed,
  IN  UINT32                        Duplex,
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32 OldValue;
  UINT32 Value;

  OldValue = DwMac4MmioRead (DwMac4Driver, GMAC_CONFIG);

  Value = OldValue & ~(GMAC_CONFIG_FES | GMAC_CONFIG_PS);

  switch (Speed) {
  case SPEED_10:
    Value |= GMAC_CONFIG_PS;
    break;
  case SPEED_100:
    Value |= GMAC_CONFIG_FES | GMAC_CONFIG_PS;
    break;
  case SPEED_1000:
    Value |= 0;
    break;
  case SPEED_2500:
    Value |= GMAC_CONFIG_FES;
    break;
  default:
    break;
  }

  if (Duplex == DUPLEX_FULL) {
    Value |= GMAC_CONFIG_DM;
  }

  if (Value != OldValue) {
    DwMac4MmioWrite (DwMac4Driver, GMAC_CONFIG, Value);
  }
}

EFI_STATUS
PhyLinkAdjustGmacConfig (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  EFI_STATUS Status;

  Status = DwMac4Driver->Phy->Status (DwMac4Driver->Phy, DwMac4Driver->PhyDev);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DwMac4Driver->PhyDev->LinkUp) {
    DEBUG ((
      DEBUG_VERBOSE,
      "Link is up - Network Cable is Plugged\r\n"
      ));
    StmmacMacLinkUp (DwMac4Driver->PhyDev->Speed, DwMac4Driver->PhyDev->Duplex, DwMac4Driver);
    Status = EFI_SUCCESS;
  } else {
    DEBUG ((
      DEBUG_VERBOSE,
      "Link is Down - Network Cable is Unplugged?\r\n"
      ));
    Status = EFI_NOT_READY;
  }

  return Status;
}

VOID
EFIAPI
StmmacDebug (
  IN  SOPHGO_SIMPLE_NETWORK_DRIVER  *DwMac4Driver
  )
{
  UINT32 Value;
  UINT32 TfcStatus;
  UINT32 Rps0;
  UINT32 Tps0;

  //
  // GMAC debug
  //
  Value = DwMac4MmioRead (DwMac4Driver, GMAC_DEBUG);

  if (Value & GMAC_DEBUG_TFCSTS_MASK) {
    TfcStatus = (Value & GMAC_DEBUG_TFCSTS_MASK) >> GMAC_DEBUG_TFCSTS_SHIFT;
    switch (TfcStatus) {
    case GMAC_DEBUG_TFCSTS_XFER:
      DEBUG ((DEBUG_INFO, "%a(): Transferring input packet for transmission.\n", __func__));
      break;
    case GMAC_DEBUG_TFCSTS_GEN_PAUSE:
      DEBUG ((DEBUG_INFO, "%a(): Generating and transmitting a Pause control packet (in full-duplex mode).\n", __func__));
      break;
    case GMAC_DEBUG_TFCSTS_WAIT:
      DEBUG ((DEBUG_INFO, "%a(): Waiting for one of the following: Status of the previous packet OR IPG or back off period to be over.\n", __func__));
      break;
    case GMAC_DEBUG_TFCSTS_IDLE:
      DEBUG ((DEBUG_INFO, "%a(): Idle State.\n", __func__));
      break;
    default:
      break;
    }
  }

  if (Value & GMAC_DEBUG_TPESTS) {
    DEBUG ((DEBUG_INFO, "%a(): MAC GMII or MII Transmit Protocol Engine Status detected\n", __func__));
  } else {
    DEBUG ((DEBUG_INFO, "%a(): MAC GMII or MII Transmit Protocol Engine Status NOT detected\n", __func__));
  }

  if (Value & GMAC_DEBUG_RFCFCSTS_MASK) {
    DEBUG ((DEBUG_INFO, "%a(): MAC Receive Packet Controller FIFO Status=0x%x\n", __func__,
                            (Value & GMAC_DEBUG_RFCFCSTS_MASK) >> GMAC_DEBUG_RFCFCSTS_SHIFT));
  }

  if (Value & GMAC_DEBUG_RPESTS) {
      DEBUG ((DEBUG_INFO, "%a(): MAC GMII or MII Receive Protocol Engine Status detected\n", __func__));
  } else {
      DEBUG ((DEBUG_INFO, "%a(): MAC GMII or MII Receive Protocol Engine Status NOT detected\n", __func__));
  }

  //
  // DMA debug
  //
  Value = DwMac4MmioRead (DwMac4Driver, DMA_STATUS);
  if (Value & BIT0) {
      DEBUG ((DEBUG_INFO, "%a(): DMA Channel 0 Interrupt Status detected\n", __func__));
  } else {
      DEBUG ((DEBUG_INFO, "%a(): DMA Channel 0 Interrupt Status NOT detected\n", __func__));
  }

  //
  // DMA debug
  //
  Value = DwMac4MmioRead (DwMac4Driver, DMA_DEBUG_STATUS_0);

  if (Value & DMA_DEBUG_STATUS_0_AXWHSTS) {
      DEBUG ((DEBUG_INFO, "%a(): AXI Master Write Channel or AHB Master Status detected\n", __func__));
  } else {
      DEBUG ((DEBUG_INFO, "%a(): AXI Master Write Channel or AHB Master Status NOT detected\n", __func__));
  }

  if (Value & DMA_DEBUG_STATUS_0_AXRHSTS) {
      DEBUG ((DEBUG_INFO, "%a(): AXI Master Read Channel Status detected\n", __func__));
  } else {
      DEBUG ((DEBUG_INFO, "%a(): AXI Master Read Channel Status NOT detected\n", __func__));
  }

  if (Value & DMA_DEBUG_STATUS_0_RPS0_MASK) {
    Rps0 = (Value & DMA_DEBUG_STATUS_0_RPS0_MASK) >> DMA_DEBUG_STATUS_0_RPS0_SHIFT;
    DEBUG ((DEBUG_INFO, "%a(): Channel0 Receive Process Status:\n", __func__));
    switch (Rps0) {
    case DMA_DEBUG_RPS0_STOP:
      DEBUG ((DEBUG_INFO, "  Stopped (Reset or Stop Receive Command issued).\n"));
      break;
    case DMA_DEBUG_RPS0_RUN_FRTD:
      DEBUG ((DEBUG_INFO, "  Running (Fetching Rx Transfer Descriptor).\n"));
      break;
    case DMA_DEBUG_RPS0_RSVD:
      DEBUG ((DEBUG_INFO, "  Reserved for future use.\n"));
      break;
    case DMA_DEBUG_RPS0_RUN_WRP:
      DEBUG ((DEBUG_INFO, "  Running (Waiting for Rx packet).\n"));
      break;
    case DMA_DEBUG_RPS0_SUSPND:
      DEBUG ((DEBUG_INFO, "  Suspended (Rx Descriptor Unavailable).\n"));
      break;
    case DMA_DEBUG_RPS0_RUN_CRD:
      DEBUG ((DEBUG_INFO, "  Running (Closing the Rx Descriptor).\n"));
      break;
    case DMA_DEBUG_RPS0_TSTMP:
      DEBUG ((DEBUG_INFO, "  Timestamp write state.\n"));
      break;
    case DMA_DEBUG_RPS0_RUN_TRP:
      DEBUG ((DEBUG_INFO, "  Running (Transferring the received packet data from the Rx buffer to the system memory).\n"));
      break;
    default:
      break;
    }
  }

  if (Value & DMA_DEBUG_STATUS_0_TPS0_MASK) {
    Tps0 = (Value & DMA_DEBUG_STATUS_0_TPS0_MASK) >> DMA_DEBUG_STATUS_0_TPS0_SHIFT;
    DEBUG ((DEBUG_INFO, "%a(): Channel0 Transmit Process Status:\n", __func__));
    switch (Tps0) {
    case DMA_DEBUG_TPS0_STOP:
      DEBUG ((DEBUG_INFO, "  Stopped (Reset or Stop Transmit Command issued).\n"));
      break;
    case DMA_DEBUG_TPS0_RUN_FTTD:
      DEBUG ((DEBUG_INFO, "  Running (Fetching Tx Transfer Descriptor).\n"));
      break;
    case DMA_DEBUG_TPS0_RUN_WS:
      DEBUG ((DEBUG_INFO, "  Running (Waiting for status).\n"));
      break;
    case DMA_DEBUG_TPS0_RUN_RDS:
      DEBUG ((DEBUG_INFO, "  Running (Reading Data from system memory buffer and queuing it to the Tx buffer (Tx FIFO)).\n"));
      break;
    case DMA_DEBUG_TPS0_TSTMP_WS:
      DEBUG ((DEBUG_INFO, "  Timestamp write state.\n"));
      break;
    case DMA_DEBUG_TPS0_RSVD:
      DEBUG ((DEBUG_INFO, "  Reserved for future use.\n"));
      break;
    case DMA_DEBUG_TPS0_SUSPND:
      DEBUG ((DEBUG_INFO, "  Suspended (Tx Descriptor Unavailable or Tx Buffer Underflow).\n"));
      break;
    case DMA_DEBUG_TPS0_RUN_CTD:
      DEBUG ((DEBUG_INFO, "  Running (Closing Tx Descriptor).\n"));
      break;
    default:
      break;
    }
  }

  DEBUG ((DEBUG_INFO, "%a(): Current Tx Buffer addr(H): 0x%lx\n", __func__, DwMac4MmioRead (DwMac4Driver, DMA_CHAN_CUR_TX_BUF_ADDR_H(0))));
  DEBUG ((DEBUG_INFO, "%a(): Current Tx Buffer addr(L): 0x%lx\n", __func__, DwMac4MmioRead (DwMac4Driver, DMA_CHAN_CUR_TX_BUF_ADDR(0))));
  DEBUG ((DEBUG_INFO, "%a(): Current Rx Buffer addr(H): 0x%lx\n", __func__, DwMac4MmioRead (DwMac4Driver, DMA_CHAN_CUR_RX_BUF_ADDR_H(0))));
  DEBUG ((DEBUG_INFO, "%a(): Current Rx Buffer addr(L): 0x%lx\n", __func__, DwMac4MmioRead (DwMac4Driver, DMA_CHAN_CUR_RX_BUF_ADDR(0))));
  DEBUG ((DEBUG_INFO, "%a(): Current Tx desc pointer: 0x%lx\n", __func__, DwMac4MmioRead (DwMac4Driver, DMA_CHAN_CUR_TX_DESC(0))));
  DEBUG ((DEBUG_INFO, "%a(): Current Rx desc pointer: 0x%lx\n", __func__, DwMac4MmioRead (DwMac4Driver, DMA_CHAN_CUR_RX_DESC(0))));
}
