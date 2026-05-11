/** @file
 *
 *  STMMAC Ethernet Driver -- MDIO bus implementation
 *  Provides Bus interface for MII registers.
 *
 *  linux/drivers/net/ethernet/stmicro/stmmac/stmmac_mdio.c
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Protocol/FdtClient.h>

#include <Include/Mdio.h>
#include "StmmacMdioDxe.h"

STATIC
EFI_STATUS
MdioCheckParam (
 IN INTN  PhyAddr,
 IN INTN  PhyReg
 )
{
  if (PhyAddr > PHY_ADDR_MASK) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Invalid PHY address 0x%x\n",
      __func__,
      PhyAddr
      ));

    return EFI_INVALID_PARAMETER;
  }

  if (PhyReg > PHY_REG_MASK) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Invalid register offset 0x%x\n",
      __func__,
      PhyReg
      ));

    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MdioWaitReady (
  IN UINTN  MdioRegister,
  IN UINTN  Busy
  )
{
  UINT32  Timeout;
  UINT32  Value;

  Timeout = STMMAC_MDIO_TIMEOUT;

  //
  // wait till the MII is not busy
  //
  do {
    //
    // read MII register
    //
    Value = MmioRead32(MdioRegister);
    if (Timeout-- == 100) {
      DEBUG ((
        DEBUG_ERROR,
	"MII busy Timeout\n"
	));

      return EFI_TIMEOUT;
    }
  } while (Value & Busy);

  return EFI_SUCCESS;
}

/*
 * Note:
 * Clause 22 capable PHY is connected to MDIO.
 */
STATIC
EFI_STATUS
MdioOperation (
  IN CONST SOPHGO_MDIO_PROTOCOL  *This,
  IN UINT32                      PhyAddr,
  IN UINT32                      PhyReg,
  IN BOOLEAN                     Write,
  IN OUT UINT32                  *PhyData
  )
{
  UINTN      MdioBase;
  UINT32     MiiAddr;
  UINT32     MiiData;
  UINT32     MiiAddrShift;
  UINT32     MiiAddrMask;
  UINT32     MiiRegShift;
  UINT32     MiiRegMask;
  UINT32     MiiClkCsrShift;
  UINT32     MiiClkCsrMask;
  UINT32     Value;
  EFI_STATUS Status;

  MdioBase       = This->BaseAddress;
  MiiAddr        = This->MiiAddr;
  MiiData        = This->MiiData;
  MiiAddrShift   = This->MiiAddrShift;
  MiiAddrMask    = This->MiiAddrMask;
  MiiRegShift    = This->MiiRegShift;
  MiiRegMask     = This->MiiRegMask;
  MiiClkCsrShift = This->MiiClkCsrShift;
  MiiClkCsrMask  = This->MiiClkCsrMask;

  Status = MdioCheckParam (PhyAddr, PhyReg);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "StmmacMdioDxe: wrong parameters\n"
      ));

    return Status;
  }

  //
  // Wait until any existing MII operation is complete.
  //
  Status = MdioWaitReady (MdioBase + MiiAddr, MII_BUSY);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "StmmacMdioDxe: MdioWaitReady error\n"
      ));

    return Status;
  }

  //
  // CR = 0x10: CSR clock = 20-35 MHz; MDC clock = CSR clock/16
  //
  Value = ((PhyAddr << MiiAddrShift) & MiiAddrMask)
	| ((PhyReg << MiiRegShift) & MiiRegMask)
        | ((0x10 << MiiClkCsrShift) & MiiClkCsrMask)
	| MII_BUSY;

  if (Write) {
    Value |= MII_GMAC4_WRITE;
  } else {
    Value |= MII_GMAC4_READ;
  }

  //
  // Set the MII address register to write.
  //
  MmioWrite32 (MdioBase + MiiData, (*PhyData & MII_DATA_MASK));
  MmioWrite32 (MdioBase + MiiAddr, Value);

  //
  // Wait until any existing MII operation is complete.
  //
  Status = MdioWaitReady (MdioBase + MiiAddr, MII_BUSY);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "StmmacMdioDxe: MdioWaitReady error\n"
      ));

    return Status;
  }

  //
  // Read the data from the MII data register.
  //
  if (!Write) {
    *PhyData = MmioRead32 (MdioBase + MiiData) & MII_DATA_MASK;
  }

  return EFI_SUCCESS;
}

/*
 * Read data from the MII register from within the phy device.
 * @PhyAddr: MII addr
 * @PhyReg: MII reg
 */
STATIC
EFI_STATUS
StmmacMdioRead (
  IN CONST SOPHGO_MDIO_PROTOCOL  *This,
  IN UINT32                      PhyAddr,
  IN UINT32                      PhyReg,
  IN UINT32                      *Data
  )
{
  EFI_STATUS Status;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a(): PhyAddr=0x%lx\tPhyReg=0x%lx\n",
    __func__,
    PhyAddr,
    PhyReg
    ));
  Status = MdioOperation (
            This,
            PhyAddr,
            PhyReg,
            FALSE,
            Data
            );

  return Status;
}

/*
 * Write the data into the MII register from within the phy device.
 * @PhyAddr: MII addr
 * @PhyReg: MII reg
 */
EFI_STATUS
StmmacMdioWrite (
  IN CONST SOPHGO_MDIO_PROTOCOL  *This,
  IN UINT32                      PhyAddr,
  IN UINT32                      PhyReg,
  IN UINT32                      Data
  )
{
  DEBUG ((
    DEBUG_VERBOSE,
    "%a(): PhyAddr=0x%lx\tPhyReg=0x%lx\tData=0x%lx\n",
    __func__,
    PhyAddr,
    PhyReg,
    Data
    ));
  return MdioOperation (
            This,
            PhyAddr,
            PhyReg,
            TRUE,
            &Data
            );
}

EFI_STATUS
EFIAPI
MdioDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  INT32                       Node;
  CONST VOID                  *Prop;
  UINT32                      PropSize;
  FDT_CLIENT_PROTOCOL         *FdtClient;
  SOPHGO_MDIO_PROTOCOL        *Mdio;
  EFI_STATUS                  Status;
  EFI_HANDLE                  Handle;
  UINTN                       RegSize;

  Handle  = NULL;

  Mdio = AllocateZeroPool (sizeof (SOPHGO_MDIO_PROTOCOL));
  if (Mdio == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "MdioDxe: Protocol allocation failed\n"
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Obtain base addresses of all possible controllers
  //
  Status = gBS->LocateProtocol (
      &gFdtClientProtocolGuid,
      NULL,
      (VOID **) &FdtClient
      );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Locate FDT_CLIENT_PROTOCOL failed (Status = %r)\n",
      __func__,
      Status
      ));

    goto ErrorInstallProto;
  }

  Status = FdtClient->FindCompatibleNode (
                                     FdtClient,
                                     "sophgo,ethernet",
                                     &Node
                                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Find ethernet node (Status = %r)\n",
      __func__,
      Status
      ));

    goto ErrorInstallProto;
  }

  Status = FdtClient->GetNodeProperty (
                                FdtClient,
                                Node,
                                "reg",
                                &Prop,
                                &PropSize
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Get reg failed (Status = %r)\n",
      __func__,
      Status
      ));

    goto ErrorInstallProto;
  }

  Mdio->BaseAddress = SwapBytes64 (((CONST UINT64 *) Prop)[0]);
  RegSize = SwapBytes64 (((CONST UINT64 *) Prop)[1]);

  Status = gDS->AddMemorySpace (
		  EfiGcdMemoryTypeMemoryMappedIo,
                  Mdio->BaseAddress,
		  RegSize,
		  EFI_MEMORY_UC
		  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Add memory space failed (Status = %r)\n",
      __func__,
      Status
      ));
    goto ErrorInstallProto;
  }

  Status = gDS->SetMemorySpaceAttributes (
		  Mdio->BaseAddress,
                  RegSize,
                  EFI_MEMORY_UC
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): Set memory attributes failed (Status = %r)\n",
      __func__,
      Status
      ));

    goto ErrorInstallProto;
  }

  Status = FdtClient->GetNodeProperty (
		                FdtClient,
				Node,
				"sophgo,gmac",
				&Prop,
				NULL
				);
  if (EFI_ERROR (Status)) {
    Status = FdtClient->GetNodeProperty (
		                FdtClient,
				Node,
				"sophgo,xlgmac",
				&Prop,
				NULL
				);
    if (!EFI_ERROR (Status)) {
        Mdio->MiiAddr = XGMAC_MDIO_ADDR;
        Mdio->MiiData = XGMAC_MDIO_DATA;
        Mdio->MiiAddrShift = 16;
        Mdio->MiiAddrMask = GENMASK(20, 16);
        Mdio->MiiRegShift = 0;
        Mdio->MiiRegMask = GENMASK(15, 0);
        Mdio->MiiClkCsrShift  = 19;
        Mdio->MiiClkCsrMask = GENMASK(21, 19);
    }
  } else {
    Mdio->MiiAddr = GMAC_MDIO_ADDR;
    Mdio->MiiData = GMAC_MDIO_DATA;
    Mdio->MiiAddrShift = 21;
    Mdio->MiiAddrMask = GENMASK(25, 21);
    Mdio->MiiRegShift = 16;
    Mdio->MiiRegMask = GENMASK(20, 16);
    Mdio->MiiClkCsrShift = 8;
    Mdio->MiiClkCsrMask = GENMASK(11, 8);
  }

  DEBUG ((DEBUG_VERBOSE, "%a():\n"
                   "  BaseAddress      = 0x%lx\tRegSize       = 0x%lx,\n"
                   "  MiiAddr          = 0x%x\tMiiData        = 0x%x,\n"
                   "  MiiAddrShift     = %d\tMiiAddrMask      = 0x%x,\n"
                   "  MiiRegShift      = %d\tMiiRegMask       = 0x%x,\n"
                   "  MiiClkCsrShift   = %d\tMiiClkCsrMask    = 0x%lx\n",
                   __func__,
                   Mdio->BaseAddress,
                   RegSize,
                   Mdio->MiiAddr,
                   Mdio->MiiData,
                   Mdio->MiiAddrShift,
                   Mdio->MiiAddrMask,
                   Mdio->MiiRegShift,
                   Mdio->MiiRegMask,
                   Mdio->MiiClkCsrShift,
                   Mdio->MiiClkCsrMask
                  ));

  Mdio->Read  = StmmacMdioRead;
  Mdio->Write = StmmacMdioWrite;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gSophgoMdioProtocolGuid,
		  Mdio,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to install interfaces\n"
      ));

    goto ErrorInstallProto;
  }

  return EFI_SUCCESS;

ErrorInstallProto:
  FreePool (Mdio);

  return Status;
}
