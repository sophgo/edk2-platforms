

#include <Library/SophgoNs16550.h>

#define UART_LCR_WLS_MSK	0x03	/* character length select mask */
#define UART_LCR_WLS_5		0x00	/* 5 bit character length */
#define UART_LCR_WLS_6		0x01	/* 6 bit character length */
#define UART_LCR_WLS_7		0x02	/* 7 bit character length */
#define UART_LCR_WLS_8		0x03	/* 8 bit character length */
#define UART_LCR_STB		0x04	/* # stop Bits, off=1, on=1.5 or 2) */
#define UART_LCR_PEN		0x08	/* Parity eneble */
#define UART_LCR_EPS		0x10	/* Even Parity Select */
#define UART_LCR_STKP		0x20	/* Stick Parity */
#define UART_LCR_SBRK		0x40	/* Set Break */
#define UART_LCR_BKSE		0x80	/* Bank select enable */
#define UART_LCR_DLAB		0x80	/* Divisor latch access bit */

#define UART_MCR_DTR		0x01	/* DTR	 */
#define UART_MCR_RTS		0x02	/* RTS	 */

#define UART_LSR_THRE		0x20	/* Transmit-hold-register empty */
#define UART_LSR_DR		0x01	/* Receiver data ready */
#define UART_LSR_TEMT		0x40	/* Xmitter empty */

#define UART_FCR_FIFO_EN	0x01	/* Fifo enable */
#define UART_FCR_RXSR		0x02	/* Receiver soft reset */
#define UART_FCR_TXSR		0x04	/* Transmitter soft reset */

#define UART_MCRVAL (UART_MCR_DTR | UART_MCR_RTS)		/* RTS/DTR */
#define UART_FCR_DEFVAL	(UART_FCR_FIFO_EN | UART_FCR_RXSR | UART_FCR_TXSR)
#define UART_LCR_8N1	0x03

#define	RBR	0	/* 0x00 Data register */
#define	IER	1	/* 0x04 Interrupt Enable Register */
#define	FCR	2	/* 0x08 FIFO Control Register */
#define	LCR	3	/* 0x0C Line control register */
#define	MCR	4	/* 0x10 Line control register */
#define	LSR	5	/* 0x14 Line Status Register */
#define	MSR	6	/* 0x18 Modem Status Register */
#define	SPR	7	/* 0x20 Scratch Register */

#define THR	RBR
#define IIR	FCR
#define DLL	RBR
#define DLM	IER

#define R_UART_LSR           5
#define   B_UART_LSR_RXRDY   BIT0
#define   B_UART_LSR_RXRDY   BIT0
#define   B_UART_LSR_TXRDY   BIT5
#define   B_UART_LSR_TEMT    BIT6

UART_NS16550_DEV  *DevNs16550;


UINT8
read_reg(
	struct ns16550 *ndev,
	UINTN reg)
{
	return (UINT8)MmioRead32(ndev->base + (reg << ndev->reg_shift));
}

UINT8
write_reg(
	struct ns16550 *ndev,
	UINTN reg,
	UINT8 val
	)
{
	return (UINT8)MmioWrite32(ndev->base + (reg << ndev->reg_shift), (UINT32)val);
}

/**
  Polls a serial device to see if there is any data waiting to be read.

  Polls aserial device to see if there is any data waiting to be read.
  If there is data waiting to be read from the serial device, then TRUE is returned.
  If there is no data waiting to be read from the serial device, then FALSE is returned.

  @retval TRUE             Data is waiting to be read from the serial device.
  @retval FALSE            There is no data waiting to be read from the serial device.

**/
BOOLEAN
EFIAPI
SophgoSerialPortPoll (
  VOID
  )
{
	if ((read_reg (DevNs16550, R_UART_LSR) & B_UART_LSR_RXRDY) != 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
  Reads data from a serial device into a buffer.

  @param  Buffer           Pointer to the data buffer to store the data read from the serial device.
  @param  NumberOfBytes    Number of bytes to read from the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes read from the serial device.
                           If this value is less than NumberOfBytes, then the read operation failed.

**/
UINTN
EFIAPI
SophgoSerialPortRead (
  OUT UINT8  *Buffer,
  IN  UINTN  NumberOfBytes
  )
{
	UINTN  Result;

	if (NULL == Buffer) {
    return 0;
  }

	for (Result = 0; NumberOfBytes-- != 0; Result++, Buffer++) {
		while((read_reg (DevNs16550, R_UART_LSR) & B_UART_LSR_RXRDY) == 0)
		;
		*Buffer = read_reg (DevNs16550, RBR);
	}
	return Result;
}



/**
  Write data from buffer to serial device.

  Writes NumberOfBytes data bytes from Buffer to the serial device.
  The number of bytes actually written to the serial device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.

  If Buffer is NULL, then ASSERT().

  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the serial device.
                           If this value is less than NumberOfBytes, then the write operation failed.

**/
UINTN
EFIAPI
SophgoSerialPortWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
	UINTN  Result;
	UINTN  Index;
	Result = NumberOfBytes;

	for(Index=0; Index < NumberOfBytes; Index ++) {
		while (!(read_reg(DevNs16550, LSR) & UART_LSR_THRE))
		;
		DEBUG ((DEBUG_INFO, "0x%x ", Buffer[Index]));
		if((Index !=0) && ((Index % 32) == 0)) {
			DEBUG ((DEBUG_INFO, "\n"));
		}
		write_reg(DevNs16550, RBR, Buffer[Index]);
	}
	DEBUG ((DEBUG_INFO, "\n"));

	return Result;
}



EFI_STATUS
EFIAPI
ns16550_init(
	struct ns16550 *ndev
	)
{
	unsigned long divisor;

	divisor = ndev->pclk / (16 * ndev->baudrate);

	write_reg(ndev, LCR, read_reg(ndev, LCR) | UART_LCR_DLAB | UART_LCR_8N1);
	write_reg(ndev, DLL, divisor & 0xff);
	write_reg(ndev, DLM, (divisor >> 8) & 0xff);
	write_reg(ndev, LCR, read_reg(ndev, LCR) & (~UART_LCR_DLAB));
	write_reg(ndev, IER, 0);
	write_reg(ndev, MCR, UART_MCRVAL);
	write_reg(ndev, FCR, UART_FCR_DEFVAL);

	return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SophgoNs16550LibConstructor (
  VOID
  )
{
	EFI_CPU_ARCH_PROTOCOL * Cpu;
	EFI_STATUS           Status;
	DEBUG ((DEBUG_INFO, "%a: Init NS16550.\n", __func__));
	DevNs16550 = AllocateZeroPool(sizeof(UART_NS16550_DEV));
	if (DevNs16550 == NULL)
  {
    return EFI_OUT_OF_RESOURCES;
  }

	DevNs16550->base                    = UART3_BASEADDRESS;
	DevNs16550->baudrate                = UART3_BAUDRATE;
	DevNs16550->pclk                    = UART3_PCLK;
	DevNs16550->reg_io_width            = UART3_REG_IO_WIDTH;
	DevNs16550->reg_shift               = UART3_REG_SHIFT;

	Status = gBS->LocateProtocol (
		    &gEfiCpuArchProtocolGuid,
		    NULL,
		    (VOID **)&Cpu
		    );
	if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,"%a: Cannot locate CPU arch service\n",	__func__	));
			return Status;
	}
	Status = Cpu->SetMemoryAttributes (
		    Cpu,
		    DevNs16550->base,
		    SIZE_4KB,
		    EFI_MEMORY_UC
		    );
	if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,"%a: Failed to set memory attributes\n",__func__));
			return Status;
	}

  return ns16550_init(DevNs16550);
}

EFI_STATUS
EFIAPI
SophgoNs16550LibDestructor (
  VOID
  )
{
  if (DevNs16550 != NULL)
  {
    FreePool(DevNs16550);
    DevNs16550 = NULL;
  }
  return EFI_SUCCESS;
}
