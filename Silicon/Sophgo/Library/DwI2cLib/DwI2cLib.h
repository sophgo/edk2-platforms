/** @file
  The header file that provides definitions and function declarations
  related to the DesignWare I2C Controller.

  Copyright (c) 2009, ST Micoelectronics, Vipin Kumar <vipin.kumar@st.com>
  Copyright (c) 2024, SOPHGO Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DW_I2C_H_
#define __DW_I2C_H_

#if !defined(IC_CLK)
#define IC_CLK                  166
#endif
#define NANO_TO_MICRO           1000
#define CMD_BUF_MAX             512

//
// High and low times in different speed modes (in ns)
//
#define MIN_SS_SCL_HIGHTIME     4000
#define MIN_SS_SCL_LOWTIME      4700
#define MIN_FS_SCL_HIGHTIME     600
#define MIN_FS_SCL_LOWTIME      1300
#define MIN_HS_SCL_HIGHTIME     60
#define MIN_HS_SCL_LOWTIME      160

//
// Worst case timeout for 1 byte is kept as 2ms
//
#define I2C_BYTE_TO             (2 * 1000)
#define I2C_STOPDET_TO          (I2C_BYTE_TO)
#define I2C_BYTE_TO_BB          (I2C_BYTE_TO * 16)

//
// i2c control register definitions
//
#define IC_CON_SD               0x0040
#define IC_CON_RE               0x0020
#define IC_CON_10BITADDRMASTER  0x0010
#define IC_CON_10BITADDR_SLAVE  0x0008
#define IC_CON_SPD_MSK          0x0006
#define IC_CON_SPD_SS           0x0002
#define IC_CON_SPD_FS           0x0004
#define IC_CON_SPD_HS           0x0006
#define IC_CON_MM               0x0001

//
// i2c target, slave address register definitions
//
#define TAR_ADDR                0x0050
#define IC_SLAVE_ADDR           0x0002

//
// i2c data buffer and command register definitions
//
#define IC_CMD                  0x0100
#define IC_STOP                 0x0200

//
// i2c interrupt status register definitions
//
#define IC_GEN_CALL             0x0800
#define IC_START_DET            0x0400
#define IC_STOP_DET             0x0200
#define IC_ACTIVITY             0x0100
#define IC_RX_DONE              0x0080
#define IC_TX_ABRT              0x0040
#define IC_RD_REQ               0x0020
#define IC_TX_EMPTY             0x0010
#define IC_TX_OVER              0x0008
#define IC_RX_FULL              0x0004
#define IC_RX_OVER              0x0002
#define IC_RX_UNDER             0x0001

//
// fifo threshold register definitions
//
#define IC_TL0                  0x00
#define IC_TL1                  0x01
#define IC_TL2                  0x02
#define IC_TL3                  0x03
#define IC_TL4                  0x04
#define IC_TL5                  0x05
#define IC_TL6                  0x06
#define IC_TL7                  0x07
#define IC_RX_TL_VALUE          IC_TL0
#define IC_TX_TL_VALUE          IC_TL0

//
// i2c enable register definitions
//
#define IC_ENABLE_0B            0x0001

//
// i2c status register  definitions
//
#define IC_STATUS_SA            0x0040
#define IC_STATUS_MA            0x0020
#define IC_STATUS_RFF           0x0010
#define IC_STATUS_RFNE          0x0008
#define IC_STATUS_TFE           0x0004
#define IC_STATUS_TFNF          0x0002
#define IC_STATUS_ACT           0x0001

//
// Speed Selection
//
#define IC_SPEED_MODE_STANDARD  1
#define IC_SPEED_MODE_FAST      2
#define IC_SPEED_MODE_MAX       3

#define I2C_MAX_SPEED           3400000
#define I2C_FAST_SPEED          400000
#define I2C_STANDARD_SPEED      100000

//
// tx abort source
//
#define IC_ABRT_7B_ADDR_NOACK  (1 << 0)

//
// DesignWare i2c registers
//
typedef struct {
  UINT32 IC_CON;             // 0x00
  UINT32 IC_TAR;             // 0x04
  UINT32 IC_SAR;             // 0x08
  UINT32 IC_HS_MADDR;        // 0x0c
  UINT32 IC_CMD_DATA;        // 0x10
  UINT32 IC_SS_SCL_HCNT;     // 0x14
  UINT32 IC_SS_SCL_LCNT;     // 0x18
  UINT32 IC_FS_SCL_HCNT;     // 0x1c
  UINT32 IC_FS_SCL_LCNT;     // 0x20
  UINT32 IC_HS_SCL_HCNT;     // 0x24
  UINT32 IC_HS_SCL_LCNT;     // 0x28
  UINT32 IC_INTR_STAT;       // 0x2c
  UINT32 IC_INTR_MASK;       // 0x30
  UINT32 IC_RAW_INTR_STAT;   // 0x34
  UINT32 IC_RX_TL;           // 0x38
  UINT32 IC_TX_TL;           // 0x3c
  UINT32 IC_CLR_INTR;        // 0x40
  UINT32 IC_CLR_RX_UNDER;    // 0x44
  UINT32 IC_CLR_RX_OVER;     // 0x48
  UINT32 IC_CLR_TX_OVER;     // 0x4c
  UINT32 IC_CLR_RD_REQ;      // 0x50
  UINT32 IC_CLR_TX_ABRT;     // 0x54
  UINT32 IC_CLR_RX_DONE;     // 0x58
  UINT32 IC_CLR_ACTIVITY;    // 0x5c
  UINT32 IC_CLR_STOP_DET;    // 0x60
  UINT32 IC_CLR_START_DET;   // 0x64
  UINT32 IC_CLR_GEN_CALL;    // 0x68
  UINT32 IC_ENABLE;          // 0x6c
  UINT32 IC_STATUS;          // 0x70
  UINT32 IC_TXFLR;           // 0x74
  UINT32 IC_RXFLR;           // 0x78
  UINT32 IC_SDA_HOLD;        // 0x7c
  UINT32 IC_TX_ABRT_SOURCE;  // 0x80
  UINT8  RES1[0x18];         // 0x84
  UINT32 IC_ENABLE_STATUS;   // 0x9c
} I2C_REGS;

typedef enum {
  I2C_M_TEN                   = 0x0010, // ten-bit chip address
  I2C_M_RD                    = 0x0001, // read data, from slave to master
  I2C_M_STOP                  = 0x8000, // send stop after this message
  I2C_M_NOSTART               = 0x4000, // no start before this message
  I2C_M_REV_DIR_ADDR          = 0x2000, // invert polarity of R/W bit
  I2C_M_IGNORE_NAK            = 0x1000, // continue after NAK
  I2C_M_NO_RD_ACK             = 0x0800, // skip the Ack bit on reads
  I2C_M_RECV_LEN              = 0x0400, // length is first received byte
} DM_I2C_MSG_FLAGS;

//
// designware i2c init data
//
typedef struct {
  UINTN  Base;
  UINTN  Freq;
  UINTN  Speed;
  UINTN  Dev[4];
} I2C_INFO;

typedef struct {
  UINT32  Addr;
  UINT32  Flags;
  UINT32  Len;
  UINT8   *Buf;
} I2C_MSG;

EFI_STATUS
I2cInit (
  IN I2C_INFO  *Info,
  IN INT32     I2cNum
  );

EFI_STATUS
I2cXfer (
  IN INT32    I2c,
  IN I2C_MSG  *Msg,
  IN INT32    NMsgs
  );

#endif /* __DW_I2C_H_ */
