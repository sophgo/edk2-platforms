/** @file
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include "NorFlashPei.h"
#include <PiPei.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PrePiLib.h>
#include <Guid/VariableFlashInfo.h>

#define SPIFMC_CTRL                0x00
#define SPIFMC_CTRL_WP_OL          BIT15

STATIC SPI_NOR              *mNorFlashInstance;
STATIC EFI_PEI_SPI_FLASH_SUPPORT_PPI *mSpiFlashSupportPpi;
//STATIC UINT8                *mVariableTempBuffer;

EFI_PEI_PPI_DESCRIPTOR mPpiNvVariableTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiNvVariableSupportPpiGuid,
    NULL
  },
};

EFI_STATUS
EFIAPI
SpiNorGetFlashId (
  IN SPI_NOR     *Nor,
  IN BOOLEAN     UseInRuntime
  )
{
  UINT8      Id[NOR_FLASH_MAX_ID_LEN];
  EFI_STATUS Status;

  Status = mSpiFlashSupportPpi->ReadRegister (Nor, SPINOR_OP_RDID, SPI_NOR_MAX_ID_LEN, Id);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ReadId: Spi error while reading id\n"
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorReadStatus (
  IN SPI_NOR     *Nor,
  IN UINT8       *Sr
  )
{
  EFI_STATUS Status;

  Status = mSpiFlashSupportPpi->ReadRegister (Nor, SPINOR_OP_RDSR, 1, Sr);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Read the Status Register - %r\n",
      __func__,
      Status
      ));
  }

  return Status;
}

/**
  Wait for a predefined amount of time for the flash to be ready,
  or timeout occurs.
**/
EFI_STATUS
SpiNorWaitTillReady (
  IN SPI_NOR *Nor
  )
{
  UINT32 WaitTime;
  /* Unit is us */
  CONST UINT32 CHECK_INTERVAL = 100;
  /*
   * Maximum 4K sector erase time of GD25LB512ME is 700ms, in -40 ~ 125 celsius.
   * Set 2 seconds for safe and compatibility.
   */
  CONST UINT32 MAX_WAIT_TIME = 2000000;

  for (WaitTime = 0; WaitTime <= MAX_WAIT_TIME / CHECK_INTERVAL; ++WaitTime) {
    MicroSecondDelay (CHECK_INTERVAL);

    //
    // Query the Status Register to see if the flash is ready for new commands.
    //
    SpiNorReadStatus (Nor, Nor->BounceBuf);

    if (!(Nor->BounceBuf[0] & SR_WIP)) {
      return EFI_SUCCESS;
    }
  }

  return EFI_TIMEOUT;
}

STATIC
EFI_STATUS
SpiNorWriteEnable (
  IN SPI_NOR  *Nor
  )
{
  EFI_STATUS Status;

  Status = mSpiFlashSupportPpi->WriteRegister (Nor, SPINOR_OP_WREN, NULL, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a: SpiNor error while write enable\n",
      __func__
      ));
    return Status;
  }

  Status = SpiNorReadStatus (Nor, Nor->BounceBuf);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a: SpiNor read status error while write enable\n",
      __func__
      ));
    return Status;
  }

  if (!(Nor->BounceBuf[0] & SR_WEL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Enable failed, get status: 0x%x\n",
      __func__,
      Nor->BounceBuf[0]
      ));
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

STATIC
EFI_STATUS
SpiNorWriteDisable (
  IN SPI_NOR  *Nor
  )
{
  EFI_STATUS Status;

  Status = mSpiFlashSupportPpi->WriteRegister (Nor, SPINOR_OP_WRDI, NULL, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a: SpiNor error while write disable\n",
      __func__
      ));

    return Status;
  }

  Status = SpiNorReadStatus (Nor, Nor->BounceBuf);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a: SpiNor read status error while write disable\n",
      __func__
      ));
    return Status;
  }

  if ((Nor->BounceBuf[0] & SR_WEL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable failed, get status: 0x%x\n",
      __func__,
      Nor->BounceBuf[0]
      ));
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

EFI_STATUS
EFIAPI
SpiNorWriteStatus (
  IN SPI_NOR     *Nor,
  IN UINT8       *Sr,
  IN UINTN       Length
  )
{
  EFI_STATUS Status;

  Status = SpiNorWriteEnable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Enable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = mSpiFlashSupportPpi->WriteRegister (Nor, SPINOR_OP_WRSR, Sr, Length);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Register - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = SpiNorWaitTillReady (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Flash is not ready for new commands - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Write disable
  //
  Status = SpiNorWriteDisable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorReadData (
  IN  SPI_NOR   *Nor,
  IN  UINTN      FlashOffset,
  IN  UINTN      Length,
  OUT UINT8      *Buffer
  )
{
  UINTN       Index;
  UINTN       Address;
  UINTN       PageOffset;
  UINTN       PageRemain;
  EFI_STATUS  Status;

  if (Length == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Length is Zero!\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Buffer is NULL!\n",
      __func__
      ));
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // read data from flash memory by PAGE
  //
  for (Index = 0; Index < Length; Index += PageRemain) {
    Address = FlashOffset + Index;
    PageOffset = IS_POW2 (Nor->Info->PageSize) ?
	         (Address & (Nor->Info->PageSize - 1)) :
		 (Address % Nor->Info->PageSize);
    PageRemain = MIN (Nor->Info->PageSize - PageOffset, Length - Index);

    DEBUG ((
      DEBUG_VERBOSE,
      "%a: Length=0x%lx\tIndex=0x%lx\tAddress=0x%lx\tPageRemain=0x%lx\tPageOffset=0x%lx\n",
      __func__,
      Length,
      Index,
      Address,
      PageRemain,
      PageOffset
      ));

    Status = mSpiFlashSupportPpi->Read (Nor, Address, PageRemain, Buffer + Index);
    if (EFI_ERROR(Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Read Data from flash memory - %r!\n",
        __func__,
        Status
        ));
        return Status;
      }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorWriteData (
  IN SPI_NOR     *Nor,
  IN UINTN       FlashOffset,
  IN UINTN       Length,
  IN UINT8       *Buffer
  )
{
  UINTN       Index;
  UINTN       Address;
  UINTN       PageOffset;
  UINTN       PageRemain;
  EFI_STATUS  Status;

  if (Length == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Length is Zero!\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Buffer is NULL!\n",
      __func__
      ));
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // Write data by PAGE
  //
  for (Index = 0; Index < Length; Index += PageRemain) {
    Address = FlashOffset + Index;
    PageOffset = IS_POW2 (Nor->Info->PageSize) ?
	         (Address & (Nor->Info->PageSize - 1)) :
		 (Address % Nor->Info->PageSize);
    PageRemain = MIN (Nor->Info->PageSize - PageOffset, Length - Index);

    DEBUG ((
      DEBUG_VERBOSE,
      "%a: Length=0x%lx\tIndex=0x%lx\tAddress=0x%lx\tPageRemain=0x%lx\n",
      __func__,
      Length,
      Index,
      Address,
      PageRemain
      ));

    Status = SpiNorWriteEnable (Nor);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Write Enable - %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Status = mSpiFlashSupportPpi->Write (Nor, Address, PageRemain, Buffer + Index);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Write Data - %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Status = SpiNorWaitTillReady (Nor);
    if (EFI_ERROR (Status)) {
      DEBUG ((
          DEBUG_ERROR,
          "%a: Flash is not ready for new commands - %r\n",
          __func__,
          Status
          ));
      return Status;
    }
  }

  Status = SpiNorWriteDisable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorErase (
  IN SPI_NOR    *Nor,
  IN UINTN      FlashOffset,
  IN UINTN      Length
  )
{
  UINT32     ErasedSectors;
  INT32      Index;
  UINTN      Address;
  UINTN      EraseSize;
  EFI_STATUS Status;

  if (Length == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Length is Zero!\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (Nor->Info->Flags & NOR_FLASH_ERASE_4K) {
    EraseSize = SIZE_4KB;
  } else {
    EraseSize = Nor->Info->SectorSize;
  }

  if ((FlashOffset % EraseSize) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: <flash offset addr> is not aligned erase sector size (0x%x)!\n",
      __func__,
      EraseSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Erase Sector
  //
  ErasedSectors = (Length + EraseSize - 1) / EraseSize;
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Start erasing %d sectors, each %d bytes\n",
    __func__,
    ErasedSectors,
    EraseSize
    ));
  for (Index = 0; Index < ErasedSectors; Index++) {
    Address = FlashOffset + Index * EraseSize;
    //
    // Write enable
    //
    Status = SpiNorWriteEnable (Nor);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Write Enable - %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "%a: Length=0x%lx\tIndex=0x%lx\tAddress=0x%lx\n",
      __func__,
      Length,
      Index,
      Address
      ));

    Status = mSpiFlashSupportPpi->Erase (Nor, Address);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Erase Sector - %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Status = SpiNorWaitTillReady (Nor);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Flash is not ready for new commands - %r\n",
        __func__,
        Status
        ));
      return Status;
    }
  }

  //
  // Write disable
  //
  Status = SpiNorWriteDisable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorEraseChip (
  IN SPI_NOR   *Nor
  )
{
  EFI_STATUS Status;

  Nor->EraseOpcode = SPINOR_OP_CHIP_ERASE;

  Status = SpiNorWriteEnable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Enable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = mSpiFlashSupportPpi->Erase (Nor, 0x0);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Erase Sector - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = SpiNorWaitTillReady (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Flash is not ready for new commands - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Write disable
  //
  Status = SpiNorWriteDisable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorGetFlashVariableOffset (
  IN SPI_NOR              *Nor
  )
{
  EFI_STATUS           Status;
  FLASH_PARTITION_INFO *Info;
  UINTN                NameLength;
  UINTN                SuffixLength;
  UINTN                Address;

  CONST CHAR8 *Suffix = ".fd";
  SuffixLength = AsciiStrLen (Suffix);
  Address = PcdGet64 (PcdFlashPartitionTableAddress);

  Info = AllocateZeroPool (sizeof(FLASH_PARTITION_INFO));
  if (Info == NULL) {
    DEBUG((
      DEBUG_ERROR,
      "SpiNor: Cannot allocate memory\n"
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  do {
    Status = SpiNorReadData (Nor, Address, sizeof (FLASH_PARTITION_INFO), (UINT8 *)Info);
    if (EFI_ERROR(Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Read partition table - %r!\n",
        __func__,
        Status
        ));
      goto Error;
    }

    if (Info->Magic != DPT_MAGIC) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Bad partition table magic, set default variable offset!\n",
        __func__,
        Status
        ));

      PcdSet64S (PcdFlashVariableOffset, PcdGet64 (PcdFdOffset) + PcdGet32 (PcdRiscVDxeFvSize));

      goto Error;
    }

    Address += sizeof (FLASH_PARTITION_INFO);
    NameLength = AsciiStrLen (Info->Name);

  } while (AsciiStrCmp (Info->Name + NameLength - SuffixLength, Suffix));

  PcdSet64S (PcdFlashVariableOffset, Info->Offset + PcdGet32 (PcdRiscVDxeFvSize));

  Status = EFI_SUCCESS;
Error:
  FreePool (Info);

  return Status;
}

EFI_STATUS
EFIAPI
SpiNorSoftReset (
  IN SPI_NOR     *Nor
  )
{
  EFI_STATUS Status;

  Status = mSpiFlashSupportPpi->WriteRegister (Nor, SPINOR_SRSTEN_OP, NULL, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Enable Soft Reset - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = mSpiFlashSupportPpi->WriteRegister (Nor, SPINOR_SRST_OP, NULL, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Soft Reset - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Software Reset is not instant, and the delay varies from flash to
  // flash. Looking at a few flashes, most range somewhere below 100
  // microseconds.
  //
  MicroSecondDelay (200);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorSetProtectAll (
  IN SPI_NOR     *Nor,
  IN BOOLEAN     IsProtectAll
  )
{
  EFI_STATUS Status;
  UINTN      SpiBase;
  UINT32     Register;
  UINT8      NorStatusReg;
  UINT8      Tmp;

  //
  // Set Wp pin level to 1 to unlock Status Register.
  //
  SpiBase = Nor->SpiBase;
  Register = MmioRead32 ((UINTN)(SpiBase + SPIFMC_CTRL));
  Register |= SPIFMC_CTRL_WP_OL;
  MmioWrite32 ((UINTN)(SpiBase + SPIFMC_CTRL), Register);

  NorStatusReg = 0;
  Status = SpiNorReadStatus (Nor, &NorStatusReg);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: Read status register - %r\n",
           __func__, Status));
    return Status;
  }

  if (IsProtectAll) {
    if ((NorStatusReg & (SR_BP2 | SR_BP3 | SR_SRP0)) != (SR_BP2 | SR_BP3 | SR_SRP0)) {
      //
      // Set BP2 and BP3 to 1 to protect all blocks.
      // Set SRP0 to 1 to enable hardware protection for status registers.
      //
      Tmp = NorStatusReg | SR_BP2 | SR_BP3 | SR_SRP0;
      Status = SpiNorWriteStatus (Nor, &Tmp, 1);
      if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR, "%a: Write status register - %r\n",
              __func__, Status));
        return Status;
      }

      NorStatusReg = 0;
      Status = SpiNorReadStatus (Nor, &NorStatusReg);
      if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR, "%a: Read status register - %r\n",
              __func__, Status));
        return Status;
      }
      if (NorStatusReg != Tmp) {
        DEBUG ((DEBUG_ERROR, "Write status register fail!\n"));
        return EFI_DEVICE_ERROR;
      }
    }

    //
    // Set Wp pin level to 0.
    // The Status Register locked and cannot be written to
    // when SRP0 and Wp are set to 1 and 0, respectively.
    //
    Register = MmioRead32 ((UINTN)(SpiBase + SPIFMC_CTRL));
    Register &= ~SPIFMC_CTRL_WP_OL;
    MmioWrite32 ((UINTN)(SpiBase + SPIFMC_CTRL), Register);

    //
    // Test whether the status register is locked
    //
    Tmp = NorStatusReg & (~(SR_BP2 | SR_BP3 | SR_SRP0));
    Status = SpiNorWriteStatus (Nor, &Tmp, 1);
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "%a: Write status register - %r\n",
             __func__, Status));
      return Status;
    }
    Tmp = 0;
    Status = SpiNorReadStatus (Nor, &Tmp);
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "%a: Read status register - %r\n",
             __func__, Status));
      return Status;
    }
    if (Tmp != NorStatusReg) {
      DEBUG ((DEBUG_ERROR, "Test status register lock fail!\n"));
      DEBUG ((DEBUG_ERROR, "Status register can be change from 0x%x to 0x%x!\n",
              NorStatusReg, Tmp));
      return EFI_DEVICE_ERROR;
    }
  } else {
    if ((NorStatusReg & (SR_BP2 | SR_BP3 | SR_SRP0)) != 0) {
      //
      // Clear BP2, BP3 and SRP0
      //
      Tmp = NorStatusReg & (~(SR_BP2 | SR_BP3 | SR_SRP0));
      Status = SpiNorWriteStatus (Nor, &Tmp, 1);
      if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR, "%a: Write status register - %r\n",
              __func__, Status));
        return Status;
      }
      Status = SpiNorReadStatus (Nor, &NorStatusReg);
      if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR, "%a: Read status register - %r\n",
              __func__, Status));
        return Status;
      }
      if (Tmp != NorStatusReg) {
        DEBUG ((DEBUG_ERROR, "Status register clear fail!\n"));
        return EFI_DEVICE_ERROR;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorInit (
  IN SPI_NOR                   *Nor
  )
{
  EFI_STATUS Status;

  Nor->AddrNbytes = (Nor->Info->Flags & NOR_FLASH_4B_ADDR) ? 4 : 3;

  Status = SpiNorWriteEnable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Enable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Nor->ReadOpcode    = SPINOR_OP_READ; // Low Frequency
  Nor->ProgramOpcode = SPINOR_OP_PP;
  Nor->EraseOpcode   = (Nor->Info->Flags & NOR_FLASH_ERASE_4K) ?
	                SPINOR_OP_BE_4K : SPINOR_OP_SE;

  if (Nor->AddrNbytes == 4) {
    //
    // Enter 4-byte mode
    //
    Status = mSpiFlashSupportPpi->WriteRegister (Nor, SPINOR_OP_EN4B, NULL, 0);
    if (EFI_ERROR (Status)) {
      DEBUG((
        DEBUG_ERROR,
        "%a: Enter 4-byte mode - %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Nor->ReadOpcode    = SPINOR_OP_READ_4B;
    Nor->ProgramOpcode = SPINOR_OP_PP_4B;
    Nor->EraseOpcode   = (Nor->Info->Flags & NOR_FLASH_ERASE_4K) ?
	                  SPINOR_OP_BE_4K_4B : SPINOR_OP_SE_4B;
  }

  //
  // Initialize flash status register
  //
  Status = SpiNorWriteStatus (Nor, Nor->BounceBuf, 1);
  if (EFI_ERROR (Status)) {
    DEBUG((
      DEBUG_ERROR,
      "%a: Initialize status register - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Write disable
  //
  Status = SpiNorWriteDisable (Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Write Disable - %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiNorEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS            Status;
  UINTN                 VariableSizes;
  UINT32                VariablePages;
  UINT8                 *mVariableTempBuffer;
  VARIABLE_FLASH_INFO   *FlashInfo;

  //
  // Locate SPI Master protocol
  //
  Status = PeiServicesLocatePpi (
                  &gEfiPeiNorFlashSupportPpiGuid,
                  0,
                  NULL,
                  (VOID **)&mSpiFlashSupportPpi
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
        DEBUG_ERROR,
        "%a: Cannot locate SPI Master Ppi\n",
        __func__
        ));
    return Status;
  }

  //
  // Initialize Nor Flash Instance
  //
  mNorFlashInstance = mSpiFlashSupportPpi->SetupDevice (0);
  if (mNorFlashInstance == NULL) {
    DEBUG((
      DEBUG_ERROR,
      "%a: Nor Flash not found!\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  Status = SpiNorGetFlashId (mNorFlashInstance, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Read Nor flash ID failed!\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  Status = SpiNorInit (mNorFlashInstance);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Cannot initialize flash device\n",
      __func__
      ));
    return EFI_DEVICE_ERROR;
  }

  Status = SpiNorGetFlashVariableOffset (mNorFlashInstance);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Get flash variable offset from partition table failed!\n",
      __func__
      ));
    return Status;
  }

  FlashInfo = (VARIABLE_FLASH_INFO *)AllocatePool (sizeof (VARIABLE_FLASH_INFO));
  if (FlashInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot allocate memory for FlashInfo struct\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  VariableSizes = PcdGet32 (PcdFlashNvStorageVariableSize)
                + PcdGet32 (PcdFlashNvStorageFtwSpareSize)
                + PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
  VariablePages = (VariableSizes >> 12);

  mVariableTempBuffer = (UINT8 *)AllocatePages (VariablePages);
  if (mVariableTempBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot allocate memory for Variable\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = SpiNorReadData (
            mNorFlashInstance,
            PcdGet64 (PcdFlashVariableOffset),
            VariableSizes,
            mVariableTempBuffer
            );
  if EFI_ERROR(Status) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot read Variable from flash\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  FlashInfo->NvVariableBaseAddress = (UINT64)mVariableTempBuffer;
  FlashInfo->NvVariableLength = (UINT64)(PcdGet32 (PcdFlashNvStorageVariableSize));
  FlashInfo->FtwSpareBaseAddress = (UINT64)(FlashInfo->NvVariableBaseAddress + FlashInfo->NvVariableLength);
  FlashInfo->FtwSpareLength = (UINT64)(PcdGet32 (PcdFlashNvStorageFtwSpareSize));
  FlashInfo->FtwWorkingBaseAddress = (UINT64)(FlashInfo->FtwSpareBaseAddress + FlashInfo->FtwSpareLength);
  FlashInfo->FtwWorkingLength = (UINT64)(PcdGet32 (PcdFlashNvStorageFtwWorkingSize));

  DEBUG ((DEBUG_INFO,
          "%a: variable at 0x%lx FtwSpare at 0x%lx FwtWork at 0x%lx\n",
          __func__,
          FlashInfo->NvVariableBaseAddress,
          PcdGet32 (PcdFlashNvStorageVariableSize),
          FlashInfo->FtwWorkingBaseAddress,
          FlashInfo->FtwSpareBaseAddress
          ));

  BuildGuidDataHob (&gEfiSophgoPeiVariableGuid, (VOID **)FlashInfo, sizeof (VARIABLE_FLASH_INFO));

  Status = PeiServicesInstallPpi (mPpiNvVariableTable);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}