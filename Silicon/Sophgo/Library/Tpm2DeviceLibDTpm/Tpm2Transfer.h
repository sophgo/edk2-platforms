#ifndef _TPM2_TRANSFER_H_
#define _TPM2_TRANSFER_H_

#include <Include/DwSpi.h>
#include <Library/DebugLib.h>

#define MIN3(Nx, Ny, Nz)    MIN((typeof(Nx))MIN(Nx, Ny), Nz)
#define UINT64_C(Val)       (Val ## UL)
#define GENMASK_64(Nh, Nl)  (((~UINT64_C(0)) << (Nl)) & (~UINT64_C(0) >> (64 - 1 - (Nh))))
#define GENMASK(Nh, Nl)     GENMASK_64(Nh, Nl)
#define BIT(Nr)             (1 << (Nr))


#define BUFFER_SIZE_8 5
#define BUFFER_SIZE_16 6
#define BUFFER_SIZE_32 8

//
// Divide positive or negative dividend by positive or negative divisor
// and round to closest integer. Result is undefined for negative
// divisors if the dividend variable type is unsigned and for negative
// dividends if the divisor variable type is unsigned.
//
#define DIV_ROUND_CLOSEST(x, Divisor)(  \
{                                       \
  typeof(x) __x = x;                    \
  typeof(Divisor) __d = Divisor;        \
  (((typeof(x))-1) > 0 ||               \
    ((typeof(Divisor))-1) > 0 ||        \
    (((__x) > 0) == ((__d) > 0))) ?     \
    (((__x) + ((__d) / 2)) / (__d)) :   \
    (((__x) - ((__d) / 2)) / (__d));    \
  }                                     \
)

#define DIV_ROUND_UP(Nn, Nd) (((Nn) + (Nd) - 1) / (Nd))

#define MIN_T(Type, Nx, Ny) ({          \
  Type __Min1 = (Nx);                   \
  Type __Min2 = (Ny);                   \
  __Min1 < __Min2 ? __Min1: __Min2; })

#define BITS_TO_BYTES(Nr)       DIV_ROUND_UP(Nr, 8)

#define NSEC_PER_SEC           (1000000000L)
#define NSEC_PER_USEC          (1000L)
#define BITS_PER_BYTE          8

#define DW_SPI_GET_BYTE(_Val, _Idx)     \
  ((_Val) >> (BITS_PER_BYTE * (_Idx)) & 0xff)

#define DW_SPI_WAIT_RETRIES    5
#define DW_SPI_SR_BUSY         BIT(0)

//
// Register offsets
//
#define DW_SPI_CTRLR0          0x00
#define DW_SPI_CTRLR1          0x04
#define DW_SPI_SSIENR          0x08
#define DW_SPI_MWCR            0x0c
#define DW_SPI_SER             0x10
#define DW_SPI_BAUDR           0x14
#define DW_SPI_TXFTLR          0x18
#define DW_SPI_RXFTLR          0x1c
#define DW_SPI_TXFLR           0x20
#define DW_SPI_RXFLR           0x24
#define DW_SPI_SR              0x28
#define DW_SPI_IMR             0x2c
#define DW_SPI_ISR             0x30
#define DW_SPI_RISR            0x34
#define DW_SPI_TXOICR          0x38
#define DW_SPI_RXOICR          0x3c
#define DW_SPI_RXUICR          0x40
#define DW_SPI_MSTICR          0x44
#define DW_SPI_ICR             0x48
#define DW_SPI_DMACR           0x4c
#define DW_SPI_DMATDLR         0x50
#define DW_SPI_DMARDLR         0x54
#define DW_SPI_IDR             0x58
#define DW_SPI_VERSION         0x5c
#define DW_SPI_DR              0x60
#define DW_SPI_RX_SAMPLE_DLY   0xf0

//
// Bit fields in CTRLR0
//
#define CTRLR0_DFS_MASK        GENMASK(3, 0)

#define CTRLR0_FRF_MASK        GENMASK(5, 4)
#define CTRLR0_FRF_SPI         0x0
#define CTRLR0_FRF_SSP         0x1
#define CTRLR0_FRF_MICROWIRE   0x2
#define CTRLR0_FRF_RESV        0x3

#define CTRLR0_DFS_OFFSET      0
#define CTRLR0_DFS_32_OFFSET   16

#define CTRLR0_MODE_MASK       GENMASK(7, 6)
#define DW_CTRLR0_SCPHA        BIT(6)
#define DW_CTRLR0_SCPOL        BIT(7)
#define DW_CTRLR0_SRL          BIT(11)
#define DW_CTRLR0_SSTE	       BIT(24)

#define CTRLR0_SPI_CPHA        BIT(0)
#define CTRLR0_SPI_CPOL        BIT(1)
#define CTRLR0_SPI_LOOP        BIT(5)

//
// check chipselect if active high or not
//
#define SPI_CS_HIGH            BIT(2)

#define CTRLR0_TMOD_MASK       GENMASK(9, 8)
#define CTRLR0_TMOD_TR         0x0
#define CTRLR0_TMOD_TO         0x1
#define CTRLR0_TMOD_RO         0x2
#define CTRLR0_TMOD_EPROMREAD  0x3

//
// Bit fields in ISR, IMR, RISR, 7 bits
//
#define DW_SPI_INT_MASK        GENMASK(5, 0)
#define DW_SPI_INT_TXEI        BIT(0)
#define DW_SPI_INT_TXOI        BIT(1)
#define DW_SPI_INT_RXUI        BIT(2)
#define DW_SPI_INT_RXOI        BIT(3)
#define DW_SPI_INT_RXFI        BIT(4)
#define DW_SPI_INT_MSTI        BIT(5)

#define TRY_TIMES                 50
#define TOP_SPI0_CS_OFFSET        0xe4
#define TOP_PIN_MUX_OFFSET        0x1000

#define GPIO_SWPORTA_DR           0x00000000
#define GPIO_SWPORTA_DDR          0x00000004
#define GPIO_EXT_PORTA            0x00000050
#define GPIO_PINS_PER_CONTROLLER  32

#define CLOCK_FREQURNCY           250000000
#define SPI_SPEED_HZ              10 * 1000 * 1000
#define SPI_BUS_NUM               0
#define SPI_SLAVE_NUM             0
#define SPI_POLARITY              0

//
// Slave spi_device related
//
typedef struct {
  UINT32   Cr0;
  UINT32   RxSampleDly;
} DW_SPI_CHIP_DATA;

//
// Slave spi_transfer/spi_mem_op related
//
typedef struct {
  UINT8    Tmode;
  UINT8    Dfs;
  UINT32   Ndf;
  UINT32   Freq;
} DW_SPI_CFG;

typedef struct {
  UINT32   Version;           // Synopsys component version
  UINTN    Regs;
  UINT32   FifoLen;           // Depth of the FIFO buffer
  UINT32   DfsOffset;         // CTRLR0 DFS field offset
  UINT32   MaxFreq;           // max bus freq supported, get from "clock-frequency" in DTB
  UINT32   MaxMemFreq;
  UINT32   NumCs;
  UINT16   BusNum;
  VOID     *Tx;
  UINT32   TxLen;
  VOID     *Rx;
  UINT32   RxLen;
  UINT8    NBytes;           // current is a 1/2 bytes op
  UINT32   CurrentFreq;      // current tranfer frequency
  UINT32   CurRxSampleDly;
  UINT32   DefRxSampleDlyNs;
  VOID     (*SetCs) (SPI_DEVICE *Spi, BOOLEAN Level);
} DW_SPI;

typedef struct {
  UINTN    Regs;
  UINT32   NrGpios;
} DW_GPIO;

typedef struct {
  UINTN    Regs;
  UINT32   Offset;
} SUB_CTRL;

UINT8 Tpm2Read8 (
  IN UINTN regs
  );

UINT16 Tpm2Read16 (
  IN UINTN regs
  );

UINT32 Tpm2Read32 (
  IN UINTN regs
  );

VOID Tpm2Write8 (
  IN UINTN regs,
  IN UINT8 Data
  );

VOID Tpm2Write32 (
  IN UINTN  regs,
  IN UINT32 Data
  );

EFI_STATUS
EFIAPI
Tpm2SpiInitialize (
  VOID
  );

EFI_STATUS
Tpm2WriteNBytes (
  IN UINTN     regs,
  IN UINT8     *Data,
  IN UINT16    Size
  );

EFI_STATUS
Tpm2readNBytes (
  IN  UINTN    regs,
  IN  UINT16   Size,
  OUT UINT8    *OutBuffer
  );

#endif
