/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2024, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PcdLib.h>
#include "TrngLibUtil.h"

/**
  Generates a random number by using Hardware TRNG.

  @param[out] Buffer      Buffer to receive the random number.
  @param[in]  BufferSize  Number of bytes in Buffer.

  @retval EFI_SUCCESS           The random value was returned successfully.
  @retval EFI_DEVICE_ERROR      A random value could not be retrieved
                                due to a hardware or firmware error.
  @retval EFI_INVALID_PARAMETER Buffer is NULL or BufferSize is zero.
**/
EFI_STATUS
EFIAPI
GenerateRandomNumbers (
  OUT UINT8  *Buffer,
  IN  UINTN  BufferSize
  )
{
  EFI_STATUS           Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  INT32                Node;
  CONST VOID           *Prop;
  UINT32               PropSize;
  SOPHGO_TRNG_DRIVER   *TrngDriver;
  UINTN                Count;
  UINTN                RandSize;
  UINT32               Value[4];

  if ((BufferSize == 0) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Allocate Resources
  //
  TrngDriver = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (SOPHGO_TRNG_DRIVER)));
  if (TrngDriver == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a() for TrngDriver is NULL!\n",
      __func__
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->LocateProtocol (
		  &gFdtClientProtocolGuid,
		  NULL,
		  (VOID **)&FdtClient
		  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: FDT client protocol not found!\n",
      __func__
      ));
    goto ErrorGetBase;
  }

  Status = FdtClient->FindCompatibleNode (
		  FdtClient,
		  "snps,designware-clp890",
		  &Node);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: TRNG device node not found!\n",
      __func__
      ));
    goto ErrorGetBase;
  }

  Status = FdtClient->GetNodeProperty (FdtClient, Node, "reg", &Prop, &PropSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: GetNodeProperty () failed (Status == %r)\n",
      __func__,
      Status
      ));

    goto ErrorGetBase;
  }

  TrngDriver->RegBase = SwapBytes64 (((CONST UINT64 *)Prop)[0]);

ErrorGetBase:
  if (EFI_ERROR (Status)) {
    TrngDriver->RegBase = FixedPcdGet64 (PcdTrngBase);
    EFI_CPU_ARCH_PROTOCOL *Cpu;

    Status = gBS->LocateProtocol (
		    &gEfiCpuArchProtocolGuid,
		    NULL,
		    (VOID **)&Cpu
		    );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
	"%a: Cannot locate CPU arch service\n",
	__func__
	));
      goto Error;
    }

    Status = Cpu->SetMemoryAttributes (
		    Cpu,
		    TrngDriver->RegBase,
		    SIZE_4KB,
		    EFI_MEMORY_UC
		    );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
	"%a: Failed to set memory attributes\n",
	__func__
	));

      goto Error;
    }
  }

  //
  // support 128-bit random data
  //
  Status = Elpclp890Init (TrngDriver, 128);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: TRGN init failed!\n",
      __func__
      ));
    goto Error;
  }

  //
  // ready the device for use
  //
  Status = Elpclp890CmdSeed (TrngDriver, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: TRGN generate seed failed!\n",
      __func__
      ));
    goto Error;
  }

  Status = Elpclp890CmdCreateState (TrngDriver, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: TRGN create state failed!\n",
      __func__
      ));
    goto Error;
  }

  //
  // generating a 128-bit, s.c. 16-byte random number once.
  //
  RandSize = 16;
  for (Count = 0; Count < (BufferSize / 16) + 1; Count++) {
    if (Count == (BufferSize / 16)) {
      RandSize = BufferSize % 16;
    }

    if (RandSize != 0) {
      Status = Elpclp890CmdGenRandom (TrngDriver, 1, Value);
      if (EFI_ERROR (Status)) {
        DEBUG ((
	  DEBUG_ERROR,
	  "%a: Failed to get random number!\n",
	  __func__
	  ));
        goto Error;
      }

      CopyMem (Buffer + Count * 16, Value, RandSize);
    }
  }

  Status = EFI_SUCCESS;
Error:

  FreePages (TrngDriver, EFI_SIZE_TO_PAGES (sizeof (SOPHGO_TRNG_DRIVER)));

  return Status;
}
